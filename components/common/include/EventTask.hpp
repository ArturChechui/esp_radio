/**
 * @file EventTask.hpp
 * @brief Header for the system's asynchronous event processing task.
 *
 * This file contains the EventTask class, which manages an internal
 * FreeRTOS queue and a dedicated thread to process system-wide AppEvents.
 */

#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "IEventQueue.hpp"

namespace common {
class IEventHandler;
class ITaskRunner;
class IStopToken;
struct StepResult;

/**
 * @class EventTask
 * @brief Concrete implementation of an event queue and background processor.
 *
 * EventTask fulfills two roles: it acts as a sink for events via the post()
 * method (implementing IEventQueue) and as a background worker that
 * dispatches those events to a registered IEventHandler.
 */
class EventTask : public IEventQueue {
   public:
    /**
     * @brief Constructs an EventTask.
     * @param taskName The string name for the FreeRTOS task (for debugging).
     * @param taskRunner Reference to the task runner used to spawn the worker thread.
     */
    EventTask(const char* taskName, ITaskRunner& taskRunner);

    /**
     * @brief Destroys the EventTask, ensuring the worker thread is stopped
     * and the queue is emptied.
     */
    ~EventTask() override;

    /**
     * @brief Initializes the internal FreeRTOS queue.
     * @return true if the queue was successfully created.
     */
    bool init();

    /**
     * @brief Starts the background task and begins dispatching events to the handler.
     * @param handler Reference to the object that will process incoming AppEvents.
     * @return true if the task was successfully started.
     */
    bool run(IEventHandler& handler);

    /**
     * @brief Adds an event to the queue from any thread.
     * * Because FreeRTOS queues use memcpy, this method performs heap allocation
     * for complex variants to ensure memory safety.
     * @param event The AppEvent to be queued.
     * @return true if the event was successfully posted.
     */
    bool post(const AppEvent& event) override;

   private:
    /**
     * @brief Static entry point for the background processing loop.
     */
    static StepResult processStepFn(void* arg, IStopToken& token);

    /**
     * @brief Logic executed on each iteration of the event task.
     * * This method waits for new events on the queue and calls the handler.
     * @param token Stop token used to gracefully terminate the loop.
     * @return Result indicating if the task should continue or stop.
     */
    StepResult processStep(IStopToken& token);

    TaskParams mTaskParams;   /**< Configuration parameters for the background task. */
    TaskHandle mTaskHandle;   /**< Handle to the running FreeRTOS task. */
    ITaskRunner& mTaskRunner; /**< Reference to the system's task manager. */

    QueueHandle_t mEventQueue; /**< FreeRTOS handle for the underlying message queue. */
    bool mIsRunning;           /**< Flag indicating if the background task is active. */

    IEventHandler* mHandler; /**< Non-owning pointer to the event dispatcher target. */
};

}  // namespace common
