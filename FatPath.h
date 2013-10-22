#ifndef _FATCAT_PATH_H
#define _FATCAT_PATH_H

#include <vector>
#include <string>

#define FAT_PATH_DELIMITER '/'

using namespace std;

class FatPath
{
    public:
        FatPath(string path);
        
        string getPath();
        string getDirname();
        string getBasename();
        vector<string> getParts();

    protected:
        string path;
        vector<string> parts;
};

#endif // _FATCAT_PATH_H
