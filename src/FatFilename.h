#ifndef _FATCAT_FILENAME_H
#define _FATCAT_FILENAME_H

#include <vector>
#include <string>

using namespace std;

/**
 * Special class to handle long file names
 */
class FatFilename
{
    public:
        string getFilename();

        void append(char *buffer);

    protected:
        string data;
        vector<string> letters;
};

#endif // _FATCAT_FILENAME_H
