#include "parser/semantics.h"
#include <assert.h>
/*

sint8 semantics_check_select_stmt(parser_ast_select *stmt1);


sint8 semantics_check_expr(parser_ast_expr *stmt1)
{
    sint8 ret;

    switch(stmt1->node_type)
    {
        case PARSER_EXPR_NODE_TYPE_OP:
            if(stmt1->op != stmt2->op)
            {
                return 1;
            }

            if(stmt1->left)
            {
                if(semantics_check_expr(stmt1->left, stmt2->left) != 0)
                {
                    return 1;
                }
            }

            if(test_parser_ptrs(stmt1->right, stmt2->right))
            {
                return 1;
            }

            if(stmt1->right)
            {
                if(semantics_check_expr(stmt1->right, stmt2->right) != 0)
                {
                    return 1;
                }
            }
            break;
        case PARSER_EXPR_NODE_TYPE_NUM:
            if(memcmp(&stmt1->num, &stmt2->num, sizeof(stmt1->num)))
            {
                return 1;
            }
            break;
        case PARSER_EXPR_NODE_TYPE_STR:
            string_literal_byte_compare(stmt1->str, stmt2->str, &ret);
            if(ret)
            {
                return 1;
            }
            break;
        case PARSER_EXPR_NODE_TYPE_NAME:
            if(semantics_check_name(&stmt1->name, &stmt2->name))
            {
                return 1;
            }
            break;
        case PARSER_EXPR_NODE_TYPE_NULL:
            break;
        default:
            return 1;
    }

    return 0;
}

sint8 semantics_check_named_expr(parser_ast_named_expr *stmt1)
{
    if(semantics_check_mem(stmt1->alias_len, stmt1->alias) != 0)
    {
        return 1;
    }

    if(semantics_check_expr(&stmt1->expr) != 0)
    {
        return 1;
    }

    return 0;
}

sint8 semantics_check_from(parser_ast_from *stmt1)
{
    if(semantics_check_mem(stmt1->alias_len, stmt1->alias) != 0)
    {
        return 1;
    }

    if(stmt1->join_type != stmt2->join_type)
    {
        return 1;
    }

    if(stmt1->type != stmt2->type)
    {
        return 1;
    }

    if(stmt1->type == PARSER_FROM_TYPE_NAME)
    {
        if(semantics_check_name(&stmt1->name) != 0)
        {
            return 1;
        }
    }
    else if(stmt1->type == PARSER_FROM_TYPE_SUBQUERY)
    {
        if(semantics_check_select_stmt(&stmt1->subquery) != 0)
        {
            return 1;
        }
    }
    else
    {
        return 1;
    }

    if(test_parser_ptrs(stmt1->next, stmt2->next))
    {
        return 1;
    }

    if(stmt1->next)
    {
        if(semantics_check_from(stmt1->next, stmt2->next) != 0)
        {
            return 1;
        }

        if(test_parser_ptrs(stmt1->on_expr, stmt2->on_expr))
        {
            return 1;
        }

        if(stmt1->on_expr)
        {
            if(semantics_check_expr(stmt1->on_expr, stmt2->on_expr) != 0)
            {
                return 1;
            }
        }
    }

    return 0;
}

sint8 semantics_check_expr_list(parser_ast_expr_list *stmt1)
{
    if(stmt1->named != stmt2->named)
    {
        return 1;
    }

    if(stmt1->named)
    {
        if(semantics_check_named_expr((parser_ast_named_expr *)&stmt1->expr, (parser_ast_named_expr *)&stmt2->expr) != 0)
        {
            return 1;
        }
    }
    else
    {
        if(semantics_check_expr(&stmt1->expr) != 0)
        {
            return 1;
        }
    }

    if(test_parser_ptrs(stmt1->next, stmt2->next))
    {
        return 1;
    }

    if(stmt1->next)
    {
        if(semantics_check_expr_list(stmt1->next, stmt2->next) != 0)
        {
            return 1;
        }
    }

    return 0;
}


sint8 semantics_check_single_select(parser_ast_single_select *stmt1)
{
    if(stmt1->uniq_type != stmt2->uniq_type)
    {
        return 1;
    }

    if(test_parser_ptrs(stmt1->from, stmt2->from))
    {
        return 1;
    }

    if(stmt1->from)
    {
        if(semantics_check_from(stmt1->from, stmt2->from) != 0)
        {
            return 1;
        }
    }

    if(test_parser_ptrs(stmt1->group_by, stmt2->group_by))
    {
        return 1;
    }

    if(stmt1->group_by)
    {
        if(semantics_check_expr_list(stmt1->group_by, stmt2->group_by) != 0)
        {
            return 1;
        }
    }

    if(test_parser_ptrs(stmt1->having, stmt2->having))
    {
        return 1;
    }

    if(stmt1->having)
    {
        if(semantics_check_expr_list(stmt1->having, stmt2->having) != 0)
        {
            return 1;
        }
    }

    if(test_parser_ptrs(stmt1->projection, stmt2->projection))
    {
        return 1;
    }

    if(stmt1->projection)
    {
        if(semantics_check_expr_list(stmt1->projection, stmt2->projection) != 0)
        {
            return 1;
        }
    }

    if(test_parser_ptrs(stmt1->where, stmt2->where))
    {
        return 1;
    }

    if(stmt1->where)
    {
        if(semantics_check_expr(stmt1->where, stmt2->where) != 0)
        {
            return 1;
        }
    }

    return 0;
}

sint8 semantics_check_orderby(parser_ast_order_by *stmt1)
{
    if(stmt1->null_order != stmt2->null_order)
    {
        return 1;
    }

    if(stmt1->sort_type != stmt2->sort_type)
    {
        return 1;
    }

    if(semantics_check_expr(&stmt1->expr) != 0)
    {
        return 1;
    }

    if(test_parser_ptrs(stmt1->next, stmt2->next))
    {
        return 1;
    }

    if(stmt1->next)
    {
        if(semantics_check_orderby(stmt1->next, stmt2->next) != 0)
        {
            return 1;
        }
    }

    return 0;
}

sint8 semantics_check_select_stmt(parser_ast_select *stmt1)
{
    if(stmt1->setop != stmt2->setop)
    {
        return 1;
    }

    if(test_parser_ptrs(stmt1->select, stmt2->select))
    {
        return 1;
    }

    if(stmt1->select)
    {
        if(semantics_check_single_select(stmt1->select, stmt2->select) != 0)
        {
            return 1;
        }
    }

    if(test_parser_ptrs(stmt1->next, stmt2->next))
    {
        return 1;
    }

    if(stmt1->next)
    {
        if(semantics_check_select_stmt(stmt1->next, stmt2->next) != 0)
        {
            return 1;
        }
    }

    if(test_parser_ptrs(stmt1->order_by, stmt2->order_by))
    {
        return 1;
    }

    if(stmt1->order_by)
    {
        if(semantics_check_orderby(stmt1->order_by, stmt2->order_by) != 0)
        {
            return 1;
        }
    }

    return 0;
}

sint8 semantics_check_colname_list(parser_ast_colname_list *stmt1)
{
    if(semantics_check_mem(stmt1->colname_len, stmt1->col_name) != 0)
    {
        return 1;
    }

    if(test_parser_ptrs(stmt1->next, stmt2->next))
    {
        return 1;
    }

    if(stmt1->next)
    {
        if(semantics_check_colname_list(stmt1->next, stmt2->next) != 0)
        {
            return 1;
        }
    }
    return 0;
}

sint8 semantics_check_insert_stmt(parser_ast_insert *stmt1)
{
    if(stmt1->type != stmt2->type)
    {
        return 1;
    }

    if(semantics_check_name(&stmt1->target) != 0)
    {
        return 1;
    }

    if(stmt1->type == PARSER_STMT_TYPE_INSERT_SELECT)
    {
        if(semantics_check_select_stmt(&stmt1->select_stmt) != 0)
        {
            return 1;
        }
    }
    else if(stmt1->type == PARSER_STMT_TYPE_INSERT_VALUES)
    {
        if(semantics_check_expr_list(&stmt1->values) != 0)
        {
            return 1;
        }
    }
    else
    {
        return 1;
    }

    if(test_parser_ptrs(stmt1->columns, stmt2->columns))
    {
        return 1;
    }

    if(stmt1->columns)
    {
        if(semantics_check_colname_list(stmt1->columns, stmt2->columns) != 0)
        {
            return 1;
        }
    }

    return 0;
}

sint8 semantics_check_set_list(parser_ast_set_list *stmt1)
{

    if(semantics_check_name(&stmt1->column) != 0)
    {
        return 1;
    }

    if(semantics_check_expr(&stmt1->expr, &stmt2->expr))
    {
        return 1;
    }

    if(test_parser_ptrs(stmt1->next, stmt2->next))
    {
        return 1;
    }

    if(stmt1->next)
    {
        if(semantics_check_set_list(stmt1->next, stmt2->next) != 0)
        {
            return 1;
        }
    }

    return 0;
}

sint8 semantics_check_update_stmt(parser_ast_update *stmt1)
{
    if(semantics_check_mem(stmt1->alias_len, stmt1->alias) != 0)
    {
        return 1;
    }

    if(semantics_check_set_list(&stmt1->set_list) != 0)
    {
        return 1;
    }

    if(semantics_check_name(&stmt1->target) != 0)
    {
        return 1;
    }

    if(test_parser_ptrs(stmt1->where, stmt2->where))
    {
        return 1;
    }

    if(stmt1->where)
    {
        if(semantics_check_expr(stmt1->where, stmt2->where) != 0)
        {
            return 1;
        }
    }

    return 0;
}

sint8 semantics_check_delete_stmt(parser_ast_delete *stmt1)
{
    if(semantics_check_mem(stmt1->alias_len, stmt1->alias) != 0)
    {
        return 1;
    }

    if(semantics_check_name(&stmt1->target) != 0)
    {
        return 1;
    }

    if(test_parser_ptrs(stmt1->where, stmt2->where))
    {
        return 1;
    }

    if(stmt1->where)
    {
        if(semantics_check_expr(stmt1->where, stmt2->where) != 0)
        {
            return 1;
        }
    }

    return 0;
}

sint8 semantics_check_datatype(parser_ast_col_datatype *stmt1)
{
    if(stmt1->datatype != stmt2->datatype)
    {
        return 1;
    }

    switch(stmt1->datatype)
    {
        case CHARACTER_VARYING:
            if(stmt1->char_len != stmt2->char_len)
            {
                return 1;
            }
            break;
        case DECIMAL:
            if(stmt1->decimal.precision != stmt2->decimal.precision)
            {
                return 1;
            }

            if(stmt1->decimal.scale != stmt2->decimal.scale)
            {
                return 1;
            }
            break;
        case TIMESTAMP:
        case TIMESTAMP_WITH_TZ:
            if(stmt1->ts_precision != stmt2->ts_precision)
            {
                return 1;
            }
            break;
        default:
            break;
    }

    return 0;
}

sint8 semantics_check_coldesc(parser_ast_col_desc *stmt1)
{
    if(stmt1->nullable != stmt2->nullable)
    {
        return 1;
    }

    if(semantics_check_mem(stmt1->name_len, stmt1->name) != 0)
    {
        return 1;
    }

    if(semantics_check_datatype(&stmt1->datatype) != 0)
    {
        return 1;
    }

    if(test_parser_ptrs(stmt1->default_value, stmt2->default_value))
    {
        return 1;
    }

    if(stmt1->default_value)
    {
        if(semantics_check_expr(stmt1->default_value, stmt2->default_value) != 0)
        {
            return 1;
        }
    }

    return 0;
}

sint8 semantics_check_coldesc_list(parser_ast_col_desc_list *stmt1)
{
    if(semantics_check_coldesc(&stmt1->col_desc) != 0)
    {
        return 1;
    }

    if(test_parser_ptrs(stmt1->next, stmt2->next))
    {
        return 1;
    }

    if(stmt1->next)
    {
        if(semantics_check_coldesc_list(stmt1->next, stmt2->next) != 0)
        {
            return 1;
        }
    }

    return 0;
}

sint8 semantics_check_constr(parser_ast_constr *stmt1)
{
    if(stmt1->type != stmt2->type)
    {
        return 1;
    }

    if(semantics_check_mem(stmt1->name_len, stmt1->name) != 0)
    {
        return 1;
    }

    switch(stmt1->type)
    {
        case PARSER_CONSTRAINT_TYPE_CHECK:
            if(semantics_check_expr(&stmt1->expr) != 0)
            {
                return 1;
            }
            break;
        case PARSER_CONSTRAINT_TYPE_UNIQUE:
        case PARSER_CONSTRAINT_TYPE_PK:
            if(semantics_check_colname_list(&stmt1->columns) != 0)
            {
                return 1;
            }
            break;
        case PARSER_CONSTRAINT_TYPE_FK:
            if(semantics_check_name(&stmt1->fk.ref_table) != 0)
            {
                return 1;
            }

            if(semantics_check_colname_list(&stmt1->fk.columns) != 0)
            {
                return 1;
            }

            if(test_parser_ptrs(stmt1->fk.ref_columns, stmt2->fk.ref_columns))
            {
                return 1;
            }

            if(stmt1->fk.ref_columns)
            {
                if(semantics_check_colname_list(stmt1->fk.ref_columns, stmt2->fk.ref_columns) != 0)
                {
                    return 1;
                }
            }

            if(stmt1->fk.fk_on_delete != stmt2->fk.fk_on_delete)
            {
                return 1;
            }
            break;
        default:
            return 1;
    }
    return 0;
}

sint8 semantics_check_constr_list(parser_ast_constr_list *stmt1)
{
    if(semantics_check_constr(&stmt1->constr) != 0)
    {
        return 1;
    }

    if(test_parser_ptrs(stmt1->next, stmt2->next))
    {
        return 1;
    }

    if(stmt1->next)
    {
        if(semantics_check_constr_list(stmt1->next, stmt2->next) != 0)
        {
            return 1;
        }
    }
    return 0;
}

sint8 semantics_check_create_table_stmt(parser_ast_create_table *stmt1)
{
    if(semantics_check_name(&stmt1->name) != 0)
    {
        return 1;
    }

    if(semantics_check_coldesc_list(&stmt1->cols) != 0)
    {
        return 1;
    }

    if(test_parser_ptrs(stmt1->constr, stmt2->constr))
    {
        return 1;
    }

    if(stmt1->constr)
    {
        if(semantics_check_constr_list(stmt1->constr, stmt2->constr) != 0)
        {
            return 1;
        }
    }

    return 0;
}

sint8 semantics_check_drop_table_stmt(parser_ast_drop_table *stmt1)
{
    if(semantics_check_name(&stmt1->table) != 0)
    {
        return 1;
    }

    return 0;
}

sint8 test_parser_alter_table_column(parser_ast_alter_table_column *stmt1)
{
    if(stmt1->nullable != stmt2->nullable)
    {
        return 1;
    }

    if(semantics_check_mem(stmt1->column_len, stmt1->column) != 0)
    {
        return 1;
    }

    if(test_parser_ptrs(stmt1->datatype, stmt2->datatype))
    {
        return 1;
    }

    if(stmt1->datatype != NULL)
    {
        if(semantics_check_datatype(stmt1->datatype, stmt2->datatype) != 0)
        {
            return 1;
        }
    }

    if(test_parser_ptrs(stmt1->default_expr, stmt2->default_expr))
    {
        return 1;
    }

    if(stmt1->default_expr)
    {
        if(semantics_check_expr(stmt1->default_expr, stmt2->default_expr) != 0)
        {
            return 1;
        }
    }

    return 0;
}

sint8 test_parser_alter_table_add_constr(parser_ast_constr *stmt1)
{
    if(semantics_check_constr(stmt1, stmt2) != 0)
    {
        return 1;
    }

    return 0;
}

sint8 test_parser_alter_table_drop_constr(parser_ast_drop_constraint *stmt1)
{
    if(semantics_check_mem(stmt1->name_len, stmt1->name) != 0)
    {
        return 1;
    }

    return 0;
}

sint8 test_parser_alter_table_rename(parser_ast_rename_table *stmt1)
{
    if(semantics_check_mem(stmt1->new_name_len, stmt1->new_name) != 0)
    {
        return 1;
    }

    return 0;
}

sint8 test_parser_alter_table_rename_column(parser_ast_rename_column *stmt1)
{
    if(semantics_check_mem(stmt1->name_len, stmt1->name) != 0)
    {
        return 1;
    }

    if(semantics_check_mem(stmt1->new_name_len, stmt1->new_name) != 0)
    {
        return 1;
    }

    return 0;
}

sint8 test_parser_alter_table_rename_constr(parser_ast_rename_constr *stmt1)
{
    if(semantics_check_mem(stmt1->name_len, stmt1->name) != 0)
    {
        return 1;
    }

    if(semantics_check_mem(stmt1->new_name_len, stmt1->new_name) != 0)
    {
        return 1;
    }

    return 0;
}


sint8 semantics_check_alter_table_stmt(parser_ast_alter_table *stmt1)
{
    if(stmt1->type != stmt2->type)
    {
        return 1;
    }

    if(semantics_check_name(&stmt1->table) != 0)
    {
        return 1;
    }

    switch(stmt1->type)
    {
        case PARSER_STMT_TYPE_ALTER_TABLE_ADD_COL:
        case PARSER_STMT_TYPE_ALTER_TABLE_MODIFY_COL:
        case PARSER_STMT_TYPE_ALTER_TABLE_DROP_COL:
            if(test_parser_alter_table_column(&stmt1->column) return 1;
            break;
        case PARSER_STMT_TYPE_ALTER_TABLE_ADD_CONSTR:
            if(test_parser_alter_table_add_constr(&stmt1->add_constr) return 1;
            break;
        case PARSER_STMT_TYPE_ALTER_TABLE_DROP_CONSTR:
            if(test_parser_alter_table_drop_constr(&stmt1->drop_constr) return 1;
            break;
        case PARSER_STMT_TYPE_ALTER_TABLE_RENAME:
            if(test_parser_alter_table_rename(&stmt1->rename_table) return 1;
            break;
        case PARSER_STMT_TYPE_ALTER_TABLE_RENAME_COLUMN:
            if(test_parser_alter_table_rename_column(&stmt1->rename_column) return 1;
            break;
        case PARSER_STMT_TYPE_ALTER_TABLE_RENAME_CONSTR:
            if(test_parser_alter_table_rename_constr(&stmt1->rename_constr) return 1;
            break;
        default:
            return 1;
    }


    return 0;
}

sint8 semantics_check_create_database_stmt(parser_ast_create_database *stmt1)
{
    if(semantics_check_mem(stmt1->name_len, stmt1->name) != 0)
    {
        return 1;
    }

    return 0;
}

sint8 semantics_check_drop_database_stmt(parser_ast_drop_database *stmt1)
{
    if(semantics_check_mem(stmt1->name_len, stmt1->name) != 0)
    {
        return 1;
    }

    return 0;
}


// return 0 if statements are identical
sint8 semantics_check_stmt(parser_ast_stmt *stmt)
{
    switch(stmt1->type)
    {
        case PARSER_STMT_TYPE_SELECT:
            return semantics_check_select_stmt(&stmt1->select_stmt);
            break;
        case PARSER_STMT_TYPE_INSERT:
            return semantics_check_insert_stmt(&stmt1->insert_stmt);
            break;
        case PARSER_STMT_TYPE_UPDATE:
            return semantics_check_update_stmt(&stmt1->update_stmt);
            break;
        case PARSER_STMT_TYPE_DELETE:
            return semantics_check_delete_stmt(&stmt1->delete_stmt);
            break;
        case PARSER_STMT_TYPE_CREATE_TABLE:
            return semantics_check_create_table_stmt(&stmt1->create_table_stmt);
            break;
        case PARSER_STMT_TYPE_DROP_TABLE:
            return semantics_check_drop_table_stmt(&stmt1->drop_table_stmt);
            break;
        case PARSER_STMT_TYPE_ALTER_TABLE:
            return semantics_check_alter_table_stmt(&stmt1->alter_table_stmt);
            break;
        case PARSER_STMT_TYPE_CREATE_DATABASE:
            return semantics_check_create_database_stmt(&stmt1->create_database_stmt);
            break;
        case PARSER_STMT_TYPE_DROP_DATABASE:
            return semantics_check_drop_database_stmt(&stmt1->drop_database_stmt);
            break;
        default:
            assert(0);
            logger_error(_ach("semantics: unknown statement type"));
            return -1;
    }

    return 0;
}
*/
