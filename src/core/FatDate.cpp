#include <stdio.h>
#include <sstream>
#include <iostream>
#include <string>
#include <time.h>

#include <FatUtils.h>
#include "FatDate.h"

using namespace std;

FatDate::FatDate()
{
}

FatDate::FatDate(char *buffer)
{
    int H = FAT_READ_SHORT(buffer, 0);
    int D = FAT_READ_SHORT(buffer, 2);

    s = 2*(H&0x1f);
    i = (H>>5)&0x3f;
    h = (H>>11)&0x1f;

    d = D&0x1f;
    m = (D>>5)&0xf;
    y = 1980+((D>>9)&0x7f);
}

string FatDate::pretty()
{
    char buffer[128];
    sprintf(buffer, "%d/%d/%04d %02d:%02d:%02d", d, m, y, h, i, s);

    return string(buffer);
}

/**
 * Returns date as a number of seconds elapsed since the Epoch,
 * 1970-01-01 00:00:00 +0000 (UTC). FAT dates are considered to be in the
 * current timezone not in UTC. So what this function returns depends on
 * the current timezone.
 */
time_t FatDate::timestamp() const
{
    struct tm tm;
    tm.tm_sec = s;         // Seconds (0-60)
    tm.tm_min = i;         // Minutes (0-59)
    tm.tm_hour = h;        // Hours (0-23)
    tm.tm_mday = d;        // Day of the month (1-31)
    tm.tm_mon = m - 1;     // Month (0-11)
    tm.tm_year = y - 1900; // Year - 1900

    // A negative value of tm_isdst means that mktime() should (use timezone
    // information and system databases to) attempt to determine whether DST
    // is in effect at the specified time.
    tm.tm_isdst = -1;      // Daylight saving time

    return mktime(&tm);
}

string FatDate::isoFormat()
{
    char buffer[128];
    sprintf(buffer, "%04d-%02d-%02dT%02d:%02d:%02d", y, m, d, h, i, s);

    return string(buffer);
}
