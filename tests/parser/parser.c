#include "tests.h"
#include "parser/parser.h"
#include "common/string_literal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


struct
{
    uint64                  cur_char;
    const achar             *stmt;
    encoding_build_char_fun build_char;
    const achar             *expected_errmsg;
    error_code              expected_errcode;
} g_test_parser_state = {0, NULL, NULL, NULL, ERROR_SYNTAX_ERROR};


sint8 test_parser_char_feeder(char_info *ch, sint8 *eos)
{
    if(g_test_parser_state.cur_char == strlen(g_test_parser_state.stmt))
    {
        *eos = 1;
    }
    else
    {
        ch->length = 0u;
        ch->ptr = 0u;
        ch->state = CHAR_STATE_INCOMPLETE;
        *eos = 0;
        do
        {
            g_test_parser_state.build_char(ch, g_test_parser_state.stmt[g_test_parser_state.cur_char++]);
        }
        while(ch->state != CHAR_STATE_COMPLETE);
    }

    return 0;
}


sint8 test_parser_error_reporter(error_code error, const achar *msg)
{
    if(g_test_parser_state.expected_errmsg == NULL)
    {
        printf("Unexpected parsing error: %s\n", msg);
        return -1;
    }
    if(strcmp(msg, g_test_parser_state.expected_errmsg)) return -1;
    if(error != g_test_parser_state.expected_errcode) return -1;
    return 0;
}

void test_parser_print_expr_tree(parser_ast_expr *e, int lvl)
{
    if(!e) return;

    for(int i=0; i<lvl; i++) fputc(' ', stdout);
    if(e->node_type == PARSER_EXPR_NODE_TYPE_OP)
    {
        switch(e->op)
        {
            case PARSER_EXPR_OP_TYPE_NONE: puts("(none)"); break;
            case PARSER_EXPR_OP_TYPE_MUL: puts("*"); break;
            case PARSER_EXPR_OP_TYPE_DIV: puts("/"); break;
            case PARSER_EXPR_OP_TYPE_ADD: puts("+"); break;
            case PARSER_EXPR_OP_TYPE_SUB: puts("-"); break;
            case PARSER_EXPR_OP_TYPE_EQ: puts("="); break;
            case PARSER_EXPR_OP_TYPE_NE: puts("<>"); break;
            case PARSER_EXPR_OP_TYPE_GT: puts(">"); break;
            case PARSER_EXPR_OP_TYPE_GE: puts(">="); break;
            case PARSER_EXPR_OP_TYPE_LT: puts("<"); break;
            case PARSER_EXPR_OP_TYPE_LE: puts("<="); break;
            case PARSER_EXPR_OP_TYPE_IS: puts("IS"); break;
            case PARSER_EXPR_OP_TYPE_IS_NOT: puts("IS NOT"); break;
            case PARSER_EXPR_OP_TYPE_BETWEEN: puts("BrETWEEN"); break;
            case PARSER_EXPR_OP_TYPE_NOT: puts("NOT"); break;
            case PARSER_EXPR_OP_TYPE_AND: puts("AND"); break;
            case PARSER_EXPR_OP_TYPE_OR: puts("OR"); break;
        }
        test_parser_print_expr_tree(e->left, lvl+3);
        test_parser_print_expr_tree(e->right, lvl+3);
    }
    else if(e->node_type == PARSER_EXPR_NODE_TYPE_NUM)
    {
        printf("num: sign=%d, n=%d, e=%d, m0=%d\n", e->num.sign, e->num.n, e->num.e, e->num.m[0]);
    }
    else if(e->node_type == PARSER_EXPR_NODE_TYPE_STR)
    {
        uint64 len=0;
        string_literal_byte_length(e->str, &len);
        printf("str: len=%lu\n", len);
    }
    else if(e->node_type == PARSER_EXPR_NODE_TYPE_NAME)
    {
        printf("name: len=%d c0=%c", e->name.first_part_len, e->name.first_part[0]);
        if(e->name.second_part_len > 0)
            printf(", len=%d c0=%c", e->name.second_part_len, e->name.second_part[0]);
        printf("\n");
    }
    else if(e->node_type == PARSER_EXPR_NODE_TYPE_NULL)
    {
        puts("NULL");
    }
    else
    {
        puts("(node type unknown)");
    }
}



///////////////////// recursive comparison of statements

sint8 test_parser_compare_select_stmt(parser_ast_select *stmt1, parser_ast_select *stmt2);

// check if pointears are null or not null
inline sint8 test_parser_ptrs(void *p1, void *p2)
{
    return ((p1 == NULL) ? 1 : 0) ^ ((p2 == NULL) ? 1 : 0);
}


sint8 test_parser_compare_mem(uint64 len1, void *ptr1, uint64 len2, void *ptr2)
{
    if(len1 != len2)
    {
        puts("Statement compare: length mismatch");
        return 1;
    }

    if(memcmp(ptr1, ptr2, len1) != 0)
    {
        puts("Statement compare: buffer contents mismatch");
        return 1;
    }
}

sint8 test_parser_compare_name(parser_ast_name *stmt1, parser_ast_name *stmt2)
{
    if(test_parser_compare_mem(stmt1->first_part_len, stmt1->first_part, stmt2->first_part_len, stmt2->first_part) != 0)
    {
        puts("Statement compare: name first_part mismatch");
        return 1;
    }

    if(test_parser_compare_mem(stmt1->second_part_len, stmt1->second_part, stmt2->second_part_len, stmt2->second_part) != 0)
    {
        puts("Statement compare: name second_part mismatch");
        return 1;
    }

    return 0;
}

sint8 test_parser_compare_expr(parser_ast_expr *stmt1, parser_ast_expr *stmt2)
{
    sint8 ret;

    if(stmt1->node_type != stmt2->node_type)
    {
        puts("Statement compare: expression node_type mismatch");
        return 1;
    }

    switch(stmt1->node_type)
    {
        case PARSER_EXPR_NODE_TYPE_OP:
            if(stmt1->op != stmt2->op)
            {
                puts("Statement compare: expression op mismatch");
                return 1;
            }

            if(test_parser_ptrs(stmt1->left, stmt2->left))
            {
                puts("Statement compare: expression left pointers mismatch");
                return 1;
            }

            if(stmt1->left)
            {
                if(test_parser_compare_expr(stmt1->left, stmt2->left) != 0)
                {
                    puts("Statement compare: expression left mismatch");
                    return 1;
                }
            }

            if(test_parser_ptrs(stmt1->right, stmt2->right))
            {
                puts("Statement compare: expression left pointers mismatch");
                return 1;
            }

            if(stmt1->right)
            {
                if(test_parser_compare_expr(stmt1->right, stmt2->right) != 0)
                {
                    puts("Statement compare: expression left mismatch");
                    return 1;
                }
            }
            break;
        case PARSER_EXPR_NODE_TYPE_NUM:
            if(memcmp(&stmt1->num, &stmt2->num, sizeof(stmt1->num)))
            {
                puts("Statement compare: expression num mismatch");
                return 1;
            }
            break;
        case PARSER_EXPR_NODE_TYPE_STR:
            string_literal_byte_compare(stmt1->str, stmt2->str, &ret);
            if(ret)
            {
                puts("Statement compare: expression str mismatch");
                return 1;
            }
            break;
        case PARSER_EXPR_NODE_TYPE_NAME:
            if(test_parser_compare_name(&stmt1->name, &stmt2->name))
            {
                puts("Statement compare: expression name mismatch");
                return 1;
            }
            break;
        case PARSER_EXPR_NODE_TYPE_NULL:
            break;
        default:
            puts("Statement compare: expression node_type matches but unknown");
            return 1;
    }

    return 0;
}

sint8 test_parser_compare_named_expr(parser_ast_named_expr *stmt1, parser_ast_named_expr *stmt2)
{
    if(test_parser_compare_mem(stmt1->alias_len, stmt1->alias, stmt2->alias_len, stmt2->alias) != 0)
    {
        puts("Statement compare: named expression alias mismatch");
        return 1;
    }

    if(test_parser_compare_expr(&stmt1->expr, &stmt2->expr) != 0)
    {
        puts("Statement compare: named expression expr mismatch");
        return 1;
    }

    return 0;
}

sint8 test_parser_compare_from(parser_ast_from *stmt1, parser_ast_from *stmt2)
{
    if(test_parser_compare_mem(stmt1->alias_len, stmt1->alias, stmt2->alias_len, stmt2->alias) != 0)
    {
        puts("Statement compare: from section alias mismatch");
        return 1;
    }

    if(stmt1->join_type != stmt2->join_type)
    {
        puts("Statement compare: from section join_type mismatch");
        return 1;
    }

    if(stmt1->type != stmt2->type)
    {
        puts("Statement compare: from section type mismatch");
        return 1;
    }

    if(stmt1->type == PARSER_FROM_TYPE_NAME)
    {
        if(test_parser_compare_name(&stmt1->name, &stmt2->name) != 0)
        {
            puts("Statement compare: from name mismatch");
            return 1;
        }
    }
    else if(stmt1->type == PARSER_FROM_TYPE_SUBQUERY)
    {
        if(test_parser_compare_select_stmt(&stmt1->subquery, &stmt2->subquery) != 0)
        {
            puts("Statement compare: from subquery mismatch");
            return 1;
        }
    }
    else
    {
        puts("Statement compare: from type matches but unknown");
        return 1;
    }

    if(test_parser_ptrs(stmt1->next, stmt2->next))
    {
        puts("Statement compare: from next pointers mismatch");
        return 1;
    }

    if(stmt1->next)
    {
        if(test_parser_compare_from(stmt1->next, stmt2->next) != 0)
        {
            puts("Statement compare: from on_expr mismatch");
            return 1;
        }

        if(test_parser_ptrs(stmt1->on_expr, stmt2->on_expr))
        {
            puts("Statement compare: from on_expr pointers mismatch");
            return 1;
        }

        if(stmt1->on_expr)
        {
            if(test_parser_compare_expr(stmt1->on_expr, stmt2->on_expr) != 0)
            {
                puts("Statement compare: from on_expr mismatch");
                return 1;
            }
        }
    }

    return 0;
}

sint8 test_parser_compare_expr_list(parser_ast_expr_list *stmt1, parser_ast_expr_list *stmt2)
{
    if(stmt1->named != stmt2->named)
    {
        puts("Statement compare: expression list named mismatch");
        return 1;
    }

    if(stmt1->named)
    {
        if(test_parser_compare_named_expr((parser_ast_named_expr *)&stmt1->expr, (parser_ast_named_expr *)&stmt2->expr) != 0)
        {
            puts("Statement compare: expression list named expr mismatch");
            return 1;
        }
    }
    else
    {
        if(test_parser_compare_expr(&stmt1->expr, &stmt2->expr) != 0)
        {
            puts("Statement compare: expression list expr mismatch");
            return 1;
        }
    }

    if(test_parser_ptrs(stmt1->next, stmt2->next))
    {
        puts("Statement compare: expression list next pointers mismatch");
        return 1;
    }

    if(stmt1->next)
    {
        if(test_parser_compare_expr_list(stmt1->next, stmt2->next) != 0)
        {
            puts("Statement compare: expression list next mismatch");
            return 1;
        }
    }

    return 0;
}


sint8 test_parser_compare_single_select(parser_ast_single_select *stmt1, parser_ast_single_select *stmt2)
{
    if(stmt1->uniq_type != stmt2->uniq_type)
    {
        puts("Statement compare: single select uniq_type mismatch");
        return 1;
    }

    if(test_parser_ptrs(stmt1->from, stmt2->from))
    {
        puts("Statement compare: single select from pointers mismatch");
        return 1;
    }

    if(stmt1->from)
    {
        if(test_parser_compare_from(stmt1->from, stmt2->from) != 0)
        {
            puts("Statement compare: single select from mismatch");
            return 1;
        }
    }

    if(test_parser_ptrs(stmt1->group_by, stmt2->group_by))
    {
        puts("Statement compare: single select group_by pointers mismatch");
        return 1;
    }

    if(stmt1->group_by)
    {
        if(test_parser_compare_expr_list(stmt1->group_by, stmt2->group_by) != 0)
        {
            puts("Statement compare: single select group_by mismatch");
            return 1;
        }
    }

    if(test_parser_ptrs(stmt1->having, stmt2->having))
    {
        puts("Statement compare: single select having pointers mismatch");
        return 1;
    }

    if(stmt1->having)
    {
        if(test_parser_compare_expr_list(stmt1->having, stmt2->having) != 0)
        {
            puts("Statement compare: single select having mismatch");
            return 1;
        }
    }

    if(test_parser_ptrs(stmt1->projection, stmt2->projection))
    {
        puts("Statement compare: single select projection pointers mismatch");
        return 1;
    }

    if(stmt1->projection)
    {
        if(test_parser_compare_expr_list(stmt1->projection, stmt2->projection) != 0)
        {
            puts("Statement compare: single select projection mismatch");
            return 1;
        }
    }

    if(test_parser_ptrs(stmt1->where, stmt2->where))
    {
        puts("Statement compare: single select where pointers mismatch");
        return 1;
    }

    if(stmt1->where)
    {
        if(test_parser_compare_expr(stmt1->where, stmt2->where) != 0)
        {
            puts("Statement compare: single select where mismatch");
            return 1;
        }
    }

    return 0;
}

sint8 test_parser_compare_orderby(parser_ast_order_by *stmt1, parser_ast_order_by *stmt2)
{
    if(stmt1->null_order != stmt2->null_order)
    {
        puts("Statement compare: order by section null_order mismatch");
        return 1;
    }

    if(stmt1->sort_type != stmt2->sort_type)
    {
        puts("Statement compare: order by section sort_type mismatch");
        return 1;
    }

    if(test_parser_compare_expr(&stmt1->expr, &stmt2->expr) != 0)
    {
        puts("Statement compare: order by section expr mismatch");
        return 1;
    }

    if(test_parser_ptrs(stmt1->next, stmt2->next))
    {
        puts("Statement compare: order by section next pointers mismatch");
        return 1;
    }

    if(stmt1->next)
    {
        if(test_parser_compare_orderby(stmt1->next, stmt2->next) != 0)
        {
            puts("Statement compare: order by section next mismatch");
            return 1;
        }
    }

    return 0;
}

sint8 test_parser_compare_select_stmt(parser_ast_select *stmt1, parser_ast_select *stmt2)
{
    if(stmt1->setop != stmt2->setop)
    {
        puts("Statement compare: select statement setop mismatch");
        return 1;
    }

    if(test_parser_ptrs(stmt1->select, stmt2->select))
    {
        puts("Statement compare: select statement single select pointers mismatch");
        return 1;
    }

    if(stmt1->select)
    {
        if(test_parser_compare_single_select(stmt1->select, stmt2->select) != 0)
        {
            puts("Statement compare: select statement single select mismatch");
            return 1;
        }
    }

    if(test_parser_ptrs(stmt1->next, stmt2->next))
    {
        puts("Statement compare: select statement next pointers mismatch");
        return 1;
    }

    if(stmt1->next)
    {
        if(test_parser_compare_select_stmt(stmt1->next, stmt2->next) != 0)
        {
            puts("Statement compare: select statement next mismatch");
            return 1;
        }
    }

    if(test_parser_ptrs(stmt1->order_by, stmt2->order_by))
    {
        puts("Statement compare: select statement order_by pointers mismatch");
        return 1;
    }

    if(stmt1->order_by)
    {
        if(test_parser_compare_orderby(stmt1->order_by, stmt2->order_by) != 0)
        {
            puts("Statement compare: select statement order_by mismatch");
            return 1;
        }
    }

    return 0;
}

sint8 test_parser_compare_colname_list(parser_ast_colname_list *stmt1, parser_ast_colname_list *stmt2)
{
    if(test_parser_compare_mem(stmt1->colname_len, stmt1->col_name, stmt2->colname_len, stmt2->col_name) != 0)
    {
        puts("Statement compare: colname list colname mismatch");
        return 1;
    }

    if(test_parser_ptrs(stmt1->next, stmt2->next))
    {
        puts("Statement compare: colname list next pointer mismatch");
        return 1;
    }

    if(stmt1->next)
    {
        if(test_parser_compare_colname_list(stmt1->next, stmt2->next) != 0)
        {
            puts("Statement compare: colname list next mismatch");
            return 1;
        }
    }
    return 0;
}

sint8 test_parser_compare_insert_stmt(parser_ast_insert *stmt1, parser_ast_insert *stmt2)
{
    if(stmt1->type != stmt2->type)
    {
        puts("Statement compare: insert statement type mismatch");
        return 1;
    }

    if(test_parser_compare_name(&stmt1->target, &stmt2->target) != 0)
    {
        puts("Statement compare: insert statement target mismatch");
        return 1;
    }

    if(stmt1->type == PARSER_STMT_TYPE_INSERT_SELECT)
    {
        if(test_parser_compare_select_stmt(&stmt1->select_stmt, &stmt2->select_stmt) != 0)
        {
            puts("Statement compare: insert statement select_stmt mismatch");
            return 1;
        }
    }
    else if(stmt1->type == PARSER_STMT_TYPE_INSERT_VALUES)
    {
        if(test_parser_compare_expr_list(&stmt1->values, &stmt2->values) != 0)
        {
            puts("Statement compare: insert statement values mismatch");
            return 1;
        }
    }
    else
    {
        puts("Statement compare: insert statement type matches but unknown");
        return 1;
    }

    if(test_parser_ptrs(stmt1->columns, stmt2->columns))
    {
        puts("Statement compare: insert statement columns pointer mismatch");
        return 1;
    }

    if(stmt1->columns)
    {
        if(test_parser_compare_colname_list(stmt1->columns, stmt2->columns) != 0)
        {
            puts("Statement compare: insert statement columns mismatch");
            return 1;
        }
    }

    return 0;
}

sint8 test_parser_compare_set_list(parser_ast_set_list *stmt1, parser_ast_set_list *stmt2)
{

    if(test_parser_compare_name(&stmt1->column, &stmt2->column) != 0)
    {
        puts("Statement compare: set list column name mismatch");
        return 1;
    }

    if(test_parser_compare_expr(&stmt1->expr, &stmt2->expr))
    {
        puts("Statement compare: set list expression mismatch");
        return 1;
    }

    if(test_parser_ptrs(stmt1->next, stmt2->next))
    {
        puts("Statement compare: set list next pointer mismatch");
        return 1;
    }

    if(stmt1->next)
    {
        if(test_parser_compare_set_list(stmt1->next, stmt2->next) != 0)
        {
            puts("Statement compare: set list next mismatch");
            return 1;
        }
    }

    return 0;
}

sint8 test_parser_compare_update_stmt(parser_ast_update *stmt1, parser_ast_update *stmt2)
{
    if(test_parser_compare_mem(stmt1->alias_len, stmt1->alias, stmt2->alias_len, stmt2->alias) != 0)
    {
        puts("Statement compare: update stmt alias mismatch");
        return 1;
    }

    if(test_parser_compare_set_list(&stmt1->set_list, &stmt2->set_list) != 0)
    {
        puts("Statement compare: update stmt alias mismatch");
        return 1;
    }

    if(test_parser_compare_name(&stmt1->target, &stmt2->target) != 0)
    {
        puts("Statement compare: update statement target mismatch");
        return 1;
    }

    if(test_parser_ptrs(stmt1->where, stmt2->where))
    {
        puts("Statement compare: update stmt where pointer mismatch");
        return 1;
    }

    if(stmt1->where)
    {
        if(test_parser_compare_expr(stmt1->where, stmt2->where) != 0)
        {
            puts("Statement compare: update stmt where mismatch");
            return 1;
        }
    }

    return 0;
}

sint8 test_parser_compare_delete_stmt(parser_ast_delete *stmt1, parser_ast_delete *stmt2)
{
    if(test_parser_compare_mem(stmt1->alias_len, stmt1->alias, stmt2->alias_len, stmt2->alias) != 0)
    {
        puts("Statement compare: delete stmt alias mismatch");
        return 1;
    }

    if(test_parser_compare_name(&stmt1->target, &stmt2->target) != 0)
    {
        puts("Statement compare: delete statement target mismatch");
        return 1;
    }

    if(test_parser_ptrs(stmt1->where, stmt2->where))
    {
        puts("Statement compare: delete stmt where pointer mismatch");
        return 1;
    }

    if(stmt1->where)
    {
        if(test_parser_compare_expr(stmt1->where, stmt2->where) != 0)
        {
            puts("Statement compare: delete stmt where mismatch");
            return 1;
        }
    }

    return 0;
}

sint8 test_parser_compare_datatype(parser_ast_col_datatype *stmt1, parser_ast_col_datatype *stmt2)
{
    if(stmt1->datatype != stmt2->datatype)
    {
        puts("Statement compare: datatype.datatype mismatch");
        return 1;
    }

    switch(stmt1->datatype)
    {
        case CHARACTER_VARYING:
            if(stmt1->char_len != stmt2->char_len)
            {
                puts("Statement compare: datatype.charlen mismatch");
                return 1;
            }
            break;
        case DECIMAL:
            if(stmt1->decimal.precision != stmt2->decimal.precision)
            {
                puts("Statement compare: datatype.dacemal.precision mismatch");
                return 1;
            }

            if(stmt1->decimal.scale != stmt2->decimal.scale)
            {
                puts("Statement compare: datatype.dacemal.scale mismatch");
                return 1;
            }
            break;
        case TIMESTAMP:
        case TIMESTAMP_WITH_TZ:
            if(stmt1->ts_precision != stmt2->ts_precision)
            {
                puts("Statement compare: datatype.ts_precision mismatch");
                return 1;
            }
            break;
        default:
            break;
    }

    return 0;
}

sint8 test_parser_compare_coldesc(parser_ast_col_desc *stmt1, parser_ast_col_desc *stmt2)
{
    if(stmt1->nullable != stmt2->nullable)
    {
        puts("Statement compare: coldesc nullable mismatch");
        return 1;
    }

    if(test_parser_compare_mem(stmt1->name_len, stmt1->name, stmt2->name_len, stmt2->name) != 0)
    {
        puts("Statement compare: coldesc name mismatch");
        return 1;
    }

    if(test_parser_compare_datatype(&stmt1->datatype, &stmt2->datatype) != 0)
    {
        puts("Statement compare: coldesc datatype mismatch");
        return 1;
    }

    if(test_parser_ptrs(stmt1->default_value, stmt2->default_value))
    {
        puts("Statement compare: coldesc default_value pointer mismatch");
        return 1;
    }

    if(stmt1->default_value)
    {
        if(test_parser_compare_expr(stmt1->default_value, stmt2->default_value) != 0)
        {
            puts("Statement compare: coldesc default_value mismatch");
            return 1;
        }
    }

    return 0;
}

sint8 test_parser_compare_coldesc_list(parser_ast_col_desc_list *stmt1, parser_ast_col_desc_list *stmt2)
{
    if(test_parser_compare_coldesc(&stmt1->col_desc, &stmt2->col_desc) != 0)
    {
        puts("Statement compare: coldesc list col desc mismatch");
        return 1;
    }

    if(test_parser_ptrs(stmt1->next, stmt2->next))
    {
        puts("Statement compare: coldesc list next pointer mismatch");
        return 1;
    }

    if(stmt1->next)
    {
        if(test_parser_compare_coldesc_list(stmt1->next, stmt2->next) != 0)
        {
            puts("Statement compare: coldesc list next mismatch");
            return 1;
        }
    }

    return 0;
}

sint8 test_parser_compare_constr(parser_ast_constr *stmt1, parser_ast_constr *stmt2)
{
    if(stmt1->type != stmt2->type)
    {
        puts("Statement compare: constr type mismatch");
        return 1;
    }

    if(test_parser_compare_mem(stmt1->name_len, stmt1->name, stmt2->name_len, stmt2->name) != 0)
    {
        puts("Statement compare: constr name mismatch");
        return 1;
    }

    switch(stmt1->type)
    {
        case PARSER_CONSTRAINT_TYPE_CHECK:
            if(test_parser_compare_expr(&stmt1->expr, &stmt2->expr) != 0)
            {
                puts("Statement compare: constr expr mismatch");
                return 1;
            }
            break;
        case PARSER_CONSTRAINT_TYPE_DEFAULT:
            if(test_parser_compare_expr(&stmt1->expr, &stmt2->expr) != 0)
            {
                puts("Statement compare: constr expr mismatch");
                return 1;
            }
            break;
        case PARSER_CONSTRAINT_TYPE_UNIQUE:
        case PARSER_CONSTRAINT_TYPE_PK:
            if(test_parser_compare_colname_list(&stmt1->columns, &stmt2->columns) != 0)
            {
                puts("Statement compare: constr columns mismatch");
                return 1;
            }
            break;
        case PARSER_CONSTRAINT_TYPE_FK:
            if(test_parser_compare_name(&stmt1->fk.ref_table, &stmt2->fk.ref_table) != 0)
            {
                puts("Statement compare: constr fk.ref_table mismatch");
                return 1;
            }

            if(test_parser_compare_colname_list(&stmt1->fk.columns, &stmt2->fk.columns) != 0)
            {
                puts("Statement compare: constr fk.columns mismatch");
                return 1;
            }

            if(test_parser_compare_colname_list(&stmt1->fk.ref_columns, &stmt2->fk.ref_columns) != 0)
            {
                puts("Statement compare: constr fk.ref_columns mismatch");
                return 1;
            }

            if(stmt1->fk.fk_on_delete != stmt2->fk.fk_on_delete)
            {
                puts("Statement compare: constr fk.on_delete mismatch");
                return 1;
            }
            break;
        default:
            puts("Statement compare: constr type is unknown");
            return 1;
    }
    return 0;
}

sint8 test_parser_compare_constr_list(parser_ast_constr_list *stmt1, parser_ast_constr_list *stmt2)
{
    if(test_parser_compare_constr(&stmt1->constr, &stmt2->constr) != 0)
    {
        puts("Statement compare: constr list constr mismatch");
        return 1;
    }

    if(test_parser_ptrs(stmt1->next, stmt2->next))
    {
        puts("Statement compare: constr list next pointer mismatch");
        return 1;
    }

    if(stmt1->next)
    {
        if(test_parser_compare_constr_list(stmt1->next, stmt2->next) != 0)
        {
            puts("Statement compare: constr list next mismatch");
            return 1;
        }
    }
    return 0;
}

sint8 test_parser_compare_create_table_stmt(parser_ast_create_table *stmt1, parser_ast_create_table *stmt2)
{
    if(test_parser_compare_name(&stmt1->name, &stmt2->name) != 0)
    {
        puts("Statement compare: create table name mismatch");
        return 1;
    }

    if(test_parser_compare_coldesc_list(&stmt1->cols, &stmt2->cols) != 0)
    {
        puts("Statement compare: create table cols mismatch");
        return 1;
    }

    if(test_parser_ptrs(stmt1->constr, stmt2->constr))
    {
        puts("Statement compare: create table constr pointer mismatch");
        return 1;
    }

    if(stmt1->constr)
    {
        if(test_parser_compare_constr_list(stmt1->constr, stmt2->constr) != 0)
        {
            puts("Statement compare: create table constr mismatch");
            return 1;
        }
    }

    return 0;
}

sint8 test_parser_compare_drop_table_stmt(parser_ast_drop_table *stmt1, parser_ast_drop_table *stmt2)
{
    if(test_parser_compare_name(&stmt1->table, &stmt2->table) != 0)
    {
        puts("Statement compare: drop table table mismatch");
        return 1;
    }

    return 0;
}

sint8 test_parser_alter_table_column(parser_ast_alter_table_column *stmt1, parser_ast_alter_table_column *stmt2)
{
    if(stmt1->nullable != stmt2->nullable)
    {
        puts("Statement compare: alter table column nullable mismatch");
        return 1;
    }

    if(test_parser_compare_mem(stmt1->column_len, stmt1->column, stmt2->column_len, stmt2->column) != 0)
    {
        puts("Statement compare: alter table column name mismatch");
        return 1;
    }

    if(test_parser_ptrs(stmt1->datatype, stmt2->datatype))
    {
        puts("Statement compare: alter table column datatype pointer mismatch");
        return 1;
    }

    if(stmt1->datatype != NULL)
    {
        if(test_parser_compare_datatype(stmt1->datatype, stmt2->datatype) != 0)
        {
            puts("Statement compare: alter table column datatype mismatch");
            return 1;
        }
    }

    if(test_parser_ptrs(stmt1->default_expr, stmt2->default_expr))
    {
        puts("Statement compare: alter table column default_expr pointer mismatch");
        return 1;
    }

    if(stmt1->default_expr)
    {
        if(test_parser_compare_expr(stmt1->default_expr, stmt2->default_expr) != 0)
        {
            puts("Statement compare: alter table column default_expr mismatch");
            return 1;
        }
    }

    return 0;
}

sint8 test_parser_alter_table_add_constr(parser_ast_constr *stmt1, parser_ast_constr *stmt2)
{
    if(test_parser_compare_constr(stmt1, stmt2) != 0)
    {
        puts("Statement compare: alter table add constraint mismatch");
        return 1;
    }

    return 0;
}

sint8 test_parser_alter_table_drop_constr(parser_ast_drop_constraint *stmt1, parser_ast_drop_constraint *stmt2)
{
    if(test_parser_compare_mem(stmt1->name_len, stmt1->name, stmt2->name_len, stmt2->name) != 0)
    {
        puts("Statement compare: alter table drop constaint name mismatch");
        return 1;
    }

    return 0;
}

sint8 test_parser_alter_table_rename(parser_ast_rename_table *stmt1, parser_ast_rename_table *stmt2)
{
    if(test_parser_compare_mem(stmt1->new_name_len, stmt1->new_name, stmt2->new_name_len, stmt2->new_name) != 0)
    {
        puts("Statement compare: alter table rename new_name mismatch");
        return 1;
    }

    return 0;
}

sint8 test_parser_alter_table_rename_column(parser_ast_rename_column *stmt1, parser_ast_rename_column *stmt2)
{
    if(test_parser_compare_mem(stmt1->name_len, stmt1->name, stmt2->name_len, stmt2->name) != 0)
    {
        puts("Statement compare: alter table rename column name mismatch");
        return 1;
    }

    if(test_parser_compare_mem(stmt1->new_name_len, stmt1->new_name, stmt2->new_name_len, stmt2->new_name) != 0)
    {
        puts("Statement compare: alter table rename column new_name mismatch");
        return 1;
    }

    if(test_parser_compare_name(&stmt1->table, &stmt2->table) != 0)
    {
        puts("Statement compare: alter table rename column table mismatch");
        return 1;
    }

    return 0;
}

sint8 test_parser_alter_table_rename_constr(parser_ast_rename_constr *stmt1, parser_ast_rename_constr *stmt2)
{
    if(test_parser_compare_mem(stmt1->name_len, stmt1->name, stmt2->name_len, stmt2->name) != 0)
    {
        puts("Statement compare: alter table rename constraint name mismatch");
        return 1;
    }

    if(test_parser_compare_mem(stmt1->new_name_len, stmt1->new_name, stmt2->new_name_len, stmt2->new_name) != 0)
    {
        puts("Statement compare: alter table rename constraint new_name mismatch");
        return 1;
    }

    if(test_parser_compare_name(&stmt1->table, &stmt2->table) != 0)
    {
        puts("Statement compare: alter table rename constraint table mismatch");
        return 1;
    }

    return 0;
}


sint8 test_parser_compare_alter_table_stmt(parser_ast_alter_table *stmt1, parser_ast_alter_table *stmt2)
{
    if(stmt1->type != stmt2->type)
    {
        puts("Statement compare: alter table type mismatch");
        return 1;
    }

    if(test_parser_compare_name(&stmt1->table, &stmt2->table) != 0)
    {
        puts("Statement compare: alter table table mismatch");
        return 1;
    }

    switch(stmt1->type)
    {
        case PARSER_STMT_TYPE_ALTER_TABLE_ADD_COL:
        case PARSER_STMT_TYPE_ALTER_TABLE_MODIFY_COL:
        case PARSER_STMT_TYPE_ALTER_TABLE_DROP_COL:
            if(test_parser_alter_table_column(&stmt1->column, &stmt2->column) != 0) return 1;
            break;
        case PARSER_STMT_TYPE_ALTER_TABLE_ADD_CONSTR:
            if(test_parser_alter_table_add_constr(&stmt1->add_constr, &stmt2->add_constr) != 0) return 1;
            break;
        case PARSER_STMT_TYPE_ALTER_TABLE_DROP_CONSTR:
            if(test_parser_alter_table_drop_constr(&stmt1->drop_constr, &stmt2->drop_constr) != 0) return 1;
            break;
        case PARSER_STMT_TYPE_ALTER_TABLE_RENAME:
            if(test_parser_alter_table_rename(&stmt1->rename_table, &stmt2->rename_table) != 0) return 1;
            break;
        case PARSER_STMT_TYPE_ALTER_TABLE_RENAME_COLUMN:
            if(test_parser_alter_table_rename_column(&stmt1->rename_column, &stmt2->rename_column) != 0) return 1;
            break;
        case PARSER_STMT_TYPE_ALTER_TABLE_RENAME_CONSTR:
            if(test_parser_alter_table_rename_constr(&stmt1->rename_constr, &stmt2->rename_constr) != 0) return 1;
            break;
        default:
            puts("Statement compare: alter table unknown statement type");
            return 1;
    }


    return 0;
}

sint8 test_parser_compare_create_database_stmt(parser_ast_create_database *stmt1, parser_ast_create_database *stmt2)
{
    if(test_parser_compare_mem(stmt1->name_len, stmt1->name, stmt2->name_len, stmt2->name) != 0)
    {
        puts("Statement compare: create database name mismatch");
        return 1;
    }

    return 0;
}

sint8 test_parser_compare_drop_database_stmt(parser_ast_drop_database *stmt1, parser_ast_drop_database *stmt2)
{
    if(test_parser_compare_mem(stmt1->name_len, stmt1->name, stmt2->name_len, stmt2->name) != 0)
    {
        puts("Statement compare: drop database name mismatch");
        return 1;
    }

    return 0;
}


// return 0 if statements are identical
sint8 test_parser_compare_stmt(parser_ast_stmt *stmt1, parser_ast_stmt *stmt2)
{
    if(stmt1->type != stmt2->type)
    {
        puts("Statement compare: different statement type");
        return 1;
    }

    switch(stmt1->type)
    {
        case PARSER_STMT_TYPE_SELECT:
            return test_parser_compare_select_stmt(&stmt1->select_stmt, &stmt2->select_stmt);
            break;
        case PARSER_STMT_TYPE_INSERT:
            return test_parser_compare_insert_stmt(&stmt1->insert_stmt, &stmt2->insert_stmt);
            break;
        case PARSER_STMT_TYPE_UPDATE:
            return test_parser_compare_update_stmt(&stmt1->update_stmt, &stmt2->update_stmt);
            break;
        case PARSER_STMT_TYPE_DELETE:
            return test_parser_compare_delete_stmt(&stmt1->delete_stmt, &stmt2->delete_stmt);
            break;
        case PARSER_STMT_TYPE_CREATE_TABLE:
            return test_parser_compare_create_table_stmt(&stmt1->create_table_stmt, &stmt2->create_table_stmt);
            break;
        case PARSER_STMT_TYPE_DROP_TABLE:
            return test_parser_compare_drop_table_stmt(&stmt1->drop_table_stmt, &stmt2->drop_table_stmt);
            break;
        case PARSER_STMT_TYPE_ALTER_TABLE:
            return test_parser_compare_alter_table_stmt(&stmt1->alter_table_stmt, &stmt2->alter_table_stmt);
            break;
        case PARSER_STMT_TYPE_CREATE_DATABASE:
            return test_parser_compare_create_database_stmt(&stmt1->create_database_stmt, &stmt2->create_database_stmt);
            break;
        case PARSER_STMT_TYPE_DROP_DATABASE:
            return test_parser_compare_drop_database_stmt(&stmt1->drop_database_stmt, &stmt2->drop_database_stmt);
            break;
        default:
            puts("Statement compare: unknown statement type");
            return 1;
    }

    return 0;
}

int test_parser_functions()
{
    puts("Starting test test_parser_functions");

    parser_ast_stmt *stmt;
    parser_interface pi;
    pi.report_error = test_parser_error_reporter;

    lexer_interface li;
    li.next_char = test_parser_char_feeder;
    li.report_error = test_parser_error_reporter;

    encoding_init();
    g_test_parser_state.build_char = encoding_get_build_char_fun(ENCODING_UTF8);

    handle strlit = string_literal_create(malloc(string_literal_alloc_sz()));
    if(NULL == strlit) return __LINE__;

    handle lexer = lexer_create(malloc(lexer_get_allocation_size()), ENCODING_UTF8, strlit, li);
    if(NULL == lexer) return __LINE__;


    puts("Testing select statement parsing");

    decimal d, d2;
    memset(&d, 0, sizeof(d));
    d.sign = DECIMAL_SIGN_POS;
    d.m[0] = 1;
    d.n = 1;
    memset(&d2, 0, sizeof(d2));
    d2.sign = DECIMAL_SIGN_POS;
    d2.m[0] = 2;
    d2.n = 1;

    parser_ast_stmt ref_stmt;

    g_test_parser_state.cur_char = 0;
    g_test_parser_state.stmt = _ach("select all \n\
              t.c as c1, \n\
              1 as c2, \n\
              -1 + a * 3 > 0 and 2*(-4 + b) or not t.c is null and d is not null as c3 \n\
            from db_name._tbl_1 as t full outer join (select *) as s on 1 = 1 cross join tbl2\n\
            where 1=1 \n\
            group by col1, col2\n\
            having 'asd', 'bgd 123' \n\
            except  \n\
            select distinct * \n\
            union all \n\
            select * \n\
            intersect \n\
            select distinct * \n\
            order by 1 asc nulls first, 1 desc nulls last, 2 desc, 2");

    handle strlit1 = string_literal_create(malloc(string_literal_alloc_sz()));
    if(NULL == strlit1) return __LINE__;
    string_literal_append_char(strlit1, (const uint8 *)_ach("asd"), strlen(_ach("asd")));
    handle strlit2 = string_literal_create(malloc(string_literal_alloc_sz()));
    if(NULL == strlit2) return __LINE__;
    string_literal_append_char(strlit2, (const uint8 *)_ach("bgd 123"), strlen(_ach("bgd 123")));

    parser_ast_single_select    ssel1, ssel2, ssel3, ssel4, subq_sel;
    parser_ast_select           sel1, sel2, sel3, sel4;

    parser_ast_expr_list        proj1, proj2, proj3;
    parser_ast_from             from, from2, from3;
    parser_ast_expr_list        grby, grby2;
    parser_ast_expr_list        having, having2;
    parser_ast_order_by         ordby, ordby2, ordby3, ordby4;

    parser_ast_expr             complex_expr, expr1, expr2, ce[100];
    parser_ast_expr             join_expr1, join_expr2, join_expr3;


    // some complex expression: -1 + a * 3 > 0 and 2*(-4 + b) or not t.c is null and d is not null
    // ce[2] = 0-ce[3]
    ce[0].node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ce[0].num, 0, sizeof(ce[0].num));
    ce[0].num.sign = DECIMAL_SIGN_POS;

    ce[1].node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ce[1].num, 0, sizeof(ce[1].num));
    ce[1].num.sign = DECIMAL_SIGN_POS;
    ce[1].num.m[0] = 1;
    ce[1].num.n = 1;

    ce[2].node_type = PARSER_EXPR_NODE_TYPE_OP;
    ce[2].op = PARSER_EXPR_OP_TYPE_SUB;
    ce[2].left = &ce[0];
    ce[2].right = &ce[3];

    // ce[3] = 1 + c[6]
    ce[3].node_type = PARSER_EXPR_NODE_TYPE_OP;
    ce[3].op = PARSER_EXPR_OP_TYPE_ADD;
    ce[3].left = &ce[1];
    ce[3].right = &ce[6];

    // ce[6] = a * 3
    ce[4].node_type = PARSER_EXPR_NODE_TYPE_NAME;
    ce[4].name.first_part[0] = _ach('a');
    ce[4].name.first_part_len = 1;
    ce[4].name.second_part_len = 0;

    ce[5].node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ce[5].num, 0, sizeof(ce[5].num));
    ce[5].num.sign = DECIMAL_SIGN_POS;
    ce[5].num.m[0] = 3;
    ce[5].num.n = 1;

    ce[6].node_type = PARSER_EXPR_NODE_TYPE_OP;
    ce[6].op = PARSER_EXPR_OP_TYPE_MUL;
    ce[6].left = &ce[4];
    ce[6].right = &ce[5];

    // ce[7] = ce[2] > 0
    ce[7].node_type = PARSER_EXPR_NODE_TYPE_OP;
    ce[7].op = PARSER_EXPR_OP_TYPE_GT;
    ce[7].left = &ce[2];
    ce[7].right = &ce[8];

    ce[8].node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ce[8].num, 0, sizeof(ce[8].num));
    ce[8].num.sign = DECIMAL_SIGN_POS;

    // ce[12] = 0-ce[14]
    ce[9].node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ce[9].num, 0, sizeof(ce[9].num));
    ce[9].num.sign = DECIMAL_SIGN_POS;

    ce[10].node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ce[10].num, 0, sizeof(ce[10].num));
    ce[10].num.sign = DECIMAL_SIGN_POS;
    ce[10].num.m[0] = 4;
    ce[10].num.n = 1;

    ce[12].node_type = PARSER_EXPR_NODE_TYPE_OP;
    ce[12].op = PARSER_EXPR_OP_TYPE_SUB;
    ce[12].left = &ce[9];
    ce[12].right = &ce[14];

    //  ce[14] = 4 + b
    ce[13].node_type = PARSER_EXPR_NODE_TYPE_NAME;
    ce[13].name.first_part[0] = _ach('b');
    ce[13].name.first_part_len = 1;
    ce[13].name.second_part_len = 0;

    ce[14].node_type = PARSER_EXPR_NODE_TYPE_OP;
    ce[14].op = PARSER_EXPR_OP_TYPE_ADD;
    ce[14].left = &ce[10];
    ce[14].right = &ce[13];

    // ce[16] = 2 * ce[12]
    ce[15].node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memset(&ce[15].num, 0, sizeof(ce[15].num));
    ce[15].num.sign = DECIMAL_SIGN_POS;
    ce[15].num.m[0] = 2;
    ce[15].num.n = 1;

    ce[16].node_type = PARSER_EXPR_NODE_TYPE_OP;
    ce[16].op = PARSER_EXPR_OP_TYPE_MUL;
    ce[16].left = &ce[15];
    ce[16].right = &ce[12];

    // ce[17] = ce[7] and ce[16]
    ce[17].node_type = PARSER_EXPR_NODE_TYPE_OP;
    ce[17].op = PARSER_EXPR_OP_TYPE_AND;
    ce[17].left = &ce[7];
    ce[17].right = &ce[16];

    // ce[20] = t.c is null
    ce[18].node_type = PARSER_EXPR_NODE_TYPE_NAME;
    ce[18].name.first_part[0] = _ach('t');
    ce[18].name.first_part_len = 1;
    ce[18].name.second_part[0] = _ach('c');
    ce[18].name.second_part_len = 1;

    ce[19].node_type = PARSER_EXPR_NODE_TYPE_NULL;

    ce[20].node_type = PARSER_EXPR_NODE_TYPE_OP;
    ce[20].op = PARSER_EXPR_OP_TYPE_IS;
    ce[20].left = &ce[18];
    ce[20].right = &ce[19];

    // ce[21] = not ce[20]
    ce[21].node_type = PARSER_EXPR_NODE_TYPE_OP;
    ce[21].op = PARSER_EXPR_OP_TYPE_NOT;
    ce[21].left = &ce[20];

    // ce[24] = t.c is null
    ce[22].node_type = PARSER_EXPR_NODE_TYPE_NAME;
    ce[22].name.first_part[0] = _ach('d');
    ce[22].name.first_part_len = 1;
    ce[22].name.second_part_len = 0;

    ce[23].node_type = PARSER_EXPR_NODE_TYPE_NULL;

    ce[24].node_type = PARSER_EXPR_NODE_TYPE_OP;
    ce[24].op = PARSER_EXPR_OP_TYPE_IS_NOT;
    ce[24].left = &ce[22];
    ce[24].right = &ce[23];

    // ce[25] = ce[21] and ce[24]
    ce[25].node_type = PARSER_EXPR_NODE_TYPE_OP;
    ce[25].op = PARSER_EXPR_OP_TYPE_AND;
    ce[25].left = &ce[21];
    ce[25].right = &ce[24];

    // complex_expr = ce[17] or ce[25]
    complex_expr.node_type = PARSER_EXPR_NODE_TYPE_OP;
    complex_expr.op = PARSER_EXPR_OP_TYPE_OR;
    complex_expr.left = &ce[17];
    complex_expr.right = &ce[25];

//    test_parser_print_expr_tree(&complex_expr, 0);

    // select projection:  t.c as c1, 1 as c2, <complex_expr> as c3
    proj1.named_expr.alias[0] = proj2.named_expr.alias[0] = proj3.named_expr.alias[0] = 'c';
    proj1.named_expr.alias[1] = '1';
    proj2.named_expr.alias[1] = '2';
    proj3.named_expr.alias[1] = '3';
    proj1.named_expr.alias_len = proj2.named_expr.alias_len = proj3.named_expr.alias_len = 2;

    proj1.named_expr.expr.node_type = PARSER_EXPR_NODE_TYPE_NAME;
    proj1.named_expr.expr.name.first_part[0] = 't';
    proj1.named_expr.expr.name.second_part[0] = 'c';
    proj1.named_expr.expr.name.first_part_len = proj1.named_expr.expr.name.second_part_len = 1;
    proj2.named_expr.expr.node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memcpy(&proj2.named_expr.expr.num, &d, sizeof(d));
    memcpy(&proj3.named_expr.expr, &complex_expr, sizeof(complex_expr));

    expr1.node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memcpy(&expr1.num, &d, sizeof(d));
    expr2.node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memcpy(&expr2.num, &d, sizeof(d));

    proj1.named = 1;
    proj1.next = &proj2;

    proj2.named = 1;
    proj2.next = &proj3;

    proj3.named = 1;
    proj3.next = NULL;

    // from expr:  db_name._tbl_1 as t full outer join (select *) as s on 1 = 1 cross join tbl2
    from.alias[0] = 't';
    from.alias_len = 1;
    from.join_type = PARSER_JOIN_TYPE_FULL;
    from.type = PARSER_FROM_TYPE_NAME;
    strcpy((char *)from.name.first_part, "db_name");
    from.name.first_part_len = strlen((char *)from.name.first_part);
    strcpy((char *)from.name.second_part, "_tbl_1");
    from.name.second_part_len = strlen((char *)from.name.second_part);
    from.next = &from2;
    from.on_expr = NULL;

    // join exoression:  1 = 1
    join_expr1.node_type = PARSER_EXPR_NODE_TYPE_OP;
    join_expr1.op = PARSER_EXPR_OP_TYPE_EQ;
    join_expr1.left = &join_expr2;
    join_expr1.right = &join_expr3;
    join_expr2.node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memcpy(&join_expr2.num, &d, sizeof(d));
    join_expr3.node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memcpy(&join_expr3.num, &d, sizeof(d));


    from2.type = PARSER_FROM_TYPE_SUBQUERY;
    from2.subquery.next = NULL;
    from2.subquery.order_by = NULL;
    from2.subquery.select = &subq_sel;
    subq_sel.from = NULL;
    subq_sel.group_by = NULL;
    subq_sel.having = NULL;
    subq_sel.projection = NULL;
    subq_sel.uniq_type = PARSER_UNIQ_TYPE_ALL;
    subq_sel.where = NULL;

    from2.join_type = PARSER_JOIN_TYPE_CROSS;
    from2.on_expr = &join_expr1;
    from2.alias_len = 1;
    from2.alias[0] = 's';
    from2.next = &from3;

    from3.type = PARSER_FROM_TYPE_NAME;
    from3.alias_len = 0;
    strcpy((char *)from3.name.first_part, "tbl2");
    from3.name.first_part_len = strlen((char *)from3.name.first_part);
    from3.name.second_part_len = 0;
    from3.next = NULL;
    from3.on_expr = NULL;


    // group by expr: col1, col2
    grby.expr.node_type = PARSER_EXPR_NODE_TYPE_NAME;
    strcpy((char *)grby.expr.name.first_part, "col1");
    grby.expr.name.first_part_len = strlen("col1");
    grby.expr.name.second_part_len = 0;
    grby.named = 0;
    grby.next = &grby2;
    grby2.expr.node_type = PARSER_EXPR_NODE_TYPE_NAME;
    strcpy((char *)grby2.expr.name.first_part, "col2");
    grby2.expr.name.first_part_len = strlen("col2");
    grby2.expr.name.second_part_len = 0;
    grby2.named = 0;
    grby2.next = NULL;

    // having expr: 'asd', 'bgd 123'
    having.named = 0;
    having.expr.node_type = PARSER_EXPR_NODE_TYPE_STR;
    having.expr.str = strlit1;
    having.next = &having2;
    having2.named = 0;
    having2.expr.node_type = PARSER_EXPR_NODE_TYPE_STR;
    having2.expr.str = strlit2;
    having2.next = NULL;

    // order by expr: 1 asc nulls first, 1 desc nulls last, 2 desc, 2
    ordby.expr.node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memcpy(&ordby.expr.num, &d, sizeof(d));
    ordby.null_order = PARSER_NULL_ORDER_TYPE_FIRST;
    ordby.sort_type = PARSER_SORT_TYPE_ASC;
    ordby.next = &ordby2;
    ordby2.expr.node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memcpy(&ordby2.expr.num, &d, sizeof(d));
    ordby2.null_order = PARSER_NULL_ORDER_TYPE_LAST;
    ordby2.sort_type = PARSER_SORT_TYPE_DESC;
    ordby2.next = &ordby3;
    ordby3.expr.node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memcpy(&ordby3.expr.num, &d2, sizeof(d2));
    ordby3.null_order = PARSER_NULL_ORDER_TYPE_LAST;
    ordby3.sort_type = PARSER_SORT_TYPE_DESC;
    ordby3.next = &ordby4;
    ordby4.expr.node_type = PARSER_EXPR_NODE_TYPE_NUM;
    memcpy(&ordby4.expr.num, &d2, sizeof(d2));
    ordby4.null_order = PARSER_NULL_ORDER_TYPE_LAST;
    ordby4.sort_type = PARSER_SORT_TYPE_ASC;
    ordby4.next = NULL;

    // single selects
    ssel1.uniq_type = PARSER_UNIQ_TYPE_ALL;
    ssel2.uniq_type = PARSER_UNIQ_TYPE_DISTINCT;
    ssel3.uniq_type = PARSER_UNIQ_TYPE_ALL;
    ssel4.uniq_type = PARSER_UNIQ_TYPE_DISTINCT;

    ssel1.projection = &proj1;
    ssel2.projection = NULL;
    ssel3.projection = NULL;
    ssel4.projection = NULL;

    ssel1.from = &from;
    ssel2.from = NULL;
    ssel3.from = NULL;
    ssel4.from = NULL;

    ssel1.where = &join_expr1;
    ssel2.where = NULL;
    ssel3.where = NULL;
    ssel4.where = NULL;

    ssel1.group_by = &grby;
    ssel2.group_by = NULL;
    ssel3.group_by = NULL;
    ssel4.group_by = NULL;

    ssel1.having = &having;
    ssel2.having = NULL;
    ssel3.having = NULL;
    ssel4.having = NULL;

    // full select statement
    sel1.select = &ssel1;
    sel1.next = &sel2;
    sel1.setop = PARSER_SETOP_TYPE_EXCEPT;
    sel1.order_by = NULL;

    sel2.select = &ssel2;
    sel2.next = &sel3;
    sel2.setop = PARSER_SETOP_TYPE_UNION_ALL;
    sel2.order_by = NULL;

    sel3.select = &ssel3;
    sel3.next = &sel4;
    sel3.setop = PARSER_SETOP_TYPE_INTERSECT;
    sel3.order_by = NULL;

    sel4.select = &ssel4;
    sel4.next = NULL;
    sel4.order_by = &ordby;


    if(parser_parse(&stmt, lexer, pi) != 0) return __LINE__;
    if(stmt == NULL) return __LINE__;

    ref_stmt.type = PARSER_STMT_TYPE_SELECT;
    ref_stmt.select_stmt = sel1;
    if(test_parser_compare_stmt(stmt, &ref_stmt) != 0) return __LINE__;

    parser_deallocate_stmt(stmt);


    // TODO: implement remaining tests
    puts("Testing insert as select statement parsing");


    puts("Testing insert values statement parsing");


    puts("Testing update statement parsing");


    puts("Testing delete statement parsing");


    puts("Testing create table statement parsing");


    puts("Testing drop table statement parsing");


    puts("Testing create database statement parsing");


    puts("Testing drop database statement parsing");


    puts("Testing alter table statement parsing");

    return 0;
}
