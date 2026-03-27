#pragma once

namespace services {
class ISensorService {
   public:
    virtual ~ISensorService() = default;

    virtual bool init() = 0;
    virtual void deinit() = 0;
    virtual void setPlaybackActive(const bool active) = 0;
};
}  // namespace services
