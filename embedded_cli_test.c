#include "acutest.h"
#include "embedded_cli.h"

static void test_insert_line(struct embedded_cli *cli, char *line)
{
	int len = strlen(line);
	for (int i = 0; i < len; i++) {
		embedded_cli_insert_char(cli, line[i]);
		//TEST_ASSERT(embedded_cli_insert_char(cli, line[i]) == (i < len - 1));
	}
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
	char *line;
	embedded_cli_init(&cli, NULL, NULL, NULL);
	embedded_cli_insert_char(&cli, 'a');
	embedded_cli_insert_char(&cli, '\b');
	embedded_cli_insert_char(&cli, 'b');
	embedded_cli_insert_char(&cli, '\n');
	line = embedded_cli_get_line(&cli);
	TEST_ASSERT_(line, "no line found");
	TEST_ASSERT_(strcmp(line, "b\n") == 0, "backspace didn't work: '%s'", line);
}

TEST_LIST = {
	{"argc", test_argc},
	{"delete", test_delete},
	{NULL, NULL},
};