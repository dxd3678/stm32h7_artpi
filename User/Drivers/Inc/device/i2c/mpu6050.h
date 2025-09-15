#pragma once

#include <device/i2c/i2c.h>

struct mpu6050_device {
    struct i2c_client dev;
};