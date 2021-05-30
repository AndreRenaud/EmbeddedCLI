#include <stdio.h>
#include <string.h>

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

void embedded_cli_init(struct embedded_cli *cli, char *prompt,
                       void (*putchar)(void *data, char ch), void *cb_data)
{
    memset(cli, 0, sizeof(*cli));
    cli->putchar = putchar;
    cli->cb_data = cb_data;
    if (prompt) {
        strncpy(cli->prompt, prompt, sizeof(cli->prompt));
        cli->prompt[sizeof(cli->prompt) - 1] = '\0';
    }
}

static void term_cursor_back(struct embedded_cli *cli, int n)
{
    char buffer[10];
    while (n > 0) {
        int count = n > 9 ? 9 : n;
        buffer[0] = '\x1b';
        buffer[1] = '[';
        buffer[2] = '0' + count;
        buffer[3] = 'D';
        buffer[4] = '\0';
        cli_puts(cli, buffer);
        n -= count;
    }
}

static void term_cursor_fwd(struct embedded_cli *cli, int n)
{
    char buffer[10];
    while (n > 0) {
        int count = n > 9 ? 9 : n;
        buffer[0] = '\x1b';
        buffer[1] = '[';
        buffer[2] = '0' + count;
        buffer[3] = 'C';
        buffer[4] = '\0';
        cli_puts(cli, buffer);
        n -= count;
    }
}

static void embedded_cli_insert_default_char(struct embedded_cli *cli,
                                             char ch)
{
    // Insert a gap in the buffer for the new character
    memmove(&cli->buffer[cli->cursor + 1], &cli->buffer[cli->cursor],
            cli->len - cli->cursor);
    cli->buffer[cli->cursor] = ch;
    cli->len++;
    cli->buffer[cli->len] = '\0';

    cli_puts(cli, &cli->buffer[cli->cursor]);
    cli->cursor++;
    term_cursor_back(cli, cli->len - cli->cursor);
}

static void embedded_cli_reset_line(struct embedded_cli *cli)
{
    cli->len = 0;
    cli->cursor = 0;
    cli->counter = 0;
    cli->have_csi = cli->have_escape = false;
}

bool embedded_cli_insert_char(struct embedded_cli *cli, char ch)
{
    //printf("Inserting char %d 0x%x '%c'\n", ch, ch, ch);
    if (cli->len < sizeof(cli->buffer) - 1) {
        if (cli->have_csi) {
            if (ch >= '0' && ch <= '9' && cli->counter < 100) {
                cli->counter = cli->counter * 10 + ch - '0';
                //printf("cli->counter -> %d\n", cli->counter);
            }
            else {
                switch (ch) {
                case 'D':
                    //printf("back %d vs %d\n", cli->cursor, cli->counter);
                    if (cli->cursor >= cli->counter) {
                        cli->cursor -= cli->counter;
                        term_cursor_back(cli, cli->counter);
                    }
                    cli->have_csi = cli->have_escape = false;
                    cli->counter = 0;
                    break;
                case 'C':
                    if (cli->cursor <= cli->len - cli->counter) {
                        cli->cursor+=cli->counter;
                        term_cursor_fwd(cli, cli->counter);
                    }
                    cli->have_csi = cli->have_escape = false;
                    cli->counter = 0;
                    break;
                default:
                    // TODO: Handle more escape sequences
                    cli->have_csi = cli->have_escape = false;
                    break;
                }
            }
        } else {
            switch (ch) {
            case '\0':
                break;
            case '\b': // Backspace
            case 0x7f: // backspace?
                if (cli->cursor > 0) {
                    memmove(&cli->buffer[cli->cursor],
                            &cli->buffer[cli->cursor + 1],
                            cli->len - cli->cursor);
                    cli->cursor--;
                    cli->len--;
                }
                cli_puts(cli, "\x1b[1D \x1b[1D");
                break;
            case '\x1b':
                cli->have_csi = false;
                cli->have_escape = true;
                cli->counter = 0;
                break;
            case '[':
                if (cli->have_escape)
                    cli->have_csi = true;
                break;
            case '\n':
                if (cli->done)
                    cli->buffer[0] = '\0';
                break;
            default:
                if (ch > 0)
                    embedded_cli_insert_default_char(cli, ch);
            }
        }
    }
    cli->done = (ch == '\n');
    if (cli->done)
        embedded_cli_reset_line(cli);
    //printf("Done with char 0x%x (done=%d)\n", ch, cli->done);
    return cli->done;
}

const char *embedded_cli_get_line(const struct embedded_cli *cli)
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