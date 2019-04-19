#ifndef _DATEOP_H
#define _DATEOP_H

// operations on dates

#include "defs/defs.h"

#define DATEOP_NUM_OF_CALENDARS 1

typedef enum _dateop_calendar
{
    DATEOP_CALENDAR_GRIGORIAN = 0
} dateop_calendar;

// set current calendar
void dateop_set_calendar(dateop_calendar calendar);

// extract and return year of certain date
extern uint64 (*dateop_extract_year)(uint64 date);

// extract and return month of year of certain date
extern uint8 (*dateop_extract_month_of_year)(uint64 date);

// extract and return day of month of certain date
extern uint8 (*dateop_extract_day_of_month)(uint64 date);

// return date truncated to beginning of the year
extern uint64 (*dateop_trunc_to_year)(uint64 date);

// determines if current year is leap year (GRIGORIAN)
extern uint8 (*dateop_is_leap_year)(uint64 date);

// extract and return hour
inline uint8 dateop_extract_hour(uint64 date)
{
    return (date % 86400) / 3600;
}

// extract and return minutes
inline uint8 dateop_extract_minutes(uint64 date)
{
    return (date % 3600) / 60;
}

// extract and return seconds
inline uint8 dateop_extract_seconds(uint64 date)
{
    return (date % 60);
}

// build date from year, month, day, hour, minute, seconds (GRIGORIAN)
// month is in 0..11, year must be > 0
// return 0 on success, non 0 if parameters are invalid
extern sint8 (*dateop_make_date_with_time)(uint64 *date, uint64 year, uint8 month, uint8 day, uint8 hour, uint8 min, uint8 sec);
extern sint8 (*dateop_make_date)(uint64 *date, uint64 year, uint8 month, uint8 day);

#endif
