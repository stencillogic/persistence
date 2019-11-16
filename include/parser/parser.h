#ifndef _PARSER_H
#define _PARSER_H


// sql statement parser


#include "defs/defs.h"
#include "table/table.h"
#include "parser/lexer.h"


#define PARSER_MAX_ALIAS_NAME_LEN           (MAX_TABLE_COL_NAME_LEN)
#define PARSER_MAX_DATABASE_NAME_LEN        (MAX_TABLE_NAME_LEN)
#define PARSER_DEFAULT_VARCHAR_LEN          (MAX_VARCHAR_LENGTH)
#define PARSER_DEFAULT_DECIMAL_PRECISION    (DECIMAL_POSITIONS)
#define PARSER_DEFAULT_DECIMAL_SCALE        (DECIMAL_POSITIONS / 2)
#define PARSER_DEFAULT_TS_PRECISION         (6)


// ENUM: statement type
typedef enum _parser_stmt_type
{
    PARSER_STMT_TYPE_SELECT = 1,
    PARSER_STMT_TYPE_INSERT = 2,
    PARSER_STMT_TYPE_INSERT_VALUES = 3,
    PARSER_STMT_TYPE_INSERT_SELECT = 4,
    PARSER_STMT_TYPE_UPDATE = 5,
    PARSER_STMT_TYPE_DELETE = 6,
    PARSER_STMT_TYPE_CREATE_TABLE = 50,
    PARSER_STMT_TYPE_DROP_TABLE = 51,
    PARSER_STMT_TYPE_ALTER_TABLE = 100,
    PARSER_STMT_TYPE_ALTER_TABLE_ADD_COL = 101,
    PARSER_STMT_TYPE_ALTER_TABLE_MODIFY_COL = 102,
    PARSER_STMT_TYPE_ALTER_TABLE_DROP_COL = 103,
    PARSER_STMT_TYPE_ALTER_TABLE_ADD_CONSTR = 104,
    PARSER_STMT_TYPE_ALTER_TABLE_DROP_CONSTR = 105,
    PARSER_STMT_TYPE_ALTER_TABLE_RENAME = 106,
    PARSER_STMT_TYPE_ALTER_TABLE_RENAME_COLUMN = 107,
    PARSER_STMT_TYPE_ALTER_TABLE_RENAME_CONSTR = 108,
    PARSER_STMT_TYPE_CREATE_DATABASE = 200,
    PARSER_STMT_TYPE_DROP_DATABASE = 201,
} parser_stmt_type;


// ENUM: setop type
typedef enum _parser_setop_type
{
    PARSER_SETOP_TYPE_UNION = 1,
    PARSER_SETOP_TYPE_INTERSECT= 2,
    PARSER_SETOP_TYPE_EXCEPT = 3,
    PARSER_SETOP_TYPE_UNION_ALL = 11,
    PARSER_SETOP_TYPE_INTERSECT_ALL= 12,
    PARSER_SETOP_TYPE_EXCEPT_ALL = 13,
} parser_setop_type;


// ENUM: sort type
typedef enum _parser_sort_type
{
    PARSER_SORT_TYPE_ASC = 1,
    PARSER_SORT_TYPE_DESC = 2,
} parser_sort_type;


// ENUM: null_order type
typedef enum _parser_null_order_type
{
    PARSER_NULL_ORDER_TYPE_FIRST = 1,
    PARSER_NULL_ORDER_TYPE_LAST = 2,
} parser_null_order_type;


// ENUM: expr_op type
// !!!!!!!!!!!!!!! when modifying this modify parser_state.expr_op_level !!!!!!!!!!!!!!!!
typedef enum _parser_expr_op_type
{
    PARSER_EXPR_OP_TYPE_NONE = 0,

    // group 1 (highest priority)
    PARSER_EXPR_OP_TYPE_MUL = 1,
    PARSER_EXPR_OP_TYPE_DIV = 2,

    // group 2
    PARSER_EXPR_OP_TYPE_ADD = 3,
    PARSER_EXPR_OP_TYPE_SUB = 4,

    // group 3
    PARSER_EXPR_OP_TYPE_EQ = 5,
    PARSER_EXPR_OP_TYPE_NE = 6,
    PARSER_EXPR_OP_TYPE_GT = 7,
    PARSER_EXPR_OP_TYPE_GE = 8,
    PARSER_EXPR_OP_TYPE_LT = 9,
    PARSER_EXPR_OP_TYPE_LE = 10,

    // group 4
    PARSER_EXPR_OP_TYPE_IS = 11,
    PARSER_EXPR_OP_TYPE_IS_NOT = 12,
    PARSER_EXPR_OP_TYPE_BETWEEN = 13,

    // group 5
    PARSER_EXPR_OP_TYPE_NOT = 14,

    // group 6
    PARSER_EXPR_OP_TYPE_AND = 15,

    // group 7 (lowest priority)
    PARSER_EXPR_OP_TYPE_OR = 16,

} parser_expr_op_type;


// ENUM: uniq type
typedef enum _parser_uniq_type
{
    PARSER_UNIQ_TYPE_ALL = 1,
    PARSER_UNIQ_TYPE_DISTINCT = 2,
} parser_uniq_type;


// ENUM: join type
typedef enum _parser_join_type
{
    PARSER_JOIN_TYPE_NONE = 0,
    PARSER_JOIN_TYPE_INNER = 1,
    PARSER_JOIN_TYPE_CROSS = 2,
    PARSER_JOIN_TYPE_LEFT = 3,
    PARSER_JOIN_TYPE_RIGHT = 4,
    PARSER_JOIN_TYPE_FULL = 5,
} parser_join_type;


// ENUM: constraint type
typedef enum _parser_constraint_type
{
    PARSER_CONSTRAINT_TYPE_CHECK = 1,
    PARSER_CONSTRAINT_TYPE_DEFAULT = 2,
    PARSER_CONSTRAINT_TYPE_UNIQUE = 3,
    PARSER_CONSTRAINT_TYPE_PK = 4,
    PARSER_CONSTRAINT_TYPE_FK = 5,
} parser_constraint_type;


// ENUM: expression tree node type
typedef enum _parser_expr_node_type
{
    PARSER_EXPR_NODE_TYPE_NUM = 1,
    PARSER_EXPR_NODE_TYPE_STR = 2,
    PARSER_EXPR_NODE_TYPE_NAME = 3,
    PARSER_EXPR_NODE_TYPE_OP = 4,
    PARSER_EXPR_NODE_TYPE_NULL = 5,
} parser_expr_node_type;


// ENUM: from type
typedef enum _parser_from_type
{
    PARSER_FROM_TYPE_NAME = 1,
    PARSER_FROM_TYPE_SUBQUERY = 2,
} parser_from_type;


// ENUM: fk "on delete" action
typedef enum _parser_on_delete
{
    PARSER_ON_DELETE_RESTRICT = 1,
    PARSER_ON_DELETE_CASCADE = 2,
    PARSER_ON_DELETE_SET_NULL = 3,
    PARSER_ON_DELETE_SET_DEFAULT = 4,
    PARSER_ON_DELETE_NO_ACTION = 5
} parser_on_delete;



// ENUM: fk "on delete" action
typedef enum _parser_select_type
{
    PARSER_SELECT_TYPE_SINGLE,
    PARSER_SELECT_TYPE_SETOP
} parser_select_type;


///////////////////////////////// GENERAL CONSTRUCTS



// AST NODE: name
typedef struct _parser_ast_name
{
    uint16  first_part_len;
    uint16  second_part_len;
    uint8   first_part[LEXER_MAX_IDENTIFIER_LEN];
    uint8   second_part[LEXER_MAX_IDENTIFIER_LEN];
} parser_ast_name;


// AST NODE: expression tree
typedef struct _parser_ast_expr parser_ast_expr;
typedef struct _parser_ast_expr
{
    union
    {
        parser_expr_op_type op;
        void                *str;
        decimal             num;
        parser_ast_name     name;
    };
    parser_expr_node_type   node_type;
    parser_ast_expr         *left;
    parser_ast_expr         *right;
} parser_ast_expr;


// AST NODE: expression list with aliases
typedef struct _parser_ast_named_expr
{
    parser_ast_expr         expr;
    uint8                   alias[PARSER_MAX_ALIAS_NAME_LEN];
    uint16                  alias_len;
} parser_ast_named_expr;


// AST NODE: expression list
typedef struct _parser_ast_expr_list parser_ast_expr_list;
typedef struct _parser_ast_expr_list
{
    union
    {
        parser_ast_named_expr   named_expr;
        parser_ast_expr         expr;
    };
    sint8                   named;      // if not 0 then named expr
    parser_ast_expr_list    *next;      // can be NULL
} parser_ast_expr_list;



///////////////////////////////// SELECT STATEMENT


// predefs
typedef struct _parser_ast_select parser_ast_select;
typedef struct _parser_ast_from parser_ast_from;


// AST NODE: parser single select
typedef struct _parser_ast_single_select
{
    parser_uniq_type        uniq_type;
    parser_ast_expr_list    *projection;    // if NULL all is selected
    parser_ast_from         *from;          // optional, can be NULL
    parser_ast_expr         *where;         // optional, can be NULL, must resolve to boolean
    parser_ast_expr_list    *group_by;      // optional, can be NULL
    parser_ast_expr_list    *having;        // optional, can be NULL, must resolve to boolean
} parser_ast_single_select;


// AST NODE: order by
typedef struct _parser_ast_order_by parser_ast_order_by;
typedef struct _parser_ast_order_by
{
    parser_ast_expr         expr;
    parser_sort_type        sort_type;
    parser_null_order_type  null_order;
    parser_ast_order_by     *next;      // optional, can be null
} parser_ast_order_by;


// AST NODE: select stmt
typedef struct _parser_ast_select
{
    parser_ast_single_select    *select;
    parser_setop_type           setop;
    parser_ast_select           *next;              // optional, can be NULL
    parser_ast_order_by         *order_by;          // optional, can be NULL
} parser_ast_select;


// AST NODE: from item
typedef struct _parser_ast_from
{
    parser_from_type        type;
    union
    {
        parser_ast_select   subquery;
        parser_ast_name     name;
    };
    uint8                   alias[MAX_TABLE_NAME_LEN];
    uint16                  alias_len;
    parser_join_type        join_type;
    parser_ast_from         *next;          // optional, can be NULL
    parser_ast_expr         *on_expr;       // optional, can be NULL, must resolve to boolean
} parser_ast_from;



///////////////////////////////// INSERT STATEMENT


// AST NODE: column name list
typedef struct _parser_ast_colname_list parser_ast_colname_list;
typedef struct _parser_ast_colname_list
{
    uint8                   col_name[MAX_TABLE_COL_NAME_LEN];
    uint16                  colname_len;
    parser_ast_colname_list *next;
} parser_ast_colname_list;


// AST NODE: insert statement
typedef struct _parser_ast_insert
{
    parser_stmt_type        type;
    parser_ast_name         target;
    parser_ast_colname_list *columns;       // optional, can be NULL
    union
    {
        parser_ast_expr_list    values;        // depending on type
        parser_ast_select       select_stmt;   // depending on type
    };
} parser_ast_insert;



///////////////////////////////// UPDATE STATEMENT


// AST NODE:
typedef struct _parser_ast_set_list parser_ast_set_list;
typedef struct _parser_ast_set_list
{
    parser_ast_name     column;
    parser_ast_expr     expr;
    parser_ast_set_list *next;      // can be NULL
} parser_ast_set_list;


// AST NODE: update
typedef struct _parser_ast_update
{
    parser_ast_name         target;
    uint8                   alias[PARSER_MAX_ALIAS_NAME_LEN];
    uint16                  alias_len;
    parser_ast_set_list     set_list;
    parser_ast_expr         *where;     // optional, can be NULL
} parser_ast_update;



///////////////////////////////// DELETE STATEMENT


// AST NODE: update
typedef struct _parser_ast_delete
{
    parser_ast_name         target;
    uint8                   alias[PARSER_MAX_ALIAS_NAME_LEN];
    uint16                  alias_len;
    parser_ast_expr         *where;     // optional, can be NULL
} parser_ast_delete;



///////////////////////////////// CREATE TABLE STATEMENT


// AST NODE: constraint
typedef struct _parser_ast_constr
{
    parser_constraint_type  type;
    uint8                   name[MAX_CONSTRAINT_NAME_LEN];
    uint16                  name_len;
    union
    {
        parser_ast_expr             expr;          // check
        parser_ast_colname_list     columns;       // unique, pk
        struct {
            parser_ast_colname_list columns;       // fk (soruce)
            parser_ast_colname_list ref_columns;   // fk (dest)
            parser_ast_name         ref_table;     // referenced table
            parser_on_delete        fk_on_delete;
        } fk;
    };
} parser_ast_constr;


// AST NODE: constaint list
typedef struct _parser_ast_constr_list parser_ast_constr_list;
typedef struct _parser_ast_constr_list
{
    parser_ast_constr       constr;
    parser_ast_constr_list  *next;
} parser_ast_constr_list;


// AST NODE: column datatype descriptor
typedef struct _parser_ast_col_datatype
{
    column_datatype     datatype;
    union
    {
        sint64 char_len;
        struct
        {
            sint8 precision;
            sint8 scale;
        } decimal;
        sint8 ts_precision;
    };
} parser_ast_col_datatype;


// AST NODE: column descriptor
typedef struct _parser_ast_col_desc
{
    uint8                   name[MAX_TABLE_COL_NAME_LEN];
    uint16                  name_len;
    parser_ast_col_datatype datatype;
    parser_ast_expr         *default_value;             // optional
    sint8                   nullable;                   // 1 - nullable, 2 - not null, 0 - not specified
} parser_ast_col_desc;


// AST NODE: column description list
typedef struct _parser_ast_col_desc_list parser_ast_col_desc_list;
typedef struct _parser_ast_col_desc_list
{
    parser_ast_col_desc         col_desc;
    parser_ast_col_desc_list    *next;
} parser_ast_col_desc_list;


// AST NODE: create table
typedef struct _parser_ast_create_table
{
    parser_ast_name             name;
    parser_ast_col_desc_list    cols;
    parser_ast_constr_list      *constr;    // optional
} parser_ast_create_table;



///////////////////////////////// DROP TABLE STATEMENT


// AST NODE: drop table
typedef struct _parser_ast_drop_table
{
    parser_ast_name     table;
} parser_ast_drop_table;



///////////////////////////////// ALTER TABLE STATEMENT



// AST NODE: alter table add/modify/drop column
typedef struct _parser_ast_alter_table_column
{
    uint8                   column[MAX_TABLE_COL_NAME_LEN];
    uint16                  column_len;
    uint8                   nullable;                       // 1 - nullable, 2 - not null, 0 - not specified
    parser_ast_col_datatype *datatype;
    parser_ast_expr         *default_expr;
} parser_ast_alter_table_column;


// AST NODE: alter table drop constraint
typedef struct _parser_ast_drop_constraint
{
    uint8           name[MAX_CONSTRAINT_NAME_LEN];
    uint16          name_len;
} parser_ast_drop_constraint;


// AST NODE: rename table
typedef struct _parser_ast_rename_table
{
    uint16          new_name_len;
    uint8           new_name[MAX_TABLE_NAME_LEN];
} parser_ast_rename_table;


// AST NODE: alter table rename constraint
typedef struct _parser_ast_rename_constr
{
    parser_ast_name table;
    uint16          name_len;
    uint16          new_name_len;
    uint8           name[MAX_CONSTRAINT_NAME_LEN];
    uint8           new_name[MAX_CONSTRAINT_NAME_LEN];
} parser_ast_rename_constr;


// AST NODE: alter table rename column
typedef struct _parser_ast_rename_column
{
    parser_ast_name table;
    uint16          name_len;
    uint16          new_name_len;
    uint8           name[MAX_TABLE_COL_NAME_LEN];
    uint8           new_name[MAX_TABLE_COL_NAME_LEN];
} parser_ast_rename_column;


// AST NODE: alter table
typedef struct _parser_ast_alter_table
{
    parser_ast_name         table;
    parser_stmt_type        type;
    union
    {
        parser_ast_alter_table_column   column;
        parser_ast_constr               add_constr;
        parser_ast_drop_constraint      drop_constr;
        parser_ast_rename_table         rename_table;
        parser_ast_rename_constr        rename_constr;
        parser_ast_rename_column        rename_column;
    };
} parser_ast_alter_table;



///////////////////////////////// CREATE DATABASE STATEMENT


// AST NODE: create database
typedef struct _parser_ast_create_database
{
    uint8           name[PARSER_MAX_DATABASE_NAME_LEN];
    uint16          name_len;
} parser_ast_create_database;



///////////////////////////////// DROP DATABASE STATEMENT


// AST NODE: drop database
typedef struct _parser_ast_drop_database
{
    uint8           name[PARSER_MAX_DATABASE_NAME_LEN];
    uint16          name_len;
} parser_ast_drop_database;



///////////////////////////////// SQL STATEMENT


// AST NODE: sql statement
typedef struct _parser_ast_stmt
{
    parser_stmt_type    type;
    union
    {
        parser_ast_select           select_stmt;
        parser_ast_insert           insert_stmt;
        parser_ast_update           update_stmt;
        parser_ast_delete           delete_stmt;
        parser_ast_create_table     create_table_stmt;
        parser_ast_create_database  create_database_stmt;
        parser_ast_alter_table      alter_table_stmt;
        parser_ast_drop_table       drop_table_stmt;
        parser_ast_drop_database    drop_database_stmt;
    };
} parser_ast_stmt;



/////////////////////////////////


typedef struct _parser_interface
{
    sint8           (*report_error)(error_code error, const achar *msg);
} parser_interface;


// parse statement into AST and return pointer to it
// return 0 on success, -1 on error, 1 on syntax error
sint8 parser_parse(parser_ast_stmt **stmt, handle lexer, parser_interface pi);


// deallocate statement
void parser_deallocate_stmt(parser_ast_stmt *stmt);


#endif
