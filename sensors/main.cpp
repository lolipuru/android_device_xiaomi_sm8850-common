/*
 * Copyright (C) 2024-2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "SensorNotifier"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include "notifiers/AodNotifier.h"

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();

    const std::string instance = std::string(ISensorManager::descriptor) + "/default";
    auto manager = ISensorManager::fromBinder(
            ndk::SpAIBinder(AServiceManager_waitForService(instance.c_str())));
    if (!manager) {
        LOG(ERROR) << "failed to get " << instance;
        return EXIT_FAILURE;
    }

    std::vector<std::unique_ptr<SensorNotifier>> notifiers;
    notifiers.push_back(std::make_unique<AodNotifier>(manager));
    for (const auto& notifier : notifiers) {
        notifier->activate();
    }

    ABinderProcess_joinThreadPool();

    // Should never reach this
    return EXIT_FAILURE;
}
