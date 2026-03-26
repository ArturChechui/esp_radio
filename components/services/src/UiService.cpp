#include "UiService.hpp"

#include <esp_log.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <variant>

#include "Dumpers.hpp"
#include "Events.hpp"
#include "Fonts.hpp"
#include "Overloaded.hpp"

namespace services {
namespace {
constexpr char Tag[] = "UiService";

// 128x64 monochrome OLED geometry
constexpr uint8_t Width = 128;
constexpr uint8_t Height = 64;
constexpr uint8_t PageH = 8;
constexpr uint8_t Pages = Height / PageH;

// Layout StatusRect + MainRect are not used, remove or reuse?
constexpr common::Rect StatusRect{0U, 0U, 128U, 16U};  // pages 0..1
constexpr common::Rect StatusTextRect{0U, 0U, 80U, 16U};
constexpr common::Rect StatusVolRect{80U, 0U, 16U, 16U};
constexpr common::Rect StatusBatRect{96U, 0U, 16U, 16U};
constexpr common::Rect StatusWifiRect{112U, 0U, 16U, 16U};

constexpr common::Rect MainRect{0U, 16U, 128U, 48U};  // pages 2..7
constexpr common::Rect MainIconRect{0U, 16U, common::fonts::Icon48W, common::fonts::Icon48H};
constexpr common::Rect MainTextRect{static_cast<uint8_t>(common::fonts::Icon48W + 4U), 16,
                                    static_cast<uint8_t>(128U - (common::fonts::Icon48W + 4U)), 48};

constexpr const char* NoDataText = "NODATA";

static_assert(Width == 128, "This UI assumes 128px Width");
static_assert(Height == 64, "This UI assumes 64px Height");

static common::Icon barsToIcon(uint8_t bars) {
    switch (bars) {
        case 3:
            return common::Icon::Wifi3;
        case 2:
            return common::Icon::Wifi2;
        case 1:
            return common::Icon::Wifi1;
        default:
            return common::Icon::WifiOff;
    }
}

static common::Icon playbackStatusToIcon(const common::PlaybackStatus status) {
    switch (status) {
        case common::PlaybackStatus::Playing:
            return common::Icon::Stop;
        case common::PlaybackStatus::Buffering:
            return common::Icon::Buffering;
        case common::PlaybackStatus::Stopped:
        default:
            return common::Icon::Play;
    }
}

static common::Icon volumeToIcon(const uint8_t vol) {
    if (vol == 0) {
        return common::Icon::Volume0;
    }
    if (vol <= 20) {
        return common::Icon::Volume1;
    }
    if (vol <= 35) {
        return common::Icon::Volume2;
    }
    if (vol <= 45) {
        return common::Icon::Volume3;
    }
    if (vol <= 69) {
        return common::Icon::Volume4;
    }
    // vol 70..100
    return common::Icon::Volume5;
}

static common::Icon batteryPercentToIcon(const uint8_t pct) {
    if (pct <= 25U) {
        return common::Icon::BatteryLow;
    }
    if (pct <= 70U) {
        return common::Icon::BatteryMid;
    }
    return common::Icon::BatteryFull;
}

static char toUpperAscii(const char ch) {
    const unsigned char u = static_cast<unsigned char>(ch);
    return static_cast<char>(std::toupper(u));
}

static inline bool validateRect(const common::Rect& r) {
    if (r.w == 0U || r.h == 0U || r.x >= Width || r.y >= Height) {
        // invalid params
        return false;
    }

    if ((r.y & 7U) != 0U || (r.h & 7U) != 0U) {
        // height or y is not aligned
        return false;
    }

    return true;
}

// TODO: Do I introduce a new struct to simplify the code?
// struct Window {
//     uint8_t col0, col1, page0, page1;
//     size_t len;
// };
// inline Window rectToWindow(const common::Rect& r) {
//     const uint16_t x0 = r.x;
//     const uint16_t y0 = r.y;
//     const uint16_t x1 = std::min<uint16_t>(static_cast<uint16_t>(r.x) + r.w, kWidth);
//     const uint16_t y1 = std::min<uint16_t>(static_cast<uint16_t>(r.y) + r.h, kHeight);
//
//     Window w{};
//     w.col0 = static_cast<uint8_t>(x0);
//     w.col1 = static_cast<uint8_t>(x1 - 1);
//     w.page0 = static_cast<uint8_t>(y0 / 8);
//     w.page1 = static_cast<uint8_t>((y1 - 1) / 8);
//
//     const size_t widthBytes = static_cast<size_t>(x1 - x0);
//     const size_t pagesCnt = static_cast<size_t>(w.page1 - w.page0 + 1);
//     w.len = widthBytes * pagesCnt;
//     return w;
// }

}  // namespace

UiService::UiService(adapters::IDisplay& display, IStationRepository& stationRepo)
    : mDisplay(display),
      mStationRepo(stationRepo),
      mFramebuffer(Width * Pages, 0x00),
      mTemperatureC(0U),
      mHumidityPct(0U),
      mWifi(common::Icon::WifiOff),
      mBattery(common::Icon::BatteryMid),
      mPlayback(common::Icon::Play),
      mVolume(common::Icon::Volume1),
      mFullFlushPending(false),
      mMode(UiMode::Booting),
      mStationDirty(false),
      mStatusDirty(false),
      mWifiDirty(false),
      mBatteryDirty(false),
      mPlaybackDirty(false),
      mVolumeDirty(false) {
    ESP_LOGI(Tag, "Creating UiService");
    mTxBuf.reserve(Width * Pages);
}

bool UiService::init() {
    ESP_LOGI(Tag, "Initializing UiService");

    if (const auto* bmp = common::fonts::bootingScreen()) {
        std::memcpy(mFramebuffer.data(), bmp, common::fonts::FullScreenBytes);

        if (!mDisplay.showFramebuffer(mFramebuffer.data(), mFramebuffer.size())) {
            ESP_LOGW(Tag, "init showFramebuffer failed");
            return false;
        }
    }
    return true;
}

void UiService::onEvent(const common::AppEvent& e) {
    ESP_LOGI(Tag, "onEvent(%s)", common::dump(e).c_str());

    const auto prevMode = mMode;

    std::visit(common::Overloaded{
                   [this](const common::CurrentStationChangedEvent&) { mStationDirty = true; },
                   [this](const common::TempHumidUpdateEvent& t) {
                       if (mTemperatureC != t.temperature || mHumidityPct != t.humidity) {
                           mTemperatureC = t.temperature;
                           mHumidityPct = t.humidity;
                           mStatusDirty = true;
                       }
                   },
                   [this](const common::WifiStateChangedEvent& w) {
                       const auto next = w.isConnected ? barsToIcon(w.bars) : common::Icon::WifiOff;
                       if (mWifi != next) {
                           mWifi = next;
                           mWifiDirty = true;
                       }
                   },
                   [this](const common::BatteryLevelUpdateEvent& b) {
                       const auto next = batteryPercentToIcon(b.percent);
                       if (mBattery != next) {
                           mBattery = next;
                           mBatteryDirty = true;
                       }
                   },
                   [this](const common::PlaybackStatusChangedEvent& p) {
                       const auto next = playbackStatusToIcon(p.status);
                       if (mPlayback != next) {
                           mPlayback = next;
                           mPlaybackDirty = true;
                       }
                   },
                   [this](const common::VolumeChangedEvent& v) {
                       const auto next = volumeToIcon(v.volume);
                       if (mVolume != next) {
                           mVolume = next;
                           mVolumeDirty = true;
                       }
                   },
                   [this](const common::SwitchToWifiProvScreenEvent&) { mMode = UiMode::WifiProv; },
                   [this](const common::SwitchToSyncInProgressScreenEvent&) {
                       mMode = UiMode::SyncInProgress;
                   },
                   [this](const common::SwitchToMainScreenEvent&) { mMode = UiMode::Main; },
                   [](const auto&) {}},
               e);

    std::visit(common::Overloaded{[this, prevMode](const common::SwitchToWifiProvScreenEvent&) {
                                      if (prevMode != mMode && mMode == UiMode::WifiProv) {
                                          showWifiProvisioningScreen();
                                      }
                                  },
                                  [this, prevMode](
                                      const common::SwitchToSyncInProgressScreenEvent&) {
                                      if (prevMode != mMode && mMode == UiMode::SyncInProgress) {
                                          showSyncInProgressScreen();
                                      }
                                  },
                                  [this, prevMode](const common::SwitchToMainScreenEvent&) {
                                      if (prevMode != mMode && mMode == UiMode::Main) {
                                          showFullMainScreen();
                                      }
                                  },
                                  [this](const auto&) {
                                      if (mMode != UiMode::Main) {
                                          return;
                                      }

                                      if (mStationDirty) {
                                          updateStationName();
                                          mStationDirty = false;
                                      } else if (mStatusDirty) {
                                          updateStatusText();
                                          mStatusDirty = false;
                                      } else if (mWifiDirty) {
                                          updateWifiIcon();
                                          mWifiDirty = false;
                                      } else if (mBatteryDirty) {
                                          updateBatteryIcon();
                                          mBatteryDirty = false;
                                      } else if (mPlaybackDirty) {
                                          updatePlaybackIcon();
                                          mPlaybackDirty = false;
                                      } else if (mVolumeDirty) {
                                          updateVolumeIcon();
                                          mVolumeDirty = false;
                                      }
                                  }},
               e);
}

void UiService::showFullMainScreen() {
    std::fill(mFramebuffer.begin(), mFramebuffer.end(), 0x00);

    const bool doFlush = false;
    updateStatusText(doFlush);
    updateVolumeIcon(doFlush);
    updateBatteryIcon(doFlush);
    updateWifiIcon(doFlush);

    updatePlaybackIcon(doFlush);
    updateStationName(doFlush);

    mStationDirty = false;
    mStatusDirty = false;
    mWifiDirty = false;
    mBatteryDirty = false;
    mPlaybackDirty = false;
    mVolumeDirty = false;

    if (!mDisplay.showFramebuffer(mFramebuffer.data(), mFramebuffer.size())) {
        ESP_LOGW(Tag, "showFullMainScreen showFramebuffer failed");
        mFullFlushPending = true;
    }
}

void UiService::showWifiProvisioningScreen() {
    if (const auto* bmp = common::fonts::wifiProvisioningScreen()) {
        std::memcpy(mFramebuffer.data(), bmp, common::fonts::FullScreenBytes);

        if (!mDisplay.showFramebuffer(mFramebuffer.data(), mFramebuffer.size())) {
            ESP_LOGW(Tag, "wifiProvisioningScreen showFramebuffer failed");
        }
    }
}

void UiService::showSyncInProgressScreen() {
    if (const auto* bmp = common::fonts::syncInProgressScreen()) {
        std::memcpy(mFramebuffer.data(), bmp, common::fonts::FullScreenBytes);

        if (!mDisplay.showFramebuffer(mFramebuffer.data(), mFramebuffer.size())) {
            ESP_LOGW(Tag, "syncInProgressScreen showFramebuffer failed");
        }
    }
}

void UiService::updateStatusText(const bool doFlush) {
    clearRect(StatusTextRect);

    uint8_t currX = StatusTextRect.x;

    std::array<char, 16UL> buf{};
    const int n = std::snprintf(buf.data(), buf.size(), "%dC %d%%", mTemperatureC, mHumidityPct);
    const size_t len = (n <= 0) ? 0UL : std::min<size_t>(static_cast<size_t>(n), buf.size() - 1UL);
    std::string_view text{buf.data(), len};
    for (char ch : text) {
        const uint8_t* glyph = common::fonts::statusGlyph(ch);
        if (!glyph) {
            glyph = common::fonts::statusGlyph('?');
        }
        if (!glyph) {
            continue;
        }

        if (static_cast<uint16_t>(currX) + common::fonts::StatusW >
            (StatusTextRect.x + StatusTextRect.w)) {
            break;
        }

        common::Rect r{currX, StatusTextRect.y, common::fonts::StatusW, common::fonts::StatusH};
        blitRect(r, glyph);
        currX = static_cast<uint8_t>(currX + common::fonts::StatusW);
    }

    if (doFlush) {
        flushRect(StatusTextRect);
    }
}

void UiService::updateWifiIcon(const bool doFlush) {
    clearRect(StatusWifiRect);

    if (const auto* bmp = common::fonts::icon16(mWifi)) {
        blitRect(StatusWifiRect, bmp);
    }

    if (doFlush) {
        flushRect(StatusWifiRect);
    }
}

void UiService::updateBatteryIcon(const bool doFlush) {
    clearRect(StatusBatRect);

    if (const auto* bmp = common::fonts::icon16(mBattery)) {
        blitRect(StatusBatRect, bmp);
    }

    if (doFlush) {
        flushRect(StatusBatRect);
    }
}

void UiService::updateVolumeIcon(const bool doFlush) {
    clearRect(StatusVolRect);

    if (const auto* bmp = common::fonts::icon16(mVolume)) {
        blitRect(StatusVolRect, bmp);
    }

    if (doFlush) {
        flushRect(StatusVolRect);
    }
}

void UiService::updatePlaybackIcon(const bool doFlush) {
    clearRect(MainIconRect);

    if (const auto* bmp = common::fonts::icon48(mPlayback)) {
        blitRect(MainIconRect, bmp);
    }

    if (doFlush) {
        flushRect(MainIconRect);
    }
}

void UiService::updateStationName(const bool doFlush) {
    clearRect(MainTextRect);

    const common::StationData& station = mStationRepo.currentStation();

    const uint8_t maxChars = static_cast<uint8_t>(MainTextRect.w / common::fonts::MainW);
    uint8_t currX = MainTextRect.x;
    const std::string_view raw =
        station.name.empty() ? std::string_view{NoDataText} : std::string_view{station.name};
    const std::string_view name = raw.substr(0, maxChars);

    for (char ch : name) {
        const char up = toUpperAscii(ch);

        const uint8_t* glyph = common::fonts::mainGlyph(up);
        if (!glyph) {
            glyph = common::fonts::mainGlyph('?');
        }
        if (!glyph) {
            continue;
        }

        common::Rect r{currX, MainTextRect.y, common::fonts::MainW, common::fonts::MainH};
        blitRect(r, glyph);

        currX = static_cast<uint8_t>(currX + common::fonts::MainW);
    }

    if (doFlush) {
        flushRect(MainTextRect);
    }
}

void UiService::clearRect(const common::Rect r) {
    if (!validateRect(r)) {
        return;
    }

    const uint16_t x0 = r.x;
    const uint16_t y0 = r.y;
    const uint16_t x1 = std::min<uint16_t>(static_cast<uint16_t>(r.x) + r.w, Width);
    const uint16_t y1 = std::min<uint16_t>(static_cast<uint16_t>(r.y) + r.h, Height);

    const uint8_t page0 = static_cast<uint8_t>(y0 / 8U);
    const uint8_t pageN = static_cast<uint8_t>((y1 - y0) / 8U);
    const uint16_t w = static_cast<uint16_t>(x1 - x0);

    for (uint8_t p = 0U; p < pageN; ++p) {
        const uint16_t base = static_cast<uint16_t>((page0 + p) * Width + x0);
        std::fill_n(&mFramebuffer[base], w, 0x00);
    }
}

void UiService::blitRect(const common::Rect r, const uint8_t* data) {
    if (!validateRect(r)) {
        return;
    }

    const uint16_t x0 = r.x;
    const uint16_t y0 = r.y;
    const uint16_t x1 = std::min<uint16_t>(static_cast<uint16_t>(r.x) + r.w, Width);
    const uint16_t y1 = std::min<uint16_t>(static_cast<uint16_t>(r.y) + r.h, Height);

    const uint8_t page0 = static_cast<uint8_t>(y0 / 8U);
    const uint8_t pageN = static_cast<uint8_t>((y1 - y0) / 8U);
    const uint16_t w = static_cast<uint16_t>(x1 - x0);

    // Data is page-major and exactly matches r.w * (r.h/8)
    for (uint8_t p = 0U; p < pageN; ++p) {
        const uint16_t dstBase = static_cast<uint16_t>((page0 + p) * Width + x0);
        const uint16_t srcBase = static_cast<uint16_t>(p * r.w);
        std::memcpy(&mFramebuffer[dstBase], &data[srcBase], w);
    }
}

void UiService::flushRect(const common::Rect r) {
    if (!validateRect(r)) {
        return;
    }

    if (mFullFlushPending) {
        // if a full flush succeeds, we can continue doing partial flushes later
        const bool ok = mDisplay.showFramebuffer(mFramebuffer.data(), mFramebuffer.size());
        if (!ok) {
            ESP_LOGW(Tag, "Full framebuffer flush failed");
        }
        mFullFlushPending = !ok;

        return;
    }

    const uint16_t x0 = r.x;
    const uint16_t y0 = r.y;
    const uint16_t x1 = std::min<uint16_t>(static_cast<uint16_t>(r.x) + r.w, Width);
    const uint16_t y1 = std::min<uint16_t>(static_cast<uint16_t>(r.y) + r.h, Height);

    const uint8_t col0 = static_cast<uint8_t>(x0);
    const uint8_t col1 = static_cast<uint8_t>(x1 - 1);
    const uint8_t page0 = static_cast<uint8_t>(y0 / 8);
    const uint8_t page1 = static_cast<uint8_t>((y1 - 1) / 8);

    const size_t w = static_cast<size_t>(x1 - x0);
    const size_t pagesCnt = static_cast<size_t>(page1 - page0 + 1);
    const size_t needed = pagesCnt * w;

    mTxBuf.resize(needed);

    size_t out = 0;
    for (uint8_t p = page0; p <= page1; ++p) {
        const uint8_t* src = &mFramebuffer[static_cast<size_t>(p) * Width + col0];
        std::memcpy(&mTxBuf[out], src, w);
        out += w;
    }

    const bool ok = mDisplay.showWindow(col0, col1, page0, page1, mTxBuf.data(), mTxBuf.size());
    if (!ok) {
        ESP_LOGW(Tag, "showWindow failed (col %u-%u, page %u-%u, len=%u)", col0, col1, page0, page1,
                 static_cast<unsigned>(mTxBuf.size()));

        mFullFlushPending = true;
    }
}

}  // namespace services
