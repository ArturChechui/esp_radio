#include "InputService.hpp"

#include <esp_log.h>

#include "BoardConfig.hpp"
#include "Events.hpp"
#include "IClock.hpp"
#include "IEventQueue.hpp"
#include "IGpioInput.hpp"
#include "IQueue.hpp"
#include "IStopToken.hpp"
#include "ITaskRunner.hpp"

namespace services {
namespace {
constexpr uint16_t TaskStackWords = 6144;
constexpr uint16_t TaskPriority = 6U;
constexpr uint16_t TaskCore = 0U;

constexpr const char* Tag = "InputService";

constexpr int ButtonPressedLevel = 0;
constexpr uint64_t ConfirmDelayMs = 20ULL;
constexpr uint64_t LockoutDelayMs = 200ULL;

static inline int findButtonIndexByGpio(const uint32_t gpio) {
    for (size_t i = 0; i < common::ButtonGpios.size(); ++i) {
        if (static_cast<uint32_t>(common::ButtonGpios[i]) == gpio) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

static inline bool indexToButton(const size_t idx, common::Button& out) {
    switch (idx) {
        case 0:
            out = common::Button::PlayStop;
            return true;
        case 1:
            out = common::Button::Next;
            return true;
        case 2:
            out = common::Button::Previous;
            return true;
        default:
            return false;
    }
}

// Number of valid quarter-steps that represent one mechanical detent
constexpr int QuarterStepsPerDetent = 4;
// How many percent points per mechanical detent
constexpr int VolumePerDetent = 3;
// Transition table: index = (prevState << 2) | currState, values = -1, 0, +1
static constexpr int8_t QuadratureTransitionTable[16] = {
    /* prev=00 curr=00 */ 0,  /* 0 */
    /* prev=00 curr=01 */ -1, /* 1 */
    /* prev=00 curr=10 */ 1,  /* 2 */
    /* prev=00 curr=11 */ 0,  /* 3 */

    /* prev=01 curr=00 */ 1,  /* 4 */
    /* prev=01 curr=01 */ 0,  /* 5 */
    /* prev=01 curr=10 */ 0,  /* 6 */
    /* prev=01 curr=11 */ -1, /* 7 */

    /* prev=10 curr=00 */ -1, /* 8 */
    /* prev=10 curr=01 */ 0,  /* 9 */
    /* prev=10 curr=10 */ 0,  /*10 */
    /* prev=10 curr=11 */ 1,  /*11 */

    /* prev=11 curr=00 */ 0,  /*12 */
    /* prev=11 curr=01 */ 1,  /*13 */
    /* prev=11 curr=10 */ -1, /*14 */
    /* prev=11 curr=11 */ 0   /*15 */
};
/**
 * Decode a single quadrature transition
 * Returns:
 *   -1 : counter-clockwise quarter-step
 *    0 : no valid quarter-step / bounce / invalid jump
 *   +1 : clockwise quarter-step
 */
static inline int8_t decodeQuadratureTransition(const uint8_t prevQuadratureState,
                                                const uint8_t currQuadratureState) {
    const uint8_t transitionIndex =
        static_cast<uint8_t>((prevQuadratureState << 2u) | (currQuadratureState & 0x3u));
    return QuadratureTransitionTable[transitionIndex];
}
}  // namespace

InputService::InputService(adapters::IGpioInput& gpioInput, common::IEventQueue& coreEventQueue,
                           common::IQueue<uint32_t>& queue, common::ITaskRunner& runner,
                           common::IClock& clock)
    : mGpioInput(gpioInput),
      mCoreEventQueue(coreEventQueue),
      mQueue(queue),
      mTaskRunner(runner),
      mTaskHandle(),
      mClock(clock),
      mLastPressMs({0}),
      mEncPrevQuadratureState(0xFF),
      mEncQuarterAccumulator(0),
      mEncInvert(false),
      mVolume(10) {
    ESP_LOGI(Tag, "InputService created");
}

bool InputService::init() {
    mTaskHandle = mTaskRunner.start(
        common::TaskParams{.name = "InputTask", .priority = TaskPriority, .core = TaskCore},
        TaskStackWords, &InputService::processStepFn, this);
    if (!mTaskHandle.isValid()) {
        ESP_LOGE(Tag, "Failed to create InputTask");
        return false;
    }

    ESP_LOGI(Tag, "InputService initialized");
    return true;
}

void InputService::deinit() {
    if (mTaskHandle.isValid()) {
        (void)mTaskRunner.stop(mTaskHandle, 7000);
        mTaskHandle.reset();
    }

    ESP_LOGI(Tag, "InputService deinitialized");
}

common::StepResult InputService::processStepFn(void* arg, common::IStopToken& token) {
    auto* self = static_cast<InputService*>(arg);
    if (!self) {
        return {.action = common::StepAction::Error};
    }

    return self->processStep(token);
}

common::StepResult InputService::processStep(common::IStopToken& token) {
    if (token.stopRequested()) {
        return {.action = common::StepAction::Done};
    }

    uint32_t gpioNum = 0;
    if (!mQueue.get(gpioNum)) {
        return {.action = common::StepAction::Continue};
    }

    if (common::EncS1Gpio == gpioNum || common::EncS2Gpio == gpioNum) {
        handleEncoderGpio();
    } else {
        handleButtonGpio(gpioNum, token);
    }

    return {.action = common::StepAction::Continue};
}

void InputService::handleEncoderGpio() {
    const int s1 = mGpioInput.getLevel(common::EncS1Gpio);
    const int s2 = mGpioInput.getLevel(common::EncS2Gpio);
    const uint8_t currQuadratureState = static_cast<uint8_t>((s1 << 1) | s2);

    if (mEncPrevQuadratureState == 0xFF) {
        mEncPrevQuadratureState = currQuadratureState;
        mEncQuarterAccumulator = 0;
        return;
    }

    // decode the transition
    const int8_t quarterStepDelta =
        decodeQuadratureTransition(mEncPrevQuadratureState, currQuadratureState);
    int detentsToEmit = 0;

    if (quarterStepDelta != 0) {
        // valid quarter-step: update accumulator
        mEncQuarterAccumulator += static_cast<int>(quarterStepDelta);

        // convert to full detents if threshold crossed
        if (mEncQuarterAccumulator >= QuarterStepsPerDetent ||
            mEncQuarterAccumulator <= -QuarterStepsPerDetent) {
            detentsToEmit = mEncQuarterAccumulator / QuarterStepsPerDetent;
            mEncQuarterAccumulator -= detentsToEmit * QuarterStepsPerDetent;
        }

        mEncPrevQuadratureState = currQuadratureState;
    } else {
        mEncPrevQuadratureState = currQuadratureState;
    }

    // apply if there are any full detents
    if (detentsToEmit != 0) {
        if (mEncInvert) {
            detentsToEmit = -detentsToEmit;
        }

        applyDetentsToVolume(detentsToEmit);
    }
}

void InputService::applyDetentsToVolume(const int detents) {
    if (detents == 0) {
        return;
    }

    const int delta = (detents * VolumePerDetent);
    int next = mVolume + delta;
    if (next < 0) {
        next = 0;
    } else if (next > 100) {
        next = 100;
    }

    if (next != mVolume) {
        mVolume = next;

        common::VolumeChangedEvent ev;
        ev.volume = static_cast<uint8_t>(next);
        (void)mCoreEventQueue.post(ev);
    }
}

void InputService::handleButtonGpio(const uint32_t gpioNum, common::IStopToken& token) {
    const uint64_t nowMs = mClock.nowMs();

    const int idx = findButtonIndexByGpio(gpioNum);
    if (idx < 0) {
        ESP_LOGW(Tag, "Unknown gpio %lu", gpioNum);
        return;
    }

    // quick lockout check (prevents double-trigger if too soon)
    if ((nowMs - mLastPressMs[static_cast<size_t>(idx)]) < LockoutDelayMs) {
        return;
    }

    // sample level now, wait a short confirm delay, sample again
    const int levelBefore = mGpioInput.getLevel(gpioNum);
    if (token.sleepMs(ConfirmDelayMs)) {
        return;
    }
    const int levelAfter = mGpioInput.getLevel(gpioNum);

    // require both samples to indicate "pressed"
    if (!(levelBefore == ButtonPressedLevel && levelAfter == ButtonPressedLevel)) {
        return;
    }

    // Now we confirmed a valid press, update timestamp
    mLastPressMs[static_cast<size_t>(idx)] = mClock.nowMs();

    // map index to logical button
    common::Button button;
    if (!indexToButton(static_cast<size_t>(idx), button)) {
        ESP_LOGW(Tag, "no button mapping for idx %d", idx);
        return;
    }

    (void)mCoreEventQueue.post(common::ButtonPressedEvent{button});
}
}  // namespace services
