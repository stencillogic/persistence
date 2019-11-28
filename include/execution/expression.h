#ifndef _EXPRESSION_H
#define _EXPRESSION_H


// calculate expressions


#include "defs/defs.h"
#include "parser/parser.h"


// calculate expresion expr of kind: type = op, left = leaf argument (str, num, name or NULL), right = leaf argument
// if ignore_name is not 0 then nodes of type PARSER_EXPR_NODE_TYPE_NAME are resolved to values
// if ignore_name is not 0 and left or right = op/name then the expression is not calculated
// return 0 on success, non-0 on error
sint8 expression_calc_base_expr(parser_ast_expr *expr, uint8 ignore_name);

// calculate constant parts of expresion expr, put result to expr
// stack of tree depth size is required for keeping pointers on parser_ast_expr *
// return 0 on success, non-0 on error
sint8 expression_calc_const_expr(parser_ast_expr *expr, handle sh);

#endif
