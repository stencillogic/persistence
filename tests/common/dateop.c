#include "tests.h"
#include "common/dateop.h"
#include "calendar/grigorian.h"
#include <stdio.h>

int test_dateop_functions()
{
    puts("Testing dateop functions");

    dateop_set_calendar(DATEOP_CALENDAR_GRIGORIAN);

    if(dateop_extract_year != grigorian_extract_year) return __LINE__;
    if(dateop_extract_month_of_year != grigorian_extract_month_of_year) return __LINE__;
    if(dateop_extract_day_of_month != grigorian_extract_day_of_month) return __LINE__;
    if(dateop_trunc_to_year != grigorian_trunc_to_year) return __LINE__;
    if(dateop_is_leap_year != grigorian_is_leap_year) return __LINE__;
    if(dateop_make_date_with_time != grigorian_make_date_with_time) return __LINE__;
    if(dateop_make_date != grigorian_make_date) return __LINE__;

    if(dateop_extract_hour(0) != 0) return __LINE__;
    if(dateop_extract_hour(60) != 0) return __LINE__;
    if(dateop_extract_hour(3599) != 0) return __LINE__;
    if(dateop_extract_hour(3600) != 1) return __LINE__;
    if(dateop_extract_hour(86400 - 1) != 23) return __LINE__;
    if(dateop_extract_hour(86400) != 0) return __LINE__;

    if(dateop_extract_minutes(0) != 0) return __LINE__;
    if(dateop_extract_minutes(59) != 0) return __LINE__;
    if(dateop_extract_minutes(60) != 1) return __LINE__;
    if(dateop_extract_minutes(3599) != 59) return __LINE__;
    if(dateop_extract_minutes(3600) != 0) return __LINE__;

    if(dateop_extract_seconds(0) != 0) return __LINE__;
    if(dateop_extract_seconds(1) != 1) return __LINE__;
    if(dateop_extract_seconds(59) != 59) return __LINE__;
    if(dateop_extract_seconds(60) != 0) return __LINE__;

    return 0;
}
