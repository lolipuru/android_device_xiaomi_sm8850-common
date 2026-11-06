/*
 * Copyright (C) 2024-2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <aidl/android/frameworks/sensorservice/BnEventQueueCallback.h>
#include <aidl/android/frameworks/sensorservice/ISensorManager.h>

#include <atomic>
#include <string>
#include <thread>

using ::aidl::android::frameworks::sensorservice::BnEventQueueCallback;
using ::aidl::android::frameworks::sensorservice::IEventQueue;
using ::aidl::android::frameworks::sensorservice::IEventQueueCallback;
using ::aidl::android::frameworks::sensorservice::ISensorManager;
using ::aidl::android::hardware::sensors::Event;

class SensorNotifier {
  public:
    SensorNotifier(std::shared_ptr<ISensorManager> manager);
    virtual ~SensorNotifier();

    void activate();

  protected:
    bool initializeSensorQueue(const std::string& typeAsString, bool wakeup,
                               std::shared_ptr<IEventQueueCallback> callback);
    void enableSensor(bool enable);
    virtual void notify() = 0;

    std::shared_ptr<IEventQueue> mQueue;
    int32_t mSensorHandle = -1;

  private:
    std::shared_ptr<ISensorManager> mManager;
    std::thread mThread;
    bool mEnabled = false;
};

float getEventValue(const Event& e);
