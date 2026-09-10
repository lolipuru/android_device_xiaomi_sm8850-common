/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Lights.h"

#define LOG_TAG "android.hardware.light-service.sm8850"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <unistd.h>
#include <chrono>
#include <thread>

using ::aidl::android::hardware::light::Lights;

int main() {
    if (android::base::GetProperty("ro.boot.hwc", "") != "CN")
        LOG(INFO) << "Non-CN variant; registering empty Lights HAL";

    constexpr int kNodeRetries = 10;
    for (int retry = 0;
         retry < kNodeRetries && access("/sys/class/leds/aw21024_led/color", W_OK) != 0; ++retry)
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    if (access("/sys/class/leds/aw21024_led/color", W_OK) != 0)
        LOG(INFO) << "AW21024 unavailable; registering empty Lights HAL";

    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<Lights> lights = ndk::SharedRefBase::make<Lights>();

    const std::string instance = std::string() + Lights::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(lights->asBinder().get(), instance.c_str());
    if (status != STATUS_OK) {
        LOG(ERROR) << "Failed to register Lights HAL: " << status;
        return 1;
    }

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
