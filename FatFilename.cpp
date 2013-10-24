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
    string rfilename;
    vector<string>::iterator it;

    for (it=letters.begin(); it!=letters.end(); it++) {
        rfilename += *it;
    }

    return string(rfilename.rbegin(), rfilename.rend());
}

void FatFilename::append(char *buffer)
{
    if (buffer[FAT_ATTRIBUTES] != 0xf) {
        return;
    }

    if (buffer[0]&FAT_LONG_NAME_LAST) {
        letters.clear();
    }

    int i;
    for (i=0; i<sizeof(longFilePos); i++) {
        unsigned char c = buffer[longFilePos[i]];
        unsigned char d = buffer[longFilePos[i]+1];
        if (c != 0 && c != 0xff) {
            string letter;
            if (d != 0x00) {
                letter += d;
            }
            letter += c;
            letters.push_back(letter);
        }
    }
}

