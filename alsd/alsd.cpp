/*
 * Copyright (C) 2026 Alex Zorzi <info@alexzorzi.it>
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "alsd"

#include <aidl/vendor/xiaomi/sensor/citsensorservice/ICitSensorService.h>
#include <android/binder_manager.h>
#include <fcntl.h>
#include <log/log.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <memory>

using aidl::vendor::xiaomi::sensor::citsensorservice::ICitSensorService;

namespace {

constexpr char kServiceName[] =
        "vendor.xiaomi.sensor.citsensorservice.ICitSensorService/default";

constexpr char kBacklightPanel0[] =
        "/sys/class/backlight/panel0-backlight/brightness";
constexpr char kBacklightPanel1[] =
        "/sys/class/backlight/panel1-backlight/brightness";

constexpr long kPeriodMs = 300;
constexpr long kIdlePeriodMs = 1500;

void sleepMs(long ms) {
    timespec ts = {ms / 1000, (ms % 1000) * 1000000};
    nanosleep(&ts, nullptr);
}

int readBacklightFd(int fd) {
    if (fd < 0) return -1;

    char buf[16] = {0};
    ssize_t bytes = pread(fd, buf, sizeof(buf) - 1, 0);
    if (bytes <= 0) return -1;

    buf[bytes] = '\0';
    return atoi(buf);
}

}  // namespace

int main() {
    while (true) {
        int fd0 = open(kBacklightPanel0, O_RDONLY | O_CLOEXEC);
        int fd1 = open(kBacklightPanel1, O_RDONLY | O_CLOEXEC);

        if (fd0 < 0 && fd1 < 0) {
            ALOGE("Failed to open both %s and %s: %s, retrying...",
                  kBacklightPanel0, kBacklightPanel1, strerror(errno));
            sleepMs(kIdlePeriodMs);
            continue;
        }

        ::ndk::SpAIBinder binder(AServiceManager_waitForService(kServiceName));
        std::shared_ptr<ICitSensorService> service = ICitSensorService::fromBinder(binder);

        if (!service) {
            ALOGE("Failed to connect to %s, retrying...", kServiceName);
            if (fd0 >= 0) close(fd0);
            if (fd1 >= 0) close(fd1);
            sleepMs(kIdlePeriodMs);
            continue;
        }

        ALOGI("Connected to citsensorservice (fd0: %d, fd1: %d)", fd0, fd1);

        int lastDbv = -1;
        int lastDisplayId = -1;

        while (true) {
            int dbv0 = readBacklightFd(fd0);
            int dbv1 = readBacklightFd(fd1);

            int activeDisplayId = 0;
            int activeDbv = 0;

            if (dbv0 > 0) {
                activeDisplayId = 0;
                activeDbv = dbv0;
            } else if (dbv1 > 0) {
                activeDisplayId = 1;
                activeDbv = dbv1;
            }

            if (activeDbv <= 0) {
                lastDbv = activeDbv;
                sleepMs(kIdlePeriodMs);
                continue;
            }

            // Update brightness when brightness or active display changes
            if (activeDbv != lastDbv || activeDisplayId != lastDisplayId) {
                int32_t ret = 0;
                auto status = service->setBrightness(activeDbv, &ret);
                if (!status.isOk()) {
                    ALOGW("citsensorservice died on setBrightness: %s",
                          status.getDescription().c_str());
                    break;
                }
                lastDbv = activeDbv;
                lastDisplayId = activeDisplayId;
            }

            int32_t ret = 0;
            auto status = service->triggerCwbDump(activeDisplayId, 0, true, &ret);
            if (!status.isOk()) {
                ALOGW("citsensorservice died on triggerCwbDump: %s",
                    status.getDescription().c_str());
                break;
            }

            sleepMs(kPeriodMs);
        }

        if (fd0 >= 0) close(fd0);
        if (fd1 >= 0) close(fd1);
    }

    return 0;
}