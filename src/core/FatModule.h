#ifndef _FATCAT_FATMODULE_H
#define _FATCAT_FATMODULE_H

#include "FatSystem.h"

class FatModule
{
    public:
        FatModule(FatSystem &system);

    protected:
        FatSystem &system;
};

#endif // _FATCAT_FATMODULE_H
