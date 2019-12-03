#include "tests.h"
#include "execution/expression.h"
#include "common/error.h"
#include "common/stack.h"
#include "common/string_literal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


int test_expression_functions()
{
    puts("Starting test test_expression_functions");

    parser_ast_expr e[20], *ee;

    handle strlit1 = string_literal_create(malloc(string_literal_alloc_sz()));
    if(NULL == strlit1) return __LINE__;
    handle strlit2 = string_literal_create(malloc(string_literal_alloc_sz()));
    if(NULL == strlit2) return __LINE__;
    if(0 != string_literal_append_char(strlit1, (const uint8 *)"abc", 3)) return __LINE__;
    if(0 != string_literal_append_char(strlit2, (const uint8 *)"abc", 3)) return __LINE__;

    uint32 stack_sz = 4;
    handle sh = stack_create(stack_sz, sizeof(parser_ast_expr *), malloc(stack_get_alloc_sz(stack_sz, sizeof(parser_ast_expr *))));
    if(NULL == sh) return __LINE__;


    puts("Testing expression_calc_base_expr");

    // 0 - 1
    e[0].node_type = PARSER_EXPR_NODE_TYPE_OP;
    e[0].op = PARSER_EXPR_OP_TYPE_SUB;
    e[0].left = &e[1];
    e[0].right = &e[2];

    e[1].node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&e[1].num, 0, sizeof(e[1].num));
    e[1].num.sign = DECIMAL_SIGN_POS;
    e[1].num.m[0] = 0;
    e[1].num.n = 0;

    e[2].node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&e[2].num, 0, sizeof(e[2].num));
    e[2].num.sign = DECIMAL_SIGN_POS;
    e[2].num.m[0] = 1;
    e[2].num.n = 1;

    if(0 != expression_calc_base_expr(e, 0)) return __LINE__;

    if(e[0].node_type != PARSER_EXPR_NODE_TYPE_NUM) return __LINE__;
    if(e[0].num.sign != DECIMAL_SIGN_NEG) return __LINE__;
    if(e[0].num.e != 0) return __LINE__;
    if(e[0].num.n != 1) return __LINE__;
    if(e[0].num.m[0] != 1) return __LINE__;

    // 1 / 0
    e[0].node_type = PARSER_EXPR_NODE_TYPE_OP;
    e[0].op = PARSER_EXPR_OP_TYPE_DIV;
    e[0].left = &e[1];
    e[0].right = &e[2];

    e[1].node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&e[1].num, 0, sizeof(e[1].num));
    e[1].num.sign = DECIMAL_SIGN_POS;
    e[1].num.m[0] = 1;
    e[1].num.n = 1;

    e[2].node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&e[2].num, 0, sizeof(e[2].num));
    e[2].num.sign = DECIMAL_SIGN_POS;
    e[2].num.m[0] = 0;
    e[2].num.n = 0;

    if(0 == expression_calc_base_expr(e, 0)) return __LINE__;
    if(error_get() != ERROR_DIVISION_BY_ZERO) return __LINE__;

    // name: 1 / a
    e[0].node_type = PARSER_EXPR_NODE_TYPE_OP;
    e[0].op = PARSER_EXPR_OP_TYPE_DIV;
    e[0].left = &e[1];
    e[0].right = &e[2];

    e[1].node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&e[1].num, 0, sizeof(e[1].num));
    e[1].num.sign = DECIMAL_SIGN_POS;
    e[1].num.m[0] = 1;
    e[1].num.n = 1;

    e[2].node_type = PARSER_EXPR_NODE_TYPE_NAME;
    e[2].name.first_part[0] = _ach('a');
    e[2].name.first_part_len = 1;
    e[2].name.second_part_len = 0;

    if(0 == expression_calc_base_expr(e, 0)) return __LINE__;
    if(error_get() != ERROR_DIVISION_BY_ZERO) return __LINE__;

    if(1 == expression_calc_base_expr(e, 1)) return __LINE__;

    // 1 + NULL
    e[0].node_type = PARSER_EXPR_NODE_TYPE_OP;
    e[0].op = PARSER_EXPR_OP_TYPE_ADD;
    e[0].left = &e[1];
    e[0].right = &e[2];

    e[1].node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&e[1].num, 0, sizeof(e[1].num));
    e[1].num.sign = DECIMAL_SIGN_POS;
    e[1].num.m[0] = 1;
    e[1].num.n = 1;

    e[2].node_type = PARSER_EXPR_NODE_TYPE_NULL;

    if(0 != expression_calc_base_expr(e, 0)) return __LINE__;
    if(e[0].node_type == PARSER_EXPR_NODE_TYPE_NUM) return __LINE__;

    // 1 is NULL
    e[0].node_type = PARSER_EXPR_NODE_TYPE_OP;
    e[0].op = PARSER_EXPR_OP_TYPE_IS;
    e[0].left = &e[1];
    e[0].right = &e[2];

    e[1].node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&e[1].num, 0, sizeof(e[1].num));
    e[1].num.sign = DECIMAL_SIGN_POS;
    e[1].num.m[0] = 1;
    e[1].num.n = 1;

    e[2].node_type = PARSER_EXPR_NODE_TYPE_NULL;

    if(0 != expression_calc_base_expr(e, 0)) return __LINE__;
    if(e[0].node_type != PARSER_EXPR_NODE_TYPE_BOOL) return __LINE__;
    if(e[0].boolean != 0) return __LINE__;

    // 1 is not NULL
    e[0].node_type = PARSER_EXPR_NODE_TYPE_OP;
    e[0].op = PARSER_EXPR_OP_TYPE_IS_NOT;
    e[0].left = &e[1];
    e[0].right = &e[2];

    e[1].node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&e[1].num, 0, sizeof(e[1].num));
    e[1].num.sign = DECIMAL_SIGN_POS;
    e[1].num.m[0] = 1;
    e[1].num.n = 1;

    e[2].node_type = PARSER_EXPR_NODE_TYPE_NULL;

    if(0 != expression_calc_base_expr(e, 0)) return __LINE__;
    if(e[0].node_type != PARSER_EXPR_NODE_TYPE_BOOL) return __LINE__;
    if(e[0].boolean != 1) return __LINE__;

    // NULL is NULL
    e[0].node_type = PARSER_EXPR_NODE_TYPE_OP;
    e[0].op = PARSER_EXPR_OP_TYPE_IS;
    e[0].left = &e[1];
    e[0].right = &e[2];

    e[1].node_type = PARSER_EXPR_NODE_TYPE_NULL;

    e[2].node_type = PARSER_EXPR_NODE_TYPE_NULL;

    if(0 != expression_calc_base_expr(e, 0)) return __LINE__;
    if(e[0].node_type != PARSER_EXPR_NODE_TYPE_BOOL) return __LINE__;
    if(e[0].boolean != 1) return __LINE__;

    // 1 is 1
    e[0].node_type = PARSER_EXPR_NODE_TYPE_OP;
    e[0].op = PARSER_EXPR_OP_TYPE_IS;
    e[0].left = &e[1];
    e[0].right = &e[2];

    e[1].node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&e[1].num, 0, sizeof(e[1].num));
    e[1].num.sign = DECIMAL_SIGN_POS;
    e[1].num.m[0] = 1;
    e[1].num.n = 1;

    e[2].node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&e[2].num, 0, sizeof(e[2].num));
    e[2].num.sign = DECIMAL_SIGN_POS;
    e[2].num.m[0] = 1;
    e[2].num.n = 1;

    if(0 != expression_calc_base_expr(e, 0)) return __LINE__;
    if(e[0].node_type != PARSER_EXPR_NODE_TYPE_BOOL) return __LINE__;
    if(e[0].boolean != 1) return __LINE__;

    // 'abc' is 'abc'
    e[0].node_type = PARSER_EXPR_NODE_TYPE_OP;
    e[0].op = PARSER_EXPR_OP_TYPE_IS;
    e[0].left = &e[1];
    e[0].right = &e[2];

    e[1].node_type = PARSER_EXPR_NODE_TYPE_STR;
    e[1].str = strlit1;

    e[2].node_type = PARSER_EXPR_NODE_TYPE_STR;
    e[2].str = strlit2;

    if(0 != expression_calc_base_expr(e, 0)) return __LINE__;
    if(e[0].node_type != PARSER_EXPR_NODE_TYPE_BOOL) return __LINE__;
    if(e[0].boolean != 1) return __LINE__;

    // not true
    e[0].node_type = PARSER_EXPR_NODE_TYPE_OP;
    e[0].op = PARSER_EXPR_OP_TYPE_NOT;
    e[0].left = &e[1];
    e[0].right = NULL;

    e[1].node_type = PARSER_EXPR_NODE_TYPE_BOOL;
    e[1].boolean = 1;

    if(0 != expression_calc_base_expr(e, 0)) return __LINE__;
    if(e[0].node_type != PARSER_EXPR_NODE_TYPE_BOOL) return __LINE__;
    if(e[0].boolean != 0) return __LINE__;

    // not false
    e[0].node_type = PARSER_EXPR_NODE_TYPE_OP;
    e[0].op = PARSER_EXPR_OP_TYPE_NOT;
    e[0].left = &e[1];
    e[0].right = NULL;

    e[1].node_type = PARSER_EXPR_NODE_TYPE_BOOL;
    e[1].boolean = 0;

    if(0 != expression_calc_base_expr(e, 0)) return __LINE__;
    if(e[0].node_type != PARSER_EXPR_NODE_TYPE_BOOL) return __LINE__;
    if(e[0].boolean != 1) return __LINE__;

    // not <name>
    e[0].node_type = PARSER_EXPR_NODE_TYPE_OP;
    e[0].op = PARSER_EXPR_OP_TYPE_NOT;
    e[0].left = &e[1];
    e[0].right = NULL;

    e[1].node_type = PARSER_EXPR_NODE_TYPE_NAME;
    e[1].name.first_part_len = 1;
    e[1].name.second_part_len = 0;

    if(0 != expression_calc_base_expr(e, 1)) return __LINE__;
    if(e[0].node_type != PARSER_EXPR_NODE_TYPE_OP) return __LINE__;
    if(e[0].op != PARSER_EXPR_OP_TYPE_NOT) return __LINE__;
    if(e[0].left != &e[1]) return __LINE__;
    if(e[0].right != NULL) return __LINE__;

    if(e[1].node_type != PARSER_EXPR_NODE_TYPE_NAME) return __LINE__;
    if(e[1].name.first_part_len != 1) return __LINE__;
    if(e[1].name.second_part_len != 0) return __LINE__;

    // true and false
    e[0].node_type = PARSER_EXPR_NODE_TYPE_OP;
    e[0].op = PARSER_EXPR_OP_TYPE_AND;
    e[0].left = &e[1];
    e[0].right = &e[2];

    e[1].node_type = PARSER_EXPR_NODE_TYPE_BOOL;
    e[1].boolean = 1;

    e[2].node_type = PARSER_EXPR_NODE_TYPE_BOOL;
    e[2].boolean = 0;

    if(0 != expression_calc_base_expr(e, 0)) return __LINE__;
    if(e[0].node_type != PARSER_EXPR_NODE_TYPE_BOOL) return __LINE__;
    if(e[0].boolean != 0) return __LINE__;

    // true and true
    e[0].node_type = PARSER_EXPR_NODE_TYPE_OP;
    e[0].op = PARSER_EXPR_OP_TYPE_AND;
    e[0].left = &e[1];
    e[0].right = &e[2];

    e[1].node_type = PARSER_EXPR_NODE_TYPE_BOOL;
    e[1].boolean = 1;

    e[2].node_type = PARSER_EXPR_NODE_TYPE_BOOL;
    e[2].boolean = 1;

    if(0 != expression_calc_base_expr(e, 0)) return __LINE__;
    if(e[0].node_type != PARSER_EXPR_NODE_TYPE_BOOL) return __LINE__;
    if(e[0].boolean != 1) return __LINE__;

    // false or true
    e[0].node_type = PARSER_EXPR_NODE_TYPE_OP;
    e[0].op = PARSER_EXPR_OP_TYPE_OR;
    e[0].left = &e[1];
    e[0].right = &e[2];

    e[1].node_type = PARSER_EXPR_NODE_TYPE_BOOL;
    e[1].boolean = 0;

    e[2].node_type = PARSER_EXPR_NODE_TYPE_BOOL;
    e[2].boolean = 1;

    if(0 != expression_calc_base_expr(e, 0)) return __LINE__;
    if(e[0].node_type != PARSER_EXPR_NODE_TYPE_BOOL) return __LINE__;
    if(e[0].boolean != 1) return __LINE__;

    // 1 <op> 2 / 2 <op> 1 / 2 <op> 2
    struct
    {
        parser_expr_op_type op;
        uint8               expected_res;
    } op_res[18] =
    {
        { PARSER_EXPR_OP_TYPE_EQ, 0},
        { PARSER_EXPR_OP_TYPE_NE, 1},
        { PARSER_EXPR_OP_TYPE_GT, 0},
        { PARSER_EXPR_OP_TYPE_GE, 0},
        { PARSER_EXPR_OP_TYPE_LT, 1},
        { PARSER_EXPR_OP_TYPE_LE, 1},

        { PARSER_EXPR_OP_TYPE_EQ, 0},
        { PARSER_EXPR_OP_TYPE_NE, 1},
        { PARSER_EXPR_OP_TYPE_GT, 1},
        { PARSER_EXPR_OP_TYPE_GE, 1},
        { PARSER_EXPR_OP_TYPE_LT, 0},
        { PARSER_EXPR_OP_TYPE_LE, 0},

        { PARSER_EXPR_OP_TYPE_EQ, 1},
        { PARSER_EXPR_OP_TYPE_NE, 0},
        { PARSER_EXPR_OP_TYPE_GT, 0},
        { PARSER_EXPR_OP_TYPE_GE, 1},
        { PARSER_EXPR_OP_TYPE_LT, 0},
        { PARSER_EXPR_OP_TYPE_LE, 1},
    };

    // 1 <op> 2
    for(int i=0; i<6; i++)
    {
        e[0].node_type = PARSER_EXPR_NODE_TYPE_OP;
        e[0].op = op_res[i].op;
        e[0].left = &e[1];
        e[0].right = &e[2];

        e[1].node_type = PARSER_EXPR_NODE_TYPE_NUM;
        memset(&e[1].num, 0, sizeof(e[1].num));
        e[1].num.sign = DECIMAL_SIGN_POS;
        e[1].num.m[0] = 1;
        e[1].num.n = 1;

        e[2].node_type = PARSER_EXPR_NODE_TYPE_NUM;
        memset(&e[2].num, 0, sizeof(e[2].num));
        e[2].num.sign = DECIMAL_SIGN_POS;
        e[2].num.m[0] = 2;
        e[2].num.n = 1;

        if(0 != expression_calc_base_expr(e, 0)) return __LINE__;
        if(e[0].node_type != PARSER_EXPR_NODE_TYPE_BOOL) return __LINE__;
        if(e[0].boolean != op_res[i].expected_res) return __LINE__;
    }

    // 2 <op> 1
    for(int i=6; i<12; i++)
    {
        e[0].node_type = PARSER_EXPR_NODE_TYPE_OP;
        e[0].op = op_res[i].op;
        e[0].left = &e[1];
        e[0].right = &e[2];

        e[1].node_type = PARSER_EXPR_NODE_TYPE_NUM;
        memset(&e[1].num, 0, sizeof(e[1].num));
        e[1].num.sign = DECIMAL_SIGN_POS;
        e[1].num.m[0] = 2;
        e[1].num.n = 1;

        e[2].node_type = PARSER_EXPR_NODE_TYPE_NUM;
        memset(&e[2].num, 0, sizeof(e[2].num));
        e[2].num.sign = DECIMAL_SIGN_POS;
        e[2].num.m[0] = 1;
        e[2].num.n = 1;

        if(0 != expression_calc_base_expr(e, 0)) return __LINE__;
        if(e[0].node_type != PARSER_EXPR_NODE_TYPE_BOOL) return __LINE__;
        if(e[0].boolean != op_res[i].expected_res) return __LINE__;
    }

    // 2 <op> 2
    for(int i=12; i<18; i++)
    {
        e[0].node_type = PARSER_EXPR_NODE_TYPE_OP;
        e[0].op = op_res[i].op;
        e[0].left = &e[1];
        e[0].right = &e[2];

        e[1].node_type = PARSER_EXPR_NODE_TYPE_NUM;
        memset(&e[1].num, 0, sizeof(e[1].num));
        e[1].num.sign = DECIMAL_SIGN_POS;
        e[1].num.m[0] = 2;
        e[1].num.n = 1;

        e[2].node_type = PARSER_EXPR_NODE_TYPE_NUM;
        memset(&e[2].num, 0, sizeof(e[2].num));
        e[2].num.sign = DECIMAL_SIGN_POS;
        e[2].num.m[0] = 2;
        e[2].num.n = 1;

        if(0 != expression_calc_base_expr(e, 0)) return __LINE__;
        if(e[0].node_type != PARSER_EXPR_NODE_TYPE_BOOL) return __LINE__;
        if(e[0].boolean != op_res[i].expected_res) return __LINE__;
    }

    // 'abc' <op> 'abc' / 'abca' <op> 'abc' / 'abca' <op> 'abcaa'
    struct
    {
        parser_expr_op_type op;
        uint8               expected_res;
    } op_res2[18] =
    {
        { PARSER_EXPR_OP_TYPE_EQ, 1},
        { PARSER_EXPR_OP_TYPE_NE, 0},
        { PARSER_EXPR_OP_TYPE_GT, 0},
        { PARSER_EXPR_OP_TYPE_GE, 1},
        { PARSER_EXPR_OP_TYPE_LT, 0},
        { PARSER_EXPR_OP_TYPE_LE, 1},

        { PARSER_EXPR_OP_TYPE_EQ, 0},
        { PARSER_EXPR_OP_TYPE_NE, 1},
        { PARSER_EXPR_OP_TYPE_GT, 1},
        { PARSER_EXPR_OP_TYPE_GE, 1},
        { PARSER_EXPR_OP_TYPE_LT, 0},
        { PARSER_EXPR_OP_TYPE_LE, 0},

        { PARSER_EXPR_OP_TYPE_EQ, 0},
        { PARSER_EXPR_OP_TYPE_NE, 1},
        { PARSER_EXPR_OP_TYPE_GT, 0},
        { PARSER_EXPR_OP_TYPE_GE, 0},
        { PARSER_EXPR_OP_TYPE_LT, 1},
        { PARSER_EXPR_OP_TYPE_LE, 1},
    };

    for(int i=0; i<6; i++)
    {
        if(i == 6)
        {
            if(0 != string_literal_append_char(strlit1, (const uint8 *)"a", 1)) return __LINE__;
        }

        if(i == 12)
        {
            if(0 != string_literal_append_char(strlit2, (const uint8 *)"aa", 2)) return __LINE__;
        }

        e[0].node_type = PARSER_EXPR_NODE_TYPE_OP;
        e[0].op = op_res2[i].op;
        e[0].left = &e[1];
        e[0].right = &e[2];

        e[1].node_type = PARSER_EXPR_NODE_TYPE_STR;
        e[1].str = strlit1;

        e[2].node_type = PARSER_EXPR_NODE_TYPE_STR;
        e[2].str = strlit2;

        if(0 != expression_calc_base_expr(e, 0)) return __LINE__;
        if(e[0].node_type != PARSER_EXPR_NODE_TYPE_BOOL) return __LINE__;
        if(e[0].boolean != op_res2[i].expected_res) return __LINE__;
    }


    puts("Testing expression_calc_expr with constants only");

    // 4/2 + 5*3 > 0 and 1=1

    // e[0] = 4
    ee = e;
    ee->node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ee->num, 0, sizeof(ee->num));
    ee->num.sign = DECIMAL_SIGN_POS;
    ee->num.m[0] = 4;
    ee->num.n = 1;

    // e[1] = 2
    ee = e + 1;
    ee->node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ee->num, 0, sizeof(ee->num));
    ee->num.sign = DECIMAL_SIGN_POS;
    ee->num.m[0] = 2;
    ee->num.n = 1;

    // e[2] = 5
    ee = e + 2;
    ee->node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ee->num, 0, sizeof(ee->num));
    ee->num.sign = DECIMAL_SIGN_POS;
    ee->num.m[0] = 5;
    ee->num.n = 1;

    // e[3] = 3
    ee = e + 3;
    ee->node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ee->num, 0, sizeof(ee->num));
    ee->num.sign = DECIMAL_SIGN_POS;
    ee->num.m[0] = 3;
    ee->num.n = 1;

    // e[4] = e[0] / e[1]
    ee = e + 4;
    ee->node_type = PARSER_EXPR_NODE_TYPE_OP;
    ee->op = PARSER_EXPR_OP_TYPE_DIV;
    ee->left = &e[0];
    ee->right = &e[1];

    // e[5] = e[2] * e[3]
    ee = e + 5;
    ee->node_type = PARSER_EXPR_NODE_TYPE_OP;
    ee->op = PARSER_EXPR_OP_TYPE_MUL;
    ee->left = &e[2];
    ee->right = &e[3];

    // e[6] = e[4] + e[5]
    ee = e + 6;
    ee->node_type = PARSER_EXPR_NODE_TYPE_OP;
    ee->op = PARSER_EXPR_OP_TYPE_ADD;
    ee->left = &e[4];
    ee->right = &e[5];

    // e[7] = 0
    ee = e + 7;
    ee->node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ee->num, 0, sizeof(ee->num));
    ee->num.sign = DECIMAL_SIGN_POS;

    // e[8] = 1
    ee = e + 8;
    ee->node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ee->num, 0, sizeof(ee->num));
    ee->num.sign = DECIMAL_SIGN_POS;
    ee->num.m[0] = 1;
    ee->num.n = 1;

    // e[9] = e[6] > e[7]
    ee = e + 9;
    ee->node_type = PARSER_EXPR_NODE_TYPE_OP;
    ee->op = PARSER_EXPR_OP_TYPE_GT;
    ee->left = &e[6];
    ee->right = &e[7];

    // e[10]: e[8] = e[8]
    ee = e + 10;
    ee->node_type = PARSER_EXPR_NODE_TYPE_OP;
    ee->op = PARSER_EXPR_OP_TYPE_EQ;
    ee->left = &e[8];
    ee->right = &e[8];

    // e[11] = e[9] AND e[10]
    ee = e + 11;
    ee->node_type = PARSER_EXPR_NODE_TYPE_OP;
    ee->op = PARSER_EXPR_OP_TYPE_AND;
    ee->left = &e[9];
    ee->right = &e[10];

    if(0 != expression_calc_const_expr(ee, sh)) return __LINE__;
    if(ee->node_type != PARSER_EXPR_NODE_TYPE_BOOL) return __LINE__;
    if(ee->boolean != 1) return __LINE__;


/*
    puts("Testing expression_calc_expr with names");

    // 4/2 + 5*3 > a and 1 = a   ->   17 > a and a = 1

    // e[0] = 4
    ee = e;
    ee->node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ee->num, 0, sizeof(ee->num));
    ee->num.sign = DECIMAL_SIGN_POS;
    ee->num.m[0] = 4;
    ee->num.n = 1;

    // e[1] = 2
    ee = e + 1;
    ee->node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ee->num, 0, sizeof(ee->num));
    ee->num.sign = DECIMAL_SIGN_POS;
    ee->num.m[0] = 2;
    ee->num.n = 1;

    // e[2] = 5
    ee = e + 2;
    ee->node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ee->num, 0, sizeof(ee->num));
    ee->num.sign = DECIMAL_SIGN_POS;
    ee->num.m[0] = 5;
    ee->num.n = 1;

    // e[3] = 3
    ee = e + 3;
    ee->node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ee->num, 0, sizeof(ee->num));
    ee->num.sign = DECIMAL_SIGN_POS;
    ee->num.m[0] = 3;
    ee->num.n = 1;

    // e[4] = e[0] / e[1]
    ee = e + 4;
    ee->node_type = PARSER_EXPR_NODE_TYPE_OP;
    ee->op = PARSER_EXPR_OP_TYPE_DIV;
    ee->left = &e[0];
    ee->right = &e[1];

    // e[5] = e[2] * e[3]
    ee = e + 5;
    ee->node_type = PARSER_EXPR_NODE_TYPE_OP;
    ee->op = PARSER_EXPR_OP_TYPE_MUL;
    ee->left = &e[2];
    ee->right = &e[3];

    // e[6] = e[4] + e[5]
    ee = e + 6;
    ee->node_type = PARSER_EXPR_NODE_TYPE_OP;
    ee->op = PARSER_EXPR_OP_TYPE_ADD;
    ee->left = &e[4];
    ee->right = &e[5];

    // e[7] = a
    ee = e + 7;
    ee->node_type = PARSER_EXPR_NODE_TYPE_NAME;
    ee->name.first_part[0] = _ach('a');
    ee->name.first_part_len = 1;
    ee->name.second_part_len = 0;

    // e[8] = 1
    ee = e + 8;
    ee->node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ee->num, 0, sizeof(ee->num));
    ee->num.sign = DECIMAL_SIGN_POS;
    ee->num.m[0] = 1;
    ee->num.n = 1;

    // e[9] = e[6] > e[7]
    ee = e + 9;
    ee->node_type = PARSER_EXPR_NODE_TYPE_OP;
    ee->op = PARSER_EXPR_OP_TYPE_GT;
    ee->left = &e[6];
    ee->right = &e[7];

    // e[10]: e[7] = e[8]
    ee = e + 10;
    ee->node_type = PARSER_EXPR_NODE_TYPE_OP;
    ee->op = PARSER_EXPR_OP_TYPE_EQ;
    ee->left = &e[7];
    ee->right = &e[8];

    // e[11] = e[9] AND e[10]
    ee = e + 11;
    ee->node_type = PARSER_EXPR_NODE_TYPE_OP;
    ee->op = PARSER_EXPR_OP_TYPE_AND;
    ee->left = &e[9];
    ee->right = &e[10];

    if(0 != expression_calc_const_expr(ee, sh)) return __LINE__;
    if(ee->node_type != PARSER_EXPR_NODE_TYPE_OP) return __LINE__;
    if(ee->op != PARSER_EXPR_OP_TYPE_AND) return __LINE__;

    ee = e[11].left;
    if(ee->node_type != PARSER_EXPR_NODE_TYPE_OP) return __LINE__;
    if(ee->op != PARSER_EXPR_OP_TYPE_GT) return __LINE__;

    ee = e[11].left->left;
    if(ee->node_type != PARSER_EXPR_NODE_TYPE_NUM) return __LINE__;
    if(ee->num.sign != DECIMAL_SIGN_POS) return __LINE__;
    if(ee->num.e != -36) return __LINE__;
    if(ee->num.n != 38) return __LINE__;
    if(ee->num.m[DECIMAL_PARTS-1] != 17) return __LINE__;

    ee = e[11].left->right;
    if(ee->node_type != PARSER_EXPR_NODE_TYPE_NAME) return __LINE__;
    if(ee->name.first_part_len != 1) return __LINE__;
    if(ee->name.second_part_len != 0) return __LINE__;
    if(ee->name.first_part[0] != _ach('a')) return __LINE__;

    ee = e[11].right;
    if(ee->node_type != PARSER_EXPR_NODE_TYPE_OP) return __LINE__;
    if(ee->op != PARSER_EXPR_OP_TYPE_EQ) return __LINE__;

    ee = e[11].right->left;
    if(ee->node_type != PARSER_EXPR_NODE_TYPE_NAME) return __LINE__;
    if(ee->name.first_part_len != 1) return __LINE__;
    if(ee->name.second_part_len != 0) return __LINE__;
    if(ee->name.first_part[0] != _ach('a')) return __LINE__;

    ee = e[11].right->right;
    if(ee->node_type != PARSER_EXPR_NODE_TYPE_NUM) return __LINE__;
    if(ee->num.sign != DECIMAL_SIGN_POS) return __LINE__;
    if(ee->num.e != 0) return __LINE__;
    if(ee->num.n != 1) return __LINE__;
    if(ee->num.m[0] != 1) return __LINE__;


    // (NOT 4/2 + 5*3 > a) and (NOT 1 = a)   ->  (NOT 17 > a) and (NOT a = 1)

    // e[0] = 4
    ee = e;
    ee->node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ee->num, 0, sizeof(ee->num));
    ee->num.sign = DECIMAL_SIGN_POS;
    ee->num.m[0] = 4;
    ee->num.n = 1;

    // e[1] = 2
    ee = e + 1;
    ee->node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ee->num, 0, sizeof(ee->num));
    ee->num.sign = DECIMAL_SIGN_POS;
    ee->num.m[0] = 2;
    ee->num.n = 1;

    // e[2] = 5
    ee = e + 2;
    ee->node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ee->num, 0, sizeof(ee->num));
    ee->num.sign = DECIMAL_SIGN_POS;
    ee->num.m[0] = 5;
    ee->num.n = 1;

    // e[3] = 3
    ee = e + 3;
    ee->node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ee->num, 0, sizeof(ee->num));
    ee->num.sign = DECIMAL_SIGN_POS;
    ee->num.m[0] = 3;
    ee->num.n = 1;

    // e[4] = e[0] / e[1]
    ee = e + 4;
    ee->node_type = PARSER_EXPR_NODE_TYPE_OP;
    ee->op = PARSER_EXPR_OP_TYPE_DIV;
    ee->left = &e[0];
    ee->right = &e[1];

    // e[5] = e[2] * e[3]
    ee = e + 5;
    ee->node_type = PARSER_EXPR_NODE_TYPE_OP;
    ee->op = PARSER_EXPR_OP_TYPE_MUL;
    ee->left = &e[2];
    ee->right = &e[3];

    // e[6] = e[4] + e[5]
    ee = e + 6;
    ee->node_type = PARSER_EXPR_NODE_TYPE_OP;
    ee->op = PARSER_EXPR_OP_TYPE_ADD;
    ee->left = &e[4];
    ee->right = &e[5];

    // e[7] = a
    ee = e + 7;
    ee->node_type = PARSER_EXPR_NODE_TYPE_NAME;
    ee->name.first_part[0] = _ach('a');
    ee->name.first_part_len = 1;
    ee->name.second_part_len = 0;

    // e[8] = 1
    ee = e + 8;
    ee->node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ee->num, 0, sizeof(ee->num));
    ee->num.sign = DECIMAL_SIGN_POS;
    ee->num.m[0] = 1;
    ee->num.n = 1;

    // e[9] = e[6] > e[7]
    ee = e + 9;
    ee->node_type = PARSER_EXPR_NODE_TYPE_OP;
    ee->op = PARSER_EXPR_OP_TYPE_GT;
    ee->left = &e[6];
    ee->right = &e[7];

    // e[10]: NOT e[9]
    ee = e + 10;
    ee->node_type = PARSER_EXPR_NODE_TYPE_OP;
    ee->op = PARSER_EXPR_OP_TYPE_NOT;
    ee->left = &e[9];
    ee->right = NULL;

    // e[11]: e[7] = e[8]
    ee = e + 11;
    ee->node_type = PARSER_EXPR_NODE_TYPE_OP;
    ee->op = PARSER_EXPR_OP_TYPE_EQ;
    ee->left = &e[7];
    ee->right = &e[8];

    // e[12]: NOT e[11]
    ee = e + 12;
    ee->node_type = PARSER_EXPR_NODE_TYPE_OP;
    ee->op = PARSER_EXPR_OP_TYPE_NOT;
    ee->left = &e[11];
    ee->right = NULL;

    // e[13] = e[10] AND e[12]
    ee = e + 13;
    ee->node_type = PARSER_EXPR_NODE_TYPE_OP;
    ee->op = PARSER_EXPR_OP_TYPE_AND;
    ee->left = &e[10];
    ee->right = &e[12];

    if(0 != expression_calc_const_expr(ee, sh)) return __LINE__;
    if(ee->node_type != PARSER_EXPR_NODE_TYPE_OP) return __LINE__;
    if(ee->op != PARSER_EXPR_OP_TYPE_AND) return __LINE__;

    ee = e[13].left;
    if(ee->node_type != PARSER_EXPR_NODE_TYPE_OP) return __LINE__;
    if(ee->op != PARSER_EXPR_OP_TYPE_NOT) return __LINE__;
    if(ee->right != NULL) return __LINE__;
    if(ee->left == NULL) return __LINE__;

    ee = e[13].right;
    if(ee->node_type != PARSER_EXPR_NODE_TYPE_OP) return __LINE__;
    if(ee->op != PARSER_EXPR_OP_TYPE_NOT) return __LINE__;
    if(ee->right != NULL) return __LINE__;
    if(ee->left == NULL) return __LINE__;

    ee = e[13].left->left;
    if(ee->node_type != PARSER_EXPR_NODE_TYPE_OP) return __LINE__;
    if(ee->op != PARSER_EXPR_OP_TYPE_GT) return __LINE__;

    ee = e[13].left->left->left;
    if(ee->node_type != PARSER_EXPR_NODE_TYPE_NUM) return __LINE__;
    if(ee->num.sign != DECIMAL_SIGN_POS) return __LINE__;
    if(ee->num.e != -36) return __LINE__;
    if(ee->num.n != 38) return __LINE__;
    if(ee->num.m[DECIMAL_PARTS-1] != 17) return __LINE__;

    ee = e[13].left->left->right;
    if(ee->node_type != PARSER_EXPR_NODE_TYPE_NAME) return __LINE__;
    if(ee->name.first_part_len != 1) return __LINE__;
    if(ee->name.second_part_len != 0) return __LINE__;
    if(ee->name.first_part[0] != _ach('a')) return __LINE__;

    ee = e[13].right->left;
    if(ee->node_type != PARSER_EXPR_NODE_TYPE_OP) return __LINE__;
    if(ee->op != PARSER_EXPR_OP_TYPE_EQ) return __LINE__;

    ee = e[13].right->left->left;
    if(ee->node_type != PARSER_EXPR_NODE_TYPE_NAME) return __LINE__;
    if(ee->name.first_part_len != 1) return __LINE__;
    if(ee->name.second_part_len != 0) return __LINE__;
    if(ee->name.first_part[0] != _ach('a')) return __LINE__;

    ee = e[13].right->left->right;
    if(ee->node_type != PARSER_EXPR_NODE_TYPE_NUM) return __LINE__;
    if(ee->num.sign != DECIMAL_SIGN_POS) return __LINE__;
    if(ee->num.e != 0) return __LINE__;
    if(ee->num.n != 1) return __LINE__;
    if(ee->num.m[0] != 1) return __LINE__;
*/
    return 0;
}
