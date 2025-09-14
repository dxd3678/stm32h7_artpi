#include <bus.h>
#include <device/spi/spi.h>

#include <errno.h>

static struct list_head spi_master_list = LIST_HEAD_INIT(spi_master_list);

static int spi_probe(struct device *dev)
{
    return dev->driver->probe(dev);
}

static int spi_remove(struct device *dev)
{
    return dev->driver->remove(dev);
}

static int spi_match(struct device *dev, struct device_driver *drv)
{
    const struct driver_match_table *ptr;

    if (&spi_bus_type != dev->bus || &spi_bus_type != drv->bus)
        return 0;

    for (ptr = drv->match_ptr; ptr && ptr->compatible; ptr++)
    {
        if (strcmp(ptr->compatible, dev->init_name) == 0)
        {
            dev->driver = drv;
            return 1;
        }
    }

    return 0;
}

struct bus_type spi_bus_type = {
    .name = "spi",
    .probe = spi_probe,
    .remove = spi_remove,
    .match = spi_match,
};

int spi_master_register(struct spi_master *master)
{
    int ret;

    if (!master->dev.init_name)
        return -EINVAL;
    
    master->dev.bus = &spi_bus_type;

    ret = device_register(&master->dev);

    if (ret)
        return ret;

    list_add_tail(&master->list, &spi_master_list);

    return 0;
}

int spi_device_register(struct spi_device *spi)
{
    int ret;

    if (!spi->master)
        return -EINVAL;

    spi->dev.bus = &spi_bus_type;

    ret = device_register(&spi->dev);

    if (ret)
        return ret;

    return 0;
}

int spi_driver_register(struct spi_driver *drv)
{
    // int ret;

    if (!drv || !drv->probe || !drv->remove)
        return -EINVAL;

    drv->driver.bus = &spi_bus_type;

    drv->driver.probe = spi_probe;
    drv->driver.remove = spi_remove;

    return driver_register(&drv->driver);
}

int spi_sync(struct spi_device *spi, struct spi_message *m)
{
    return 0;
}

register_bus_type(spi_bus_type);