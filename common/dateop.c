#include "common/dateop.h"
#include "calendar/grigorian.h"
#include <assert.h>


uint64 dateop_trunc_to_year_default_impl(uint64 date)
{
    return 0;
}

uint8 dateop_is_leap_year_default_impl(uint64 date)
{
    return 0;
}

uint64 dateop_extract_year_default_impl(uint64 date)
{
    return 0;
}

uint8 dateop_extract_month_of_year_default_impl(uint64 date)
{
    return 0;
}

uint8 dateop_extract_day_of_month_default_impl(uint64 date)
{
    return 0;
}

sint8 dateop_make_date_with_time_default_impl(uint64 *date, uint64 year, uint8 month, uint8 day, uint8 hour, uint8 min, uint8 sec)
{
    return 0;
}

sint8 dateop_make_date_default_impl(uint64 *date, uint64 year, uint8 month, uint8 day)
{
    return 0;
}

uint64 (*dateop_extract_year) (uint64 date) = dateop_extract_year_default_impl;
uint8  (*dateop_extract_month_of_year) (uint64 date) = dateop_extract_month_of_year_default_impl;
uint8  (*dateop_extract_day_of_month) (uint64 date) = dateop_extract_day_of_month_default_impl;
uint64 (*dateop_trunc_to_year) (uint64 date) = dateop_trunc_to_year_default_impl;
uint8  (*dateop_is_leap_year) (uint64 date) = dateop_is_leap_year_default_impl;
sint8  (*dateop_make_date_with_time) (uint64 *date, uint64 year, uint8 month, uint8 day, uint8 hour, uint8 min, uint8 sec) = dateop_make_date_with_time_default_impl;
sint8  (*dateop_make_date) (uint64 *date, uint64 year, uint8 month, uint8 day) = dateop_make_date_default_impl;

void dateop_set_calendar(dateop_calendar calendar)
{
    assert(calendar >= 0 && calendar < DATEOP_NUM_OF_CALENDARS);
    switch(calendar)
    {
        case DATEOP_CALENDAR_GRIGORIAN:
            dateop_extract_year         = grigorian_extract_year;
            dateop_extract_month_of_year = grigorian_extract_month_of_year;
            dateop_extract_day_of_month = grigorian_extract_day_of_month;
            dateop_trunc_to_year        = grigorian_trunc_to_year;
            dateop_is_leap_year         = grigorian_is_leap_year;
            dateop_make_date_with_time  = grigorian_make_date_with_time;
            dateop_make_date            = grigorian_make_date;
            break;
        default:
            dateop_extract_year         = dateop_extract_year_default_impl;
            dateop_extract_month_of_year = dateop_extract_month_of_year_default_impl;
            dateop_extract_day_of_month = dateop_extract_day_of_month_default_impl;
            dateop_trunc_to_year        = dateop_trunc_to_year_default_impl;
            dateop_is_leap_year         = dateop_is_leap_year_default_impl;
            dateop_make_date_with_time  = dateop_make_date_with_time_default_impl;
            dateop_make_date            = dateop_make_date_default_impl;
            break;
    }
}

