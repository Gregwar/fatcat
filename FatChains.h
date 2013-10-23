#ifndef _FATCAT_FATCHAINS_H
#define _FATCAT_FATCHAINS_H

#include <map>
#include <vector>
#include <iostream>
#include "FatSystem.h"
#include "FatModule.h"

using namespace std;

class FatChains : public FatModule
{
    public:
        FatChains(FatSystem &system);

        void findChains();
};

#endif // _FATCAT_FATCHAINS_H
