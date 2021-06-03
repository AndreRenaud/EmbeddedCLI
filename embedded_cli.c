/**
 * Useful links:
 *    http://www.asciitable.com
 *    https://en.wikipedia.org/wiki/ANSI_escape_code
 */

#include <stdio.h>
#include <string.h>

#include "embedded_cli.h"

#define CTRL_R 0x12

#define CLEAR_EOL "\x1b[0K"
#define MOVE_BOL "\x1b[1G"

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

static void embedded_cli_reset_line(struct embedded_cli *cli)
{
    cli->len = 0;
    cli->cursor = 0;
    cli->counter = 0;
    cli->have_csi = cli->have_escape = false;
#if EMBEDDED_CLI_HISTORY_LEN
    cli->history_pos = -1;
    cli->searching = false;
#endif
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

    embedded_cli_reset_line(cli);
}

static void cli_ansi(struct embedded_cli *cli, int n, char code)
{
    char buffer[5] = {'\x1b', '[', '0' + (n % 10), code, '\0'};
    cli_puts(cli, buffer);
}

static void term_cursor_back(struct embedded_cli *cli, int n)
{
    while (n > 0) {
        int count = n > 9 ? 9 : n;
        cli_ansi(cli, count, 'D');
        n -= count;
    }
}

static void term_cursor_fwd(struct embedded_cli *cli, int n)
{
    while (n > 0) {
        int count = n > 9 ? 9 : n;
        cli_ansi(cli, count, 'C');
        n -= count;
    }
}

#if EMBEDDED_CLI_HISTORY_LEN
static void term_backspace(struct embedded_cli *cli, int n)
{
    // printf("backspace %d ('%s': %d)\n", n, cli->buffer, cli->done);
    while (n--)
        cli_putchar(cli, '\b');
}

static const char *embedded_cli_get_history_search(struct embedded_cli *cli)
{
    for (int i = 0;; i++) {
        const char *h = embedded_cli_get_history(cli, i);
        if (!h)
            return NULL;
        if (strstr(h, cli->buffer))
            return h;
    }
    return NULL;
}
#endif

static void embedded_cli_insert_default_char(struct embedded_cli *cli,
                                             char ch)
{
    // Insert a gap in the buffer for the new character
    memmove(&cli->buffer[cli->cursor + 1], &cli->buffer[cli->cursor],
            cli->len - cli->cursor);
    cli->buffer[cli->cursor] = ch;
    cli->len++;
    cli->buffer[cli->len] = '\0';
    cli->cursor++;

#if EMBEDDED_CLI_HISTORY_LEN
    if (cli->searching) {
        cli_puts(cli, MOVE_BOL CLEAR_EOL "search:");
        const char *h = embedded_cli_get_history_search(cli);
        if (h)
            cli_puts(cli, h);

    } else
#endif
    {
        cli_puts(cli, &cli->buffer[cli->cursor - 1]);
        term_cursor_back(cli, cli->len - cli->cursor);
    }
}

const char *embedded_cli_get_history(struct embedded_cli *cli,
                                     int history_pos)
{
#if EMBEDDED_CLI_HISTORY_LEN
    int pos = 0;

    if (history_pos < 0)
        return NULL;

    // Search back through the history buffer for `history_pos` entry
    for (int i = 0; i < history_pos; i++) {
        int len = strlen(&cli->history[pos]);
        if (len == 0)
            return NULL;
        pos += len + 1;
        if (pos == sizeof(cli->history))
            return NULL;
    }

    return &cli->history[pos];
#else
    (void)cli;
    (void)history_pos;
    return NULL;
#endif
}

#if EMBEDDED_CLI_HISTORY_LEN
static void embedded_cli_extend_history(struct embedded_cli *cli)
{
    int len = strlen(cli->buffer);
    if (len > 0) {
        // If the new entry is the same as the most recent history entry,
        // then don't insert it
        if (strcmp(cli->buffer, cli->history) == 0)
            return;
        memmove(&cli->history[len + 1], &cli->history[0],
                sizeof(cli->history) - len + 1);
        memcpy(cli->history, cli->buffer, len + 1);
        // Make sure it's always nul terminated
        cli->history[sizeof(cli->history) - 1] = '\0';
    }
}

static void embedded_cli_stop_search(struct embedded_cli *cli, bool print)
{
    const char *h = embedded_cli_get_history_search(cli);
    if (h)
        strcpy(cli->buffer, h);
    else
        cli->buffer[0] = '\0';
    cli->len = cli->cursor = strlen(cli->buffer);
    cli->searching = false;
    if (print) {
        cli_puts(cli, MOVE_BOL CLEAR_EOL);
        cli_puts(cli, cli->prompt);
        cli_puts(cli, cli->buffer);
    }
}
#endif

bool embedded_cli_insert_char(struct embedded_cli *cli, char ch)
{
    // If we're inserting a character just after a finished line, clear things
    // up
    if (cli->done) {
        cli->buffer[0] = '\0';
        cli->done = false;
    }
    // printf("Inserting char %d 0x%x '%c' searching: %d\n", ch, ch, ch,
    // cli->searching);
    if (cli->len < (int)sizeof(cli->buffer) - 1) {
        if (cli->have_csi) {
            if (ch >= '0' && ch <= '9' && cli->counter < 100) {
                cli->counter = cli->counter * 10 + ch - '0';
                // printf("cli->counter -> %d\n", cli->counter);
            } else {
                switch (ch) {
                case 'A': {
#if EMBEDDED_CLI_HISTORY_LEN
                    // Backspace over our current line
                    term_backspace(cli, cli->done ? 0 : strlen(cli->buffer));
                    const char *line =
                        embedded_cli_get_history(cli, cli->history_pos + 1);
                    if (line) {
                        int len = strlen(line);
                        cli->history_pos++;
                        // printf("history up %d = '%s'\n", cli->history_pos,
                        // line);
                        strncpy(cli->buffer, line, sizeof(cli->buffer));
                        cli->buffer[sizeof(cli->buffer) - 1] = '\0';
                        cli->len = len;
                        cli->cursor = len;
                        cli_puts(cli, cli->buffer);
                        cli_puts(cli, CLEAR_EOL);
                    } else {
                        embedded_cli_reset_line(cli);
                    }
#endif
                    break;
                }

                case 'B': {
#if EMBEDDED_CLI_HISTORY_LEN
                    term_backspace(cli, cli->done ? 0 : strlen(cli->buffer));
                    const char *line =
                        embedded_cli_get_history(cli, cli->history_pos - 1);
                    if (line) {
                        int len = strlen(line);
                        cli->history_pos--;
                        // printf("history down %d = '%s'\n",
                        // cli->history_pos, line);
                        strncpy(cli->buffer, line, sizeof(cli->buffer));
                        cli->buffer[sizeof(cli->buffer) - 1] = '\0';
                        cli->len = len;
                        cli->cursor = len;
                        cli_puts(cli, cli->buffer);
                        cli_puts(cli, CLEAR_EOL);
                    } else {
                        embedded_cli_reset_line(cli);
                    }
#endif
                    break;
                }

                case 'D':
                    // printf("back %d vs %d\n", cli->cursor, cli->counter);
                    if (cli->counter == 0)
                        cli->counter = 1;
                    if (cli->cursor >= cli->counter) {
                        cli->cursor -= cli->counter;
                        term_cursor_back(cli, cli->counter);
                    }
                    break;
                case 'C':
                    if (cli->counter == 0)
                        cli->counter = 1;
                    if (cli->cursor <= cli->len - cli->counter) {
                        cli->cursor += cli->counter;
                        term_cursor_fwd(cli, cli->counter);
                    }
                    break;
                default:
                    // TODO: Handle more escape sequences
                    break;
                }
                cli->have_csi = cli->have_escape = false;
                cli->counter = 0;
            }
        } else {
            switch (ch) {
            case '\0':
                break;
            case '\x03':
                cli_puts(cli, "^C\n");
                cli_puts(cli, cli->prompt);
                embedded_cli_reset_line(cli);
                cli->buffer[0] = '\0';
                break;
            case '\b': // Backspace
            case 0x7f: // backspace?
                       // printf("backspace %d\n", cli->cursor);
#if EMBEDDED_CLI_HISTORY_LEN
                if (cli->searching)
                    embedded_cli_stop_search(cli, true);
#endif
                if (cli->cursor > 0) {
                    memmove(&cli->buffer[cli->cursor - 1],
                            &cli->buffer[cli->cursor],
                            cli->len - cli->cursor + 1);
                    cli->cursor--;
                    cli->len--;
                    term_cursor_back(cli, 1);
                    cli_puts(cli, &cli->buffer[cli->cursor]);
                    cli_puts(cli, " ");
                    term_cursor_back(cli, cli->len - cli->cursor + 1);
                }
                break;
            case CTRL_R:
#if EMBEDDED_CLI_HISTORY_LEN
                if (!cli->searching) {
                    cli_puts(cli, "\nsearch:");
                    cli->searching = true;
                }
#endif
                break;
            case '\x1b':
#if EMBEDDED_CLI_HISTORY_LEN
                if (cli->searching)
                    embedded_cli_stop_search(cli, true);
#endif
                cli->have_csi = false;
                cli->have_escape = true;
                cli->counter = 0;
                break;
            case '[':
                if (cli->have_escape)
                    cli->have_csi = true;
                else
                    embedded_cli_insert_default_char(cli, ch);
                break;
            case '\n':
                cli_putchar(cli, '\n');
                break;
            default:
                if (ch > 0)
                    embedded_cli_insert_default_char(cli, ch);
            }
        }
    }
    cli->done = (ch == '\n');

    if (cli->done) {
#if EMBEDDED_CLI_HISTORY_LEN
        if (cli->searching)
            embedded_cli_stop_search(cli, false);
        embedded_cli_extend_history(cli);
#endif
        embedded_cli_reset_line(cli);
    }
    // printf("Done with char 0x%x (done=%d)\n", ch, cli->done);
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
    bool in_escape = false;
    char in_string = '\0';
    if (!cli->done)
        return 0;
    for (size_t i = 0; i < sizeof(cli->buffer) && cli->buffer[i] != '\0';
         i++) {

        // If we're escaping this character, just absorb it regardless
        if (in_escape) {
            in_escape = false;
            continue;
        }

        if (in_string) {
            // If we're finishing a string, blank it out
            if (cli->buffer[i] == in_string) {
                memmove(&cli->buffer[i], &cli->buffer[i + 1],
                        sizeof(cli->buffer) - i - 1);
                in_string = '\0';
                i--;
            }
            continue;
        }

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

        if (cli->buffer[i] == '\\') {
            // Absorb the escape character
            memmove(&cli->buffer[i], &cli->buffer[i + 1],
                    sizeof(cli->buffer) - i - 1);
            i--;
            in_escape = true;
        }

        // If we're starting a new string, absorb the character and shuffle
        // things back
        if (cli->buffer[i] == '\'' || cli->buffer[i] == '"') {
            in_string = cli->buffer[i];
            memmove(&cli->buffer[i], &cli->buffer[i + 1],
                    sizeof(cli->buffer) - i - 1);
            i--;
        }

    }
    *argv = cli->argv;
    return pos;
}

void embedded_cli_prompt(struct embedded_cli *cli)
{
    cli_puts(cli, cli->prompt);
}