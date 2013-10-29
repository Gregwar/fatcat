#ifndef _FATCAT_FATFIX_H
#define _FATCAT_FATFIX_H

#include <string>
#include <set>
#include "FatWalk.h"
#include "FatSystem.h"

using namespace std;

class FatFix : public FatWalk
{
    public:
        FatFix(FatSystem &system);

        /**
         * Fix reachable things
         */
        void fix();

    protected: 
        virtual void onEntry(FatEntry &parent, FatEntry &entry, string name);
        
        void fixChain(int cluster, int size);
};

#endif // _FATCAT_FATFIX_H
