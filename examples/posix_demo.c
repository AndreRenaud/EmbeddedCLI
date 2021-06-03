/**
 * Example of using EmbeddedCLI in a posix tty environment.
 * This is useful as a local test for new functionality
 */
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <termios.h>
#include <unistd.h>

#include "embedded_cli.h"

static struct embedded_cli cli;

/**
 * This function retrieves exactly one character from stdin,
 * in character-by-character mode (as opposed to reading a full line)
 */
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

static void intHandler(int dummy)
{
    (void)dummy;
    embedded_cli_insert_char(&cli, '\x03');
}

/**
 * This function outputs a single character to stdout, to be used as the
 * callback from embedded cli
 */
static void posix_putch(void *data, char ch)
{
    FILE *fp = data;
    fputc(ch, fp);
    fflush(fp);
}

int main(void)
{
    bool done = false;

    /**
     * Start up the Embedded CLI instance with the appropriate
     * callbacks/userdata
     */
    embedded_cli_init(&cli, "POSIX> ", posix_putch, stdout);
    embedded_cli_prompt(&cli);

    /* Capture Ctrl-C */
    signal(SIGINT, intHandler);

    while (!done) {
        char ch = getch();

        /**
         * If we have entered a command, try and process it
         */
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