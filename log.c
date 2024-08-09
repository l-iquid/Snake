/*
** Error logger.
! loggings a lil weird rn
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "head.h"


static char* lines = NULL;
static char** formatted_lines = NULL;
static int total_lines = 1;
static char* current_filename = NULL;

static void _newline(int* counter, size_t* line_siz, int* start_line) {
    formatted_lines[*counter] = calloc(*line_siz, 1);
    char* formatted = formatted_lines[*counter];
    *start_line += *line_siz;
    int k = 0;
    int stop_skipping_trailings = 0;
    for (int j = (*start_line)-(*line_siz); j < (*start_line); j++) {
        /* j is the line position, k is the formatted position */
        switch (lines[j]) {
            /* skip all newlines */
            case '\n': case '\r': {
                                      continue;
                                  }
            /* skip trailing whitespaces */
            case ' ': case '\f': case '\t': case '\v': {
                if (stop_skipping_trailings < 1 && lines[j+1] != ' ' && lines[j+1] != '\f' && lines[j+1] != '\t' && lines[j+1] != '\v') {
                    stop_skipping_trailings = 1;
                    continue;
                }
                if (stop_skipping_trailings == 0)
                    continue;
            }

            default: {
                if (stop_skipping_trailings == 0 && k < 1) {
                    stop_skipping_trailings = 1;
                }
            }
        }
        formatted[k] = lines[j];
        k++;
    }
    *line_siz = 1;
    (*counter)++;
}

/*
** API
*/
void logger_init(char *_lines, char *filename) {
    /* cleanup */
    if (lines != NULL) {
        free(lines);
    }
    lines = NULL;
    if (formatted_lines != NULL) {
        for (int i = 0; i < total_lines; i++) {
            free(formatted_lines[i]);
        }
        free(formatted_lines);
        formatted_lines = NULL;
    }
    total_lines = 1;

    lines = strdup(_lines);
    current_filename = filename;

    /* count the total lines */
    char* i = lines;
    while (*i != '\0') {
        switch (*i) {
            case '\n': case '\r': {
                                      total_lines++;
                                      break;
                                  }
          
        }
        *i++;
    }

    /* formatted lines */
    formatted_lines = malloc(sizeof(void*)*total_lines);
    i = lines;
    int counter = 0;
    int start_line = 0;
    size_t line_siz = 1;

    while (*i != '\0') {
        switch (*i) {
            case '\n': case '\r': {
                                      _newline(&counter, &line_siz, &start_line);
                                  }
            default: {
                         line_siz++;
                         break;
                     }
        }
        *i++;
    }

    _newline(&counter, &line_siz, &start_line);
}

void logger_error(int line_num, int start, int end, const char *code, const char *type_of_err) {
    printf("PyToASM: ");
    VGA_YELLOW();
    printf("%s ", current_filename);
    VGA_RED();
    printf("%s Error!\n", type_of_err);
    VGA_RESET();

    if (line_num-1 > 0) {
        printf("%d | ...\n", line_num-1);
    }

    VGA_CYAN();
    printf("%d ", line_num);
    VGA_RESET();
    printf("| %s\n", formatted_lines[line_num-1]);

    /* print spaces before ^ */
        for (int i = 0; i < start+4; i++)
        printf(" ");

    VGA_MAGENTA();
    for (int i = start; i < end; i++)
        printf("^");
    
    char* buf = malloc(strlen(code)+15);
    sprintf(buf, "%s", code);
    printf(" %s\n", buf);
    free(buf);
    
    VGA_RESET();
    if (line_num+1 <= total_lines) {
        printf("%d | ...\n", line_num+1);
    }
    exit(EXIT_FAILURE);
}

void logger_dev_warning(int line_num, const char* type_of_warn) {
    VGA_YELLOW();
    printf("PyToASM: Internal warning in %s; Line %d: %s\n", current_filename, line_num, type_of_warn);
    VGA_RESET();
}
