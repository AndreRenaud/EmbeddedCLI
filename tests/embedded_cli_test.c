#include "acutest.h"
#include "embedded_cli.h"

// Some ANSI escape sequences

#define CSI "\x1b["
#define UP CSI "1A"
#define DOWN CSI "1B"
#define RIGHT CSI "1C"
#define LEFT CSI "1D"
#define HOME CSI "H"
#define END CSI "F"
#define DELETE CSI "3~"
#define CTRL_A "\x01"
#define CTRL_C "\x03"
#define CTRL_E "\x05"
#define CTRL_K "\x0b"
#define CTRL_L "\x0c"
#define CTRL_R "\x12"

static void cli_equals(const struct embedded_cli *cli, const char *line)
{
    const char *cli_line = embedded_cli_get_line(cli);
    TEST_ASSERT_(cli_line != NULL, "No line available");
    TEST_ASSERT_(strcmp(cli_line, line) == 0, "Expected line '%s' got '%s'",
                 line, cli_line);
}

static void test_insert_line(struct embedded_cli *cli, const char *line)
{
    for (; line && *line; line++)
        embedded_cli_insert_char(cli, *line);
}

static void test_simple(void)
{
    struct embedded_cli cli;
    embedded_cli_init(&cli, NULL, NULL, NULL);
    test_insert_line(&cli, "a\n");
    cli_equals(&cli, "a");
}

static void test_argc(void)
{
    struct embedded_cli cli;
    char **argv;
    embedded_cli_init(&cli, NULL, NULL, NULL);
    test_insert_line(&cli, " foo  \t blah blarg   \n");
    TEST_ASSERT(embedded_cli_argc(&cli, &argv) == 3);
    TEST_ASSERT(strcmp(argv[0], "foo") == 0);
    TEST_ASSERT(strcmp(argv[1], "blah") == 0);
    TEST_ASSERT(strcmp(argv[2], "blarg") == 0);
}

static void test_delete(void)
{
    struct embedded_cli cli;
    embedded_cli_init(&cli, NULL, NULL, NULL);
    test_insert_line(&cli, "a\bb\n");
    cli_equals(&cli, "b");
}

static void test_cursor_left(void)
{
    struct embedded_cli cli;
    embedded_cli_init(&cli, NULL, NULL, NULL);
    test_insert_line(&cli, "AB" LEFT "C\n");
    cli_equals(&cli, "ACB");

    embedded_cli_init(&cli, NULL, NULL, NULL);
    test_insert_line(&cli, "AB" LEFT LEFT "C\n");
    cli_equals(&cli, "CAB");

    embedded_cli_init(&cli, NULL, NULL, NULL);
    test_insert_line(&cli, "AB" LEFT LEFT "C\n");
    cli_equals(&cli, "CAB");
}

static void test_cursor_right(void)
{
    struct embedded_cli cli;
    embedded_cli_init(&cli, NULL, NULL, NULL);
    test_insert_line(&cli, "AB" LEFT LEFT RIGHT "C\n");
    cli_equals(&cli, "ACB");
}

#if EMBEDDED_CLI_HISTORY_LEN
static void test_history(void)
{
    struct embedded_cli cli;
    const char *line;
    embedded_cli_init(&cli, NULL, NULL, NULL);
    test_insert_line(&cli, "First\n");
    test_insert_line(&cli, "Second\n");
    test_insert_line(&cli, "Third\n");
    line = embedded_cli_get_history(&cli, 0);
    TEST_ASSERT(line && strcmp(line, "Third") == 0);
    line = embedded_cli_get_history(&cli, 1);
    TEST_ASSERT(line && strcmp(line, "Second") == 0);
    line = embedded_cli_get_history(&cli, 2);
    TEST_ASSERT(line && strcmp(line, "First") == 0);
}

static void test_history_keys(void)
{
    struct embedded_cli cli;
    embedded_cli_init(&cli, NULL, NULL, NULL);
    test_insert_line(&cli, "First\n");
    test_insert_line(&cli, "Second\n");
    test_insert_line(&cli, "Third\n");
    test_insert_line(&cli, UP UP UP DOWN "\n");
    cli_equals(&cli, "Second");
}

static void test_search(void)
{
    struct embedded_cli cli;
    embedded_cli_init(&cli, NULL, NULL, NULL);
    test_insert_line(&cli, "First\n");
    test_insert_line(&cli, "Second\n");
    test_insert_line(&cli, "Third\n");
    test_insert_line(&cli, CTRL_R "Se\n");
    cli_equals(&cli, "Second");
}

static char output[1024];

// Super minimal tty code interpreter so we can work out what
// the user's display looks like
static void output_putchar(void *data, char ch, bool is_last)
{
    static int output_pos = 0;
    static bool have_escape = false;
    static bool have_csi = false;
    (void)is_last;
    (void)data;
    if (ch == '\x1b') {
        have_escape = true;
        have_csi = false;
        return;
    } else if (have_escape && ch == '[') {
        have_csi = true;
        return;
    }

    if (have_csi) {
        // just ignore them
        if (ch >= 'A' && ch <= 'Z') {
            if (ch == 'K') // CLEAR_EOL
                memset(&output[output_pos], 0, sizeof(output) - output_pos);
            have_csi = false;
        }
    } else {
        if (ch == '\b') {
            output_pos = output_pos > 0 ? output_pos - 1 : 0;
        } else if (ch == '\n') {
            output_pos = 0;
            memset(output, 0, sizeof(output));
        } else {
            output[output_pos++] = ch;
            // output[output_pos] = '\0';
        }
    }
}

#define UP_ARROW "\x1b[A"
#define DOWN_ARROW "\x1b[B"

static void test_up_down(void)
{
    struct embedded_cli cli;
    embedded_cli_init(&cli, "prompt> ", output_putchar, NULL);
    embedded_cli_prompt(&cli);
    TEST_ASSERT(strcmp(output, "prompt> ") == 0);
    test_insert_line(&cli, "cmd 1\n");
    test_insert_line(&cli, "cmd 2\n");
    test_insert_line(&cli, "cmd 3\n");
    test_insert_line(&cli, "cmd 4\n");
    embedded_cli_prompt(&cli);
    test_insert_line(&cli, UP_ARROW);
    TEST_ASSERT(strcmp(output, "prompt> cmd 4") == 0);
    test_insert_line(&cli, UP_ARROW);
    TEST_ASSERT(strcmp(output, "prompt> cmd 3") == 0);
    test_insert_line(&cli, DOWN_ARROW);
    test_insert_line(&cli, DOWN_ARROW);
    TEST_ASSERT(strcmp(output, "prompt> ") == 0);
}
#endif

/**
 * The above tests are all quite specific. This test is where we can put any
 * other random ideas/corner cases
 */
static void test_multiple(void)
{
    struct {
        const char *input;
        const char *output;
    } test_cases[] = {
        {"abc" LEFT LEFT "\b\n", "bc"},
        {"abc\b\b\b\b\b\b\b\b\n", ""},
        {"abc\b\b\b\bc\n", "c"},
        {LEFT LEFT RIGHT RIGHT "a" LEFT RIGHT "\n", "a"},
#if EMBEDDED_CLI_HISTORY_LEN
        {UP UP "\n", "c"},
        {"foo" UP "\n", "c"},
#endif
        {"abc" CTRL_C "xyz\n", "xyz"},
        {"abc" CTRL_A "def\n", "defabc"},
        {"abc" CTRL_A "d" CTRL_E "fg\n", "dabcfg"},
        {"abc" HOME "def\n", "defabc"},
        {"abc" HOME "d" END "fg\n", "dabcfg"},
        {"abc" HOME DELETE DELETE "\n", "c"},
        {"abc" LEFT LEFT CTRL_K "\n", "a"},
        {"abc" LEFT LEFT CTRL_L "\n", "abc"},
        {NULL, NULL},
    };

    struct embedded_cli cli;
    embedded_cli_init(&cli, NULL, NULL, NULL);
    for (int i = 0; test_cases[i].input; i++) {
        test_insert_line(&cli, test_cases[i].input);
        cli_equals(&cli, test_cases[i].output);
    }
}

#define MAX_OUTPUT_LEN 50

static void callback(void *data, char ch, bool is_last)
{
    (void)is_last;
    char *d = (char *)data;
    if (d) {
        int len = strlen(d);
        if (len < MAX_OUTPUT_LEN - 1) {
            d[len] = ch;
            d[len + 1] = '\0';
        }
    }
}

static void test_echo(void)
{
    struct embedded_cli cli;
    char output[MAX_OUTPUT_LEN] = "\0";
    embedded_cli_init(&cli, "prompt> ", callback, output);
    embedded_cli_prompt(&cli);
    test_insert_line(&cli, "foo\n");
    TEST_ASSERT(strcmp(output, "prompt> foo\r\n") == 0);
}

static void test_quotes(void)
{
    struct embedded_cli cli;
    char **argv;
    embedded_cli_init(&cli, NULL, NULL, NULL);
    test_insert_line(&cli, "this 'is some' \"text with\" '\"quotes\"' "
                           "'concat'enated \\\"escape\\\" \n");
    TEST_ASSERT(embedded_cli_argc(&cli, &argv) == 6);
    TEST_ASSERT(strcmp(argv[0], "this") == 0);
    TEST_ASSERT(strcmp(argv[1], "is some") == 0);
    TEST_ASSERT(strcmp(argv[2], "text with") == 0);
    TEST_ASSERT(strcmp(argv[3], "\"quotes\"") == 0);
    TEST_ASSERT(strcmp(argv[4], "concatenated") == 0);
    TEST_ASSERT(strcmp(argv[5], "\"escape\"") == 0);
}

static void test_too_many_args(void)
{
    struct embedded_cli cli;
    char **argv;
    embedded_cli_init(&cli, NULL, NULL, NULL);
    test_insert_line(&cli, "a b c d e f g h i j k l m n o p q r s\n");
    TEST_ASSERT(embedded_cli_argc(&cli, &argv) == EMBEDDED_CLI_MAX_ARGC - 1);
    TEST_ASSERT(strcmp(argv[0], "a") == 0);
    TEST_ASSERT(strcmp(argv[1], "b") == 0);
    TEST_ASSERT(strcmp(argv[13], "n") == 0);
    TEST_ASSERT(strcmp(argv[14], "o") == 0);
    TEST_ASSERT(argv[15] == NULL);
}

static void test_max_chars(void)
{
    struct embedded_cli cli;
    embedded_cli_init(&cli, NULL, NULL, NULL);
    // Fill in the buffer
    for (int i = 0; i < EMBEDDED_CLI_MAX_LINE; i++) {
        embedded_cli_insert_char(&cli, 'b');
    }
    // Make sure we cannot insert a character now
    embedded_cli_insert_char(&cli, 'x');
    TEST_ASSERT(cli.buffer[sizeof(cli.buffer) - 2] == 'b');
    TEST_ASSERT(cli.buffer[sizeof(cli.buffer) - 1] == '\0');
    // Make sure we can backspace & change the last character
    embedded_cli_insert_char(&cli, '\b');
    embedded_cli_insert_char(&cli, 'f');
    // There is always a nul at the end, so the one before that should now be
    // an f
    TEST_ASSERT(cli.buffer[sizeof(cli.buffer) - 2] == 'f');
}

TEST_LIST = {
    {"simple", test_simple},
    {"argc", test_argc},
    {"delete", test_delete},
    {"cursor_left", test_cursor_left},
    {"cursor_right", test_cursor_right},
#if EMBEDDED_CLI_HISTORY_LEN
    {"history", test_history},
    {"history_keys", test_history_keys},
    {"search", test_search},
    {"up_down", test_up_down},
#endif
    {"multiple", test_multiple},
    {"echo", test_echo},
    {"quotes", test_quotes},
    {"too_many_args", test_too_many_args},
    {"max_chars", test_max_chars},
    {NULL, NULL},
};