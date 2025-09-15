#include <device/i2c/i2c.h>
#include <device/i2c/gpio-i2c.h>

#include <gpio.h>

static void delay_us(uint32_t us)
{
    HAL_Delay(us / 1000);
    if (us % 1000) {
        uint32_t start = HAL_GetTick();
        uint32_t elapsed = 0;
        
        while (elapsed < (us % 1000)) {
            elapsed = (HAL_GetTick() - start) * 1000;
        }
    }
}

static void scl_high(struct gpio_i2c_data *data)
{
    HAL_GPIO_WritePin((GPIO_TypeDef *)data->port, data->scl_pin, GPIO_PIN_SET);
}

static void scl_low(struct gpio_i2c_data *data)
{
    HAL_GPIO_WritePin((GPIO_TypeDef *)data->port, data->scl_pin, GPIO_PIN_RESET);
}

static void sda_high(struct gpio_i2c_data *data)
{
    HAL_GPIO_WritePin((GPIO_TypeDef *)data->port, data->sda_pin, GPIO_PIN_SET);
}

static void sda_low(struct gpio_i2c_data *data)
{
    HAL_GPIO_WritePin((GPIO_TypeDef *)data->port, data->sda_pin, GPIO_PIN_RESET);
}

static int sda_read(struct gpio_i2c_data *data)
{
    return HAL_GPIO_ReadPin((GPIO_TypeDef *)data->port, data->sda_pin) == GPIO_PIN_SET;
}

static int scl_read(struct gpio_i2c_data *data)
{
    return HAL_GPIO_ReadPin((GPIO_TypeDef *)data->port, data->scl_pin) == GPIO_PIN_SET;
}

static void i2c_start(struct gpio_i2c_data *data)
{
    sda_high(data);
    scl_high(data);
    delay_us(data->delay_us);
    sda_low(data);
    delay_us(data->delay_us);
    scl_low(data);
    delay_us(data->delay_us);
}

static void i2c_stop(struct gpio_i2c_data *data)
{
    sda_low(data);
    scl_high(data);
    delay_us(data->delay_us);
    sda_high(data);
    delay_us(data->delay_us);
}

static int i2c_wait_ack(struct gpio_i2c_data *data)
{
    int timeout = data->timeout;
    int ack;
    
    sda_high(data);
    
    while (scl_read(data) == 0 && timeout--) {
        delay_us(1);
    }
    
    if (timeout <= 0) {
        return -1;
    }
    
    scl_high(data);
    delay_us(data->delay_us);
    
    while (timeout--) {
        ack = sda_read(data);
        if (!ack) {
            scl_low(data);
            delay_us(data->delay_us);
            return 0;
        }
        delay_us(1);
    }
    
    scl_low(data);
    delay_us(data->delay_us);
    return -1;
}

static void i2c_ack(struct gpio_i2c_data *data)
{
    int timeout = data->timeout;
    
    sda_low(data);
    
    while (scl_read(data) == 0 && timeout--) {
        delay_us(1);
    }
    
    if (timeout <= 0) {
        return;
    }
    
    scl_high(data);
    delay_us(data->delay_us);
    scl_low(data);
    delay_us(data->delay_us);
    sda_high(data);
}

static void i2c_nack(struct gpio_i2c_data *data)
{
    int timeout = data->timeout;
    
    sda_high(data);
    
    while (scl_read(data) == 0 && timeout--) {
        delay_us(1);
    }
    
    if (timeout <= 0) {
        return;
    }
    
    scl_high(data);
    delay_us(data->delay_us);
    scl_low(data);
    delay_us(data->delay_us);
}

static int i2c_write_byte(struct gpio_i2c_data *data, uint8_t byte)
{
    int i;
    int ack;
    int timeout = data->timeout;
    
    for (i = 7; i >= 0; i--) {
        if (byte & (1 << i)) {
            sda_high(data);
        } else {
            sda_low(data);
        }
        delay_us(data->delay_us);
        
        timeout = data->timeout;
        while (scl_read(data) == 0 && timeout--) {
            delay_us(1);
        }
        
        if (timeout <= 0) {
            return -1;
        }
        
        scl_high(data);
        delay_us(data->delay_us);
        scl_low(data);
        delay_us(data->delay_us);
    }
    
    ack = i2c_wait_ack(data);
    return ack;
}

static uint8_t i2c_read_byte(struct gpio_i2c_data *data, int ack)
{
    int i;
    uint8_t byte = 0;
    int timeout = data->timeout;
    
    sda_high(data);
    
    for (i = 7; i >= 0; i--) {
        timeout = data->timeout;
        while (scl_read(data) == 0 && timeout--) {
            delay_us(1);
        }
        
        if (timeout <= 0) {
            return 0;
        }
        
        scl_high(data);
        delay_us(data->delay_us);
        if (sda_read(data)) {
            byte |= (1 << i);
        }
        scl_low(data);
        delay_us(data->delay_us);
    }
    
    if (ack) {
        i2c_ack(data);
    } else {
        i2c_nack(data);
    }
    
    return byte;
}

static int stm32_master_xfer(struct i2c_adapter *adap, struct i2c_message *msgs, int num)
{
    struct gpio_i2c_data *data = (struct gpio_i2c_data *)adap->algo_data;
    int i, ret;
    int retries = data->retries;
    
    if (!data || !msgs || num <= 0) {
        return -1;
    }
    
retry:
    for (i = 0; i < num; i++) {
        struct i2c_message *msg = &msgs[i];
        uint8_t addr = msg->addr << 1;
        
        if (msg->flags & I2C_M_RD) {
            addr |= 1;
        }
        
        if (!(msg->flags & I2C_M_NOSTART))
            i2c_start(data);
        
        ret = i2c_write_byte(data, addr);
        if (ret < 0) {
            i2c_stop(data);
            if (retries--) {
                goto retry;
            }
            return ret;
        }
        
        if (msg->flags & I2C_M_RD) {
            int j;
            for (j = 0; j < msg->len; j++) {
                msg->buf[j] = i2c_read_byte(data, j < msg->len - 1);
            }
        } else {
            int j;
            for (j = 0; j < msg->len; j++) {
                ret = i2c_write_byte(data, msg->buf[j]);
                if (ret < 0) {
                    i2c_stop(data);
                    if (retries--) {
                        goto retry;
                    }
                    return ret;
                }
            }
        }
    }
    
    i2c_stop(data);
    
    return num;
}

static int stm32_smbus_xfer(struct i2c_adapter *adap, unsigned short addr,
                     unsigned short flags, char read_write,
                     unsigned char command, int size, void *data)
{
    struct gpio_i2c_data *i2c_data = (struct gpio_i2c_data *)adap->algo_data;
    int ret;
    uint8_t addr_byte = addr << 1;
    
    if (read_write) {
        addr_byte |= 1;
    }
    
    i2c_start(i2c_data);
    
    ret = i2c_write_byte(i2c_data, addr_byte);
    if (ret < 0) {
        i2c_stop(i2c_data);
        return ret;
    }
    
    if (size != SMBUS_QUICK) {
        ret = i2c_write_byte(i2c_data, command);
        if (ret < 0) {
            i2c_stop(i2c_data);
            return ret;
        }
    }
    
    switch (size) {
    case SMBUS_QUICK:
        break;
        
    case SMBUS_BYTE:
        if (read_write) {
            uint8_t *byte = (uint8_t *)data;
            *byte = i2c_read_byte(i2c_data, 0);
        } else {
            uint8_t byte = *(uint8_t *)data;
            ret = i2c_write_byte(i2c_data, byte);
            if (ret < 0) {
                i2c_stop(i2c_data);
                return ret;
            }
        }
        break;
        
    case SMBUS_BYTE_DATA:
        if (read_write) {
            uint8_t *byte = (uint8_t *)data;

            i2c_start(i2c_data);
            
            ret = i2c_write_byte(i2c_data, addr_byte | 1);
            if (ret < 0) {
                i2c_stop(i2c_data);
                return ret;
            }
            
            *byte = i2c_read_byte(i2c_data, 0);
        } else {
            uint8_t byte = *(uint8_t *)data;
            ret = i2c_write_byte(i2c_data, byte);
            if (ret < 0) {
                i2c_stop(i2c_data);
                return ret;
            }
        }
        break;
        
    case SMBUS_WORD_DATA:
        if (read_write) {
            uint16_t *word = (uint16_t *)data;
            uint8_t low, high;
            
            i2c_start(i2c_data);
            
            ret = i2c_write_byte(i2c_data, addr_byte | 1);
            if (ret < 0) {
                i2c_stop(i2c_data);
                return ret;
            }

            low = i2c_read_byte(i2c_data, 1);
            
            high = i2c_read_byte(i2c_data, 0);
            
            *word = (high << 8) | low;
        } else {
            uint16_t word = *(uint16_t *)data;
            uint8_t low = word & 0xFF;
            uint8_t high = (word >> 8) & 0xFF;
            
            ret = i2c_write_byte(i2c_data, low);
            if (ret < 0) {
                i2c_stop(i2c_data);
                return ret;
            }
            
            ret = i2c_write_byte(i2c_data, high);
            if (ret < 0) {
                i2c_stop(i2c_data);
                return ret;
            }
        }
        break;
        
    case SMBUS_BLOCK_DATA:
        if (read_write) {
            uint8_t *block = (uint8_t *)data;
            uint8_t length;
            int i;
            
            i2c_start(i2c_data);
            
            ret = i2c_write_byte(i2c_data, addr_byte | 1);
            if (ret < 0) {
                i2c_stop(i2c_data);
                return ret;
            }
            
            length = i2c_read_byte(i2c_data, 1);
            
            for (i = 0; i < length; i++) {
                block[i] = i2c_read_byte(i2c_data, i < length - 1);
            }
        } else {
            uint8_t *block = (uint8_t *)data;
            uint8_t length = block[0];
            int i;
            
            ret = i2c_write_byte(i2c_data, length);
            if (ret < 0) {
                i2c_stop(i2c_data);
                return ret;
            }
            
            for (i = 1; i <= length; i++) {
                ret = i2c_write_byte(i2c_data, block[i]);
                if (ret < 0) {
                    i2c_stop(i2c_data);
                    return ret;
                }
            }
        }
        break;
        
    default:
        i2c_stop(i2c_data);
        return -1;
    }
    
    i2c_stop(i2c_data);
    
    return 0;
}

static unsigned int stm32_functionality(struct i2c_adapter *adap)
{
    return I2C_FUNC_I2C | I2C_FUNC_PROTOCOL_MANGLING;
}

static void stm32_gpio_init(struct gpio_i2c_data *data)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    if (!data || !data->port) {
        return;
    }
    
    GPIO_InitStruct.Pin = data->scl_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init((GPIO_TypeDef *)data->port, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = data->sda_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init((GPIO_TypeDef *)data->port, &GPIO_InitStruct);
    
    scl_high(data);
    sda_high(data);
    
    if (data->delay_us == 0) {
        data->delay_us = 5;
    }

    if (data->timeout == 0) {
        data->timeout = 1000;
    }
}


static const struct i2c_algorithm stm32_gpio_algo = {
    .master_xfer = stm32_master_xfer,
    .smbus_xfer = stm32_smbus_xfer,
    .functionality = stm32_functionality,
};

static int stm32_gpio_adapter_probe(struct device *dev)
{
    struct i2c_adapter *adap = to_i2c_adapter(dev);
    struct gpio_i2c_data *data = dev_get_drvdata(dev);

    stm32_gpio_init(data);

    adap->algo = &stm32_gpio_algo;
    
    return 0;
}

static int stm32_gpio_adapter_remove(struct device *dev)
{
    struct i2c_adapter *adap = to_i2c_adapter(dev);

    adap->algo = NULL;

    return 0;
}

static void stm32_gpio_adapter_init(struct device_driver *drv)
{
    driver_register(drv);
}

static const struct device_match_table stm32_gpio_i2c_ids[] = {
    {
        .compatible = "stm32-gpio-i2c"
    },
    {
       
    }
};

static struct device_driver stm32_gpio_i2c_drv = {
    .name = "stm32-gpio-i2c",
    .init = stm32_gpio_adapter_init,
    .bus = &i2c_bus_type,
    .match_ptr = stm32_gpio_i2c_ids,
    .probe = stm32_gpio_adapter_probe,
    .remove = stm32_gpio_adapter_remove,
};

register_driver(stm32_gpio_i2c, stm32_gpio_i2c_drv);