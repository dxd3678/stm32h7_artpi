#pragma once

#include <device/device.h>

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
    uint16_t dma_align;
    uint32_t mode_bits;
    uint32_t max_speed;
    uint32_t min_speed;
    uint16_t flags;
    union {
        bool savle;
        bool target;
    };
    size_t (*max_transfer_size)(struct spi_device *dev);
    size_t (*max_message_size)(struct spi_device *dev);
    int (*setup)(struct spi_device *dev);
    int (*transfer)(struct spi_device *dev, struct spi_message *msg);
}; 


struct spi_transfer {
    const void *tx_buf;
    void *rx_buf;
    unsigned len;
    struct list_head transfer_list;
    uint32_t speed;
};

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

