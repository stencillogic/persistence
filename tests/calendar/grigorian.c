#include "tests.h"
#include "calendar/grigorian.h"
#include <stdio.h>

#define SECONDS_IN_DAY (86400)
#define DAYS_IN_4_PERIOD    (1461UL)     // 4*365 + 1
#define DAYS_IN_100_PERIOD  (36524UL)    // 25 * DAYS_IN_4_PERIOD - 1
#define DAYS_IN_400_PERIOD  (146097UL)   // 4 * DAYS_IN_100_PERIOD + 1

int test_grigorian_calendar()
{
    puts("Testing grigorian_extract_year");
    if(1 != grigorian_extract_year(0)) return 1;
    if(1 != grigorian_extract_year(365*SECONDS_IN_DAY-1)) return 1;
    if(2 != grigorian_extract_year(365*SECONDS_IN_DAY)) return 1;
    if(4 != grigorian_extract_year((365*4+1)*SECONDS_IN_DAY-1)) return 1;
    if(5 != grigorian_extract_year((365*4+1)*SECONDS_IN_DAY)) return 1;
    if(100 != grigorian_extract_year(((uint64)(365*4 + 1)*25 - 1)*SECONDS_IN_DAY-1)) return 1;
    if(101 != grigorian_extract_year(((uint64)(365*4 + 1)*25 - 1)*SECONDS_IN_DAY)) return 1;
    if(400 != grigorian_extract_year((((uint64)(365*4 + 1)*25 - 1)*4 + 1)*SECONDS_IN_DAY-1)) return 1;
    if(401 != grigorian_extract_year((((uint64)(365*4 + 1)*25 - 1)*4 + 1)*SECONDS_IN_DAY)) return 1;
    if(2018 != grigorian_extract_year((uint64)737059*SECONDS_IN_DAY-1)) return 1;
    if(2019 != grigorian_extract_year((uint64)737059*SECONDS_IN_DAY)) return 1;


    puts("Testing grigorian_is_leap_year");
    if('n' != (grigorian_is_leap_year((uint64)0) ? 'y' : 'n')) return 1;
    if('n' != (grigorian_is_leap_year((uint64)365*3*SECONDS_IN_DAY-1) ? 'y' : 'n')) return 1;
    if('y' != (grigorian_is_leap_year((uint64)365*3*SECONDS_IN_DAY) ? 'y' : 'n')) return 1;
    if('y' != (grigorian_is_leap_year((uint64)(365*4+1)*SECONDS_IN_DAY-1) ? 'y' : 'n')) return 1;
    if('n' != (grigorian_is_leap_year((uint64)(365*4+1)*SECONDS_IN_DAY) ? 'y' : 'n')) return 1;
    if('y' != (grigorian_is_leap_year((uint64)((365*4+1)*24)*SECONDS_IN_DAY-1) ? 'y' : 'n')) return 1;
    if('n' != (grigorian_is_leap_year((uint64)((365*4+1)*25-1)*SECONDS_IN_DAY-1) ? 'y' : 'n')) return 1;
    if('n' != (grigorian_is_leap_year((uint64)((365*4+1)*25-1)*3*SECONDS_IN_DAY-1) ? 'y' : 'n')) return 1;
    if('y' != (grigorian_is_leap_year((uint64)((365*4+1)*25-1)*4*SECONDS_IN_DAY-1) ? 'y' : 'n')) return 1;
    if('y' != (grigorian_is_leap_year(((uint64)((365*4+1)*25-1)*4+1)*SECONDS_IN_DAY-1) ? 'y' : 'n')) return 1;
    if('n' != (grigorian_is_leap_year(((uint64)((365*4+1)*25-1)*4+1)*SECONDS_IN_DAY) ? 'y' : 'n')) return 1;


    puts("Testing grigorian_trunc_to_year");
    if(0 != grigorian_trunc_to_year((uint64)0)) return 1;
    if(0 != grigorian_trunc_to_year((uint64)365*SECONDS_IN_DAY-1)) return 1;
    if(365 != grigorian_trunc_to_year((uint64)365*SECONDS_IN_DAY)/SECONDS_IN_DAY) return 1;
    if(DAYS_IN_4_PERIOD-366 != grigorian_trunc_to_year((uint64)(365*4+1)*SECONDS_IN_DAY-1)/SECONDS_IN_DAY) return 1;
    if(DAYS_IN_4_PERIOD != grigorian_trunc_to_year((uint64)(365*4+1)*SECONDS_IN_DAY)/SECONDS_IN_DAY) return 1;
    if(DAYS_IN_100_PERIOD-365 != grigorian_trunc_to_year((uint64)((365*4+1)*25-1)*SECONDS_IN_DAY-1)/SECONDS_IN_DAY) return 1;
    if(DAYS_IN_100_PERIOD != grigorian_trunc_to_year((uint64)((365*4+1)*25-1)*SECONDS_IN_DAY)/SECONDS_IN_DAY) return 1;
    if(DAYS_IN_400_PERIOD-366 != grigorian_trunc_to_year((uint64)(((365*4+1)*25-1)*4+1)*SECONDS_IN_DAY-1)/SECONDS_IN_DAY) return 1;
    if(DAYS_IN_400_PERIOD != grigorian_trunc_to_year((uint64)(((365*4+1)*25-1)*4+1)*SECONDS_IN_DAY)/SECONDS_IN_DAY) return 1;


    puts("Testing grigorian_extract_month_of_year");
    if(1 != grigorian_extract_month_of_year((uint64)0)) return 1;
    if(1 != grigorian_extract_month_of_year((uint64)31*SECONDS_IN_DAY-1)) return 1;
    if(2 != grigorian_extract_month_of_year((uint64)31*SECONDS_IN_DAY)) return 1;
    if(2 != grigorian_extract_month_of_year((uint64)(31+28)*SECONDS_IN_DAY-1)) return 1;
    if(3 != grigorian_extract_month_of_year((uint64)(31+28)*SECONDS_IN_DAY)) return 1;
    if(11 != grigorian_extract_month_of_year((uint64)334*SECONDS_IN_DAY-1)) return 1;
    if(12 != grigorian_extract_month_of_year((uint64)334*SECONDS_IN_DAY)) return 1;
    if(12 != grigorian_extract_month_of_year((uint64)365*SECONDS_IN_DAY-1)) return 1;
    if(1 != grigorian_extract_month_of_year((uint64)365*SECONDS_IN_DAY)) return 1;
    uint64 first_leap=365*3*SECONDS_IN_DAY;
    if(1 != grigorian_extract_month_of_year((uint64)365*3*SECONDS_IN_DAY)) return 1;
    if(1 != grigorian_extract_month_of_year(first_leap + (uint64)31*SECONDS_IN_DAY-1)) return 1;
    if(2 != grigorian_extract_month_of_year(first_leap + (uint64)31*SECONDS_IN_DAY)) return 1;
    if(2 != grigorian_extract_month_of_year(first_leap + (uint64)(31+29)*SECONDS_IN_DAY-1)) return 1;
    if(3 != grigorian_extract_month_of_year(first_leap + (uint64)(31+29)*SECONDS_IN_DAY)) return 1;
    if(11 != grigorian_extract_month_of_year(first_leap + (uint64)335*SECONDS_IN_DAY-1)) return 1;
    if(12 != grigorian_extract_month_of_year(first_leap + (uint64)335*SECONDS_IN_DAY)) return 1;
    if(12 != grigorian_extract_month_of_year(first_leap + (uint64)366*SECONDS_IN_DAY-1)) return 1;
    if(1 != grigorian_extract_month_of_year(first_leap + (uint64)366*SECONDS_IN_DAY)) return 1;


    puts("Testing grigorian_extract_day_of_month");
    if(1 != grigorian_extract_day_of_month((uint64)0)) return 1;
    if(31 != grigorian_extract_day_of_month((uint64)31*SECONDS_IN_DAY-1)) return 1;
    if(1 != grigorian_extract_day_of_month((uint64)31*SECONDS_IN_DAY)) return 1;
    if(28 != grigorian_extract_day_of_month((uint64)(31+28)*SECONDS_IN_DAY-1)) return 1;
    if(1 != grigorian_extract_day_of_month((uint64)(31+28)*SECONDS_IN_DAY)) return 1;
    if(30 != grigorian_extract_day_of_month((uint64)334*SECONDS_IN_DAY-1)) return 1;
    if(1 != grigorian_extract_day_of_month((uint64)334*SECONDS_IN_DAY)) return 1;
    if(31 != grigorian_extract_day_of_month((uint64)365*SECONDS_IN_DAY-1)) return 1;
    if(1 != grigorian_extract_day_of_month((uint64)365*SECONDS_IN_DAY)) return 1;
    if(29 != grigorian_extract_day_of_month(first_leap+(uint64)(31+29)*SECONDS_IN_DAY-1)) return 1;
    if(1 != grigorian_extract_day_of_month(first_leap+(uint64)(31+29)*SECONDS_IN_DAY)) return 1;


    puts("Testing grigorian_make_date");
    uint64 date;
    uint8 res = grigorian_make_date(&date, 1, 1, 0);
    if(2 != res) return 1;
    res = grigorian_make_date(&date, 1, 0, 1);
    if(1 != res) return 1;
    res = grigorian_make_date(&date, 0, 1, 1);
    if(6 != res) return 1;
    res = grigorian_make_date(&date, 1, 1, 1);
    if(0 != res || date != 0) return 1;
    res = grigorian_make_date(&date, 1, 12, 31);
    if(0 != res || date != 364*SECONDS_IN_DAY) return 1;
    res = grigorian_make_date(&date, 4, 1, 1);
    if(0 != res || date != 365*3*SECONDS_IN_DAY) return 1;
    res = grigorian_make_date(&date, 4, 12, 31);
    if(0 != res || date != (DAYS_IN_4_PERIOD-1)*SECONDS_IN_DAY) return 1;
    res = grigorian_make_date(&date, 5, 1, 1);
    if(0 != res || date != DAYS_IN_4_PERIOD*SECONDS_IN_DAY) return 1;
    res = grigorian_make_date(&date, 100, 12, 31);
    if(0 != res || date != (DAYS_IN_100_PERIOD-1)*SECONDS_IN_DAY) return 1;
    res = grigorian_make_date(&date, 101, 1, 1);
    if(0 != res || date != DAYS_IN_100_PERIOD*SECONDS_IN_DAY) return 1;
    res = grigorian_make_date(&date, 400, 12, 31);
    if(0 != res || date != (DAYS_IN_400_PERIOD-1)*SECONDS_IN_DAY) return 1;
    res = grigorian_make_date(&date, 401, 1, 1);
    if(0 != res || date != DAYS_IN_400_PERIOD*SECONDS_IN_DAY) return 1;


    puts("Testing grigorian_make_date_with_time");
    res = grigorian_make_date_with_time(&date, 1, 1, 1, 0, 0, 0);
    if(0 != res) return 1;
    res = grigorian_make_date_with_time(&date, 1, 1, 1, 0, 0, 1);
    if(0 != res || date != 1) return 1;
    res = grigorian_make_date_with_time(&date, 1, 1, 1, 0, 1, 0);
    if(0 != res || date != 60) return 1;
    res = grigorian_make_date_with_time(&date, 1, 1, 1, 1, 0, 0);
    if(0 != res || date != 3600) return 1;
    res = grigorian_make_date_with_time(&date, 1, 1, 1, 24, 0, 0);
    if(3 != res) return 1;
    res = grigorian_make_date_with_time(&date, 1, 1, 1, 0, 60, 0);
    if(4 != res) return 1;
    res = grigorian_make_date_with_time(&date, 1, 1, 1, 0, 0, 60);
    if(5 != res) return 1;
    res = grigorian_make_date_with_time(&date, 1, 1, 1, 23, 59, 59);
    if(0 != res || date != SECONDS_IN_DAY-1) return 1;

    return 0;
}

