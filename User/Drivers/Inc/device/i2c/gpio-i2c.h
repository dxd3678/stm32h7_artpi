#pragma once

#include <stdint.h>

struct gpio_i2c_data {
    void *port;
    uint16_t scl_pin;
    uint16_t sda_pin;
    uint16_t retries;
    uint16_t timeout;
    uint16_t delay_us;
};