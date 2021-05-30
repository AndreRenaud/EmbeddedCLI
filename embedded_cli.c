#include <string.h>
#include <stdio.h>

#include "embedded_cli.h"

static void cli_puts(struct embedded_cli *cli, const char *s)
{
    if (!cli->putchar)
        return;
    for (; *s; s++)
        cli->putchar(cli->cb_data, *s);
}

static void cli_putchar(struct embedded_cli *cli, char ch)
{
    if (cli->putchar)
        cli->putchar(cli->cb_data, ch);
}

void embedded_cli_init(struct embedded_cli *cli, char *prompt, void (*putchar)(void *data, char ch), void *cb_data)
{
    memset(cli, 0, sizeof(*cli));
    cli->putchar = putchar;
    cli->cb_data = cb_data;
    if (prompt) {
        strncpy(cli->prompt, prompt, sizeof(cli->prompt));
        cli->prompt[sizeof(cli->prompt) - 1] = '\0';
    }
}

bool embedded_cli_insert_char(struct embedded_cli *cli, char ch)
{
    //printf("Inserting char %d 0x%x\n", ch, ch);
    if (cli->len < sizeof(cli->buffer) - 1) {
        switch (ch) {
            case '\b': // Backspace
            case 0x7f: // backspace?
                if (cli->len > 0) {
                    cli->len--;
                    cli->buffer[cli->len] = '\0';
                }
                cli_puts(cli, "\x1b[1D \x1b[1D");
                break;
            default:
                cli_putchar(cli, ch);
                cli->buffer[cli->len] = ch;
                cli->len++;
        }
        cli->buffer[cli->len] = '\0';
    }
    cli->done = (ch == '\n');
    if (cli->done)
        cli->len = 0;
    return cli->done;
}

char *embedded_cli_get_line(struct embedded_cli *cli)
{
    if (!cli->done)
        return NULL;
    return cli->buffer;
}

static bool is_whitespace(char ch)
{
    return (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');
}

int embedded_cli_argc(struct embedded_cli *cli, char ***argv)
{
    int pos = 0;
    bool in_arg = false;
    if (!cli->done)
        return 0;
    for (int i = 0; i < sizeof(cli->buffer) && cli->buffer[i] != '\0'; i++) {
        // Skip over whitespace, and replace it with nul terminators so
        // each argv is nul terminated
        if (is_whitespace(cli->buffer[i])) {
            if (in_arg)
                cli->buffer[i] = '\0';
            in_arg = false;
            continue;
        }
        if (!in_arg) {
            cli->argv[pos] = &cli->buffer[i];
            pos++;
            in_arg = true;
        }
    }
    *argv = cli->argv;
    return pos;
}

void embedded_cli_prompt(struct embedded_cli *cli)
{
    cli_puts(cli, cli->prompt);
}