#ifndef _FATCAT_FATWALK_H
#define _FATCAT_FATWALK_H

#include <string>
#include <set>
#include "FatSystem.h"
#include "FatModule.h"

using namespace std;

class FatWalk : public FatModule
{
    public:
        FatWalk(FatSystem &system);

        void walk();
        void doWalk(set<int> &visited, FatEntry &entry, string name);

    protected:
        bool walkErased;
        
        virtual void onEntry(FatEntry &parent, FatEntry &entry, string name);
};

#endif // _FATCAT_FATWALK_H
