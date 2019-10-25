#ifndef _FATCAT_FATDATE_H
#define _FATCAT_FATDATE_H

#include <string>
#include <time.h>

using namespace std;

class FatDate
{
    public:
        FatDate();
        FatDate(char *buffer);

        int h, i, s;
        int y, m, d;

        string pretty();
        time_t timestamp() const;

        string isoFormat();
};

#endif // _FATCAT_FATDATE_H
