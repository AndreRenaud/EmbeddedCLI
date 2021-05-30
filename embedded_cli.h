#ifndef EMBEDDED_CLI
#define EMBEDDED_CLI

#include <stdbool.h>

/**
 * Maximum number of bytes to accept in a single line
 */
#define EMBEDDED_CLI_MAX_LINE 120

/**
 * Maximum number of bytes to retain of history data
 * Undefine this to remove support for history
 */
#define EMBEDDED_CLI_HISTORY_LEN 1000

/**
 * What is the maximum number of arguments we reserve space for
 */
#define EMBEDDED_CLI_MAX_ARGC 20

/**
 * Maximum number of bytes in the prompt
 */
#define EMBEDDED_CLI_MAX_PROMPT_LEN 10

struct embedded_cli {
    /**
     * Internal buffer. This should not be accessed directly, use the
     * access functions below
     */
    char buffer[EMBEDDED_CLI_MAX_LINE];

    /**
     * Number of characters in buffer at the moment
     */
    int len;

    /**
     * Position of the cursor
     */
    int cursor;

    /**
     * Have we just parsed a full line?
     */
    bool done;

    /**
     * Callback function to output a single character to the user
     */
    void (*putchar)(void *data, char ch);

    /**
     * Data to provide to the putchar callback
     */
    void *cb_data;

    bool have_escape;
    bool have_csi;

    /**
     * counter of the value for the CSI code
     */
    int counter;

    char *argv[EMBEDDED_CLI_MAX_ARGC];
    int argc;

    char prompt[EMBEDDED_CLI_MAX_PROMPT_LEN];
};

/**
 * Start up the Embedded CLI subsystem. This should only be called once.
 */
void embedded_cli_init(struct embedded_cli *, char *prompt,
                       void (*putchar)(void *data, char ch), void *cb_data);

/**
 * Adds a new character into the buffer. Returns true if
 * the buffer should now be processed
 */
bool embedded_cli_insert_char(struct embedded_cli *cli, char ch);

/**
 * Returns the nul terminated internal buffer. This will
 * return NULL if the buffer is not yet complete
 */
const char *embedded_cli_get_line(const struct embedded_cli *cli);

/**
 * Parses the internal buffer and returns it as an argc/argc combo
 * @return number of values in argv (maximum of EMBEDDED_CLI_MAX_ARGC)
 */
int embedded_cli_argc(struct embedded_cli *cli, char ***argv);

/**
 * Outputs the CLI prompt
 * This should be called after @ref embedded_cli_argc or @ref
 * embedded_cli_get_line has been called and the command fully processed
 */
void embedded_cli_prompt(struct embedded_cli *cli);

#endif