#include "acutest.h"
#include "embedded_cli.h"

// Some ANSI escape sequences

#define CSI "\x1b["
#define LEFT_ARROW CSI "1D"
#define RIGHT_ARROW CSI "1C"

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
    test_insert_line(&cli, "AB" LEFT_ARROW "C\n");
    cli_equals(&cli, "ACB");

    embedded_cli_init(&cli, NULL, NULL, NULL);
    test_insert_line(&cli, "AB" LEFT_ARROW LEFT_ARROW "C\n");
    cli_equals(&cli, "CAB");

    embedded_cli_init(&cli, NULL, NULL, NULL);
    test_insert_line(&cli, "AB" LEFT_ARROW LEFT_ARROW "C\n");
    cli_equals(&cli, "CAB");
}

static void test_cursor_right(void)
{
    struct embedded_cli cli;
    embedded_cli_init(&cli, NULL, NULL, NULL);
    test_insert_line(&cli, "AB" LEFT_ARROW LEFT_ARROW RIGHT_ARROW "C\n");
    cli_equals(&cli, "ACB");
}

TEST_LIST = {
    {"simple", test_simple},
    {"argc", test_argc},
    {"delete", test_delete},
    {"cursor_left", test_cursor_left},
    {"cursor_right", test_cursor_right},
    {NULL, NULL},
};