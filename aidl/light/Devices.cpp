#include <Devices.h>
#include <Utils.h>
#include <unistd.h>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace aidl::android::hardware::light {

static constexpr uint32_t kMaxPeriodMs = 8300;
static constexpr uint32_t kBreathPhaseMs = 510;

Devices::Devices() = default;
bool Devices::hasNotificationDevices() const {
    return access((mBasePath + "color").c_str(), W_OK) == 0;
}

bool Devices::setSolid(const std::string& color, uint8_t brightness) {
    bool ok = true;
    if (mCurrentMode != 1) {
        if (mCurrentMode != 0) {
            writeToFile(mBasePath + "run", 0);
            writeToFile(mBasePath + "hwen", 0);
            usleep(10000);
        }
        ok &= writeToFile(mBasePath + "hwen", 1);
        ok &= writeToFile(mBasePath + "run", 1);
        mCurrentMode = 1;
    }

    ok &= writeToFile(mBasePath + "color", color);
    for (int segment = 0; segment < 8; ++segment) {
        ok &= writeToFile(mBasePath + "rgbcolor", std::to_string(segment) + " " + color);
    }
    ok &= writeToFile(mBasePath + "brightness", static_cast<int>(brightness));
    return ok;
}

bool Devices::setBreath(const std::string& color, uint8_t brightness, uint32_t riseMs,
                        uint32_t onMs, uint32_t fallMs, uint32_t offMs) {
    bool ok = true;
    if (mCurrentMode != 2) {
        if (mCurrentMode != 0) {
            writeToFile(mBasePath + "run", 0);
            writeToFile(mBasePath + "hwen", 0);
            usleep(10000);
        }
        ok &= writeToFile(mBasePath + "hwen", 1);
        ok &= writeToFile(mBasePath + "run", 2);
        mCurrentMode = 2;
    }

    std::ostringstream period;
    period << std::min(riseMs, kMaxPeriodMs) << " " << std::min(onMs, kMaxPeriodMs) << " "
           << std::min(fallMs, kMaxPeriodMs) << " " << std::min(offMs, kMaxPeriodMs);

    ok &= writeToFile(mBasePath + "color", color);
    for (int segment = 0; segment < 8; ++segment) {
        ok &= writeToFile(mBasePath + "rgbcolor", std::to_string(segment) + " " + color);
    }
    ok &= writeToFile(mBasePath + "period", period.str());
    ok &= writeToFile(mBasePath + "repeat", 1);
    ok &= writeToFile(mBasePath + "brightness", static_cast<int>(brightness));
    return ok;
}

void Devices::setNotificationState(const State& state) {
    if (!state.color.isLit()) {
        writeToFile(mBasePath + "run", 0);
        writeToFile(mBasePath + "hwen", 0);
        mCurrentMode = 0;
        return;
    }

    const uint32_t rgb = (static_cast<uint32_t>(state.color.red) << 16) |
                         (static_cast<uint32_t>(state.color.green) << 8) | state.color.blue;
    std::ostringstream color;
    color << std::uppercase << std::hex << std::setw(6) << std::setfill('0') << rgb;

    uint8_t brightness = state.color.brightness ? state.color.brightness : mLastBrightness;
    mLastBrightness = brightness;

    const auto& timed = state.effect.timed;
    const bool blink =
            state.effect.type == Effect::Type::HARDWARE ||
            (state.effect.type == Effect::Type::TIMED && timed.onMs > 0 && timed.offMs > 0);

    if (blink) {
        uint32_t riseMs, onMs, fallMs, offMs;
        if ((timed.onMs > 0xFFFF) || (timed.offMs > 0xFFFF)) {
            riseMs = (timed.onMs >> 16) & 0xFFFF;
            onMs = timed.onMs & 0xFFFF;
            fallMs = (timed.offMs >> 16) & 0xFFFF;
            offMs = timed.offMs & 0xFFFF;
        } else if (state.effect.type == Effect::Type::TIMED && timed.onMs > 0 && timed.offMs > 0) {
            riseMs = timed.onMs / 2;
            onMs = timed.onMs - riseMs;
            fallMs = timed.offMs / 2;
            offMs = timed.offMs - fallMs;
        } else {
            riseMs = kBreathPhaseMs;
            onMs = kBreathPhaseMs;
            fallMs = kBreathPhaseMs;
            offMs = kBreathPhaseMs;
        }
        setBreath(color.str(), brightness, riseMs, onMs, fallMs, offMs);
    } else {
        setSolid(color.str(), brightness);
    }
}

void Devices::dump(int fd) const {
    dprintf(fd, "AW21024 path: %s, available: %d, currentMode: %d, lastBrightness: %u\n",
            mBasePath.c_str(), hasNotificationDevices(), mCurrentMode, mLastBrightness);
}

}  // namespace aidl::android::hardware::light
