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
        string name;
        string ext = trim(shortName.substr(8,3));
        string base = trim(shortName.substr(0,8));

        if (isErased()) {
            base = base.substr(1);
        }

        name = base;

        if (ext != "") {
            name += "." + ext;
        }

        name = strtolower(name);

        return name;
    }
}
        
bool FatEntry::isDirectory()
{
    return attributes&FAT_ATTRIBUTES_DIR;
}

bool FatEntry::isHidden()
{
    return attributes&FAT_ATTRIBUTES_HIDE;
}

bool FatEntry::isErased()
{
    return ((shortName[0]&0xff) == FAT_ERASED);
}
