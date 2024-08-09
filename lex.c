/*
** Lexical analyzer.
** Generates manageable tokens from source.
** Also does syntax error checking (unclosed literal, malformed number etc.).
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "head.h"

/* config */
#define LEX_TKS_INIT_CAPACITY       4
#define LEX_TKS_REALLOC_CAPACITY    4
#define DEBUG_PRINT_TOKENS          1

/* dictionary */
static const char* KEYWORDS[] = {
    "class", "and", "as", "async", "await", "break", "continue", "def",
    "del", "elif", "else", "except", "finally", "for", "from", "global",
    "if", "import", "in", "is", "lambda", "nonlocal", "not", "or", "pass",
    "raise", "return", "try", "while", "with", "yield", "def", "end"
};
static const char SYMBOLS[] = { /* flags to start the symbol reader, not the actual symbol defs */
    '(', ')', '{', '}', ':', '-', '+', '*', '/', '#', '=', ','
};
static const char* TYPES[] = {
    "i8", "i16", "i32", "i64",
    "u8", "u16", "u32", "u64",
    "string", "boolean",
};

#define in_str_dict_tab(tab, str) {                                 \
    for (int i = 0; i < sizeof(tab)/sizeof(tab[0]); i++)            \
        if (strcmp(str, tab[i]) == 0)                               \
            return 1;                                               \
    return 0;}


static inline int is_a_symbol(char CHR) {
   for (int i = 0; i < sizeof(SYMBOLS)/sizeof(SYMBOLS[0]); i++) {
        if (CHR == SYMBOLS[i])
            return 1;
   }
   return 0; 
}

static inline int is_a_keyword(char* STR) in_str_dict_tab(KEYWORDS, STR);
static inline int is_a_type(char* STR) in_str_dict_tab(TYPES, STR);
static inline int is_a_bool(char* STR) {return strcmp(STR, "True") == 0 || strcmp(STR, "False") == 0;}

/*}=======================*/


/*
** Creates a new token.
** strdup's the value parameter (heap alloc).
*/
static Token* make_token(TK_Kind kind, char* value, int line, int column, int columns_traversed) {
    Token* tk = malloc(sizeof(Token));
    tk->kind = kind;
    tk->value = strdup(value);
    tk->line = line;
    tk->column = column-strlen(value)+1;
    tk->columns_traversed = columns_traversed;
    tk->next = NULL;
    tk->prev = NULL;
    return tk;
}

/*
** Inserts character into tk_buf (buffer_write).
*/
static inline void insert_char_into_ls(LexState* ls, char CHR) {
    buffer_write(ls->tk_buf, ls->tk_buf->siz, CHR);
}

/*
** Inserts a valid token into the lexer state.
*/
static void insert_tk_into_ls(LexState* ls, Token* tk) {
    if (ls->tks.siz + 1 > ls->tks.cap) {
        ls->tks.cap += LEX_TKS_REALLOC_CAPACITY;
        ls->tks.v = realloc(ls->tks.v, sizeof(void*)*ls->tks.cap);
    }

    /* next and prev */
    if (ls->tks.siz > 0) {
        ls->tks.v[ls->tks.siz-1]->next = tk;
        tk->prev = ls->tks.v[ls->tks.siz-1];
    }

    ls->tks.v[ls->tks.siz] = tk;
    ls->tks.siz++;
}

/*
** Creates a token from the lexer state, into the lexer state.
** (insert_tk_into_ls(make_token(ls info))
*/
static inline void create_tk_into_ls(LexState* ls, TK_Kind kind) {
    insert_tk_into_ls(ls, make_token(kind, ls->tk_buf->data, ls->line, ls->column, ls->columns_traversed));
}

/*
** Token maker for symbols. 
*/
#define tk_symbol(k, v)                      \
    if (ls->tk_buf->siz > 0) {               \
        create_tk_into_ls(ls, TK_Identifier);\
        zero_buffer(ls->tk_buf);             \
    }                                        \
    insert_tk_into_ls(ls, make_token(k, v, ls->line, ls->column, ls->columns_traversed))

static inline int next_char(char** _i, const int steps, const char CHR) {
    if ((*_i)[steps] == CHR)
        return 1;
    return 0;
}

/*}=======================*/
// real code starts here

#define is_valid_chr(CHR) ((CHR >= 'A' && CHR <= 'Z') || (CHR >= 'a' && CHR <= 'z') || (CHR == '_') || (CHR >= '0' && CHR <= '9'))

static void advance(LexState* ls, char** _i) {
    (*_i)++;
    ls->column++;
    ls->columns_traversed++;
}

static void newline(LexState* ls) {
    ls->line++;
    ls->column = 0;
    ls->newline_column = ls->columns_traversed;

    if (ls->in_comment == 1 && ls->is_comment_multiline == 0) ls->in_comment = 0;
}

static void whitespace(LexState* ls) {
    if (ls->tk_buf->siz > 0) {
        create_tk_into_ls(ls, TK_Identifier);
        zero_buffer(ls->tk_buf);
    }
}

/* pause main loop */
static void read_string_literal(LexState* ls, char** _i) {
    char* iptr = *_i;
    char clause_type = *iptr;

    /* creates token if not empty buffer & inserts quote */
    tk_symbol(TK_Quote, ((char[2]){clause_type, '\0'}));

    advance(ls, _i);

    for (;;) {
        switch (*(*_i)) {
            case '\n': case '\r':
            case '\0': {
                /* unclosed string literal error */
                logger_token_error(ls->line, ls->tks.v[ls->tks.siz-1]->columns_traversed-ls->newline_column, "Unclosed string literal.");
            }
            
            case '\\': { /* escape characters */
                switch ((*_i)[1]) { /* the next character */
                    case '\'': case '"': {
                        advance(ls, _i);
                        insert_char_into_ls(ls, *(*_i));
                        break;
                    }
                    case 'n': {
                        insert_char_into_ls(ls, '\n');
                        goto stop;
                    }
                    case '\\': {
                        insert_char_into_ls(ls, '\\');
                        goto stop;
                    }
                    case 't': {
                        insert_char_into_ls(ls, '\t');
                        goto stop;
                    }
                    case 'r': {
                        insert_char_into_ls(ls, '\r');
                        goto stop;
                    }
                    case 'b': {
                        insert_char_into_ls(ls, '\b');
                        goto stop;
                    }
                    case 'f': {
                        insert_char_into_ls(ls, '\f');
                        goto stop;
                    }

                    default: {
                        logger_token_error(ls->line, ls->columns_traversed-ls->newline_column, "Invalid escape character.");
                    }

                    stop:
                        advance(ls, _i);
                        advance(ls, _i);
                        break;
                }
            }

            default: {
                if ((*(*_i)) == clause_type) {
                    /* close string */
                    create_tk_into_ls(ls, TK_String);
                    zero_buffer(ls->tk_buf);
                    tk_symbol(TK_Quote, ((char[2]){clause_type, '\0'})); /* close symbol */
                    return;
                }

                /* regular character */
                insert_char_into_ls(ls, *(*_i));
            }
        }

        advance(ls, _i);
    }
}

/* pause main loop */
static void read_numeric(LexState* ls, char** _i) {

    /* create token if not empty buffer */
    if (ls->tk_buf->siz > 0) {
        create_tk_into_ls(ls, TK_Identifier);
        zero_buffer(ls->tk_buf);
    }

    for (;;) {
        switch (*(*_i)) {
            case '\0':
            case '\n': case '\r':
            case ' ': case '\f': case '\t': case '\v': {
                /* exit function */
                create_tk_into_ls(ls, TK_Numeric);
                zero_buffer(ls->tk_buf);
                return;
            }

            case 'x': {
                /* hexadecimal? */
                if (ls->tk_buf->siz != 1)
                    goto bad_number; /* invalid x placement */

                insert_char_into_ls(ls, *(*_i));
                break;
            }
            
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9': {
                insert_char_into_ls(ls, *(*_i));
                break;
            }
            default: goto bad_number; /* no other characters allowed */

            bad_number:
                    logger_token_error(ls->line, ls->columns_traversed-ls->newline_column, "Malformed number.");
        }

        advance(ls, _i);
    }
}

static void read_symbol(LexState* ls, char** _i) {
    for (;;) {
        switch (*(*_i)) {
            /* single-line comment */
            case '#': ls->in_comment = 1; advance(ls, _i); return;

            /* single char */
            case '(': tk_symbol(TK_OpenParenthesis, "("); return;
            case ')': tk_symbol(TK_CloseParenthesis, ")"); return;
            case '{': tk_symbol(TK_OpenParenthesis, "{"); return;
            case '}': tk_symbol(TK_CloseSquirly, "}"); return; 
            case ':': tk_symbol(TK_Colon, ":"); return;
            case ',': tk_symbol(TK_Comma, ","); return;
            
            /* operators */
            case '+': {
                if (next_char(_i, 1, '=')) {
                    // +=
                    tk_symbol(TK_Increment, "+=");
                    advance(ls, _i);
                    return;
                }
                
                // +
                tk_symbol(TK_Add, "+");
                return;
            }
            case '-': { /* -, -=, -> */
                if (next_char(_i, 1, '>')) {
                    // arrow
                    tk_symbol(TK_Arrow, "->");
                    advance(ls, _i);
                    return;
                }
                if (next_char(_i, 1, '=')) {
                    // -=
                    tk_symbol(TK_Decrement, "-=");
                    advance(ls, _i);
                    return;
                }

                // -
                tk_symbol(TK_Sub, "-");
                return;
            }
            case '*': {
                if (next_char(_i, 1, '=')) {
                    // *=
                    tk_symbol(TK_Multiment, "*=");
                    advance(ls, _i);
                    return;
                }

                // *
                tk_symbol(TK_Mul, "*");
                return;
            }
            case '/': {
                if (next_char(_i, 1, '=')) {
                    // /=
                    tk_symbol(TK_Divement, "/=");
                    advance(ls, _i);
                    return;
                }

                // /
                tk_symbol(TK_Div, "/");
                return;
            }
            /* comparisons */
            case '=': {
                if (next_char(_i, 1, '=')) {
                    // ==
                    tk_symbol(TK_EqualsEquals, "==");
                    advance(ls, _i);
                    return;
                }

                // =
                tk_symbol(TK_Equals, "=");
                return;
            }

        }

        advance(ls, _i);
    }
}

/*
** Main loop.
*/
static void lex_head_loop(LexState* ls, char* txt) {
    char* iptr = txt;

    for (;;) {
        switch (*iptr) {
            case '\0': goto cleanup; /* end of file */
            case '\n': case '\r': { /* new line */
                newline(ls);
            }
            case ' ': case '\f': case '\t': case '\v': {
                whitespace(ls); /* newline is also whitespace */
                break;
            }

            case '\'': {
                /* multiline comment detection */
                if (next_char(&iptr, 1, '\'') && next_char(&iptr, 2, '\'')) { 
                        ls->in_comment = !ls->in_comment;
                        ls->is_comment_multiline = 1;
                        advance(ls, &iptr);
                        advance(ls, &iptr);
                        break;
                }
                // is also a string
            }
            case '"': { /* strings */
                if (ls->in_comment == 1) break;
                read_string_literal(ls, &iptr);
                break;
            }

            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9': {
                if (ls->in_comment == 1) break;
                if (ls->tk_buf->siz > 0)
                    goto is_character; /* if number is part of a variable name */

                read_numeric(ls, &iptr);
                break;
            }

            default: goto is_character;

            /*
            ** Goto here is necessary, as multiple cases need to go to the same code.
            */
            is_character:
                if (ls->in_comment == 1) break;
                if (!is_valid_chr(*iptr)) {
                    if (is_a_symbol(*iptr)) { /* is it a symbol? */
                        read_symbol(ls, &iptr);
                        break;
                    }

                    /* bad character error */
                    logger_token_error(ls->line, ls->columns_traversed-ls->newline_column, "Bad character.");
                }

                /* valid character, not symbol */
                insert_char_into_ls(ls, *iptr);
                break;


            cleanup: /* wraps up lexer, exits function */
                if (ls->tk_buf->siz > 0) {
                    //ls->line--;
                    create_tk_into_ls(ls, TK_Identifier);
                    //ls->line++; /* -- and ++ because it'll spawn it with an extra line */
                    zero_buffer(ls->tk_buf);
                }
                return;
        }

        advance(ls, &iptr);
    }
}

/*
** After the main loop, detect and make new types.
** Also syntax checks.
*/
static void token_checks(LexState* ls) {
    // error check vars
    int parenthesis_balance = 0;

    for (int i = 0; i < ls->tks.siz; i++) {
        Token* tk = ls->tks.v[i];

        switch (tk->kind) {
            case TK_Identifier: {
                if (is_a_keyword(tk->value)) {
                    tk->kind = TK_Keyword;
                    break;
                }
                if (is_a_type(tk->value)) {
                    tk->kind = TK_Type;
                    break;
                }
                if (is_a_bool(tk->value)) {
                    tk->kind = TK_Boolean;
                    break;
                }
                if (strcmp(tk->value, "None") == 0) {
                    tk->kind = TK_None;
                    break;
                }
                break;
            }
            case TK_OpenParenthesis: parenthesis_balance++; break;
            case TK_CloseParenthesis: parenthesis_balance--; break;
        }
    }

    switch (parenthesis_balance) {
        case 0: break;
        default: /* parenthesis balance error */
            logger_token_error(ls->line-1, ls->tks.v[ls->tks.siz-1]->columns_traversed-ls->newline_column+1, "Unclosed scope.");
    }
    
}

/*}=======================*/

/*
** API
*/
LexOut* lex_generate(char* txt) {
    LexOut* lo = malloc(sizeof(LexOut)); /* the final result */
    
    LexState ls = {
        .tk_buf = buffer_create(64),
        .tks = {
            .v = malloc(sizeof(void*)*LEX_TKS_INIT_CAPACITY),
            .siz = 0,
            .cap = LEX_TKS_INIT_CAPACITY,
        },

        .line = 1,
        .column = 1,
        .columns_traversed = 1,
        .newline_column = 1,
        
    };
    zero_buffer(ls.tk_buf);

    /* main code */
    lex_head_loop(&ls, txt);
    token_checks(&ls);

    /* transport state into out */
    lo->siz = ls.tks.siz;
    lo->tks = malloc(sizeof(void*)*lo->siz);
    for (int i = 0; i < ls.tks.siz; i++)
        lo->tks[i] = ls.tks.v[i];

    /* optional printing */
#if(DEBUG_PRINT_TOKENS == 1)
    for (int i = 0; i < lo->siz; i++) {
        Token* tk = lo->tks[i];
        printf("kind: %d. value: %s. :%d:%d:\n", tk->kind, tk->value, tk->line, tk->column);
    }
#endif

    /* clean up memory */
    buffer_free(ls.tk_buf);
    free(ls.tks.v);
    return lo;
}

void lex_free(LexOut* lo) {
    for (int i = 0; i < lo->siz; i++) {
        free(lo->tks[i]->value);
        free(lo->tks[i]);
    }
    free(lo->tks);
    free(lo);
}
