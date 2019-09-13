#include <string>
#include <iostream>
#include <locale>
#include <codecvt>
#include "FatFilename.h"
#include "FatEntry.h"

using namespace std;

#define FAT_LONG_NAME_LAST      0x40

string FatFilename::getFilename()
{
    string filename;
    vector<string>::reverse_iterator it;
    wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> convert;

    for (it=letters.rbegin(); it!=letters.rend(); it++) {
        filename += *it;
    }
    letters.clear();

    if (!filename.length()) {
        return {};
    }
    return convert.to_bytes((char16_t *)filename.c_str());
}

void FatFilename::append(char *buffer)
{
    if (buffer[FAT_ATTRIBUTES] != 0xf) {
        return;
    }

    if (buffer[0]&FAT_LONG_NAME_LAST && (buffer[0]&0xff)!=FAT_ERASED) {
        letters.clear();
    }

    string letter;
    letter.append(buffer + 1, 5 * 2);
    letter.append(buffer + 14, 6 * 2);
    letter.append(buffer + 28, 2 * 2);
    letters.push_back(letter);
}

