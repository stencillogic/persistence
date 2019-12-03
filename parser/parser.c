#include "parser/parser.h"
#include "common/decimal.h"
#include "common/error.h"
#include "common/string_literal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>


#define PARSER_ERRMES_BUF_SZ    (1024)


struct _parser_state
{
    handle              lexer;                              // lexer instance
    lexer_lexem         lexem;                              // currently read lexem
    uint8               expr_op_level[17];                  // "operator -> precedence" correspondence
    parser_ast_stmt     *stmt_base;                         // statement base address
    uint64              total_sz;                           // total allocated size
    achar               errmes[PARSER_ERRMES_BUF_SZ];       // buffer for formatted error message
    sint8               (*report_error)(error_code error, const achar *msg);
    parser_expr_op_type saved_op;                           // first operator with priority lower than prio of "NOT" (NOT is special case)
} g_parser_state =
{
    .stmt_base = NULL,
    .lexem =
    {
        .type = 0,
        .line = 0,
        .col = 0,
        .str_literal = NULL
    },
    .total_sz = 0,
    .report_error = NULL,
    .lexer = NULL,
    .expr_op_level = {0, 1,1, 2,2, 3,3,3,3,3,3, 4,4,4, 5, 6, 7},
    .saved_op = PARSER_EXPR_OP_TYPE_NONE
};


// predefinitions
sint8 parser_parse_expr(parser_ast_expr *stmt);
sint8 parser_parse_expr_with_op_prio(parser_ast_expr *stmt, uint8 highest_lvl);
sint8 parser_parse_select(parser_ast_select *stmt, uint64 *cnt);



///////////////////////////////// UTILITY FUNCTIONS


sint8 parser_report_error(const achar *msg, ...)
{
    va_list args;
    va_start(args, msg);

    vsnprintf(g_parser_state.errmes, PARSER_ERRMES_BUF_SZ, msg, args);
    g_parser_state.errmes[PARSER_ERRMES_BUF_SZ-1] = _ach('\0');

    va_end(args);

    if(g_parser_state.report_error(ERROR_SYNTAX_ERROR, g_parser_state.errmes) != 0) return -1;

    return 0;
}


// free up space
void parser_deallocate_stmt(parser_ast_stmt *stmt)
{
    free(stmt);
}


sint8 parser_allocate_ast_el(void **ptr, size_t sz)
{
    // TODO: spill to disk if statement is too big to reside in memory
    void *new_base = realloc(g_parser_state.stmt_base, g_parser_state.total_sz + sz);
    if(NULL == new_base)
    {
        if(g_parser_state.report_error(ERROR_OUT_OF_MEMORY, NULL) != 0) return -1;
        return 1;
    }

    *ptr = new_base + g_parser_state.total_sz;
    g_parser_state.stmt_base = new_base;
    g_parser_state.total_sz += sz;
    memset(*ptr, 0, sz);

    return 0;
}


sint8 parser_parse_identifier(uint8 *identifier, uint16 *identifier_len, uint16 max_len)
{
    sint8 res;

    if(g_parser_state.lexem.type == LEXEM_TYPE_IDENTIFIER)
    {
        if(g_parser_state.lexem.identifier_len > max_len)
        {
            if(0 != parser_report_error(_ach("identifier is too long at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
            return 1;
        }

        *identifier_len = g_parser_state.lexem.identifier_len;
        memcpy(identifier, g_parser_state.lexem.identifier, g_parser_state.lexem.identifier_len);
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
    }
    else
    {
        if(0 != parser_report_error(_ach("identifier is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
        return 1;
    }

    return 0;
}


sint8 parser_parse_name(parser_ast_name *stmt)
{
    sint8 res;

    stmt->pos.line = g_parser_state.lexem.line;
    stmt->pos.col = g_parser_state.lexem.col;

    if(g_parser_state.lexem.type == LEXEM_TYPE_IDENTIFIER)
    {
        stmt->first_part_len = g_parser_state.lexem.identifier_len;
        memcpy(stmt->first_part, g_parser_state.lexem.identifier, stmt->first_part_len);

        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
    }
    else
    {
        if(0 != parser_report_error(_ach("identifier is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
        return 1;
    }

    if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_DOT)
    {
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

        if(g_parser_state.lexem.type == LEXEM_TYPE_IDENTIFIER)
        {
            stmt->second_part_len = g_parser_state.lexem.identifier_len;
            memcpy(stmt->second_part, g_parser_state.lexem.identifier, stmt->second_part_len);

            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
        else
        {
            if(0 != parser_report_error(_ach("identifier is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
            return 1;
        }
    }

    return 0;
}


///////////////////////////////// EXPRESSION



// parse expression argument
sint8 parser_parse_expr_arg(parser_ast_expr *stmt)
{
    sint8 res;
    parser_expr_op_type saved_op;

    // in case of NOT operator parser_parse_expr sets this value
    g_parser_state.saved_op = PARSER_EXPR_OP_TYPE_NONE;

    if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN)
    {
        if(g_parser_state.lexem.token == LEXER_TOKEN_LPAR)         // subexpr
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

            saved_op = g_parser_state.saved_op;
            if((res = parser_parse_expr(stmt)) != 0) return res;
            g_parser_state.saved_op = saved_op;

            if(!(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_RPAR))
            {
                if(0 != parser_report_error(_ach(" ) is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                return 1;
            }

            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
        else
        {
            if(0 != parser_report_error(_ach("string literal, numeric literal, identifier or ( is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
            return 1;
        }
    }
    else if(g_parser_state.lexem.type == LEXEM_TYPE_STR_LITERAL)   // string
    {
        stmt->node_type = PARSER_EXPR_NODE_TYPE_STR;
        void *strlit_buf;
        if((res = parser_allocate_ast_el((void **)&strlit_buf, string_literal_alloc_sz())) != 0) return res;
        if((stmt->str = string_literal_create(strlit_buf)) == NULL ||
            string_literal_move(g_parser_state.lexem.str_literal, stmt->str) != 0)
        {
            if(0 != parser_report_error(_ach("internal server error at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
            return -1;
        }
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
    }
    else if(g_parser_state.lexem.type == LEXEM_TYPE_NUM_LITERAL)   // decimal
    {
        stmt->node_type = PARSER_EXPR_NODE_TYPE_NUM;
        memcpy(&stmt->num, &g_parser_state.lexem.num_literal, sizeof(stmt->num));
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
    }
    else if(g_parser_state.lexem.type == LEXEM_TYPE_IDENTIFIER)   // identifier or name
    {
        stmt->node_type = PARSER_EXPR_NODE_TYPE_NAME;
        if((res = parser_parse_name(&stmt->name)) != 0) return res;
    }
    else if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_NULL)   // NULL
    {
        stmt->node_type = PARSER_EXPR_NODE_TYPE_NULL;
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
    }
    else if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_NOT)
    {
        // unary NOT; parse expr until priority less than prio of NOT
        stmt->node_type = PARSER_EXPR_NODE_TYPE_OP;
        stmt->op = PARSER_EXPR_OP_TYPE_NOT;
        if((res = parser_allocate_ast_el((void **)&stmt->left, sizeof(*stmt->left))) != 0) return res;
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

        // the next call sets value of g_parser_state.saved_op
        if((res = parser_parse_expr_with_op_prio(stmt->left, g_parser_state.expr_op_level[PARSER_EXPR_OP_TYPE_NOT])) != 0) return res;
    }
    else
    {
        if(0 != parser_report_error(_ach("string literal, numeric literal, identifier or ( is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
        return 1;
    }

    return 0;
}


// binary expression operator
sint8 parser_parse_expr_op(parser_expr_op_type *operator)
{
    sint8 res;
    parser_expr_op_type op = PARSER_EXPR_OP_TYPE_NONE;

    if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN)
    {
        if(g_parser_state.lexem.token == LEXER_TOKEN_ASTERISK)
        {
            op = PARSER_EXPR_OP_TYPE_MUL;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
        else if(g_parser_state.lexem.token == LEXER_TOKEN_SLASH)
        {
            op = PARSER_EXPR_OP_TYPE_DIV;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
        else if(g_parser_state.lexem.token == LEXER_TOKEN_PLUS)
        {
            op = PARSER_EXPR_OP_TYPE_ADD;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
        else if(g_parser_state.lexem.token == LEXER_TOKEN_MINUS)
        {
            op = PARSER_EXPR_OP_TYPE_SUB;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
        else if(g_parser_state.lexem.token == LEXER_TOKEN_EQ)
        {
            op = PARSER_EXPR_OP_TYPE_EQ;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
        else if(g_parser_state.lexem.token == LEXER_TOKEN_LT)
        {
            op = PARSER_EXPR_OP_TYPE_LT;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_EQ)
            {
                op = PARSER_EXPR_OP_TYPE_LE;
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            }
            else if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_GT)
            {
                op = PARSER_EXPR_OP_TYPE_NE;
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            }
        }
        else if(g_parser_state.lexem.token == LEXER_TOKEN_GT)
        {
            op = PARSER_EXPR_OP_TYPE_GT;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_EQ)
            {
                op = PARSER_EXPR_OP_TYPE_GE;
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            }
        }
    }
    else if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD)
    {
        if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_IS)
        {
            op = PARSER_EXPR_OP_TYPE_IS;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_NOT)
            {
                op = PARSER_EXPR_OP_TYPE_IS_NOT;
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            }
        }
        else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_AND)
        {
            op = PARSER_EXPR_OP_TYPE_AND;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
        else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_OR)
        {
            op = PARSER_EXPR_OP_TYPE_OR;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
    }

    *operator = op;

    return 0;
}


// Build expression tree.
// Parsing stops if next operator has priority lower than specified ( > highest_lvl);
//   the parsed operator then is saved as g_parser_state.saved_op.
// This is required to parse argument with prefixing operator NOT
sint8 parser_parse_expr_with_op_prio(parser_ast_expr *stmt, uint8 highest_lvl)
{
    sint8 res;
    parser_ast_expr *new_stmt, *node_for_level[8] = {stmt, stmt, stmt, stmt, stmt, stmt, stmt, stmt};
    parser_expr_op_type op = PARSER_EXPR_OP_TYPE_NONE;
    uint8 new_level;

    stmt->left = stmt->right = NULL;

    // unary operation in the beginning of expr
    if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN)
    {
        if(g_parser_state.lexem.token == LEXER_TOKEN_MINUS)
        {
            // node.type = operator "-", node.left = "0", node.right = <term>
            stmt->node_type = PARSER_EXPR_NODE_TYPE_OP;
            stmt->op = PARSER_EXPR_OP_TYPE_SUB;

            if((res = parser_allocate_ast_el((void **)&stmt->left, sizeof(*stmt->left))) != 0) return res;
            stmt->left->node_type = PARSER_EXPR_NODE_TYPE_NUM;
            memset(&stmt->left->num, 0, sizeof(stmt->left->num));
            stmt->left->num.sign = DECIMAL_SIGN_POS;

            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

            if((res = parser_allocate_ast_el((void **)&stmt->right, sizeof(*stmt->right))) != 0) return res;
            if((res = parser_parse_expr_arg(stmt->right)) != 0) return res;

            node_for_level[2] = stmt;   // last node representing operator of certain priority
        }
        else if(g_parser_state.lexem.token == LEXER_TOKEN_PLUS)
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
    }

    if(!stmt->left)
    {
        // <term> expected
        if((res = parser_parse_expr_arg(stmt)) != 0) return res;

        // bring to the state when root node is operator
        if(g_parser_state.saved_op == PARSER_EXPR_OP_TYPE_NONE)
        {
            if((res = parser_parse_expr_op(&op)) != 0) return res;
        }
        else
        {
            op = g_parser_state.saved_op;
            g_parser_state.saved_op = PARSER_EXPR_OP_TYPE_NONE;
        }

        if(op != PARSER_EXPR_OP_TYPE_NONE)
        {
            if(g_parser_state.expr_op_level[op] > highest_lvl)
            {
                g_parser_state.saved_op = op;
                return 0;
            }

            if((res = parser_allocate_ast_el((void **)&new_stmt, sizeof(*new_stmt))) != 0) return res;
            *new_stmt = *stmt;
            stmt->left = new_stmt;
            stmt->node_type = PARSER_EXPR_NODE_TYPE_OP;
            stmt->op = op;
            stmt->right = NULL;

            node_for_level[g_parser_state.expr_op_level[op]] = stmt;

            // second argument
            if((res = parser_allocate_ast_el((void **)&stmt->right, sizeof(*stmt->right))) != 0) return res;
            if((res = parser_parse_expr_arg(stmt->right)) != 0) return res;
        }
        else
        {
            return 0;
        }
    }

    do
    {
        // binary operator
        if(g_parser_state.saved_op == PARSER_EXPR_OP_TYPE_NONE)
        {
            if((res = parser_parse_expr_op(&op)) != 0) return res;
        }
        else
        {
            op = g_parser_state.saved_op;
            g_parser_state.saved_op = PARSER_EXPR_OP_TYPE_NONE;
        }

        if(op != PARSER_EXPR_OP_TYPE_NONE)
        {
            // check operator precedence (level)
            new_level = g_parser_state.expr_op_level[op];

            // for processing "NOT <subexpr>" check if the level is hiegher than level of NOT
            if(new_level > highest_lvl)
            {
                g_parser_state.saved_op = op;
                return 0;
            }

            if(g_parser_state.expr_op_level[stmt->op] < new_level)
            {
                // go to the last operator with the same level
                stmt = node_for_level[new_level];

                if((res = parser_allocate_ast_el((void **)&new_stmt, sizeof(*new_stmt))) != 0) return res;
                *new_stmt = *stmt;
                stmt->left = new_stmt;
                stmt->op = op;
                stmt->right = NULL;
            }
            else
            {
                if((res = parser_allocate_ast_el((void **)&new_stmt, sizeof(*new_stmt))) != 0) return res;
                new_stmt->node_type = PARSER_EXPR_NODE_TYPE_OP;
                new_stmt->left = stmt->right;
                new_stmt->op = op;

                stmt->right = new_stmt;
                stmt = new_stmt;
            }

            node_for_level[new_level] = stmt;

            // second argument
            if((res = parser_allocate_ast_el((void **)&stmt->right, sizeof(*stmt->right))) != 0) return res;
            if((res = parser_parse_expr_arg(stmt->right)) != 0) return res;
        }
        else
        {
            return 0;
        }
    }
    while(1);

    return 0;
}

sint8 parser_parse_expr(parser_ast_expr *stmt)
{
    stmt->pos.line = g_parser_state.lexem.line;
    stmt->pos.col = g_parser_state.lexem.col;

    g_parser_state.saved_op = PARSER_EXPR_OP_TYPE_NONE;
    return parser_parse_expr_with_op_prio(stmt, 255);
}

// alias: [ AS <alias> | <alias> ]
sint8 parser_parse_optional_alias(uint8 *alias, uint16 *alias_len)
{
    sint8 res;

    *alias_len = 0;

    if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_AS)
    {
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
    }

    if(g_parser_state.lexem.type == LEXEM_TYPE_IDENTIFIER)
    {
        if((res = parser_parse_identifier(alias, alias_len, PARSER_MAX_ALIAS_NAME_LEN)) != 0) return res;
    }

    return 0;
}


// expression with alias: <expr> [ AS <alias> | <alias> ]
sint8 parser_parse_named_expr(parser_ast_named_expr *stmt)
{
    sint8 res;

    if((res = parser_parse_expr(&stmt->expr)) != 0) return res;

    if((res = parser_parse_optional_alias(stmt->alias, &stmt->alias_len)) != 0) return res;

    return 0;
}


// expression list with aliases: <expr1> as a, <expr2> b, <expr3> , ...
sint8 parser_parse_named_expr_list(parser_ast_expr_list *stmt, uint64 *cnt)
{
    sint8 res;

    stmt->named = 1;
    if((res = parser_parse_named_expr(&stmt->named_expr)) != 0) return res;
    *cnt = 1;

    while(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_COMMA)
    {
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

        if((res = parser_allocate_ast_el((void **)&stmt->next, sizeof(*stmt->next))) != 0) return res;

        stmt = stmt->next;
        stmt->named = 1;

        if((res = parser_parse_named_expr(&stmt->named_expr)) != 0) return res;
        (*cnt)++;
    }

    return 0;
}


// expression list: <expr1> , <expr2> , <expr3> ...
sint8 parser_parse_expr_list(parser_ast_expr_list *stmt, uint64 *cnt)
{
    sint8 res;

    if((res = parser_parse_expr(&stmt->expr)) != 0) return res;
    *cnt = 1;

    while(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_COMMA)
    {
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

        if((res = parser_allocate_ast_el((void **)&stmt->next, sizeof(*stmt->next))) != 0) return res;

        stmt = stmt->next;

        if((res = parser_parse_expr(&stmt->expr)) != 0) return res;
        (*cnt)++;
    }

    return 0;
}



///////////////////////////////// SELECT STATEMENT



// from part of select with joins
sint8 parser_parse_from(parser_ast_from *stmt, uint64 *cnt)
{
    sint8 res, first_run = 1, from_item, completed = 0;

    *cnt = 0;
    do
    {
        from_item = 0;

        stmt->join_type = PARSER_JOIN_TYPE_NONE;

        // from item: table or subquery
        if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_LPAR)
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_SELECT)
            {
                stmt->type = PARSER_FROM_TYPE_SUBQUERY;

                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                if((res = parser_parse_select(&stmt->subquery, &stmt->subquery_cnt)) != 0) return res;

                if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_RPAR)
                {
                    if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                }
                else
                {
                    if(0 != parser_report_error(_ach(" ) is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                    return 1;
                }
            }
            else
            {
                if(0 != parser_report_error(_ach("SELECT is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                return 1;
            }

            from_item = 1;
        }
        else if(g_parser_state.lexem.type == LEXEM_TYPE_IDENTIFIER)
        {
            stmt->type = PARSER_FROM_TYPE_NAME;

            if((res = parser_parse_name(&stmt->name)) != 0) return res;

            from_item = 1;
        }
        else if(first_run == 1)
        {
            if(0 != parser_report_error(_ach("identifier or ( is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
            return 1;
        }

        if(from_item)
        {
            (*cnt)++;

            // check for alias
            if((res = parser_parse_optional_alias(stmt->alias, &stmt->alias_len)) != 0) return res;
        }

        if(first_run == 1)
        {
            first_run = 0;
        }
        else
        {
            // not first run -> there is join -> read optional join condition
            if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD)
            {
                if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_ON)
                {
                    if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                    if((res = parser_allocate_ast_el((void **)&stmt->on_expr, sizeof(*stmt->on_expr))) != 0) return res;
                    if((res = parser_parse_expr(stmt->on_expr)) != 0) return res;
                }
            }
        }

        // determine join type
        if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD)
        {
            if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_INNER)
            {
                stmt->join_type = PARSER_JOIN_TYPE_INNER;
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            }
            else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_CROSS)
            {
                stmt->join_type = PARSER_JOIN_TYPE_CROSS;
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            }
            else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_LEFT)
            {
                stmt->join_type = PARSER_JOIN_TYPE_LEFT;
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            }
            else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_RIGHT)
            {
                stmt->join_type = PARSER_JOIN_TYPE_RIGHT;
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            }
            else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_FULL)
            {
                stmt->join_type = PARSER_JOIN_TYPE_FULL;
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            }
        }

        if(stmt->join_type == PARSER_JOIN_TYPE_LEFT ||
            stmt->join_type == PARSER_JOIN_TYPE_RIGHT ||
            stmt->join_type == PARSER_JOIN_TYPE_FULL)
        {
            if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_OUTER)
            {
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            }
        }

        if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_JOIN)
        {
            if(stmt->join_type == PARSER_JOIN_TYPE_NONE)
            {
                stmt->join_type = PARSER_JOIN_TYPE_INNER;
            }
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }

        if(stmt->join_type != PARSER_JOIN_TYPE_NONE)
        {
            if((res = parser_allocate_ast_el((void **)&stmt->next, sizeof(*stmt->next))) != 0) return res;
            stmt = stmt->next;
        }
        else
        {
            completed = 1;
        }
    }
    while(!completed);

    return 0;
}


// a single set described in select
sint8 parser_parse_single_select(parser_ast_single_select *stmt)
{
    sint8 res;

    stmt->uniq_type = PARSER_UNIQ_TYPE_ALL;

    if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD)
    {
        if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_DISTINCT)
        {
            stmt->uniq_type = PARSER_UNIQ_TYPE_DISTINCT;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
        else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_ALL)
        {
            stmt->uniq_type = PARSER_UNIQ_TYPE_ALL;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
    }

    if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_ASTERISK)
    {
        stmt->projection = NULL;
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
    }
    else
    {
        if((res = parser_allocate_ast_el((void **)&stmt->projection, sizeof(*stmt->projection))) != 0) return res;
        if((res = parser_parse_named_expr_list(stmt->projection, &stmt->projection_cnt)) != 0) return res;
    }

    if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_FROM)
    {
        if((res = parser_allocate_ast_el((void **)&stmt->from, sizeof(*stmt->from))) != 0) return res;
        stmt->from->pos.line = g_parser_state.lexem.line;
        stmt->from->pos.col = g_parser_state.lexem.col;

        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        if((res = parser_parse_from(stmt->from, &stmt->from_cnt)) != 0) return res;
    }

    if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_WHERE)
    {
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        if((res = parser_allocate_ast_el((void **)&stmt->where, sizeof(*stmt->where))) != 0) return res;
        if((res = parser_parse_expr(stmt->where)) != 0) return res;
    }

    if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_GROUP)
    {
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_BY)
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if((res = parser_allocate_ast_el((void **)&stmt->group_by, sizeof(*stmt->group_by))) != 0) return res;
            if((res = parser_parse_expr_list(stmt->group_by, &stmt->group_by_cnt)) != 0) return res;

            if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_HAVING)
            {
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                if((res = parser_allocate_ast_el((void **)&stmt->having, sizeof(*stmt->having))) != 0) return res;
                if((res = parser_parse_expr_list(stmt->having, &stmt->having_cnt)) != 0) return res;
            }
        }
        else
        {
            if(0 != parser_report_error(_ach("BY is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
            return 1;
        }

    }

    return 0;
}


// parse order by: <expr> [ASC|DESC] [NULLS LAST | NULLS FIRST] , ...
sint8 parser_parse_order_by(parser_ast_order_by *stmt, uint64 *cnt)
{
    sint8 res;

    *cnt = 0;
    do
    {
        (*cnt)++;

        stmt->sort_type = PARSER_SORT_TYPE_ASC;
        stmt->null_order = PARSER_NULL_ORDER_TYPE_LAST;

        // expression
        if((res = parser_parse_expr(&stmt->expr)) != 0) return res;

        // asc, desc
        if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD)
        {
            if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_ASC)
            {
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            }
            else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_DESC)
            {
                stmt->sort_type = PARSER_SORT_TYPE_DESC;
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            }
        }

        // nulls last, nulls first
        if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_NULLS)
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD)
            {
                if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_LAST)
                {
                    if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                }
                else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_FIRST)
                {
                    stmt->null_order = PARSER_NULL_ORDER_TYPE_FIRST;
                    if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                }
                else
                {
                    if(0 != parser_report_error(_ach("FIRST or LAST is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                    return 1;
                }
            }
            else
            {
                if(0 != parser_report_error(_ach("FIRST or LAST is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                return 1;
            }
        }

        // comma
        if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_COMMA)
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

            if((res = parser_allocate_ast_el((void **)&stmt->next, sizeof(*stmt->next))) != 0) return res;

            stmt = stmt->next;
        }
        else
        {
            return 0;
        }
    }
    while(1);

    return res;
}


// full select statement
sint8 parser_parse_select(parser_ast_select *stmt, uint64 *cnt)
{
    sint8 res, setop;

    *cnt = 0;
    do
    {
        (*cnt)++;

        if((res = parser_parse_single_select(&stmt->select)) != 0) return res;

        setop = 0;
        if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD)
        {
            // union, intersect, except
            if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_UNION)
            {
                stmt->setop = PARSER_SETOP_TYPE_UNION;
                setop = 1;
            }
            else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_INTERSECT)
            {
                stmt->setop = PARSER_SETOP_TYPE_INTERSECT;
                setop = 1;
            }
            else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_EXCEPT)
            {
                stmt->setop = PARSER_SETOP_TYPE_EXCEPT;
                setop = 1;
            }

            if(setop)
            {
                // optional all
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_ALL)
                {
                    stmt->setop += 10;
                    if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                }

                if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_SELECT)
                {
                    if((res = parser_allocate_ast_el((void **)&stmt->next, sizeof(*stmt->next))) != 0) return res;

                    stmt = stmt->next;
                    stmt->select.pos.line = g_parser_state.lexem.line;
                    stmt->select.pos.col = g_parser_state.lexem.col;

                    if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

                }
                else
                {
                    if(0 != parser_report_error(_ach("SELECT is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                    return 1;
                }
            }
        }
    }
    while(setop);

    // order by
    if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_ORDER)
    {
        uint64 line, col;
        line = g_parser_state.lexem.line;
        col = g_parser_state.lexem.col;

        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_BY)
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

            if((res = parser_allocate_ast_el((void **)&stmt->order_by, sizeof(*stmt->order_by))) != 0) return res;
            stmt->order_by->pos.line = line;
            stmt->order_by->pos.col = col;

            if((res = parser_parse_order_by(stmt->order_by, &stmt->order_by_cnt)) != 0) return res;
        }
        else
        {
            if(0 != parser_report_error(_ach("BY is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
            return 1;
        }
    }

    return 0;
}



///////////////////////////////// INSERT STATEMENT



// column name list: <colname_1> { , <colname_2> }
sint8 parser_parse_colname_list(parser_ast_colname_list *stmt, uint64 *cnt)
{
    sint8 res;

    stmt->pos.line = g_parser_state.lexem.line;
    stmt->pos.col = g_parser_state.lexem.col;

    *cnt = 0;
    do
    {
        (*cnt)++;

        if((res = parser_parse_identifier(stmt->col_name, &stmt->colname_len, MAX_TABLE_COL_NAME_LEN)) != 0) return res;

        // read ,
        if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_COMMA)
        {
            if((res = parser_allocate_ast_el((void **)&stmt->next, sizeof(*stmt->next))) != 0) return res;
            stmt = stmt->next;

            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
        else
        {
            return 0;
        }
    }
    while(1);

    return 0;
}


// parse column list inside ( ), skip parsing if there is no (
sint8 parser_parse_optional_enclosed_colname_list(parser_ast_colname_list **pstmt, uint64 *cnt)
{
    sint8 res;
    parser_ast_colname_list *stmt;

    if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_LPAR)
    {
        // column list
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        if((res = parser_allocate_ast_el((void **)&stmt, sizeof(*stmt))) != 0) return res;
        *pstmt = stmt;
        if((res = parser_parse_colname_list(stmt, cnt)) != 0) return res;

        if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_RPAR)
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
        else
        {
            if(0 != parser_report_error(_ach(") is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
            return 1;
        }
    }

    return 0;
}


// parse column list inside ( )
sint8 parser_parse_enclosed_colname_list(parser_ast_colname_list *stmt, uint64 *cnt)
{
    sint8 res;

    if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_LPAR)
    {
        // column list
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        if((res = parser_parse_colname_list(stmt, cnt)) != 0) return res;

        if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_RPAR)
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
        else
        {
            if(0 != parser_report_error(_ach(") is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
            return 1;
        }
    }
    else
    {
        if(0 != parser_report_error(_ach("( is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
        return 1;
    }

    return 0;
}


// insert into <db.table> [ ( <colname_list> ) ]
sint8 parser_parse_insert_header(parser_ast_insert *stmt)
{
    sint8 res;

    if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_INTO)
    {
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

        if((res = parser_parse_name(&stmt->target)) != 0) return res;
        if((res = parser_parse_optional_enclosed_colname_list(&stmt->columns, &stmt->columns_cnt)) != 0) return res;
    }
    else
    {
        if(0 != parser_report_error(_ach("INTO is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
        return 1;
    }

    return 0;
}


// insert statement
sint8 parser_parse_insert(parser_ast_insert *stmt)
{
    sint8 res;

    if((res = parser_parse_insert_header(stmt)) != 0) return res;

    if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD)
    {
        if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_VALUES)
        {
            stmt->type = PARSER_STMT_TYPE_INSERT_VALUES;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_LPAR)
            {
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                if((res = parser_parse_expr_list(&stmt->values, &stmt->values_cnt)) != 0) return res;

                if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_RPAR)
                {
                    if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                }
                else
                {
                    if(0 != parser_report_error(_ach(") is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                    return 1;
                }
            }
            else
            {
                if(0 != parser_report_error(_ach("( is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                return 1;
            }
        }
        else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_SELECT)
        {
            // insert as select
            stmt->type = PARSER_STMT_TYPE_INSERT_SELECT;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if((res = parser_parse_select(&stmt->select_stmt, &stmt->select_stmt_cnt)) != 0) return res;
        }
        else
        {
            if(0 != parser_report_error(_ach("VALUES or SELECT is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
            return 1;
        }
    }
    else
    {
        if(0 != parser_report_error(_ach("VALUES or SELECT is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
        return 1;
    }

    return 0;
}



///////////////////////////////// UPDATE STATEMENT



// update statement
sint8 parser_parse_update(parser_ast_update *stmt)
{
    sint8 res, done;
    parser_ast_set_list *set_list;


    // target table name
    if((res = parser_parse_name(&stmt->target)) != 0) return res;

    // alias
    if((res = parser_parse_optional_alias(stmt->alias, &stmt->alias_len)) != 0) return res;

    // set
    if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_SET)
    {
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
    }
    else
    {
        if(0 != parser_report_error(_ach("SET is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
        return 1;
    }

    set_list = &stmt->set_list;

    done = 0;
    stmt->set_list_cnt = 0;
    do
    {
        stmt->set_list_cnt++;

        // alias.col = expr
        if((res = parser_parse_name(&set_list->column)) != 0) return res;

        // =
        if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_EQ)
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
        else
        {
            if(0 != parser_report_error(_ach("= is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
            return 1;
        }

        // assignment expr
        if((res = parser_parse_expr(&set_list->expr)) != 0) return res;

        // comma
        if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_COMMA)
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if((res = parser_allocate_ast_el((void **)&set_list->next, sizeof(*set_list->next))) != 0) return res;
            set_list = set_list->next;
        }
        else
        {
            done = 1;
        }
    }
    while(done == 0);

    // where
    if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_WHERE)
    {
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        if((res = parser_allocate_ast_el((void **)&stmt->where, sizeof(*stmt->where))) != 0) return res;
        if((res = parser_parse_expr(stmt->where)) != 0) return res;
    }

    return 0;
}



///////////////////////////////// DELETE STATEMENT



// delete from <table> [ where  <condition> ]
sint8 parser_parse_delete(parser_ast_delete *stmt)
{
    sint8 res;

    if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_FROM)
    {
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
    }

    // target table name
    if((res = parser_parse_name(&stmt->target)) != 0) return res;

    // alias
    if((res = parser_parse_optional_alias(stmt->alias, &stmt->alias_len)) != 0) return res;

    // where
    if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_WHERE)
    {
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        if((res = parser_allocate_ast_el((void **)&stmt->where, sizeof(*stmt->where))) != 0) return res;
        if((res = parser_parse_expr(stmt->where)) != 0) return res;
    }

    return 0;
}



///////////////////////////////// CREATE TABLE STATEMENT



sint8 parser_process_integer(sint64 *val, sint64 min, sint64 max)
{
    sint8 res;

    if(g_parser_state.lexem.type != LEXEM_TYPE_NUM_LITERAL)
    {
        if(0 != parser_report_error(_ach("integer value is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
        return 1;
    }

    if(g_parser_state.lexem.integer < min || g_parser_state.lexem.integer > max)
    {
        if(0 != parser_report_error(_ach("value is beyond allowed range at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
        return 1;
    }

    *val = g_parser_state.lexem.integer;

    if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

    return 0;
}


// parse constraint body, if for_column = 1 then column-level constraints are parsed as well (default, null, not null)
sint8 parser_parse_constr_body(parser_ast_constr *stmt)
{
    sint8 res;

    // constr type
    if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_CHECK)
    {
        stmt->type = PARSER_CONSTRAINT_TYPE_CHECK;
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_LPAR)
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if((res = parser_parse_expr(&stmt->expr)) != 0) return res;
            if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_RPAR)
            {
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            }
            else
            {
                if(0 != parser_report_error(_ach(") is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                return 1;
            }
        }
        else
        {
            if(0 != parser_report_error(_ach("( is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
            return 1;
        }
    }
    else if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_UNIQUE)
    {
        stmt->type = PARSER_CONSTRAINT_TYPE_UNIQUE;
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        if((res = parser_parse_enclosed_colname_list(&stmt->columns, &stmt->columns_cnt)) != 0) return res;
    }
    else if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_PRIMARY)
    {
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_KEY)
        {
            stmt->type = PARSER_CONSTRAINT_TYPE_PK;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if((res = parser_parse_enclosed_colname_list(&stmt->columns, &stmt->columns_cnt)) != 0) return res;
        }
        else
        {
            if(0 != parser_report_error(_ach("KEY is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
            return 1;
        }
    }
    else if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_FOREIGN)
    {
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_KEY)
        {
            stmt->type = PARSER_CONSTRAINT_TYPE_FK;

            // columns
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if((res = parser_parse_enclosed_colname_list(&stmt->fk.columns, &stmt->fk.columns_cnt)) != 0) return res;

            if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_REFERENCES)
            {
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            }
            else
            {
                if(0 != parser_report_error(_ach("REFERENCES is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                return 1;
            }

            // ref table
            if((res = parser_parse_name(&stmt->fk.ref_table)) != 0) return res;

            // ref columns
            if((res = parser_parse_optional_enclosed_colname_list(&stmt->fk.ref_columns, &stmt->fk.ref_columns_cnt)) != 0) return res;

            // on delete
            if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_ON)
            {
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_DELETE)
                {
                    if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                    if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_RESTRICT)
                    {
                        stmt->fk.fk_on_delete = PARSER_ON_DELETE_RESTRICT;
                        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                    }
                    else if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_CASCADE)
                    {
                        stmt->fk.fk_on_delete = PARSER_ON_DELETE_CASCADE;
                        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                    }
                    else if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_SET)
                    {
                        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                        if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_NULL)
                        {
                            stmt->fk.fk_on_delete = PARSER_ON_DELETE_SET_NULL;
                            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                        }
                        else if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_DEFAULT)
                        {
                            stmt->fk.fk_on_delete = PARSER_ON_DELETE_SET_DEFAULT;
                            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                        }
                        else
                        {
                            if(0 != parser_report_error(_ach("one of NULL, DEFAULT is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                            return 1;
                        }
                    }
                    else if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_NO)
                    {
                        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                        if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_ACTION)
                        {
                            stmt->fk.fk_on_delete = PARSER_ON_DELETE_NO_ACTION;
                            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                        }
                        else
                        {
                            if(0 != parser_report_error(_ach("one of ACTION is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                            return 1;
                        }
                    }
                    else
                    {
                        if(0 != parser_report_error(_ach("one of RESTRICT, CASCADE, SET, NO is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                        return 1;
                    }
                }
                else
                {
                    if(0 != parser_report_error(_ach("DELETE is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                    return 1;
                }
            }
        }
        else
        {
            if(0 != parser_report_error(_ach("KEY is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
            return 1;
        }
    }
    else
    {
        if(0 != parser_report_error(_ach("one of CHECK, PRIMARY, UNIQUE, FOREIGN is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
        return 1;
    }

    return 0;
}


// parse datatype precision
sint8 parser_parse_dt_precision(parser_ast_col_datatype *dt)
{
    sint8 res;
    sint64 v;

    if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_LPAR)
    {
        lexer_num_mode_integer(g_parser_state.lexer, 1);
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

        if(dt->datatype == CHARACTER_VARYING)
        {
            if((res = parser_process_integer(&dt->char_len, 1, MAX_VARCHAR_LENGTH)) != 0) return res;
        }
        else if(dt->datatype == DECIMAL)
        {
            if((res = parser_process_integer(&v, 1, DECIMAL_POSITIONS)) != 0) return res;
            dt->decimal.precision = (sint8)v;
            dt->decimal.scale = 0;

            if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_COMMA)
            {
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                if((res = parser_process_integer(&v, 0, dt->decimal.precision)) != 0) return res;
                dt->decimal.scale = (sint8)v;
            }
        }
        else if(dt->datatype == TIMESTAMP || dt->datatype == TIMESTAMP_WITH_TZ)
        {
            if((res = parser_process_integer(&v, 0, 6)) != 0) return res;
            dt->ts_precision = (sint8)v;
        }
        else
        {
            if(0 != parser_report_error(_ach("datatype has no parameters at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
            return 1;
        }

        if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_RPAR)
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
        else
        {
            if(0 != parser_report_error(_ach(") is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
            return 1;
        }

        lexer_num_mode_integer(g_parser_state.lexer, 0);
    }

    return 0;
}


sint8 parser_parse_col_datatype(parser_ast_col_datatype *datatype, sint8 *dt_determined)
{
    sint8 res;

    datatype->pos.line = g_parser_state.lexem.line;
    datatype->pos.col = g_parser_state.lexem.col;

    if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD)
    {
        if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_CHARACTER)
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_VARYING)
            {
                datatype->datatype = CHARACTER_VARYING;
                datatype->char_len = PARSER_DEFAULT_VARCHAR_LEN;

                *dt_determined = 1;
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                if((res = parser_parse_dt_precision(datatype)) != 0) return res;
            }
            else
            {
                if(0 != parser_report_error(_ach("VARYING is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                return 1;
            }
        }
        else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_VARCHAR)
        {
            datatype->datatype = CHARACTER_VARYING;
            datatype->char_len = PARSER_DEFAULT_VARCHAR_LEN;

            *dt_determined = 1;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if((res = parser_parse_dt_precision(datatype)) != 0) return res;
        }
        else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_DECIMAL || g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_NUMBER)
        {
            datatype->datatype = DECIMAL;
            datatype->decimal.precision = PARSER_DEFAULT_DECIMAL_PRECISION;
            datatype->decimal.scale = PARSER_DEFAULT_DECIMAL_SCALE;

            *dt_determined = 1;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if((res = parser_parse_dt_precision(datatype)) != 0) return res;
        }
        else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_TIMESTAMP)
        {
            datatype->datatype = TIMESTAMP;
            datatype->ts_precision = PARSER_DEFAULT_TS_PRECISION;

            *dt_determined = 1;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_WITH)
            {
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_TIME)
                {
                    if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                    if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_ZONE)
                    {
                        datatype->datatype = TIMESTAMP_WITH_TZ;
                        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                    }
                    else
                    {
                        if(0 != parser_report_error(_ach("ZONE is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                        return 1;
                    }
                }
                else
                {
                    if(0 != parser_report_error(_ach("TIME is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                    return 1;
                }
            }
            if((res = parser_parse_dt_precision(datatype)) != 0) return res;
        }
        else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_SMALLINT)
        {
            datatype->datatype = SMALLINT;
            *dt_determined = 1;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
        else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_INTEGER)
        {
            datatype->datatype = INTEGER;
            *dt_determined = 1;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
        else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_FLOAT)
        {
            datatype->datatype = FLOAT;
            *dt_determined = 1;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
        else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_DOUBLE)
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_PRECISION)
            {
                datatype->datatype = DOUBLE_PRECISION;
                *dt_determined = 1;
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            }
            else
            {
                if(0 != parser_report_error(_ach("PRECISION is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                return 1;
            }
        }
        else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_DATE)
        {
            datatype->datatype = DATE;
            *dt_determined = 1;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
    }

    return 0;
}


// table columns
sint8 parser_parse_col_desc_list(parser_ast_col_desc_list *stmt, uint64 *cnt)
{
    sint8 res, dt_determined;

    *cnt = 0;
    do
    {
        (*cnt)++;

        stmt->col_desc.pos.line = g_parser_state.lexem.line;
        stmt->col_desc.pos.col = g_parser_state.lexem.col;

        // column name
        if((res = parser_parse_identifier(stmt->col_desc.name, &stmt->col_desc.name_len, MAX_TABLE_COL_NAME_LEN)) != 0) return res;

        // datatype
        dt_determined = 0;
        if((res = parser_parse_col_datatype(&stmt->col_desc.datatype, &dt_determined)) != 0) return res;

        if(0 == dt_determined)
        {
            if(0 != parser_report_error(_ach("one of VARCHAR, DECIMAL, TIMESTAMP, SMALLINT, INTEGER, FLOAT, DOUBLE PRECISION, DATE is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
            return 1;
        }

        // default value
        if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_DEFAULT)
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if((res = parser_allocate_ast_el((void **)&stmt->col_desc.default_value, sizeof(*stmt->col_desc.default_value))) != 0) return res;
            if((res = parser_parse_expr(stmt->col_desc.default_value)) != 0) return res;
        }

        // null / not null
        stmt->col_desc.nullable = 0;
        if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_NOT)
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_NULL)
            {
                stmt->col_desc.nullable = 2;
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            }
            else
            {
                if(0 != parser_report_error(_ach("NULL is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                return 1;
            }
        }
        else if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_NULL)
        {
            stmt->col_desc.nullable = 1;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }

        if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_COMMA)
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_CONSTRAINT)
            {
                return 0;
            }
            else
            {
                if((res = parser_allocate_ast_el((void **)&stmt->next, sizeof(*stmt->next))) != 0) return res;
                stmt = stmt->next;
            }
        }
        else
        {
            return 0;
        }
    }
    while(1);

    return 0;
}


// table constraints
sint8 parser_parse_table_constraints(parser_ast_constr_list *stmt, uint64 *cnt)
{
    sint8 res;

    *cnt = 0;
    do
    {
        (*cnt)++;

        stmt->constr.pos.line = g_parser_state.lexem.line;
        stmt->constr.pos.col = g_parser_state.lexem.col;

        // constr name
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        if((res = parser_parse_identifier(stmt->constr.name, &stmt->constr.name_len, MAX_CONSTRAINT_NAME_LEN)) != 0) return res;

        // constr body
        if((res = parser_parse_constr_body(&stmt->constr)) != 0) return res;

        if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_COMMA)
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_CONSTRAINT)
            {
                if((res = parser_allocate_ast_el((void **)&stmt->next, sizeof(*stmt->next))) != 0) return res;
                stmt = stmt->next;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }
    while(1);

    return 0;
}


// create table
sint8 parser_parse_create_table(parser_ast_create_table *stmt)
{
    sint8 res;

    // name
    if((res = parser_parse_name(&stmt->name)) != 0) return res;

    if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_LPAR)
    {
        // cloumn list
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        if((res = parser_parse_col_desc_list(&stmt->cols, &stmt->cols_cnt)) != 0) return res;

        // table constraints
        if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_CONSTRAINT)
        {
            if((res = parser_allocate_ast_el((void **)&stmt->constr, sizeof(*stmt->constr))) != 0) return res;
            if((res = parser_parse_table_constraints(stmt->constr, &stmt->constr_cnt)) != 0) return res;
        }

        if(g_parser_state.lexem.type == LEXEM_TYPE_TOKEN && g_parser_state.lexem.token == LEXER_TOKEN_RPAR)
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
        else
        {
            if(0 != parser_report_error(_ach(") is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
            return 1;
        }
    }
    else
    {
        if(0 != parser_report_error(_ach("( is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
        return 1;
    }

    return 0;
}



///////////////////////////////// CREATE DATABASE STATEMENT



sint8 parser_parse_create_database(parser_ast_create_database *stmt)
{
    sint8 res;

    if((res = parser_parse_identifier(stmt->name, &stmt->name_len, PARSER_MAX_DATABASE_NAME_LEN)) != 0) return res;

    return 0;
}



///////////////////////////////// DROP TABLE STATEMENT



sint8 parser_parse_drop_table(parser_ast_drop_table *stmt)
{
    sint8 res;

    if((res = parser_parse_name(&stmt->table)) != 0) return res;

    return 0;
}



///////////////////////////////// DROP DATABASE STATEMENT



sint8 parser_parse_drop_database(parser_ast_drop_database *stmt)
{
    sint8 res;

    if((res = parser_parse_identifier(stmt->name, &stmt->name_len, PARSER_MAX_DATABASE_NAME_LEN)) != 0) return res;

    return 0;
}



///////////////////////////////// ALTER STATEMENT



sint8 parser_parse_alter_column(parser_ast_alter_table_column *stmt)
{
    sint8 res, dt_determined;
    parser_ast_col_datatype datatype;

    stmt->pos.line = g_parser_state.lexem.line;
    stmt->pos.col = g_parser_state.lexem.col;

    // column name
    if((res = parser_parse_identifier(stmt->column, &stmt->column_len, MAX_TABLE_COL_NAME_LEN)) != 0) return res;

    dt_determined = 0;
    if((res = parser_parse_col_datatype(&datatype, &dt_determined)) != 0) return res;

    if(dt_determined != 0)
    {
        if((res = parser_allocate_ast_el((void **)&stmt->datatype, sizeof(*stmt->datatype))) != 0) return res;
        memcpy(stmt->datatype, &datatype, sizeof(datatype));
    }

    // default value
    if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_DEFAULT)
    {
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        if((res = parser_allocate_ast_el((void **)&stmt->default_expr, sizeof(*stmt->default_expr))) != 0) return res;
        if((res = parser_parse_expr(stmt->default_expr)) != 0) return res;
    }

    // null / not null
    stmt->nullable = 0;
    if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_NOT)
    {
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_NULL)
        {
            stmt->nullable = 2;
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
        }
        else
        {
            if(0 != parser_report_error(_ach("NULL is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
            return 1;
        }
    }
    else if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_NULL)
    {
        stmt->nullable = 1;
        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
    }

    return 0;
}


sint8 parser_parse_alter_table(parser_ast_alter_table *stmt)
{
    sint8 res;

    // table name
    if((res = parser_parse_name(&stmt->table)) != 0) return res;

    if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD)
    {
        if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_ADD)
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD)
            {
                if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_COLUMN)
                {
                    // add column
                    stmt->type = PARSER_STMT_TYPE_ALTER_TABLE_ADD_COL;
                    if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                    if((res = parser_parse_alter_column(&stmt->column)) != 0) return res;
                }
                else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_CONSTRAINT)
                {
                    // add constraint
                    stmt->type = PARSER_STMT_TYPE_ALTER_TABLE_ADD_CONSTR;
                    if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

                    stmt->add_constr.pos.line = g_parser_state.lexem.line;
                    stmt->add_constr.pos.col = g_parser_state.lexem.col;

                    if((res = parser_parse_identifier(stmt->add_constr.name, &stmt->add_constr.name_len, MAX_CONSTRAINT_NAME_LEN)) != 0) return res;
                    if((res = parser_parse_constr_body(&stmt->add_constr)) != 0) return res;
                }
                else
                {
                    if(0 != parser_report_error(_ach("COLUMN, CONSTRAINT or identifier is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                    return 1;
                }
            }
            else if(g_parser_state.lexem.type == LEXEM_TYPE_IDENTIFIER)
            {
                // add column
                stmt->type = PARSER_STMT_TYPE_ALTER_TABLE_ADD_COL;
                if((res = parser_parse_alter_column(&stmt->column)) != 0) return res;
            }
            else
            {
                if(0 != parser_report_error(_ach("COLUMN, CONSTRAINT or identifier is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                return 1;
            }
        }
        else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_ALTER || g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_MODIFY)
        {
            // mmodify column
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_COLUMN)
            {
                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            }

            stmt->type = PARSER_STMT_TYPE_ALTER_TABLE_MODIFY_COL;
            if((res = parser_parse_alter_column(&stmt->column)) != 0) return res;
        }
        else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_DROP)
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD)
            {
                if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_COLUMN)
                {
                    // drop column
                    stmt->type = PARSER_STMT_TYPE_ALTER_TABLE_DROP_COL;
                    if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

                    stmt->column.pos.line = g_parser_state.lexem.line;
                    stmt->column.pos.col = g_parser_state.lexem.col;

                    if((res = parser_parse_identifier(stmt->column.column, &stmt->column.column_len, MAX_TABLE_COL_NAME_LEN)) != 0) return res;
                }
                else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_CONSTRAINT)
                {
                    // drop constraint
                    stmt->type = PARSER_STMT_TYPE_ALTER_TABLE_DROP_CONSTR;
                    if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

                    stmt->drop_constr.pos.line = g_parser_state.lexem.line;
                    stmt->drop_constr.pos.col = g_parser_state.lexem.col;

                    if((res = parser_parse_identifier(stmt->drop_constr.name, &stmt->drop_constr.name_len, MAX_CONSTRAINT_NAME_LEN)) != 0) return res;
                }
                else
                {
                    if(0 != parser_report_error(_ach("COLUMN, CONSTRAINT or identifier is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                    return 1;
                }
            }
            else if(g_parser_state.lexem.type == LEXEM_TYPE_IDENTIFIER)
            {
                // drop column
                stmt->type = PARSER_STMT_TYPE_ALTER_TABLE_DROP_COL;

                stmt->column.pos.line = g_parser_state.lexem.line;
                stmt->column.pos.col = g_parser_state.lexem.col;

                if((res = parser_parse_identifier(stmt->column.column, &stmt->column.column_len, MAX_TABLE_COL_NAME_LEN)) != 0) return res;
            }
            else
            {
                if(0 != parser_report_error(_ach("COLUMN, CONSTRAINT or identifier is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                return 1;
            }
        }
        else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_RENAME)
        {
            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD)
            {
                if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_COLUMN)
                {
                    // rename column
                    stmt->type = PARSER_STMT_TYPE_ALTER_TABLE_RENAME_COLUMN;
                    if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

                    stmt->rename_column.pos.line = g_parser_state.lexem.line;
                    stmt->rename_column.pos.col = g_parser_state.lexem.col;

                    if((res = parser_parse_identifier(stmt->rename_column.name, &stmt->rename_column.name_len, MAX_CONSTRAINT_NAME_LEN)) != 0) return res;
                    if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_TO)
                    {
                        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

                        stmt->rename_column.new_pos.line = g_parser_state.lexem.line;
                        stmt->rename_column.new_pos.col = g_parser_state.lexem.col;

                        if((res = parser_parse_identifier(stmt->rename_column.new_name, &stmt->rename_column.new_name_len, MAX_CONSTRAINT_NAME_LEN)) != 0) return res;
                    }
                    else
                    {
                        if(0 != parser_report_error(_ach("TO is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                        return 1;
                    }
                }
                else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_CONSTRAINT)
                {
                    // rename constraint
                    stmt->type = PARSER_STMT_TYPE_ALTER_TABLE_RENAME_CONSTR;
                    if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

                    stmt->rename_constr.pos.line = g_parser_state.lexem.line;
                    stmt->rename_constr.pos.col = g_parser_state.lexem.col;

                    if((res = parser_parse_identifier(stmt->rename_constr.name, &stmt->rename_constr.name_len, MAX_CONSTRAINT_NAME_LEN)) != 0) return res;
                    if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_TO)
                    {
                        if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

                        stmt->rename_constr.new_pos.line = g_parser_state.lexem.line;
                        stmt->rename_constr.new_pos.col = g_parser_state.lexem.col;

                        if((res = parser_parse_identifier(stmt->rename_constr.new_name, &stmt->rename_constr.new_name_len, MAX_CONSTRAINT_NAME_LEN)) != 0) return res;
                    }
                    else
                    {
                        if(0 != parser_report_error(_ach("TO is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                        return 1;
                    }
                }
                else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_TO)
                {
                    // rename table
                    stmt->type = PARSER_STMT_TYPE_ALTER_TABLE_RENAME;
                    if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

                    stmt->rename_table.pos.line = g_parser_state.lexem.line;
                    stmt->rename_table.pos.col = g_parser_state.lexem.col;

                    if((res = parser_parse_identifier(stmt->rename_table.new_name, &stmt->rename_table.new_name_len, MAX_CONSTRAINT_NAME_LEN)) != 0) return res;
                }
                else
                {
                    if(0 != parser_report_error(_ach("COLUMN, CONSTRAINT or TO is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                    return 1;
                }
            }
        }
        else
        {
            if(0 != parser_report_error(_ach("ADD, ALTER, DROP or RENAME is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
            return 1;
        }
    }
    else
    {
        if(0 != parser_report_error(_ach("ADD, ALTER, DROP or RENAME is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
        return 1;
    }

    return 0;
}



///////////////////////////////// SQL STATEMENT



// parse statement
sint8 parser_parse(parser_ast_stmt **pstmt, handle lexer, parser_interface pi)
{
    sint8 res;
    parser_ast_stmt *stmt = NULL;

    g_parser_state.lexer = lexer;
    g_parser_state.report_error = pi.report_error;

    if(lexer_reset(lexer) != 0) return -1;

    g_parser_state.stmt_base = stmt;
    g_parser_state.total_sz = 0;
    g_parser_state.saved_op = PARSER_EXPR_OP_TYPE_NONE;

    if((res = parser_allocate_ast_el((void **)&stmt, sizeof(*stmt))) != 0) return res;
    *pstmt = stmt;
    lexer_num_mode_integer(g_parser_state.lexer, 0);

    if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

    if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD)
    {
        if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_SELECT)
        {
            stmt->type = PARSER_STMT_TYPE_SELECT;
            stmt->select_stmt.select.pos.line = g_parser_state.lexem.line;
            stmt->select_stmt.select.pos.col = g_parser_state.lexem.col;

            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if((res = parser_parse_select(&stmt->select_stmt, &stmt->select_stmt_cnt)) != 0)
            {
                parser_deallocate_stmt(stmt);
                return res;
            }
        }
        else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_INSERT)
        {
            stmt->type = PARSER_STMT_TYPE_INSERT;
            stmt->insert_stmt.pos.line = g_parser_state.lexem.line;
            stmt->insert_stmt.pos.col = g_parser_state.lexem.col;

            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if((res = parser_parse_insert(&stmt->insert_stmt)) != 0)
            {
                parser_deallocate_stmt(stmt);
                return res;
            }
        }
        else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_UPDATE)
        {
            stmt->type = PARSER_STMT_TYPE_UPDATE;
            stmt->update_stmt.pos.line = g_parser_state.lexem.line;
            stmt->update_stmt.pos.col = g_parser_state.lexem.col;

            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if((res = parser_parse_update(&stmt->update_stmt)) != 0)
            {
                parser_deallocate_stmt(stmt);
                return res;
            }
        }
        else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_DELETE)
        {
            stmt->type = PARSER_STMT_TYPE_DELETE;
            stmt->delete_stmt.pos.line = g_parser_state.lexem.line;
            stmt->delete_stmt.pos.col = g_parser_state.lexem.col;

            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
            if((res = parser_parse_delete(&stmt->delete_stmt)) != 0)
            {
                parser_deallocate_stmt(stmt);
                return res;
            }
        }
        else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_CREATE)
        {
            uint64 line, col;
            line = g_parser_state.lexem.line;
            col = g_parser_state.lexem.col;

            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

            if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_TABLE)
            {
                // create table
                stmt->type = PARSER_STMT_TYPE_CREATE_TABLE;
                stmt->create_table_stmt.pos.line = line;
                stmt->create_table_stmt.pos.col = col;

                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                if((res = parser_parse_create_table(&stmt->create_table_stmt)) != 0)
                {
                    parser_deallocate_stmt(stmt);
                    return res;
                }
            }
            else if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_DATABASE)
            {
                // create database
                stmt->type = PARSER_STMT_TYPE_CREATE_DATABASE;
                stmt->create_database_stmt.pos.line = line;
                stmt->create_database_stmt.pos.col = col;

                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                if((res = parser_parse_create_database(&stmt->create_database_stmt)) != 0)
                {
                    parser_deallocate_stmt(stmt);
                    return res;
                }
            }
            else
            {
                if(0 != parser_report_error(_ach("TABLE or DATABASE is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                return 1;
            }

        }
        else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_DROP)
        {
            uint64 line, col;
            line = g_parser_state.lexem.line;
            col = g_parser_state.lexem.col;

            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

            if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_TABLE)
            {
                // drop table
                stmt->type = PARSER_STMT_TYPE_DROP_TABLE;
                stmt->drop_database_stmt.pos.line = line;
                stmt->drop_database_stmt.pos.col = col;

                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                if((res = parser_parse_drop_table(&stmt->drop_table_stmt)) != 0)
                {
                    parser_deallocate_stmt(stmt);
                    return res;
                }
            }
            else if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_DATABASE)
            {
                // drop database
                stmt->type = PARSER_STMT_TYPE_DROP_DATABASE;
                stmt->drop_database_stmt.pos.line = line;
                stmt->drop_database_stmt.pos.col = col;

                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                if((res = parser_parse_drop_database(&stmt->drop_database_stmt)) != 0)
                {
                    parser_deallocate_stmt(stmt);
                    return res;
                }
            }
            else
            {
                if(0 != parser_report_error(_ach("TABLE or DATABASE is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                return 1;
            }
        }
        else if(g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_ALTER)
        {
            uint64 line, col;
            line = g_parser_state.lexem.line;
            col = g_parser_state.lexem.col;

            if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;

            if(g_parser_state.lexem.type == LEXEM_TYPE_RESERVED_WORD && g_parser_state.lexem.reserved_word == LEXER_RESERVED_WORD_TABLE)
            {
                // drop table
                stmt->type = PARSER_STMT_TYPE_ALTER_TABLE;
                stmt->alter_table_stmt.pos.line = line;
                stmt->alter_table_stmt.pos.col = col;

                if((res = lexer_next(g_parser_state.lexer, &g_parser_state.lexem)) != 0) return res;
                if((res = parser_parse_alter_table(&stmt->alter_table_stmt)) != 0)
                {
                    parser_deallocate_stmt(stmt);
                    return res;
                }
            }
            else
            {
                if(0 != parser_report_error(_ach("TABLE is expected at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
                return 1;
            }
        }
    }
    else if(g_parser_state.lexem.type == LEXEM_TYPE_EOS)
    {
        // empty statement
        if(0 != parser_report_error(_ach("Empty statement"))) return -1;
        return 1;
    }
    else
    {
        // not valid starting keyword
        if(0 != parser_report_error(_ach("Unexpected beginning of statement"))) return -1;
        return 1;
    }

    // statement end is expected
    if(g_parser_state.lexem.type != LEXEM_TYPE_EOS)
    {
        if(0 != parser_report_error(_ach("Unexpected continuation at line %d, column %d"), g_parser_state.lexem.line, g_parser_state.lexem.col)) return -1;
        return 1;
    }

    return 0;
}

