#pragma once

#include <FreeRTOS.h>
#include <cmsis_os2.h>

extern osThreadId_t shellTaskHandle;

extern const osThreadAttr_t shellTask_attrbutes;

void StartShellTask(void *argument);