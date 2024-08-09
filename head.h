/*
** Internal header. Not meant for API use.
*/
#pragma once
#include <stdio.h>
#include <string.h>

/* utilities for coloring text */
#define VGA_YELLOW() printf("\033[93m")
#define VGA_RESET() printf("\033[0m")
#define VGA_RED() printf("\033[91m")
#define VGA_CYAN() printf("\033[96m")
#define VGA_MAGENTA() printf("\033[95m");

/* buffer.c */
typedef struct Buffer {
    char* data;
    size_t siz;
    size_t cap; /* capacity */
} Buffer;

Buffer* buffer_create(size_t cap);
void zero_buffer(Buffer* buf);
void buffer_free(Buffer* buf);
void buffer_expand(Buffer* buf, size_t to);
void buffer_write(Buffer* buf, size_t location, char CHR);
void buffer_write_long(Buffer* buf, size_t location, char* STR); /* Write a whole string at the location. */
char buffer_read(Buffer* buf, size_t location);
#define buffer_realign_siz(_buf) _buf->siz = strlen(_buf->data)

/* log.c */
void logger_init(char* _lines, char* filename);
void logger_error(int line_num, int start, int end, const char* code, const char* type_of_err);
void logger_dev_warning(int line_num, const char* type_of_warn); 
#define logger_token_error(line_num, start, code) logger_error(line_num, start, start+1, code, "Syntax")
#define logger_parse_error(line_num, start, code) logger_error(line_num, start, start+1, code, "Parse")

/* head.c */

void compile_text(char* txt, char* filename);

/* lex.c */
typedef enum TK_Kind {
    TK_Identifier,
    TK_String,
    TK_Numeric,
    TK_Boolean,
    TK_Keyword,
    TK_Type,
    TK_Global,
    TK_None,

    /* symbol */
    TK_OpenParenthesis,
    TK_CloseParenthesis,
    TK_OpenSquirly,
    TK_CloseSquirly,
    TK_Quote,
    TK_Colon,
    TK_Arrow, // ->
    TK_Add,
    TK_Sub,
    TK_Mul,
    TK_Div,
    TK_Increment,
    TK_Decrement,
    TK_Multiment,
    TK_Divement,
    TK_Equals,
    TK_EqualsEquals,
    TK_Comma,
} TK_Kind;
typedef struct Token {
    TK_Kind kind;
    char* value;

    struct Token* next;
    struct Token* prev;

    int line;
    int column;

    int columns_traversed;
} Token;

typedef struct LexState {
    Buffer* tk_buf; /* buffer for scanning */
    struct {
        Token** v;
        size_t siz;
        size_t cap;
    } tks;

    int line;
    int column;
    int columns_traversed;
    int newline_column;
    int in_comment;
    int is_comment_multiline;
} LexState;

typedef struct LexOut {
    Token** tks;
    size_t siz;
} LexOut;

LexOut* lex_generate(char* txt);
void lex_free(LexOut* lo);

/* parse.c */
typedef enum ND_Kind {
    ND_Unknown, // root or completely unknown
    ND_Undefined, // anything undefined (TODO'S)
    /* literals */
    ND_StringLiteral,
    ND_NumberLiteral,
    ND_BooleanLiteral,
    
    ND_GenericKeyword, // any keyword undefined (or fillers, or doesn't need its own type)
    
    /* statement */
    ND_VarDeclStatement,
    ND_VarReassignStatement,
    ND_FunctionDefStatement,
    ND_ReturnStatement,
    ND_IfStatement,
    ND_PassStatement,

    /* expression */
    ND_EqualsExpression,
    ND_IdentifierExpression,
    ND_StringCastExpression,
    ND_ArithmeticExpression,
    ND_ArgumentListExpression,
    ND_ExplicitArgumentExpression, // a: type, has a type expression underneath
    ND_ArgumentExpression, // literal/expression
    ND_TypeResolveExpression,
    ND_ConditionalExpression,
    ND_VarSeperationExpression, // ,

    /* expression statement */
    ND_CallExpressionStatement,
} ND_Kind;



typedef struct Node {
    ND_Kind kind;
    char* value;
    
    struct Node* prev;
    struct {
        struct Node** refs;
        size_t siz;
        size_t cap;
    } next;

    /* etc */
    int line;
    int column;
    int columns_traversed;
} Node;

typedef enum ScopeKind {
    ScopeUndefined, // root is this
    ScopeClause,
    ScopeTemporaryExpr, // temporary scope for processing expressions
} ScopeKind;

typedef struct Scope {
    ScopeKind kind;
    Node* node; /* the node the scope refers to */
} Scope;

typedef struct ParseVariable {
    char* name;
    Node* node;
} ParseVariable;

typedef struct ParseState {
    struct {
        Node** ptrs;
        size_t siz;
        size_t cap;
    } nodes;
    struct {
        Scope** ptrs;
        size_t siz;
        size_t cap;
    } scopes;

    Node* root; /* every node is below this */
    Node* current_node; /* The current relevant node, used for parenting etc. */
    ND_Kind current_statement; /* the current, most top level statement as of now */
    ND_Kind current_expression; /* the current expression being dealt with */
    Token* current_token; /* the current token that is being dealt with to make statements/expressions/literals */
    struct {
        ParseVariable** v;
        size_t siz;
        size_t cap;
    } vars_allowed_in_scope; /* All the defined variables allowed in this specific scope */
} ParseState;

typedef struct AbstractSyntaxTree {
    Node** nodes;
    size_t siz;
} AbstractSyntaxTree;

AbstractSyntaxTree* parse_generate(LexOut* lo);
void parse_free(AbstractSyntaxTree* ast);
