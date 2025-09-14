#pragma once

#include "list.h"

struct device;
struct device_driver;

struct bus_type {
    const char *name;
    int (*probe)(struct device *dev);
    int (*remove)(struct device *dev);
    int (*match)(struct device *dev, struct device_driver *drv);
    struct list_head list;
};

extern struct bus_type virtual_bus_type;

#define register_bus_type(_bus) \
static struct bus_type __attribute__((used, section("bus_type_list"))) *_##_bus = &_bus

void bus_type_init(void);
int device_probe(struct device_driver *);