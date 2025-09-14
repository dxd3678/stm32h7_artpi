#pragma once

struct shell_command {
    const char *name;
    const char *help_str;
    int (*func)(int argc, char *argv[]);
    void (*help_fn)(int argc, char *argv[]);
};

extern const struct shell_command __shell_cmd_list_start[];
extern const struct shell_command __shell_cmd_list_end[];

#define SHELL_CMD_LIST_START __shell_cmd_list_start
#define SHELL_CMD_LIST_END __shell_cmd_list_end

#define SHELL_CMD_COUNT ((size_t)(SHELL_CMD_LIST_END - SHELL_CMD_LIST_START))

int shell_init(const char *tty_name, const char *prompt);
int shell_puts(const char *str);
int shell_printf(const char *fmt, ...);
void shell_run(void);

int shell_show_available_cmd();
int shell_show_cmd_help(const char *name, int argc, char *argv[]);

#define shell_command_register(name_str, help, cb, show_help_fn)  \
static const struct shell_command name_str##_cmd __attribute__((used, section("shell_cmd_list"))) = { \
    .name = #name_str,  \
    .help_str = help,   \
    .func = cb,  \
    .help_fn = show_help_fn \
}
