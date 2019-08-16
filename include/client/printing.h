#ifndef _PRINTING_H
#define _PRINTING_H

#include "defs/defs.h"

uint8 table_padding();

void table_set_col_num(uint16 col_num);

void table_set_col_width(uint16 c, uint32 w);

uint32 table_get_col_width(uint16 c);

void table_print_separator();

void table_print_cell(const char *str, uint32 sz, uint32 c);

void table_finish_row();


#endif
