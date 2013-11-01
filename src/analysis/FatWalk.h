#ifndef _FATCAT_FATWALK_H
#define _FATCAT_FATWALK_H

#include <string>
#include <set>
#include <core/FatSystem.h>
#include <core/FatModule.h>

using namespace std;

/**
 * Walks the directory tree and call the onEntry virtual method for
 * each of entries that are found
 *
 * This can be overloaded to perform actions on each nodes of the
 * filesystem
 */
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
