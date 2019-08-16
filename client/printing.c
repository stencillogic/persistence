#include "client/printing.h"
#include <stdio.h>
#include <string.h>

#define PRINTING_MAX_COL_NUM    (1000)
#define PRINTING_STRBUF_SZ      (4000)

struct _g_printing_state
{
    uint8   padding;
    uint16  col_num;
    uint32  col_width[PRINTING_MAX_COL_NUM];
    uint8   strbuf[PRINTING_STRBUF_SZ];
} g_printing_state = {.col_num = 0, .padding = 1};

uint8 table_padding()
{
    return g_printing_state.padding;
}

void table_set_col_num(uint16 col_num)
{
    g_printing_state.col_num = (col_num > PRINTING_MAX_COL_NUM ? PRINTING_MAX_COL_NUM : col_num);
}

void table_set_col_width(uint16 c, uint32 w)
{
    if(c > g_printing_state.col_num) c = g_printing_state.col_num;
    if(w > PRINTING_STRBUF_SZ - 2) w = PRINTING_STRBUF_SZ - 2;

    g_printing_state.col_width[c] = w;
}

void table_print_separator()
{
    fputc(' ', stdout);
    for(uint16 c = 0; c < g_printing_state.col_num; c++)
    {
        g_printing_state.strbuf[0] = '+';
        memset(g_printing_state.strbuf + 1, '-', g_printing_state.col_width[c]);
        g_printing_state.strbuf[g_printing_state.col_width[c] + 1] = '\0';
        fputs((const char*)g_printing_state.strbuf, stdout);
    }
    fputs("+\n", stdout);
}

void table_print_cell(const char *str, uint32 w, uint32 c)
{
    fputs(" | ", stdout);
    fprintf(stdout, "\e[s");
    if(w + 2 * g_printing_state.padding > g_printing_state.col_width[c])
    {
        w = g_printing_state.col_width[c] - 2 * g_printing_state.padding;
    }
    fwrite(str, w, 1, stdout);
    fprintf(stdout, "\e[u\e");
}

void table_finish_row()
{
    fputs(" |\n", stdout);
}

uint32 table_get_col_width(uint16 c)
{
    return g_printing_state.col_width[c];
}
