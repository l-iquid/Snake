/*
** Abstract Syntax Tree generator.
** Parses the tokens into a syntax tree, and detects a lot of syntax issues.



TODO ISSUES:
    - FunctionDefStatement isn't freed properly.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "head.h"

/* config */
#define AST_INIT_CAPACITY       4
#define AST_REALLOC_CAPACITY    4
#define DEBUG_PRINT_NODES       1
/*}==================================*/

/*
** Creates/allocates a node with no parent.
*/
static Node* create_node(ND_Kind kind, Token* tk) {
    Node* nd = malloc(sizeof(Node));
    nd->kind = kind;
    nd->value = strdup(tk->value);
    nd->line = tk->line;
    nd->column = tk->column;
    nd->columns_traversed = tk->columns_traversed;
    nd->prev = NULL;
    nd->next.siz = 0;
    nd->next.cap = 4;
    nd->next.refs = malloc(sizeof(void*)*4);
    return nd;
}


/*
** Puts a node into ps->nodes.ptrs.
** Also reallocates if needed.
*/
static void put_node_into_ps(ParseState* ps, Node* nd) {
    if (ps->nodes.siz + 1 > ps->nodes.cap) {
        ps->nodes.cap += AST_REALLOC_CAPACITY;
        ps->nodes.ptrs = realloc(ps->nodes.ptrs, sizeof(void*)*ps->nodes.cap);
    }

    ps->nodes.ptrs[ps->nodes.siz] = nd;
    ps->nodes.siz++;
}

/*
** Puts the <child> into <parent>->next.refs and reallocates if necessary.
** <child>->prev is overriden to <parent>.
*/
static void set_node_parent(Node* parent, Node* child) {
    if (parent->next.siz + 1 > parent->next.cap) {
        parent->next.cap += 2;
        parent->next.refs = realloc(parent->next.refs, sizeof(void*)*parent->next.cap);
    }

    parent->next.refs[parent->next.siz] = child;
    parent->next.siz++;

    child->prev = parent;
}

/* combines put_node_into_ps and set_node_parent */
#define autoset_node_parent(_nd) put_node_into_ps(ps, _nd); set_node_parent(ps->current_node, _nd)



/*
** Is the given variable name in the variable tray?
*/
static int is_var_in_var_tray(ParseState* ps, char* name) {
    for (int i = 0; i < ps->vars_allowed_in_scope.siz; i++)
        if (strcmp(ps->vars_allowed_in_scope.v[i]->name, name) == 0)
            return 1;
    return 0;
}

/*
** Inserts a variable name into the variable tray, and reallocates if necessary.
** Also throws an error if a variable is already in it.
*/
static void insert_var_into_tray(ParseState* ps, char* name, Node* nd) {
    if (is_var_in_var_tray(ps, name)) {
        logger_parse_error(ps->current_token->line, ps->current_token->columns_traversed, "Variable defined twice.");
    }

    if (ps->vars_allowed_in_scope.siz + 1 > ps->vars_allowed_in_scope.cap) {
        ps->vars_allowed_in_scope.cap += 2;
        ps->vars_allowed_in_scope.v = realloc(ps->vars_allowed_in_scope.v, sizeof(void*)*ps->vars_allowed_in_scope.cap);
    }


    ps->vars_allowed_in_scope.v[ps->vars_allowed_in_scope.siz] = malloc(sizeof(ParseVariable));
    ps->vars_allowed_in_scope.v[ps->vars_allowed_in_scope.siz]->name = strdup(name);
    ps->vars_allowed_in_scope.v[ps->vars_allowed_in_scope.siz]->node = nd;
    ps->vars_allowed_in_scope.siz++;
}


/*
** Deletes and deallocates the given variable from the var tray.
*/
static void wipe_var_from_var_tray(ParseState* ps, size_t location) {
    free(ps->vars_allowed_in_scope.v[location]->name);
    free(ps->vars_allowed_in_scope.v[location]);
    ps->vars_allowed_in_scope.v[location] = NULL;
    ps->vars_allowed_in_scope.siz--;
}


/*
** Creates/allocates a scope.
*/
static Scope* create_scope(ScopeKind kind, Node* nd) {
    Scope* scp = malloc(sizeof(Scope));
    scp->kind = kind;
    scp->node = nd;
    return scp;
}

/*
** Puts scope into ps->scopes.ptrs.
** Also reallocates if needed.
*/
static void put_scope_into_ps(ParseState* ps, Scope* scp) {
    if (ps->scopes.siz + 1 > ps->scopes.cap) {
        ps->scopes.cap += 2;
        ps->scopes.ptrs = realloc(ps->scopes.ptrs, sizeof(void*)*ps->scopes.cap);
    }

    ps->scopes.ptrs[ps->scopes.siz] = scp;
    ps->scopes.siz++;
}

/*
** Removes and frees the scope.
*/
static void kill_scope(ParseState* ps, size_t location) {
    /* wipe variables in the scope from the tray */
    for (int i = 0; i < ps->vars_allowed_in_scope.siz; i++) {
        if (ps->vars_allowed_in_scope.v[i]->node == ps->scopes.ptrs[location]->node) {
            wipe_var_from_var_tray(ps, i);
        }
    }
    free(ps->scopes.ptrs[location]);
    ps->scopes.ptrs[location] = NULL;
    ps->scopes.siz--;
}

/* Kills the top most scope. */
#define kill_this_scope(_ps) kill_scope(_ps, _ps->scopes.siz-1)
/* Access the second top scope. */
#define second_this_scope(_ps) _ps->scopes.ptrs[_ps->scopes.siz-2]
/* Access the top most scope. */
#define this_scope(_ps) _ps->scopes.ptrs[_ps->scopes.siz-1]
/* Jump the ps->current_node back to this scope */
#define jump_back_to_this_scope(_ps) _ps->current_node = this_scope(_ps)->node

/* sets ps->current_expression and ps->current_statement to normal */
static void erase_tmp_state(ParseState* ps) {
    ps->current_statement = ND_Unknown;
    ps->current_expression = ND_Unknown;
}


/*}=========================================================================================*/
//* real code starts here

/* Advance ps->current_token towards the next in line. */
#define token_advance(_ps) _ps->current_token = _ps->current_token->next

/*
** Expressions
*/

static void ArgumentListExpression(ParseState* ps) {
    Node* child = create_node(ND_ArgumentListExpression, ps->current_token);
    child->value = "";
    autoset_node_parent(child);
    ps->current_node = child;

    /* create a scope */
    Scope* scp = create_scope(ScopeTemporaryExpr, child);
    put_scope_into_ps(ps, scp);
    ps->current_expression = ND_ArgumentListExpression;
}

static void TypeResolveExpression(ParseState* ps) {
    Node* child = create_node(ND_TypeResolveExpression, ps->current_token);
    autoset_node_parent(child);
    jump_back_to_this_scope(ps);
    ps->current_node = child;
    ps->current_expression = ND_TypeResolveExpression;
}

static void ExplicitArgumentExpression(ParseState* ps) {
    Node* child = create_node(ND_ExplicitArgumentExpression, ps->current_token);
    autoset_node_parent(child);
    ps->current_node = child;

    /* error checks */
    if (ps->current_token->next == NULL || ps->current_token->next->kind != TK_Colon) {
        logger_parse_error(ps->current_token->line, ps->current_token->columns_traversed, "Invalid argument definition (No colon).");
    }
    token_advance(ps); // go to colon
    if (ps->current_token->next == NULL || ps->current_token->next->kind != TK_Type) { /* same scheise */
        logger_parse_error(ps->current_token->line, ps->current_token->columns_traversed, "Invalid argument definition (No type).");
    }
    token_advance(ps); // go to actual type

    ps->current_expression = ND_ExplicitArgumentExpression;
    TypeResolveExpression(ps); /* ExplicitArgumentExpression requires TypeResolveExpression to be under */
}

static void EqualsExpression(ParseState* ps) {
    ps->current_expression = ND_EqualsExpression;
}

static void VarSeperationExpression(ParseState* ps) {
    Node* child = create_node(ND_VarSeperationExpression, ps->current_token);
    autoset_node_parent(child);
    ps->current_node = child;
    ps->current_expression = ND_VarSeperationExpression;
}

/*
** Statements
*/

static void VarDeclStatement(ParseState* ps) {
    /* VarReassignStatement is decided here, no func for it */
    const int is_cur_node_in_var_tray = is_var_in_var_tray(ps, ps->current_node->value);
    ps->current_node->kind = is_cur_node_in_var_tray ? ND_VarReassignStatement : ND_VarDeclStatement;
    ps->current_statement = is_cur_node_in_var_tray ? ND_VarReassignStatement : ND_VarDeclStatement;

    /* var seperation support */
    if (ps->current_node->prev != NULL && ps->current_node->prev->kind == ND_VarSeperationExpression) {
        Node* prev = ps->current_node->prev;
        while (prev->kind == ND_VarSeperationExpression || prev->kind == ND_IdentifierExpression) {
            if (prev->kind == ND_IdentifierExpression) {
                const int is_prev_in_var_tray = is_var_in_var_tray(ps, prev->value);
                prev->kind = is_prev_in_var_tray ? ND_VarReassignStatement : ND_VarDeclStatement;
            }
            prev = prev->prev;
        }
    }

    if (is_cur_node_in_var_tray < 1) /* oopsies cant define twice */
        insert_var_into_tray(ps, ps->current_node->value, ps->current_node);
    EqualsExpression(ps);
}

static void FunctionDefStatement(ParseState* ps) {
    if (ps->current_expression != ND_Unknown) {
        logger_parse_error(ps->current_token->line, ps->current_token->columns_traversed, "Invalid expression.");
    }
    if (ps->current_statement != ND_Unknown) {
        logger_parse_error(ps->current_token->line, ps->current_token->columns_traversed, "Invalid statement.");
    }

    ps->current_statement = ND_FunctionDefStatement;

    // build node
    Node* child = create_node(ND_FunctionDefStatement, ps->current_token);
    child->value = ps->current_token->next->value;
    autoset_node_parent(child);
    ps->current_node = child;

    // build scope
    Scope* scp = create_scope(ScopeClause, child);
    put_scope_into_ps(ps, scp);

    token_advance(ps); /* function name would be IdentifierExpression without this */
}

/*}==================================*/
//* handlers are next (funcs that directly interact with the main loop & call the statement/expression functions)

/* TK_Identifier */
static void identifier_handler(ParseState* ps) {
    int jump_back_to_this_scope_after = 0;
    int erase_tmp_state_after = 0;

    switch (ps->current_node->kind) {
        /*
        ** FYI if the case doesn't have a return;, it'll do the default case below
        */

        case ND_ArgumentListExpression: {

            switch (ps->current_statement) { /* background check the statement */
                case ND_FunctionDefStatement: {
                    ExplicitArgumentExpression(ps);
                    break;
                }

                default: {
                    //! later
                    break;
                }
            }

            return;
        }

        case ND_EqualsExpression: {

            switch (ps->current_statement) { /* background check the statement */
                case ND_VarReassignStatement: case ND_VarDeclStatement: {
                    /* = this, go back to scope */
                    jump_back_to_this_scope_after = 1;
                    erase_tmp_state_after = 1;
                    break;
                }

                default: {
                    //! later
                    break;
                }
            }
            break;
        }

        default: break;
    }

    /* default case */
    Node* child = create_node(ND_IdentifierExpression, ps->current_token);
    autoset_node_parent(child);
    ps->current_node = child;
    ps->current_expression = ND_IdentifierExpression;

    if (jump_back_to_this_scope_after)
        jump_back_to_this_scope(ps);
    if (erase_tmp_state_after)
        erase_tmp_state(ps);
}


/* TK_keyword */
static void keyword_handler(ParseState* ps) {
    
    if (strcmp(ps->current_token->value, "def") == 0) {
        FunctionDefStatement(ps);
    }

}


/* TK_Equals */
static void equals_handler(ParseState* ps) {

    switch (ps->current_node->kind) {
        case ND_VarSeperationExpression: case ND_IdentifierExpression: {
            VarDeclStatement(ps);
            break;
        }
        

        default: {
            logger_parse_error(ps->current_token->line, ps->current_token->columns_traversed, "Invalid symbol.");
            break;
        }
    }

    /* equals expression */
    Node* child = create_node(ND_EqualsExpression, ps->current_token);
    autoset_node_parent(child);
    ps->current_node = child;
}

/* TK_OpenParenthesis */
static void open_parenthesis_handler(ParseState* ps) {
    
    switch (this_scope(ps)->node->kind) {
        case ND_FunctionDefStatement: {
            ArgumentListExpression(ps);
            break;
        }

        default: break;
    }

}

/* TK_CloseParenthesis */
static void close_parenthesis_handler(ParseState* ps) {

    switch (this_scope(ps)->node->kind) {
        case ND_ArgumentListExpression: {
            /* end the argument list */
            kill_this_scope(ps);
            jump_back_to_this_scope(ps);
        }

        default: break;
    }

}

/* TK_Comma */
static void comma_handler(ParseState* ps) {

    switch (this_scope(ps)->node->kind) {
        case ND_ArgumentListExpression: {
            /* jump back for the next argument */
            jump_back_to_this_scope(ps);
            return;
        }

        default: break;
    }

    switch (ps->current_expression) {
        case ND_IdentifierExpression: {
            VarSeperationExpression(ps);
            return;
        }

        default: logger_parse_error(ps->current_token->line, ps->current_token->columns_traversed, "Invalid symbol.");
    }

}

/* TK_Numeric, TK_String, TK_Boolean */
static void literal_handler(ParseState* ps) {
    ND_Kind chosen_kind = ND_Unknown;
    switch (ps->current_token->kind) {
        case TK_Numeric: chosen_kind = ND_NumberLiteral; break;
        case TK_String: chosen_kind = ND_StringLiteral; break;
        case TK_Boolean: chosen_kind = ND_BooleanLiteral; break;
        default: break;
    }
    Node* child = create_node(chosen_kind, ps->current_token);
    autoset_node_parent(child);
    ps->current_node = child;

    switch (ps->current_expression) {
        case ND_EqualsExpression: {
            /* = this, go back to scope */
            jump_back_to_this_scope(ps);
            erase_tmp_state(ps);
            break;
        }
        default: {
            logger_parse_error(ps->current_token->line, ps->current_token->columns_traversed, "Invalid expression.");
        };
    }
}

/* TK_Arrow */
static void arrow_handler(ParseState* ps) {

    switch (ps->current_statement) {
        case ND_FunctionDefStatement: {
            if (ps->current_token->next == NULL || ps->current_token->prev->kind != TK_CloseParenthesis)
                logger_parse_error(ps->current_token->line, ps->current_token->columns_traversed, "Invalid arrow use.");

            token_advance(ps); // jump to type

            switch (ps->current_token->kind) {
                case TK_Type: case TK_None: {
                    TypeResolveExpression(ps);
                    break;
                }
                default: {
                    logger_parse_error(ps->current_token->line, ps->current_token->columns_traversed, "Invalid type.");
                }
            }

            break;
        }

        default: logger_parse_error(ps->current_token->line, ps->current_token->columns_traversed, "Invalid arrow use.");
    }

}

/* TK_Colon */
static void colon_handler(ParseState* ps) {
    //* explicit argument handling is in identifier_handler -> ExplicitArgumentExpression()
    switch (this_scope(ps)->kind) {
        case ScopeClause: {

            switch (ps->current_statement) {
                case ND_FunctionDefStatement: {
                    /* functions require a type before the colon */
                    if (ps->current_node->kind != ND_TypeResolveExpression)
                        logger_parse_error(ps->current_token->line, ps->current_token->columns_traversed, "No return type!");
                }
                default: {
                    jump_back_to_this_scope(ps);
                    break;
                }
            }
            break;
        }

        default: {
            logger_parse_error(ps->current_token->line, ps->current_token->columns_traversed, "Invalid symbol.");
        }
    }
}

/* TK_Type */
static void type_handler(ParseState* ps) {
    //* this is just a error checker, all other instances of types are handled elsewhere
    logger_parse_error(ps->current_token->line, ps->current_token->columns_traversed, "Invalid expression.");
}

/* TK_None */
static void none_handler(ParseState* ps) {

    type_handler(ps);
}

/*}================================================*/

/*
** Main loop.
*/
static void consume_tokens(ParseState* ps, LexOut* lo) { 
    ps->current_token = lo->tks[0];

     while (ps->current_token != NULL) {
        switch (ps->current_token->kind) {
            /* basic */
            case TK_Identifier: identifier_handler(ps); break;
            case TK_Keyword: keyword_handler(ps); break;
            case TK_Numeric: case TK_String: case TK_Boolean: literal_handler(ps); break;
            case TK_None: none_handler(ps); break;
            case TK_Type: type_handler(ps); break;
            /* operators */
            case TK_Equals: equals_handler(ps); break;
            case TK_OpenParenthesis: open_parenthesis_handler(ps); break;
            case TK_CloseParenthesis: close_parenthesis_handler(ps); break;
            case TK_Comma: comma_handler(ps); break;
            case TK_Arrow: arrow_handler(ps); break;
            case TK_Colon: colon_handler(ps); break;

            /* discared symbols */
            case TK_Quote: break;

            /* every other token */
            default: {
                char buf[64];
                sprintf(buf, "consume_tokens unsupported case %d.", ps->current_token->kind);
                logger_dev_warning(ps->current_token->line, buf);
                Node* child = create_node(ND_IdentifierExpression, ps->current_token);
                autoset_node_parent(child);
                ps->current_node = child;
                break;
            }
        }
        
        // next
        token_advance(ps);
    }
}



/*
** Prints node.
*/
static void _print_node(Node* nd, int layer) {
    for (int i = 0; i < layer; i++)
        printf("  ");
    /* map back kind to name */
    char kind_name[64];
    switch (nd->kind) {
        case ND_FunctionDefStatement: strcpy(kind_name, "FunctionDefStatement"); break;
        case ND_IdentifierExpression: strcpy(kind_name, "IdentifierExpression"); break;
        case ND_NumberLiteral: strcpy(kind_name, "NumberLiteral"); break;
        case ND_StringLiteral: strcpy(kind_name, "StringLiteral"); break;
        case ND_BooleanLiteral: strcpy(kind_name, "BooleanLiteral"); break;
        case ND_ArgumentListExpression: strcpy(kind_name, "ArgumentListExpression"); break;
        case ND_ExplicitArgumentExpression: strcpy(kind_name, "ExplicitArgumentExpression"); break;
        case ND_TypeResolveExpression: strcpy(kind_name, "TypeResolveExpression"); break;
        case ND_VarDeclStatement: strcpy(kind_name, "VarDeclStatement"); break;
        case ND_VarReassignStatement: strcpy(kind_name, "VarReassignStatement"); break;
        case ND_EqualsExpression: strcpy(kind_name, "EqualsExpression"); break;
        case ND_VarSeperationExpression: strcpy(kind_name, "VarSeperationExpression"); break;

        default: strcpy(kind_name, "Undefined"); break;
    }
    printf("kind: %s. value: %s.\n", kind_name, nd->value);
    for (int i = 0; i < nd->next.siz; i++)
        _print_node(nd->next.refs[i], layer+1);
}

/*{==================================*/
/*
** API
*/
AbstractSyntaxTree* parse_generate(LexOut* lo) {
    AbstractSyntaxTree* ast = malloc(sizeof(AbstractSyntaxTree));

    ParseState ps = {
        .nodes = {
            .ptrs = malloc(sizeof(void*)*AST_INIT_CAPACITY),
            .cap = AST_INIT_CAPACITY,
            .siz = 0,
        },
        .scopes = {
            .ptrs = malloc(sizeof(void*)*4),
            .cap = 4,
            .siz = 0,
        },
        .vars_allowed_in_scope = {
            .v = malloc(sizeof(void*)*4),
            .cap = 4,
            .siz = 0,
        },

        .root = NULL,
        .current_node = NULL,
        .current_statement = ND_Unknown,
        .current_expression = ND_Unknown,
        .current_token = NULL,
    };

    /* create root */
    ps.root = create_node(ND_Unknown, &(Token){.value=""});
    put_node_into_ps(&ps, ps.root);
    ps.current_node = ps.root;
    Scope* root_scp = create_scope(ScopeUndefined, ps.root); /* root is the bottom scope */
    put_scope_into_ps(&ps, root_scp);

    /* main code */
    consume_tokens(&ps, lo);


    /* copy to ast */
    ast->siz = ps.nodes.siz;
    ast->nodes = malloc(sizeof(void*)*ast->siz);
    for (int i = 0; i < ps.nodes.siz; i++)
        ast->nodes[i] = ps.nodes.ptrs[i];

    /* print nodes */
#if(DEBUG_PRINT_NODES == 1)
    printf("AST:\n");
    _print_node(ast->nodes[0], 0);
#endif

    /* free up memory */
    /*
    free(ps.nodes.ptrs);
    for (int i = 0; i < ps.scopes.siz; i++)
        free(ps.scopes.ptrs[i]);
    free(ps.scopes.ptrs);
    for (int i = 0; i < ps.vars_allowed_in_scope.siz; i++) {
        free(ps.vars_allowed_in_scope.v[i]->name);
        free(ps.vars_allowed_in_scope.v[i]);
    }
    free(ps.vars_allowed_in_scope.v);
    */
    return ast;
} // ! fix func def not freeing
void parse_free(AbstractSyntaxTree *ast) {
    printf("FREE DEBUG:\n");
    _print_node(ast->nodes[0], 0);
    for (int i = 0; i < ast->siz; i++) {
        Node* nd = ast->nodes[i];
        //puts("A");
        free(nd->value);
        //puts("B");
        free(nd->next.refs);
        //puts("C");
        free(nd);
        //puts("D");
    }
    free(ast->nodes);
    free(ast);
}