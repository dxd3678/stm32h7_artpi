#include <device/i2c/mpu6050.h>

struct mpu6050_data {
    struct i2c_driver drv;
};

static int mpu6050_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    return 0;
}

static int mpu6050_remove(struct i2c_client *client)
{
    return 0;
}

static void mpu6050_driver_init(struct device_driver *drv)
{
    struct i2c_driver *_drv = to_i2c_driver(drv);
    i2c_register_driver(_drv);
}

static const struct i2c_device_id  mpu6050_ids[] = {
    {
        .name = "mpu6050"
    },
    {

    }
};

static struct mpu6050_data mpu6050_drv = {
    .drv = {
        .drv = {
            .bus = &i2c_bus_type,
            .init = mpu6050_driver_init,
            .name = "mpu6050-device"
        },
        .probe = mpu6050_probe,
        .remove = mpu6050_remove,
        .match_table = mpu6050_ids,
    }
};

register_driver(mpu6050, mpu6050_drv.drv.drv);
