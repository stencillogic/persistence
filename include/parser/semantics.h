#ifndef _SEMANTICS_H
#define _SEMANTICS_H


// sql statement semantic checking


#include "defs/defs.h"
#include "parser/parser.h"


// perform semantic analysys and different transformations on ast
// return 0 on success, 1 on semantic error, -1 on internal error
sint8 semantics_check_stmt(parser_ast_stmt *stmt);


#endif
