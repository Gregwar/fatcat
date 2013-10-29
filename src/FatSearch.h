#ifndef _FATCAT_FATSEARCH_H
#define _FATCAT_FATSEARCH_H

#include <string>
#include <set>
#include "FatWalk.h"
#include "FatSystem.h"

using namespace std;

class FatSearch : public FatWalk
{
    public:
        FatSearch(FatSystem &system);

        /**
         * Searches an entry matching given cluster
         */
        void search(int cluster);

    protected:
        int searchCluster;
        int found;
        
        virtual void onEntry(FatEntry &parent, FatEntry &entry, string name);
};

#endif // _FATCAT_FATSEARCH_H
