#ifndef _FATCAT_FATDIFF_H
#define _FATCAT_FATDIFF_H

#include "FatModule.h"

class FatDiff : public FatModule
{
    public:
        FatDiff(FatSystem &system);

        /**
         * Compare the 2 FATs
         */
        bool compare();

        /**
         * Merge the 2 FATs
         */
        void merge();
};

#endif // _FATCAT_FATDIFF_H
