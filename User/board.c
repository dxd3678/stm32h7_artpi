#include <shell.h>

#include <board.h>

#include <usart.h>

#include <device/device.h>
#include <device/tty/stm32_uart.h>

void
StartShellTask (void *argument)
{
  shell_init ("ttyS4", "stm32h7>");
  for (;;)
    {
      shell_run ();
    }
}

osThreadId_t shellTaskHandle;
const osThreadAttr_t shellTask_attrbutes = {
  .name = "shellTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t)osPriorityNormal1,
};

void
stm32h7_usart3_init (struct device *dev)
{
  struct tty_device *tty = to_tty_device (dev);
  MX_USART3_UART_Init ();
  tty->baudrate = huart3.Init.BaudRate;
  tty->data_bits = huart3.Init.WordLength;
  tty->parity = huart3.Init.Parity;
  tty->stop_bits = huart3.Init.StopBits;
  dev->private_data = &huart3;

  tty_device_register (tty);
}

void
stm32h7_uart4_init (struct device *dev)
{
  struct tty_device *tty = to_tty_device (dev);
  MX_UART4_Init ();
  tty->baudrate = huart4.Init.BaudRate;
  tty->data_bits = huart4.Init.WordLength;
  tty->parity = huart4.Init.Parity;
  tty->stop_bits = huart4.Init.StopBits;
  dev->private_data = &huart4;
  tty_device_register (tty);
}

static struct stm32_uart stm32h7_uart3 = {
    .tty = {
        .dev = {
            .init_name = "stm32-uart",
            .name = "ttyS3",
            .init = stm32h7_usart3_init,
        },
        .port_num = 3,
        .mode = TTY_MODE_STREAM,
    }
};

static struct stm32_uart stm32h7_uart4 = {
.tty = {
    .dev = {
        .init_name = "stm32-uart",
        .name = "ttyS4",
        .init = stm32h7_uart4_init,
    },
    .parity = 4,
    .mode = TTY_MODE_CONSOLE
    }
};

register_device (stm32h7_uart3, stm32h7_uart3.tty.dev);
register_device (stm32h7_uart4, stm32h7_uart4.tty.dev);