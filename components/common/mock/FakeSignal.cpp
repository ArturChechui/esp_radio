
#include "Signal.hpp"

namespace common {
struct Signal::Impl {};

Signal::Signal() : m(std::make_unique<Impl>()) {}

Signal::~Signal() = default;

bool Signal::wait(const uint32_t& timeoutMs) const {
    return true;
}

void Signal::signal() {}

bool Signal::isValid() const {
    return true;
}

void Signal::reset() {}

}  // namespace common
