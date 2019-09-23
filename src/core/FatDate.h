#ifndef _FATCAT_FATDATE_H
#define _FATCAT_FATDATE_H

#include <string>
#ifndef __WIN__
#include <time.h>
#endif

using namespace std;

class FatDate
{
    public:
        FatDate();
        FatDate(char *buffer);

        int h, i, s;
        int y, m, d;

        string pretty();
        #ifndef __WIN__
        time_t timestamp() const;
        #endif

        string isoFormat();
};

#endif // _FATCAT_FATDATE_H
