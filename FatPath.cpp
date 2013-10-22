#include <string>
#include <iostream>
#include "FatPath.h"
#include "utils.h"

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

vector<string> FatPath::getParts()
{
    return parts;
}
