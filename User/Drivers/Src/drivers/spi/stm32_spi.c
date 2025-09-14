#include <device/spi/spi.h>

struct stm32_spi {
    struct device_driver drv;
};

static struct list_head master_list = LIST_HEAD_INIT(master_list);


static size_t stm32_spi_max_transfer_size(struct spi_device *dev)
{
    return 0;
}

static size_t stm32_spi_max_message_size(struct spi_device *dev)
{
    return 0;
}

static int stm32_spi_setup(struct spi_device *spi)
{
    return 0;
}

static int stm32_spi_transfer(struct spi_device *spi, struct spi_message *msg)
{
    return 0;
}

static int stm32_spi_probe(struct device *dev)
{
    struct spi_master *master = to_spi_master(dev);

    master->max_message_size = stm32_spi_max_transfer_size;
    master->max_transfer_size = stm32_spi_max_message_size;
    master->setup = stm32_spi_setup;
    master->transfer = stm32_spi_transfer;

    list_add_tail(&master->list, &master_list);

    return 0;
}

static int stm32_spi_remove(struct device *dev)
{
    return 0;
}

static const struct driver_match_table stm32_spi_ids[] = {
    {
        .compatible = "stm32-spi-controller"
    },
    {

    }
};

static void stm32_spi_init(struct device_driver *drv)
{
    driver_register(drv);
}

static struct stm32_spi stm32_spi_drv = {
    .drv = {
        .name = "stm32-spi-drv",
        .init = stm32_spi_init,
        .probe = stm32_spi_probe,
        .remove = stm32_spi_remove,
        .match_ptr = stm32_spi_ids,
        .bus = &spi_bus_type,
    }
};

register_driver(stm32_spi, stm32_spi_drv.drv);