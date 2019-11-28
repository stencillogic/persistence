#include "execution/expression.h"
#include "common/decimal.h"
#include "common/string_literal.h"
#include "common/stack.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>


// resolve node of type name to value
sint8 expression_resolve_name(const parser_ast_expr *expr, parser_ast_expr *result)
{
    // TODO:
    return -1;
}


sint8 expression_calc_base_expr(parser_ast_expr *expr, uint8 ignore_name)
{
    parser_ast_expr *left, *right;
    sint16          cmp_res;
    uint8           arg_null = 0;
    sint8           res;
    parser_expr_op_type op = expr->op;

    left = expr->left;
    right = expr->right;

    assert(expr->node_type == PARSER_EXPR_NODE_TYPE_OP);

    if(0 == ignore_name)
    {
        assert(left->node_type != PARSER_EXPR_NODE_TYPE_OP);
        assert(op == PARSER_EXPR_OP_TYPE_NOT || right->node_type != PARSER_EXPR_NODE_TYPE_OP);

        if(left->node_type == PARSER_EXPR_NODE_TYPE_NAME)
        {
            if(expression_resolve_name(left, left) != 0) return -1;
        }

        if(op != PARSER_EXPR_OP_TYPE_NOT && right->node_type == PARSER_EXPR_NODE_TYPE_NAME)
        {
            if(expression_resolve_name(right, right) != 0) return -1;
        }
    }
    else
    {
        if(left->node_type == PARSER_EXPR_NODE_TYPE_NAME || left->node_type == PARSER_EXPR_NODE_TYPE_OP
                || (op != PARSER_EXPR_OP_TYPE_NOT && (right->node_type == PARSER_EXPR_NODE_TYPE_OP || right->node_type == PARSER_EXPR_NODE_TYPE_NAME)))
        {
            return 0;
        }
    }

    if(left->node_type == PARSER_EXPR_NODE_TYPE_NULL || (op != PARSER_EXPR_OP_TYPE_NOT && right->node_type == PARSER_EXPR_NODE_TYPE_NULL))
    {
        arg_null = 1;
    }

    switch(op)
    {
        case PARSER_EXPR_OP_TYPE_MUL:
            if(left->node_type == PARSER_EXPR_NODE_TYPE_NUM && right->node_type == PARSER_EXPR_NODE_TYPE_NUM)
            {
                expr->node_type = PARSER_EXPR_NODE_TYPE_NUM;
                if(decimal_mul(&left->num, &right->num, &expr->num) != 0) return -1;
            }
            else if(arg_null)
            {
                expr->node_type = PARSER_EXPR_NODE_TYPE_NULL;
            }
            else
            {
                error_set(ERROR_DATATYPE_MISMATCH);
                return -1;
            }
            break;
        case PARSER_EXPR_OP_TYPE_DIV:
            if(left->node_type == PARSER_EXPR_NODE_TYPE_NUM && right->node_type == PARSER_EXPR_NODE_TYPE_NUM)
            {
                expr->node_type = PARSER_EXPR_NODE_TYPE_NUM;
                if(decimal_div(&left->num, &right->num, &expr->num) != 0) return -1;
            }
            else if(arg_null)
            {
                expr->node_type = PARSER_EXPR_NODE_TYPE_NULL;
            }
            else
            {
                error_set(ERROR_DATATYPE_MISMATCH);
                return -1;
            }
            break;
        case PARSER_EXPR_OP_TYPE_ADD:
            if(left->node_type == PARSER_EXPR_NODE_TYPE_NUM && right->node_type == PARSER_EXPR_NODE_TYPE_NUM)
            {
                expr->node_type = PARSER_EXPR_NODE_TYPE_NUM;
                if(decimal_add(&left->num, &right->num, &expr->num) != 0) return -1;
            }
            else if(arg_null)
            {
                expr->node_type = PARSER_EXPR_NODE_TYPE_NULL;
            }
            else
            {
                error_set(ERROR_DATATYPE_MISMATCH);
                return -1;
            }
            break;
        case PARSER_EXPR_OP_TYPE_SUB:
            if(left->node_type == PARSER_EXPR_NODE_TYPE_NUM && right->node_type == PARSER_EXPR_NODE_TYPE_NUM)
            {
                expr->node_type = PARSER_EXPR_NODE_TYPE_NUM;
                if(decimal_sub(&left->num, &right->num, &expr->num) != 0)
                {
                    return -1;
                }
            }
            else if(arg_null)
            {
                expr->node_type = PARSER_EXPR_NODE_TYPE_NULL;
            }
            else
            {
                error_set(ERROR_DATATYPE_MISMATCH);
                return -1;
            }
            break;
        case PARSER_EXPR_OP_TYPE_EQ:
        case PARSER_EXPR_OP_TYPE_NE:
        case PARSER_EXPR_OP_TYPE_GT:
        case PARSER_EXPR_OP_TYPE_GE:
        case PARSER_EXPR_OP_TYPE_LT:
        case PARSER_EXPR_OP_TYPE_LE:
            if(left->node_type == PARSER_EXPR_NODE_TYPE_NUM && right->node_type == PARSER_EXPR_NODE_TYPE_NUM)
            {
                expr->node_type = PARSER_EXPR_NODE_TYPE_BOOL;
                cmp_res = decimal_cmp(&left->num, &right->num);
                if((0 == cmp_res && (PARSER_EXPR_OP_TYPE_EQ == op || PARSER_EXPR_OP_TYPE_GE == op || PARSER_EXPR_OP_TYPE_LE == op))
                        || (0 > cmp_res && (PARSER_EXPR_OP_TYPE_NE == op || PARSER_EXPR_OP_TYPE_LT == op || PARSER_EXPR_OP_TYPE_LE == op))
                        || (0 < cmp_res && (PARSER_EXPR_OP_TYPE_NE == op || PARSER_EXPR_OP_TYPE_GT == op || PARSER_EXPR_OP_TYPE_GE == op)))
                {
                    expr->boolean = 1;
                }
                else
                {
                    expr->boolean = 0;
                }
            }
            else if(left->node_type == PARSER_EXPR_NODE_TYPE_STR && right->node_type == PARSER_EXPR_NODE_TYPE_STR)
            {
                expr->node_type = PARSER_EXPR_NODE_TYPE_BOOL;
                if(string_literal_byte_compare(left->str, right->str, &res) != 0)
                {
                    return -1;
                }

                if((0 == res && (PARSER_EXPR_OP_TYPE_EQ == op || PARSER_EXPR_OP_TYPE_GE == op || PARSER_EXPR_OP_TYPE_LE == op))
                        || (0 > res && (PARSER_EXPR_OP_TYPE_NE == op || PARSER_EXPR_OP_TYPE_LT == op || PARSER_EXPR_OP_TYPE_LE == op))
                        || (0 < res && (PARSER_EXPR_OP_TYPE_NE == op || PARSER_EXPR_OP_TYPE_GT == op || PARSER_EXPR_OP_TYPE_GE == op)))
                {
                    expr->boolean = 1;
                }
                else
                {
                    expr->boolean = 0;
                }
            }
            else if(arg_null)
            {
                expr->boolean = 0;
                expr->node_type = PARSER_EXPR_NODE_TYPE_BOOL;
            }
            else
            {
                error_set(ERROR_DATATYPE_MISMATCH);
                return -1;
            }

            break;
        case PARSER_EXPR_OP_TYPE_IS:
        case PARSER_EXPR_OP_TYPE_IS_NOT:
            if(left->node_type != right->node_type)
            {
                expr->boolean = 0;
                expr->node_type = PARSER_EXPR_NODE_TYPE_BOOL;
            }
            else if(left->node_type == PARSER_EXPR_NODE_TYPE_NUM)
            {
                expr->node_type = PARSER_EXPR_NODE_TYPE_BOOL;
                cmp_res = decimal_cmp(&left->num, &right->num);
                if(0 == cmp_res)
                {
                    expr->boolean = 1;
                }
                else
                {
                    expr->boolean = 0;
                }
            }
            else if(left->node_type == PARSER_EXPR_NODE_TYPE_STR)
            {
                expr->node_type = PARSER_EXPR_NODE_TYPE_BOOL;
                if(string_literal_byte_compare(left->str, right->str, &res) != 0)
                {
                    return -1;
                }

                if(0 == res)
                {
                    expr->boolean = 1;
                }
                else
                {
                    expr->boolean = 0;
                }
            }
            else if(left->node_type == PARSER_EXPR_NODE_TYPE_BOOL)
            {
                expr->boolean = left->boolean & right->boolean;
                expr->node_type = PARSER_EXPR_NODE_TYPE_BOOL;
            }
            else if(arg_null)
            {
                expr->boolean = 1;
                expr->node_type = PARSER_EXPR_NODE_TYPE_BOOL;
            }
            else
            {
                error_set(ERROR_DATATYPE_MISMATCH);
                return -1;
            }

            if(op == PARSER_EXPR_OP_TYPE_IS_NOT)
            {
                expr->boolean ^= 1;
            }

            break;
        case PARSER_EXPR_OP_TYPE_BETWEEN:
            // TODO:
            break;
        case PARSER_EXPR_OP_TYPE_NOT:
            if(left->node_type == PARSER_EXPR_NODE_TYPE_BOOL)
            {
                expr->boolean = left->boolean ^ 1;
                expr->node_type = PARSER_EXPR_NODE_TYPE_BOOL;
            }
            else
            {
                error_set(ERROR_DATATYPE_MISMATCH);
                return -1;
            }
            break;
        case PARSER_EXPR_OP_TYPE_AND:
            if(left->node_type == PARSER_EXPR_NODE_TYPE_BOOL && right->node_type == PARSER_EXPR_NODE_TYPE_BOOL)
            {
                expr->boolean = left->boolean & right->boolean;
                expr->node_type = PARSER_EXPR_NODE_TYPE_BOOL;
            }
            else
            {
                error_set(ERROR_DATATYPE_MISMATCH);
                return -1;
            }
            break;
        case PARSER_EXPR_OP_TYPE_OR:
            if(left->node_type == PARSER_EXPR_NODE_TYPE_BOOL && right->node_type == PARSER_EXPR_NODE_TYPE_BOOL)
            {
                expr->boolean = left->boolean | right->boolean;
                expr->node_type = PARSER_EXPR_NODE_TYPE_BOOL;
            }
            else
            {
                error_set(ERROR_DATATYPE_MISMATCH);
                return -1;
            }
            break;
        default:
            assert(1 == 0);
            return -1;
    }

    return 0;
}

sint8 expression_calc_const_expr(parser_ast_expr *expr, handle sh)
{
    parser_ast_expr *prev = expr;

    stack_reset_sp(sh);

    if(PARSER_EXPR_NODE_TYPE_OP == expr->node_type)
    {
        do
        {
            if(prev != expr->left && prev != expr->right && PARSER_EXPR_NODE_TYPE_OP == expr->left->node_type)
            {
                if(stack_push(sh, &expr) != 0) return -1;
                expr = expr->left;
            }
            else
            {
                if(PARSER_EXPR_OP_TYPE_NOT != expr->op && prev != expr->right && PARSER_EXPR_NODE_TYPE_OP == expr->right->node_type)
                {
                    if(stack_push(sh, &expr) != 0) return -1;
                    expr = expr->right;
                }
                else
                {
                    if(expression_calc_base_expr(expr, 1) != 0) return -1;

                    if(stack_elnum(sh) > 0)
                    {
                        prev = expr;
                        if(stack_pop(sh, &expr) != 0) return -1;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
        while(1);
    }

    return 0;
}

