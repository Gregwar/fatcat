#ifndef _FATCAT_FATEXTRACT_H
#define _FATCAT_FATEXTRACT_H

#include <string>

#include <core/FatSystem.h>
#include "FatWalk.h"

using namespace std;

class FatExtract : public FatWalk
{
    public:
        FatExtract(FatSystem &system);

        /**
         * Extract all the files to the given directory
         */
        void extract(unsigned int cluster, string directory, bool erased = false);

        virtual void onDirectory(FatEntry &parent, FatEntry &entr, string name);
        virtual void onEntry(FatEntry &parent, FatEntry &entry, string name);

    protected:
        string targetDirectory;
};

#endif // _FATCAT_FATEXTRACT_H
