/*
 * Copyright (C) 2024-2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "SensorNotifier.h"

class AodNotifier : public SensorNotifier {
  public:
    AodNotifier(std::shared_ptr<ISensorManager> manager);

  protected:
    void notify() override;
};
