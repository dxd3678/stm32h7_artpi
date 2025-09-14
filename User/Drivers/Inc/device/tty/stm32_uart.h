#pragma once

#ifdef STM32G4
#include <stm32g4xx.h>
#include <stm32g4xx_hal.h>
#include <stm32g4xx_hal_uart.h>
#elif STM32H7
#include <stm32h750xx.h>
#include <stm32h7xx_hal.h>
#include <stm32h7xx_hal_uart.h>
#endif

#include <ring.h>

struct stm32_uart {
    struct tty_device tty;
    uint8_t buf[128];
    size_t buf_len;
    bool is_open;
    struct ring ringbuf;
    void *tid;
    bool tx_cplt;
    uint8_t chart;
};