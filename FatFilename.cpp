#include "FatFilename.h"
#include "FatEntry.h"
#include <string>

using namespace std;

#define FAT_LONG_NAME_LAST      0x40

// Offset of letters position in a special "long file name" entry
static unsigned char longFilePos[] = {
    30, 28, 24, 22, 20, 18, 16, 14, 9, 7, 5, 3, 1
};

string FatFilename::getFilename()
{
    string filename = string(data.rbegin(), data.rend());
    data = "";

    return filename;
}

void FatFilename::append(char *buffer)
{
    if (buffer[FAT_ATTRIBUTES] != 0xf) {
        return;
    }

    if (buffer[0]&FAT_LONG_NAME_LAST) {
        data = "";
    }

    int i;
    for (i=0; i<sizeof(longFilePos); i++) {
        unsigned char c = buffer[longFilePos[i]];
        if (c != 0 && c != 0xff) {
            data += c;
        }
    }
}

