/*
 * Copyright (C) 2024-2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "SensorNotifier"

#include "SensorNotifier.h"

#include <android-base/logging.h>

using ::aidl::android::hardware::sensors::SensorInfo;

SensorNotifier::SensorNotifier(std::shared_ptr<ISensorManager> manager) : mManager(manager) {}

SensorNotifier::~SensorNotifier() {
    if (mThread.joinable()) {
        mThread.detach();
    }
}

bool SensorNotifier::initializeSensorQueue(const std::string& typeAsString, bool wakeup,
                                           std::shared_ptr<IEventQueueCallback> callback) {
    std::vector<SensorInfo> sensorList;
    auto ret = mManager->getSensorList(&sensorList);
    if (!ret.isOk()) {
        LOG(ERROR) << "failed to get sensor list: " << ret.getDescription();
        return false;
    }

    auto it = std::find_if(sensorList.begin(), sensorList.end(), [&](const SensorInfo& sensor) {
        return sensor.typeAsString == typeAsString &&
               ((sensor.flags & SensorInfo::SENSOR_FLAG_BITS_WAKE_UP) != 0) == wakeup;
    });
    if (it == sensorList.end()) {
        LOG(ERROR) << "failed to find " << typeAsString << " sensor with wake-up: " << wakeup;
        return false;
    }
    mSensorHandle = it->sensorHandle;

    ret = mManager->createEventQueue(callback, &mQueue);
    if (!ret.isOk() || !mQueue) {
        LOG(ERROR) << "failed to create event queue: " << ret.getDescription();
        mQueue = nullptr;
        return false;
    }

    return true;
}

void SensorNotifier::enableSensor(bool enable) {
    if (!mQueue || enable == mEnabled) {
        return;
    }

    auto ret = enable ? mQueue->enableSensor(mSensorHandle, 20000 /* sample period */,
                                             0 /* latency */)
                      : mQueue->disableSensor(mSensorHandle);
    if (!ret.isOk()) {
        LOG(ERROR) << "failed to " << (enable ? "enable" : "disable")
                   << " sensor: " << ret.getDescription();
        return;
    }
    mEnabled = enable;
}

void SensorNotifier::activate() {
    if (!mQueue || mThread.joinable()) {
        return;
    }
    mThread = std::thread(&SensorNotifier::notify, this);
}

float getEventValue(const Event& e) {
    switch (e.payload.getTag()) {
        case Event::EventPayload::Tag::scalar:
            return e.payload.get<Event::EventPayload::Tag::scalar>();
        case Event::EventPayload::Tag::data:
            return e.payload.get<Event::EventPayload::Tag::data>().values[0];
        default:
            LOG(WARNING) << "unexpected event payload: " << e.payload.toString();
            return 0;
    }
}
