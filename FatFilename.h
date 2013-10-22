#ifndef _FATSCAN_FILENAME_H
#define _FATSCAN_FILENAME_H

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
};

#endif // _FATSCAN_FILENAME_H
