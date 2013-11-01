#include <string>
#include <iostream>
#include <FatUtils.h>
#include "FatPath.h"

using namespace std;

FatPath::FatPath(string path_)
    : path(path_)
{
    split(path, FAT_PATH_DELIMITER, parts);
}

string FatPath::getPath()
{
    return path;
}

string FatPath::getDirname()
{
    string dirname = "";
    for (int i=0; i<parts.size()-1; i++) {
        dirname += parts[i] + "/";
    }

    return dirname;
}
        
string FatPath::getBasename()
{
    return parts[parts.size()-1];
}

vector<string> FatPath::getParts()
{
    return parts;
}
