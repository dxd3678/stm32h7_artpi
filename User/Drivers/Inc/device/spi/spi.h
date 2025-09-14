#pragma once

#include <device/device.h>
#include <device/driver.h>

#include "../../list.h"

#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct spi_master;

struct spi_device {
    struct device dev;
    struct list_head list;
    struct spi_master *master;
    uint32_t max_speed;
    uint8_t chip_select;
    uint8_t bits_per_word;
    uint8_t mode;
    int irq;
    void *master_data;
};

struct spi_message {
    struct list_head transfers;
    struct spi_device *spi;
    struct list_head queue;
    void (*complete)(void *ctx);
    void *ctx;
    unsigned frame_length;
    unsigned actual_length;
    void *state;
};

struct spi_master {
    struct device dev;
    struct list_head list;
    int16_t bus_num;
    uint16_t num_chipselect;
    union {
        bool slave;
        bool target;
    };
    size_t (*max_transfer_size)(struct spi_device *spi);
    size_t (*max_message_size)(struct spi_device *spi);
    int (*setup)(struct spi_device *spi);
    int (*transfer)(struct spi_device *spi, struct spi_message *msg);
    uint32_t mode;
}; 


struct spi_transfer {
    const void *tx_buf;
    void *rx_buf;
    unsigned len;
    struct list_head transfer_list;
    uint32_t speed;
};

struct spi_device_id {
    char name[32];
    uint32_t data;
};

struct spi_driver {
    const struct spi_device_id *id_table;
    struct device_driver driver;
    int (*probe)(struct spi_device *spi);
    int (*remove)(struct spi_device *spi);
};

extern struct bus_type spi_bus_type;

#define to_spi_master(d)    container_of(d, struct spi_master, dev)

static inline void spi_message_init(struct spi_message *m)
{
    memset(m, 0, sizeof(*m));
    INIT_LIST_HEAD(&m->transfers);
}

static inline void spi_message_add_tail(struct spi_transfer *t, struct spi_message *m)
{
    list_add_tail(&t->transfer_list, &m->transfers);
}

static inline void spi_message_init_with_transfers(struct spi_message *m, struct spi_transfer *xfers, size_t num)
{
    int i;
    spi_message_init(m);

    for (i = 0; i < num; i++) {
        spi_message_add_tail(xfers, m);
    }
}

int spi_sync(struct spi_device *spi, struct spi_message *m);

static inline int spi_sync_message(struct spi_device *spi, struct spi_transfer *xfers, size_t num)
{
    struct spi_message m;
    spi_message_init_with_transfers(&m, xfers, num);
    return spi_sync(spi, &m);
}

int spi_master_register(struct spi_master *master);
int spi_device_register(struct spi_device *spi);
