#pragma once

#include "../list.h"

#include <stddef.h>
#include <stdbool.h>

struct device;
struct bus_type;

struct device_match_table {
    const char *compatible;
    uint32_t data;
};

struct device_driver {
    const char *name;
    struct list_head list;
    int (*probe)(struct device *dev);
    int (*remove)(struct device *dev);
    size_t private_data_size;
    void *private_data;
    bool private_data_auto_alloc;
    const struct device_match_table *match_ptr;
    struct bus_type *bus;
    void (*init)(struct device_driver *);
};

extern struct device_driver *__device_driver_list_start[];
extern struct device_driver *__device_driver_list_end[];

int driver_register(struct device_driver *drv);
int driver_probe(struct device *dev);

#define register_driver(__name, __drv)  \
static struct device_driver *__name##_driver __attribute__((used, section("device_driver_list"))) = &__drv

void driver_init(void);



