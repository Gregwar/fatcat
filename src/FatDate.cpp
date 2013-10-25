#include <stdio.h>
#include <sstream>
#include <iostream>
#include <string>
#include "FatDate.h"
#include "utils.h"

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
