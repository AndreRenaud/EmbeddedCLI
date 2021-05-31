#include "acutest.h"
#include "embedded_cli.h"

// Some ANSI escape sequences

#define CSI "\x1b["
#define UP CSI "1A"
#define DOWN CSI "1B"
#define RIGHT CSI "1C"
#define LEFT CSI "1D"
#define CTRL_R "\x11"

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

static void test_history(void)
{
    struct embedded_cli cli;
    char *line;
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
    /**
     * Insert three commands, then make sure we can search back for them
     * using Ctrl-R
     */
    test_insert_line(&cli, "First\n");
    test_insert_line(&cli, "Second\n");
    test_insert_line(&cli, "Third\n");
    test_insert_line(&cli, UP UP UP DOWN "\n");
    cli_equals(&cli, "Second");
}

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
        {UP UP "\n", "c"},
        {"foo" UP "\n", "c"},
        {NULL, NULL},
    };

    struct embedded_cli cli;
    embedded_cli_init(&cli, NULL, NULL, NULL);
    for (int i = 0; test_cases[i].input; i++) {
        test_insert_line(&cli, test_cases[i].input);
        cli_equals(&cli, test_cases[i].output);
    }
}

TEST_LIST = {
    {"simple", test_simple},
    {"argc", test_argc},
    {"delete", test_delete},
    {"cursor_left", test_cursor_left},
    {"cursor_right", test_cursor_right},
    {"history", test_history},
    {"history_keys", test_history_keys},
    {"multiple", test_multiple},
    {NULL, NULL},
};