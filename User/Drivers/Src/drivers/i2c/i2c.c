#include <bus.h>
#include <device/driver.h>
#include <device/i2c/i2c.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>

static struct list_head adapter_list = LIST_HEAD_INIT(adapter_list);
static struct list_head driver_list = LIST_HEAD_INIT(driver_list);

static uint8_t next_adapter_nr = 0;

static int i2c_adapter_probe(struct device *dev)
{
    return dev->driver->probe(dev);
}

static int i2c_adapter_remove(struct device *dev)
{
    return dev->driver->remove(dev);
}

static int i2c_device_match(struct device *dev, struct device_driver *drv)
{
    const struct device_match_table *ptr;

    for (ptr = drv->match_ptr; ptr && ptr->compatible; ptr++) {
        if (strcmp(dev->init_name, ptr->compatible) == 0) {
            dev->driver = drv;
            return 1;
        }
    }

    if (dev->init_name && drv->name) {
        if (strcmp(dev->init_name, drv->name) == 0) {
            dev->driver = drv;
            return 1;
        }
    }

    return 0;
}

struct bus_type i2c_bus_type = {
    .name = "i2c",
    .probe = i2c_adapter_probe,
    .remove = i2c_adapter_remove,
    .match = i2c_device_match,
};

int i2c_add_addapter(struct i2c_adapter *adap)
{
    adap->nr = next_adapter_nr++;
    return i2c_register_adapter(adap);
}

int i2c_register_adapter(struct i2c_adapter *adap)
{
    int ret;

    if (!adap->dev.init_name)
        return -EINVAL;

    adap->dev.bus = &i2c_bus_type;

    ret = device_register(&adap->dev);

    if (ret)
        return ret;

    INIT_LIST_HEAD(&adap->client_list);

    list_add_tail(&adap->list, &adapter_list);

    return 0;
}

static int i2c_device_probe(struct device *dev)
{
    struct i2c_client *client = to_i2c_device(dev);
    struct i2c_driver *drv = to_i2c_driver(dev->driver);
    const struct i2c_device_id *id;

    for (id = drv->match_table; id; id++) {
        if (strcmp(client->name, id->name) == 0) {
            client->drv = drv;
            return drv->probe(client, id);
        }
    }
    return -1;
}

static int i2c_device_remove(struct device *dev)
{
    return 0;
}

int i2c_register_driver(struct i2c_driver *drv)
{
    int ret;

    if (!drv || !drv->probe || !drv->remove ||
        !drv->drv.name)
        return -EINVAL;

    drv->drv.probe = &i2c_device_probe;
    drv->drv.remove = &i2c_device_remove;
    drv->drv.bus = &i2c_bus_type;

    ret = driver_register(&drv->drv);
    if (ret)
        return ret;

    INIT_LIST_HEAD(&drv->device_list);

    list_add_tail(&drv->list, &driver_list);

    return 0;
}

int i2c_transfer(struct i2c_adapter *adap, struct i2c_message *msg, unsigned int count)
{
    return adap->algo->master_xfer(adap, msg, count);
}

int i2c_register_device(struct i2c_client *client)
{
    if (!client || 
        !client->adap || 
        !client->addr)
        return -EINVAL;

    client->dev.bus = &i2c_bus_type;

    return device_register(&client->dev);
}

register_bus_type(i2c_bus_type);
