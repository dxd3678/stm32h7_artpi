#include <device/driver.h>
#include <bus.h>
#include <list.h>
#include <init.h>

#include <errno.h>
#include <stdlib.h>

static struct list_head driver_list = LIST_HEAD_INIT(driver_list);

int driver_register(struct device_driver *drv)
{
    if (!drv ||
        !drv->probe ||
        !drv->remove ||
        !drv->name)
        return -EINVAL;

    if (drv->private_data_size && drv->private_data_auto_alloc) {
        drv->private_data = malloc(drv->private_data_size);
        if (!drv->private_data)
            return -ENOMEM;
    }
    
    list_add_tail(&drv->list, &driver_list);
    device_probe(drv);
    return 0;
}

int driver_probe(struct device *dev)
{
    struct device_driver *drv;
    
    list_for_each_entry(drv, &driver_list, list)
    {
        if (drv->bus->match(dev, drv)) {
            drv->bus->probe(dev);
        }
    }
    return -1;
}

void driver_init()
{
    struct device_driver **start = __device_driver_list_start;
    struct device_driver **end = __device_driver_list_end;
    int count = ((uint32_t)end - (uint32_t)start) / sizeof(void *);
    int i;
    struct device_driver *drv;
    
    for (i = 0; i < count; i++)
    {
        drv = start[i];
        drv->init(drv);
    }
}