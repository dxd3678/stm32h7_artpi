#include <init.h>
#include <bus.h>
#include <list.h>
#include <device/device.h>
#include <device/driver.h>

#include <string.h>
#include <errno.h>

static struct list_head bus_list = LIST_HEAD_INIT(bus_list);

extern struct bus_type *__bus_type_list_start[];
extern struct bus_type *__bus_type_list_end[];

static int virtual_bus_probe(struct device *dev)
{
    int ret;

    struct device_driver *drv = dev->driver;

    ret = drv->probe(dev);
    if (ret)
        dev->driver = NULL;
    return ret;
}

static int virtual_bus_remove(struct device *dev)
{
    struct device_driver *drv = dev->driver;
    drv->remove(dev);
    dev->driver = NULL;
    return 0;
}

static int virtual_bus_match(struct device *dev, struct device_driver *drv)
{
    const struct driver_match_table *ptr;

    if (&virtual_bus_type != dev->bus ||&virtual_bus_type != drv->bus)
        return 0;

    for (ptr = drv->match_ptr; ptr && ptr->compatible; ptr++) {
        if (strcmp(dev->init_name, ptr->compatible) == 0) {
            dev->driver = drv;
            return 1;
        }
    }
    return 0;
}

int bus_register(struct bus_type *bus)
{
    if (!bus || !bus->match || !bus->probe || !bus->remove)
        return -EINVAL;

    list_add_tail(&bus->list, &bus_list);
    return 0;
}

struct bus_type virtual_bus_type = {
    .name = "virtual",
    .probe = virtual_bus_probe,
    .remove = virtual_bus_remove,
    .match = virtual_bus_match,
};

struct bus_type *get_virtual_bus_type()
{
    return &virtual_bus_type;
}

void bus_type_init(void)
{
    struct bus_type **start = __bus_type_list_start;
    struct bus_type **end = __bus_type_list_end;
    int count = ((uint32_t)end - (uint32_t)start) / sizeof(void *);
    struct bus_type *bus;
    int i;

    for (i = 0; i < count; i++) {
        bus = start[i];
        bus_register(bus);
    }
}

register_bus_type(virtual_bus_type);