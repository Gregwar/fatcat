#ifndef _FATCAT_FATCHAINS_H
#define _FATCAT_FATCHAINS_H

#include <map>
#include <vector>
#include <iostream>
#include "FatSystem.h"

using namespace std;

class FatChains
{
    public:
        FatChains(FatSystem &system);

        void findChains();

    protected:
        FatSystem &system;
};

#endif // _FATCAT_FATCHAINS_H
