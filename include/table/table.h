#ifndef _TABLE_H
#define _TABLE_H

#include "defs/defs.h"

#define MAX_TABLE_COLUMNS 1024;			// 2^16 is max value here
#define MAX_TABLE_NAME_LEN 256;
#define MAX_TABLE_COL_NAME_LEN 256;
#define MAX_CONSTRAINT_NAME_LEN 256;
#define MAX_CHECK_CONSTRAINT_EXPRESSION_LEN 8192;
#define MAX_DEFAULT_VALUE_CHAR_LEN 256;

typedef enum _column_datatype
{
    CHARACTER_VARYING = 0x01,
	DECIMAL = 0x02,
    INTEGER = 0x03,
	SMALLINT = 0x04,
	FLOAT = 0x05,
	DOUBLE_PRECISION = 0x06,
	DATE = 0x07,
    TIMESTAMP = 0x08
} column_datatype;

typedef enum _constraint_type
{
	PK_CONSTRAINT = 1,
	UNIQUE_CONSTRAINT,
	FK_CONSTRAINT,
	CHECK_CONSTRAINT
} constraint_type;

typedef struct _table_constraint_desc
{
	uint32 			table_desc_id;
	uint32			id;
	uint8  			name[MAX_CONSTRAINT_NAME_LEN];
	constraint_type type;
} table_constraint_desc;

typedef struct _uk_constraint_col_desc	// constraint-column correspondence
{
	uint32	table_desc_id;
	uint32  table_constraint_desc_id;
	uint16	table_col_desc_id;
	uint16	col_pos;
} uk_constraint_col_desc;

typedef struct _fk_constraint_col_desc	// constraint-column correspondence
{
	uint32	table_desc_id;
	uint32  table_constraint_desc_id;
	uint16	table_col_desc_id;
	uint16	col_pos;
	uint32	ref_table_desc_id;		// referenced col for fk constraint
	uint16	ref_table_col_dec_id;
} fk_constraint_col_desc;

typedef struct _check_constraint_details_desc
{
	uint32  table_constraint_desc_id;
	uint8	expression[MAX_CHECK_CONSTRAINT_EXPRESSION_LEN];	// TODO: replace with expression tree
} check_constraint_details_desc;

typedef struct _col_default_value_desc
{
	uint32 table_desc_id;
    uint16 col_desc_id;
    uint8  has_default;
    uint16 value_size;
	union
	{
		uint64  datetime_val;
		float64 double_val;
		float32 float_val;
		int32	integer_val;
		uint8   decimal_val[20];
		uint8	char_val[MAX_DEFAULT_VALUE_CHAR_LEN];
	} value;
} col_default_value_desc;

typedef struct _table_col_desc
{
	uint32				   table_desc_id;
	uint16				   id;
    uint8                  name[MAX_TABLE_COL_NAME_LEN];
    column_datatype        datatype;
	uint32				   len;		// aka precision for numeric types and fraction of second in timestamp
	uint32				   scale;
	uint8                  is_nullable;
	uint16                 col_pos;
} table_col_desc;

typedef struct _table_desc
{
	uint32	 id;
    uint8 	 name[MAX_TABLE_NAME_LEN];
	uint8	 is_temporary;
	uint16	 degree;	// number of columns
} table_desc;

#endif
