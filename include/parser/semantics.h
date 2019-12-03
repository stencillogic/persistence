#ifndef _SEMANTICS_H
#define _SEMANTICS_H


// sql statement semantic checking


#include "defs/defs.h"
#include "parser/parser.h"


typedef struct _semantics_interface
{
    sint8           (*report_error)(error_code error, const achar *msg);
} semantics_interface;


// perform semantic analysys and different transformations on ast
// return 0 on success, 1 on semantic error, -1 on internal error
sint8 semantics_check_stmt(const parser_ast_stmt *stmt, const semantics_interface *si);


#endif
