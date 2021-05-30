#include "embedded_cli.c"

int LLVMFuzzerTestOneInput(const char *data, int size)
{
    struct embedded_cli cli;

    embedded_cli_init(&cli, NULL, NULL, NULL);

    for (int i = 0; i < size; i++)
        embedded_cli_insert_char(&cli, data[i]);

    return 0;
}
