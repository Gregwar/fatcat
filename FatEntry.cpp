#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string.h>
#include "FatEntry.h"
#include "utils.h"

using namespace std;
        
string FatEntry::getFilename()
{
    if (longName != "") {
        return longName;
    } else {
        string ext = trim(shortName.substr(8,3));
        string base = trim(shortName.substr(0,8));

        if (ext == "") {
            return base;
        } else {
            return base + "." + ext;
        }
    }
}
        
bool FatEntry::isDirectory()
{
    return attributes&FAT_ATTRIBUTES_DIR;
}
