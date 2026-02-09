#pragma once

#include "Types.hpp"

namespace adapters {

/**
 * @brief Input adapter interface (GPIO, buttons, etc.)
 */
class IGpioInput {
   public:
    virtual ~IGpioInput() = default;

    /**
     * @brief Initialize input adapter
     */
    virtual bool init() = 0;

    /**
     * @brief Deinitialize and cleanup
     */
    virtual void deinit() = 0;

    /**
     * @brief Register callback for button events
     */
    virtual void setInputCallback(common::GpioInputDataCallback cb) = 0;
};

}  // namespace adapters
