#pragma once

#include <stdint.h>

struct i2c_gpio {
    void *port;
    uint16_t scl_pin;
    uint16_t sda_pin;
};