/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <models/IDumpable.h>
#include <models/State.h>

#include <cstdint>
#include <string>

namespace aidl {
namespace android {
namespace hardware {
namespace light {

class Devices : public IDumpable {
  public:
    Devices();

    void dump(int fd) const override;

    bool hasNotificationDevices() const;

    void setNotificationState(const State& state);

  private:
    bool setBreath(const std::string& color, uint32_t riseMs, uint32_t onMs, uint32_t fallMs,
                   uint32_t offMs);

    std::string mBasePath = "/sys/class/leds/aw21024_led/";
};

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
