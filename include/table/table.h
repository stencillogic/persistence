#ifndef _TABLE_H
#define _TABLE_H

#include "defs/defs.h"
#include "common/decimal.h"


// dictionary structures and associated limits


#define MAX_TABLE_COLUMNS                   (1024)            // 2^16 is the max value here
#define MAX_TABLE_NAME_LEN                  (256)
#define MAX_TABLE_COL_NAME_LEN              (256)
#define MAX_CONSTRAINT_NAME_LEN             (256)
#define MAX_CHECK_CONSTRAINT_EXPRESSION_LEN (8192)
#define MAX_DEFAULT_VALUE_CHAR_LEN          (256)
#define MAX_VARCHAR_LENGTH                  (1024L * 1024L * 1024L * 2L)



#define COLUMN_DATA_TYPE_NUM                (10)

typedef enum _column_datatype
{
    CHARACTER_VARYING = 0x01,
    DECIMAL = 0x02,
    INTEGER = 0x03,
    SMALLINT = 0x04,
    FLOAT = 0x05,
    DOUBLE_PRECISION = 0x06,
    DATE = 0x07,
    TIMESTAMP = 0x08,
    TIMESTAMP_WITH_TZ = 0x09
} column_datatype;

typedef enum _constraint_type
{
    PK_CONSTRAINT = 1,
    UNIQUE_CONSTRAINT = 2,
    FK_CONSTRAINT = 3,
    CHECK_CONSTRAINT = 4
} constraint_type;

typedef struct _table_constraint_desc
{
    uint32          table_desc_id;
    uint32          id;
    uint8           name[MAX_CONSTRAINT_NAME_LEN];
    constraint_type type;
} table_constraint_desc;

typedef struct _uk_constraint_col_desc    // constraint-column correspondence
{
    uint32  table_desc_id;
    uint32  table_constraint_desc_id;
    uint16  table_col_desc_id;
    uint16  col_pos;
} uk_constraint_col_desc;

typedef struct _fk_constraint_col_desc    // constraint-column correspondence
{
    uint32  table_desc_id;
    uint32  table_constraint_desc_id;
    uint16  table_col_desc_id;
    uint16  col_pos;
    uint32  ref_table_desc_id;        // referenced col for fk constraint
    uint16  ref_table_col_dec_id;
} fk_constraint_col_desc;

typedef struct _check_constraint_details_desc
{
    uint32  table_constraint_desc_id;
    uint8   expression[MAX_CHECK_CONSTRAINT_EXPRESSION_LEN];    // TODO: replace with expression tree
} check_constraint_details_desc;

typedef struct _col_default_value_desc
{
    uint32 table_desc_id;
    uint16 col_desc_id;
    uint8  has_default;
    uint16 value_size;
    union
    {
        uint64  date_val;
        float64 double_val;
        float32 float_val;
        sint32  integer_val;
        sint16  smallint_val;
        decimal decimal_val;
        uint8   char_val[MAX_DEFAULT_VALUE_CHAR_LEN];
        uint64  timestamp_val;
        struct
        {
            uint64  ts;
            sint16  tz;
        } ts_with_tz_val;
    } value;
} col_default_value_desc;

typedef struct _table_col_desc
{
    uint32              table_desc_id;
    uint16              id;
    uint8               name[MAX_TABLE_COL_NAME_LEN];
    column_datatype     datatype;
    uint32              len;        // aka precision for numeric types and fraction of second in timestamp
    uint32              scale;
    uint8               is_nullable;
    uint16              col_pos;
} table_col_desc;

typedef struct _table_desc
{
    uint32      id;
    uint8       name[MAX_TABLE_NAME_LEN];
    uint8       is_temporary;
    uint16      degree;    // number of columns
} table_desc;

#endif
