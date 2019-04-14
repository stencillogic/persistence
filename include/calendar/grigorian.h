#ifndef _CALENDAR_GRIGORIAN
#define _CALENDAR_GRIGORIAN

// grigorian calendar functions

#include "defs/defs.h"

// return date trunced to beginning of the year
uint64 grigorian_trunc_to_year(uint64 date);

// determines if current year is leap year
uint8 grigorian_is_leap_year(uint64 date);

// extract and return year of certain date
uint64 grigorian_extract_year(uint64 date);

// extract and return month of year of certain date
uint8 grigorian_extract_month_of_year(uint64 date);

// extract and return day of month of certain date
uint32 grigorian_extract_day_of_month(uint64 date);

// build date from year, month, day, hour, minute, seconds
// month is in 0..11, year must be > 0
// return 0 on success, non 0 if parameters are invalid
sint8 grigorian_make_date_with_time(uint64 *date, uint64 year, uint8 month, uint8 day, uint8 hour, uint8 min, uint8 sec);
sint8 grigorian_make_date(uint64 *date, uint64 year, uint8 month, uint8 day);

#endif
