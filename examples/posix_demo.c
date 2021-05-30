#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <termios.h>
#include <unistd.h>

#include "embedded_cli.h"

static char getch(void)
{
    char buf = 0;
    struct termios old = {0};
    if (tcgetattr(0, &old) < 0)
        perror("tcsetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0)
        perror("tcsetattr ICANON");
    if (read(0, &buf, 1) < 0)
        perror("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0)
        perror("tcsetattr ~ICANON");
    return (buf);
}

static void posix_putch(void *data, char ch)
{
    FILE *fp = data;
    fputc(ch, fp);
    fflush(fp);
}

int main(int argc, char **argv)
{
    bool done = false;
    struct embedded_cli cli;

    embedded_cli_init(&cli, "POSIX> ", posix_putch, stdout);
    embedded_cli_prompt(&cli);

    while (!done) {
        char ch = getch();

        if (embedded_cli_insert_char(&cli, ch)) {
            int cli_argc;
            char **cli_argv;
            cli_argc = embedded_cli_argc(&cli, &cli_argv);
            printf("Got %d args\n", cli_argc);
            for (int i = 0; i < cli_argc; i++) {
                printf("Arg %d/%d: '%s'\n", i, cli_argc, cli_argv[i]);
            }
            done = cli_argc >= 1 && (strcmp(cli_argv[0], "quit") == 0);

            if (!done)
                embedded_cli_prompt(&cli);
        }
    }

    return 0;
}