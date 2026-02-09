#include "InputService.hpp"

#include <esp_log.h>

#include "BoardConfig.hpp"
#include "Events.hpp"
#include "IEventQueue.hpp"
#include "IGpioInput.hpp"

namespace services {
namespace {
constexpr const char* Tag = "InputService";

bool mapGpioToButton(const uint32_t gpio, common::Button& out) {
    if (gpio == static_cast<uint32_t>(common::BUTTON_PLAY_STOP_GPIO)) {
        out = common::Button::PlayStop;
        return true;
    }
    if (gpio == static_cast<uint32_t>(common::BUTTON_UP_GPIO)) {
        out = common::Button::Up;
        return true;
    }
    if (gpio == static_cast<uint32_t>(common::BUTTON_DOWN_GPIO)) {
        out = common::Button::Down;
        return true;
    }

    return false;
}

}  // namespace

InputService::InputService(adapters::IGpioInput& gpioInput, common::IEventQueue& coreEventQueue)
    : mGpioInput(gpioInput), mCoreEventQueue(coreEventQueue) {
    ESP_LOGI(Tag, "InputService created");
}

bool InputService::init() {
    // GpioInput callback is executed from GpioInputTask context (NOT ISR),
    // so it is safe to use std::function, logging, and normal queue posts here.
    mGpioInput.setInputCallback(
        [this](const common::GpioInputData& data) { this->onGpioInputData(data); });

    ESP_LOGI(Tag, "InputService initialized");
    return true;
}

void InputService::deinit() {
    // Nothing to deinit yet
    ESP_LOGI(Tag, "InputService deinitialized");
}

void InputService::onGpioInputData(const common::GpioInputData& data) {
    if (data.type != common::GpioInputType::ButtonPressed) {
        return;
    }
    ESP_LOGI(Tag, "button pressed buttonId: %lu", data.buttonId);
    common::Button button = common::Button::PlayStop;
    if (!mapGpioToButton(data.buttonId, button)) {
        ESP_LOGW(Tag, "Unknown GPIO buttonId: %lu", data.buttonId);
        return;
    }

    (void)mCoreEventQueue.post(common::ButtonPressedEvent{button});
}

}  // namespace services
