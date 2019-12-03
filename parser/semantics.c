#include "parser/semantics.h"
#include "logging/logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>


#define SEMANTICS_ERRMES_BUF_SZ    (1024)


struct _semantics_state
{
    achar               errmes[SEMANTICS_ERRMES_BUF_SZ];       // buffer for formatted error message
    sint8               (*report_error)(error_code error, const achar *msg);
} g_semantics_state =
{
    .report_error = NULL,
};


sint8 semantics_check_select_stmt(const parser_ast_select *stmt);


sint8 semantics_report_error(const achar *msg, ...)
{
    va_list args;
    va_start(args, msg);

    vsnprintf(g_semantics_state.errmes, SEMANTICS_ERRMES_BUF_SZ, msg, args);
    g_semantics_state.errmes[SEMANTICS_ERRMES_BUF_SZ - 1] = _ach('\0');

    va_end(args);

    if(g_semantics_state.report_error(ERROR_SEMANTIC_ERROR, g_semantics_state.errmes) != 0) return -1;

    return 0;
}


sint8 semantics_check_single_select(const parser_ast_single_select *stmt, uint64 colnum)
{
    sint8 res;

    // check colnum matches the number of columns in projection
    if(colnum > 0 && stmt->projection_cnt > 0 && colnum != stmt->projection_cnt)
    {
        if(0 != semantics_report_error(_ach("number of selected columns doesn't match with the number of columns in first select at line %lu, col %lu"),
                    stmt->pos.line, stmt->pos.col)) return -1;
        return 1;
    }

    // if select "*" then from section is mandatory
    if(NULL == stmt->projection && NULL == stmt->from)
    {
        if(0 != semantics_report_error(_ach("FROM statement is required when selecting with * at line %lu, col %lu"),
                    stmt->pos.line, stmt->pos.col)) return -1;
        return 1;
    }

    // check group by is specified
    if(NULL != stmt->having && NULL == stmt->group_by)
    {
        if(0 != semantics_report_error(_ach("GROUP BY is required when HAVING is specified at line %lu, col %lu"),
                    stmt->pos.line, stmt->pos.col)) return -1;
        return 1;
    }

    return 0;
}

sint8 semantics_check_orderby(const parser_ast_order_by *stmt)
{

    return 0;
}


sint8 semantics_check_select_stmt(const parser_ast_select *stmt)
{
    sint8 res;
    uint64 colnum = 0;

    colnum = stmt->select.projection_cnt;

    do
    {
        if(0 != (res = semantics_check_single_select(&stmt->select, colnum))) return res;

        if(NULL != stmt->order_by && NULL != stmt->next)
        {
            if(0 != semantics_report_error(_ach("ORDER BY is not allowed here at line %lu, col %lu"),
                        stmt->order_by->expr.pos.line, stmt->order_by->expr.pos.col)) return -1;
            return 1;
        }

        stmt = stmt->next;
    }
    while(stmt);

    if(stmt->order_by)
    {
        if(0 != (res = semantics_check_orderby(stmt->order_by))) return res;
    }

    return 0;
}

sint8 semantics_check_insert_stmt(const parser_ast_insert *stmt)
{

    return 0;
}

sint8 semantics_check_update_stmt(const parser_ast_update *stmt)
{

    return 0;
}

sint8 semantics_check_delete_stmt(const parser_ast_delete *stmt)
{

    return 0;
}

sint8 semantics_check_create_table_stmt(const parser_ast_create_table *stmt)
{

    return 0;
}

sint8 semantics_check_drop_table_stmt(const parser_ast_drop_table *stmt)
{
    return 0;
}

sint8 semantics_checkalter_table_column(parser_ast_alter_table_column *stmt)
{

    return 0;
}

sint8 semantics_checkalter_table_add_constr(parser_ast_constr *stmt)
{
    return 0;
}

sint8 semantics_checkalter_table_drop_constr(parser_ast_drop_constraint *stmt)
{
    return 0;
}

sint8 semantics_checkalter_table_rename(parser_ast_rename_table *stmt)
{
    return 0;
}

sint8 semantics_checkalter_table_rename_column(parser_ast_rename_column *stmt)
{
    return 0;
}

sint8 semantics_checkalter_table_rename_constr(parser_ast_rename_constr *stmt)
{
    return 0;
}

sint8 semantics_check_alter_table_stmt(const parser_ast_alter_table *stmt)
{
    return 0;
}

sint8 semantics_check_create_database_stmt(const parser_ast_create_database *stmt)
{
    return 0;
}

sint8 semantics_check_drop_database_stmt(const parser_ast_drop_database *stmt)
{
    return 0;
}

sint8 semantics_check_stmt(const parser_ast_stmt *stmt, const semantics_interface *si)
{
    g_semantics_state.report_error = si->report_error;

    switch(stmt->type)
    {
        case PARSER_STMT_TYPE_SELECT:
            return semantics_check_select_stmt(&stmt->select_stmt);
            break;
        case PARSER_STMT_TYPE_INSERT:
            return semantics_check_insert_stmt(&stmt->insert_stmt);
            break;
        case PARSER_STMT_TYPE_UPDATE:
            return semantics_check_update_stmt(&stmt->update_stmt);
            break;
        case PARSER_STMT_TYPE_DELETE:
            return semantics_check_delete_stmt(&stmt->delete_stmt);
            break;
        case PARSER_STMT_TYPE_CREATE_TABLE:
            return semantics_check_create_table_stmt(&stmt->create_table_stmt);
            break;
        case PARSER_STMT_TYPE_DROP_TABLE:
            return semantics_check_drop_table_stmt(&stmt->drop_table_stmt);
            break;
        case PARSER_STMT_TYPE_ALTER_TABLE:
            return semantics_check_alter_table_stmt(&stmt->alter_table_stmt);
            break;
        case PARSER_STMT_TYPE_CREATE_DATABASE:
            return semantics_check_create_database_stmt(&stmt->create_database_stmt);
            break;
        case PARSER_STMT_TYPE_DROP_DATABASE:
            return semantics_check_drop_database_stmt(&stmt->drop_database_stmt);
            break;
        default:
            assert(0);
            logger_error(_ach("semantics: unknown statement type"));
            return -1;
    }

    return 0;
}
