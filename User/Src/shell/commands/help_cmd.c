#include <shell.h>

static int do_help(int argc, char *argv[])
{
    if (argc > 1)
        shell_show_cmd_help(argv[1], --argc, ++argv);
    else 
        shell_show_available_cmd();

    return 0;
}

shell_command_register(help, "show available commands", do_help, do_help);
