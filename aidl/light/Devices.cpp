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
bool Devices::setBreath(const std::string& color, uint32_t riseMs, uint32_t onMs, uint32_t fallMs,
                        uint32_t offMs) {
    std::ostringstream period;
    period << std::min(riseMs, kMaxPeriodMs) << " " << std::min(onMs, kMaxPeriodMs) << " "
           << std::min(fallMs, kMaxPeriodMs) << " " << std::min(offMs, kMaxPeriodMs);

    bool ok = writeToFile(mBasePath + "run", 2);
    ok &= writeToFile(mBasePath + "color", color);
    ok &= writeToFile(mBasePath + "period", period.str());
    ok &= writeToFile(mBasePath + "repeat", 0);
    return ok;
}
void Devices::setNotificationState(const State& state) {
    const uint32_t rgb = (static_cast<uint32_t>(state.color.red) << 16) |
                         (static_cast<uint32_t>(state.color.green) << 8) | state.color.blue;
    std::ostringstream color;
    color << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << rgb;
    if (!state.color.isLit()) {
        writeToFile(mBasePath + "run", 0);
        writeToFile(mBasePath + "hwen", 0);
        return;
    }
    bool ok = writeToFile(mBasePath + "hwen", 1);
    const auto& timed = state.effect.timed;
    const bool blink =
            state.effect.type == Effect::Type::HARDWARE ||
            (state.effect.type == Effect::Type::TIMED && timed.onMs > 0 && timed.offMs > 0);
    if (blink) {
        ok &= setBreath(color.str(), kBreathPhaseMs, kBreathPhaseMs, kBreathPhaseMs,
                        kBreathPhaseMs);
    } else {
        for (int segment = 0; segment < 8; ++segment)
            ok &= writeToFile(mBasePath + "rgbcolor", std::to_string(segment) + " " + color.str());
    }
    (void)ok;
}
void Devices::dump(int fd) const {
    dprintf(fd, "AW21024 path: %s, available: %d\n", mBasePath.c_str(), hasNotificationDevices());
}

}  // namespace aidl::android::hardware::light
