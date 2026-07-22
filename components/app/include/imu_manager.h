#pragma once

#include "esp_err.h"

void imu_suspend(void);
void imu_resume(void);
void imu_deinit(void);
void imu_init_after_wakeup(void);