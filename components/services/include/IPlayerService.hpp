#pragma once

#include <cstdint>
#include <string>

#include "Types.hpp"

namespace services {
class IPlayerService {
   public:
    virtual ~IPlayerService() = default;

    virtual bool init() = 0;
    virtual bool playStation(const std::string& url) = 0;
    virtual bool stop() = 0;
    virtual common::PlaybackStatus getStatus() const = 0;
    virtual std::string getCurrentUrl() const = 0;
};

}  // namespace services
