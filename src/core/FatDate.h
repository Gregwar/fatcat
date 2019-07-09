#ifndef _FATCAT_FATDATE_H
#define _FATCAT_FATDATE_H

#include <string>

using namespace std;

class FatDate
{
    public:
        FatDate();
        FatDate(char *buffer);

        int h, i, s;
        int y, m, d;

        string pretty();

        string isoFormat();
};

#endif // _FATCAT_FATDATE_H
