#pragma once

typedef struct {
    int (*init)(void);
}initcall_t;


#define early_initcall(_name, fn)   \
static initcall_t _name __attirbute__((used, section("early_init_list"))) = {  \
    .init = fn, \
}



