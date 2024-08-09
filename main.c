/*
** CLI interface.
*/
#include "head.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* command strings */
#define NO_ARGUMENTS_STRING "PyToASM: A python to assembly compiler.\nType --help for commands or --info for more information.\n"
#define HELP_STRING         "--version (--v) -- Version string.\n--playground (--p) -- Playground mode.\n"
#define PLAYGROUND_STRING   "PyToASM CLI mode.\nType RUN to run code or EXIT.\n"
#define VERSION_STRING      "PyToASM Version %s. (C) All rights reserved.\n"

/* for commands that are just string printers */
#define printf_esc(...) \
    printf(__VA_ARGS__);\
    exit(0)

/*}=============================*/

/*
** Run any string.
*/
static void run_string(char* STR) {
   compile_text(STR, "CLI"); 
}



/*
==================================
COMMANDS
==================================
*/

static void playground() {
    // CLI mode
    printf(PLAYGROUND_STRING);

    Buffer* line_buffer = buffer_create(256); /* buffer for importing each line */
    Buffer* inputs = buffer_create(128); /* buffer for every line */
    zero_buffer(line_buffer);
    zero_buffer(inputs);

    for (;;) {
        VGA_YELLOW();
        printf(">>> "); /* fancy >>> before typing input */
        VGA_RESET();
        fgets(line_buffer->data, 256, stdin); /* grab the input */
        buffer_realign_siz(line_buffer); /* realign buffer size */
        
        /* comparisons */
        if (strcmp(line_buffer->data, "EXIT\n") == 0) {
            break;
        }
        
        if (strcmp(line_buffer->data, "RUN\n") == 0) {
            /* run code */
            run_string(inputs->data);

            /* reset buffers */
            zero_buffer(inputs);
            zero_buffer(line_buffer);
            continue;
        }

        /* Is a line of code. */
        if (line_buffer->siz+inputs->cap+1 > inputs->cap) {
            buffer_expand(inputs, inputs->cap + 128);
        }
        buffer_write_long(inputs, inputs->siz, line_buffer->data);

        zero_buffer(line_buffer);
    }

    exit(0);
}


/*
==================================
ENTRY
==================================
*/
int main(int argc, char** argv) {
    if (argc < 2) {
        /* No parameters. */
        printf_esc(NO_ARGUMENTS_STRING);
    }

    /* There is a parameter. */
    const char* cmd = argv[1];

    if (strcmp(cmd, "--help") == 0) {
        printf_esc(HELP_STRING);
    }
    if (strcmp(cmd, "--playground") == 0 || strcmp(cmd, "--p") == 0) {
        playground();
    }
    if (strcmp(cmd, "--version") == 0 || strcmp(cmd, "--v") == 0) {
        printf_esc(VERSION_STRING, "unknown");
    }

    /* Invalid command. */
    fprintf(stderr, "Invalid command %s.", cmd);
    return 1;
}
