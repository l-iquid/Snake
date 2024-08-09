/*
** Main internal handler of files.
*/
#include <stdio.h>
#include "head.h"

void compile_text(char* txt, char* filename) {
    if (strlen(txt) < 1) {
        VGA_YELLOW();
        printf("PyToASM: No code to process.\n");
        VGA_RESET();
        return;
    }
    logger_init(txt, filename);
    /* parse */
    LexOut* lo = lex_generate(txt);
    AbstractSyntaxTree* ast = parse_generate(lo);

    /* free up memory */
    lex_free(lo);
    parse_free(ast);
}