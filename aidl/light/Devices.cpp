#include <Devices.h>
#include <Utils.h>
#include <android-base/properties.h>
#include <unistd.h>
#include <algorithm>
#include <sstream>
#include <vector>

namespace aidl::android::hardware::light {

static constexpr uint32_t kMaxPeriodMs = 8300;
static constexpr uint32_t kBreathPhaseMs = 510;

Devices::Devices() {
    initVariantColor();
}

void Devices::initVariantColor() {
    std::string hwversion = android::base::GetProperty("ro.boot.hwversion", "");
    std::stringstream ss(hwversion);
    std::string segment;
    std::vector<std::string> tokens;

    while (std::getline(ss, segment, '.')) {
        tokens.push_back(segment);
    }

    // 9 = Black (Red Led, default color)
    // 19 = Red  (White Led)
    mVariantColor = "FF0000";

    if (tokens.size() >= 2) {
        int colorId = std::atoi(tokens[1].c_str());
        if (colorId == 19) {
            mVariantColor = "FFFFFF";
        }
    }
}

bool Devices::hasNotificationDevices() const {
    return access((mBasePath + "color").c_str(), W_OK) == 0;
}

bool Devices::setSolid(uint8_t brightness) {
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

    ok &= writeToFile(mBasePath + "color", mVariantColor);
    ok &= writeToFile(mBasePath + "brightness", static_cast<int>(brightness));
    return ok;
}

bool Devices::setBreath(uint8_t brightness, uint32_t riseMs,
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

    ok &= writeToFile(mBasePath + "repeat", 1);
    ok &= writeToFile(mBasePath + "period", period.str());
    ok &= writeToFile(mBasePath + "color", mVariantColor);
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
        setBreath(brightness, riseMs, onMs, fallMs, offMs);
    } else {
        setSolid(brightness);
    }
}

void Devices::dump(int fd) const {
    dprintf(fd, "AW21024 path: %s, available: %d, variantColor: %s, currentMode: %d, lastBrightness: %u\n",
            mBasePath.c_str(), hasNotificationDevices(), mVariantColor.c_str(), mCurrentMode, mLastBrightness);
}

}  // namespace aidl::android::hardware::light
