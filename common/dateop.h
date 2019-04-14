#ifndef _DATEOP_H
#define _DATEOP_H

// operations on date

typedef enum _dateop_calendar
{
    GRIGORIAN
} dateop_calendar;

// set current calendar
void dateop_set_calendar(dateop_calendar calendar);

// extract and return year of certain date
uint32 dateop_extract_year(uint64 date);

// extract and return month of year of certain date
uint8 dateop_extract_month_of_year(uint64 date);

// extract and return day of month of certain date
uint32 dateop_extract_day_of_month(uint64 date);
