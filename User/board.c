#include <shell.h>

#include <board.h>

#include <usart.h>

#include <device/device.h>
#include <device/tty/stm32_uart.h>
#include <device/i2c/i2c.h>
#include <device/i2c/gpio-i2c.h>
#include <device/i2c/mpu6050.h>

#include <gpio.h>

void StartShellTask (void *argument)
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

void stm32h7_usart3_init (struct device *dev)
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

void stm32h7_uart4_init (struct device *dev)
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

static void stm32_gpio_i2c_preinit(struct device *dev)
{
    struct i2c_adapter *adap = to_i2c_adapter(dev);
    static struct gpio_i2c_data data = {
        .port = GPIOH,
        .sda_pin = GPIO_PIN_12,
        .scl_pin = GPIO_PIN_11,
    };

    dev_set_drvdata(dev, &data);

    i2c_add_addapter(adap);
}

static void mpu6050_preinit(struct device *dev)
{
  struct i2c_client *client = to_i2c_device(dev);
  i2c_register_device(client);
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

static struct i2c_adapter stm32_gpio_i2c = {
    .dev = {
        .init_name = "stm32-gpio-i2c",
        .init = stm32_gpio_i2c_preinit,
    }
};

static struct mpu6050_device mpu6050 = {
  .dev = {
    .dev = {
      .init = mpu6050_preinit,
      .init_name = "mpu6050-device"
    },
    .adap = &stm32_gpio_i2c,
    .addr = 0x68,
    .name = "mpu6050"
  }
};

register_device(stm32_gpio_i2c, stm32_gpio_i2c.dev);
register_device(mpu6050, mpu6050.dev.dev);
register_device(stm32h7_uart3, stm32h7_uart3.tty.dev);
register_device(stm32h7_uart4, stm32h7_uart4.tty.dev);