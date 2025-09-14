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

int stm32_uart_device_register(struct tty_device *tty);