/*
 * Copyright (C) 2024-2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "AodNotifier"

#include "AodNotifier.h"

#include <android-base/logging.h>
#include <android-base/unique_fd.h>

#include <fcntl.h>
#include <linux/types.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace {

constexpr const char* kDispFeatureDevice = "/dev/mi_display/disp_feature";

constexpr __u32 MI_DISP_PRIMARY = 0;
constexpr __u32 MI_DISP_EVENT_POWER = 0;

enum : __u32 {
    DOZE_TO_NORMAL = 0,
    DOZE_BRIGHTNESS_HBM = 1,
    DOZE_BRIGHTNESS_LBM = 2,
};

enum : __u8 {
    MI_DISP_POWER_ON = 0,
    MI_DISP_POWER_LP1 = 1,
    MI_DISP_POWER_LP2 = 2,
};

struct disp_base {
    __u32 flag;
    __u32 disp_id;
};

struct disp_doze_brightness_req {
    struct disp_base base;
    __u32 doze_brightness;
};

struct disp_event_req {
    struct disp_base base;
    __u32 type;
};

struct disp_event {
    __s32 disp_id;
    __u32 type;
    __u32 length;
};

#define MI_DISP_IOCTL_SET_DOZE_BRIGHTNESS _IOW('D', 0x02, struct disp_doze_brightness_req)
#define MI_DISP_IOCTL_REGISTER_EVENT _IOW('D', 0x07, struct disp_event_req)

void requestDozeBrightness(int fd, __u32 dozeBrightness) {
    disp_doze_brightness_req req = {
            .base = {.flag = 0, .disp_id = MI_DISP_PRIMARY},
            .doze_brightness = dozeBrightness,
    };
    if (ioctl(fd, MI_DISP_IOCTL_SET_DOZE_BRIGHTNESS, &req) < 0) {
        PLOG(ERROR) << "failed to set doze brightness " << dozeBrightness;
    }
}

class AodSensorCallback : public BnEventQueueCallback {
  public:
    AodSensorCallback() {
        mDispFd.reset(open(kDispFeatureDevice, O_RDWR | O_CLOEXEC));
        if (mDispFd.get() < 0) {
            PLOG(ERROR) << "failed to open " << kDispFeatureDevice;
        }
    }

    ndk::ScopedAStatus onEvent(const Event& e) override {
        int value = static_cast<int>(getEventValue(e));
        LOG(DEBUG) << "aod: " << value;
        requestDozeBrightness(mDispFd.get(), (value == 3 || value == 5) ? DOZE_BRIGHTNESS_LBM
                                                                        : DOZE_BRIGHTNESS_HBM);
        return ndk::ScopedAStatus::ok();
    }

  private:
    android::base::unique_fd mDispFd;
};

}  // namespace

AodNotifier::AodNotifier(std::shared_ptr<ISensorManager> manager) : SensorNotifier(manager) {
    initializeSensorQueue("xiaomi.sensor.aod", true, ndk::SharedRefBase::make<AodSensorCallback>());
}

void AodNotifier::notify() {
    android::base::unique_fd dispFd(open(kDispFeatureDevice, O_RDWR | O_CLOEXEC));
    if (dispFd.get() < 0) {
        PLOG(ERROR) << "failed to open " << kDispFeatureDevice;
        return;
    }

    disp_event_req req = {
            .base = {.flag = 0, .disp_id = MI_DISP_PRIMARY},
            .type = MI_DISP_EVENT_POWER,
    };
    if (ioctl(dispFd.get(), MI_DISP_IOCTL_REGISTER_EVENT, &req) < 0) {
        PLOG(ERROR) << "failed to register power event";
        return;
    }

    pollfd pfd = {.fd = dispFd.get(), .events = POLLIN};
    while (true) {
        if (poll(&pfd, 1, -1) < 0) {
            if (errno == EINTR) continue;
            PLOG(ERROR) << "failed to poll " << kDispFeatureDevice;
            return;
        }

        alignas(disp_event) char buf[512];
        ssize_t len = read(dispFd.get(), buf, sizeof(buf));
        if (len < 0) {
            PLOG(ERROR) << "failed to read display event";
            continue;
        }

        int power = -1;
        for (ssize_t off = 0; off + static_cast<ssize_t>(sizeof(disp_event)) <= len;) {
            auto* event = reinterpret_cast<disp_event*>(buf + off);
            if (event->length <= sizeof(disp_event) || off + event->length > len) {
                LOG(ERROR) << "malformed display event, length: " << event->length;
                break;
            }
            if (event->type == MI_DISP_EVENT_POWER) {
                power = static_cast<__u8>(buf[off + sizeof(disp_event)]);
            }
            off += event->length;
        }
        if (power < 0) {
            continue;
        }

        LOG(DEBUG) << "display power: " << power;
        switch (power) {
            case MI_DISP_POWER_LP1:
            case MI_DISP_POWER_LP2:
                enableSensor(true);
                break;
            case MI_DISP_POWER_ON:
                requestDozeBrightness(dispFd.get(), DOZE_TO_NORMAL);
                [[fallthrough]];
            default:
                enableSensor(false);
                break;
        }
    }
}
