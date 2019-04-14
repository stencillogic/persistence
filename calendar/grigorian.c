#include "calendar/grigorian.h"

#define SECONDS_IN_DAY      (86400UL)
#define DAYS_IN_4_PERIOD    (1461UL)     // 4*365 + 1
#define DAYS_IN_100_PERIOD  (36524UL)    // 25 * DAYS_IN_4_PERIOD - 1
#define DAYS_IN_400_PERIOD  (146097UL)   // 4 * DAYS_IN_100_PERIOD + 1

static sint8 grigorian_month_days[24] = {
    31, 28, 31,
    30, 31, 30,
    31, 31, 30,
    31, 30, 31,

    31, 29, 31,
    30, 31, 30,
    31, 31, 30,
    31, 30, 31 };

static sint16 grigorian_year_day_by_month[24] = {
    0,   31,  59,
    90,  120, 151,
    181, 212, 243,
    273, 304, 334,

    0,   31,  60,
    91,  121, 152,
    182, 213, 244,
    274, 305, 335,
};

uint64 grigorian_trunc_to_year(uint64 date)
{
    uint64 res          = date / SECONDS_IN_DAY;  // whole days
    uint64 res400       = res / DAYS_IN_400_PERIOD;   // number of 400 year periods
    uint64 res400_rem   = res % DAYS_IN_400_PERIOD;

    if(res400_rem == DAYS_IN_400_PERIOD-1)
        return (res - 365)*SECONDS_IN_DAY;

    uint64 res100       = res400_rem / DAYS_IN_100_PERIOD;    // number of 100 year periods
    uint64 res100_rem   = res400_rem % DAYS_IN_100_PERIOD;
    uint64 res4         = res100_rem / DAYS_IN_4_PERIOD;   // etc
    uint64 res4_rem     = res100_rem % DAYS_IN_4_PERIOD;   // etc
    res                 = (res4_rem == DAYS_IN_4_PERIOD-1) ? 3 : res4_rem / 365;

    return (res400 * DAYS_IN_400_PERIOD
         + res100 * DAYS_IN_100_PERIOD
         + res4 * DAYS_IN_4_PERIOD
         + res * 365) * SECONDS_IN_DAY;
}

uint8 grigorian_is_leap_year(uint64 date)
{
    uint64 res          = date / SECONDS_IN_DAY;
    uint64 res400_rem   = res % DAYS_IN_400_PERIOD;
    if(res400_rem >= DAYS_IN_400_PERIOD-366) return 1;

    uint64 res100_rem   = res400_rem % DAYS_IN_100_PERIOD;
    if(res100_rem >= DAYS_IN_100_PERIOD-365) return 0;

    if(res100_rem % DAYS_IN_4_PERIOD >= DAYS_IN_4_PERIOD-366) return 1;
    return 0;
}

uint64 grigorian_extract_year(uint64 date)
{
    uint64 res          = date / SECONDS_IN_DAY;  // whole days
    uint64 res400       = res / DAYS_IN_400_PERIOD;   // number of 400 year periods
    uint64 res400_rem   = res % DAYS_IN_400_PERIOD;
    uint64 res100       = (res400_rem == DAYS_IN_400_PERIOD-1) ? 3 : res400_rem / DAYS_IN_100_PERIOD;    // number of 100 year periods
    uint64 res100_rem   = (res400_rem == DAYS_IN_400_PERIOD-1) ? DAYS_IN_100_PERIOD : res400_rem % DAYS_IN_100_PERIOD;
    uint64 res4         = res100_rem / DAYS_IN_4_PERIOD;   // etc
    uint64 res4_rem     = res100_rem % DAYS_IN_4_PERIOD;
    res                 = (res4_rem == DAYS_IN_4_PERIOD-1) ? 3 : res4_rem / 365;

    return res400 * 400 + res100 * 100 + res4 * 4 + res + 1;  // starts with 1
}

uint8 grigorian_extract_month_of_year(uint64 date)
{
    sint16 year_day = (date - grigorian_trunc_to_year(date)) / SECONDS_IN_DAY;
    uint8 is_leap = grigorian_is_leap_year(date);

    sint8 *month_days = grigorian_month_days + 12 * is_leap;
    uint8 month = 0;
    do
    {
        year_day -= month_days[month++];
    }
    while(year_day >= 0);

    return month;
}

uint32 grigorian_extract_day_of_month(uint64 date)
{
    sint16 year_day = (date - grigorian_trunc_to_year(date)) / SECONDS_IN_DAY;
    uint8 is_leap = grigorian_is_leap_year(date);

    sint8 *month_days = grigorian_month_days + 12 * is_leap;
    uint8 month = 0;
    while(year_day >= month_days[month])
    {
        year_day -= month_days[month++];
    }
    return year_day+1;
}

sint8 grigorian_make_date_with_time(uint64 *date, uint64 year, uint8 month, uint8 day, uint8 hour, uint8 min, uint8 sec)
{
    if(hour > 23) return 3;
    if(min > 59) return 4;
    if(sec > 59) return 5;

    uint8 res = grigorian_make_date(date, year, month, day);
    if(res == 0)
    {
        *date += hour * 3600 + min * 60 + sec;
    }

    return res;
}

sint8 grigorian_make_date(uint64 *date, uint64 year, uint8 month, uint8 day)
{
    if(month < 1 || month > 12) return 1;
    if(year < 1) return 6;
    month--;

    uint64 y400 = (year-1) / 400;
    uint64 y100 = ((year-1) % 400) / 100;
    uint64 y4   = ((year-1) % 100) / 4;

    uint8 leap = ((((year % 4 == 0) && (year % 100 != 0)) || year % 400 == 0) && year != 0) ? 12 : 0;
    sint16 *year_day_by_month = grigorian_year_day_by_month + leap;
    if(day < 1 || day > grigorian_month_days[month + leap]) return 2;
    day--;

    *date = (y400 * DAYS_IN_400_PERIOD + y100 * DAYS_IN_100_PERIOD + y4 * DAYS_IN_4_PERIOD + ((year-1) % 4) * 365
        + year_day_by_month[month] + day) * SECONDS_IN_DAY;

    return 0;
}
