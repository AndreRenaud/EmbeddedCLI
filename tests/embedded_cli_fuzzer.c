#include "embedded_cli.c"

int LLVMFuzzerTestOneInput(const char *data, int size)
{
    struct embedded_cli cli;
    char **argv;

    embedded_cli_init(&cli, NULL, NULL, NULL);

    for (int i = 0; i < size; i++)
        embedded_cli_insert_char(&cli, data[i]);

    embedded_cli_argc(&cli, &argv);
    embedded_cli_get_history(&cli, 0);
    return 0;
}
