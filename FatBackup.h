#ifndef _FATCAT_FATBACKUP_H
#define _FATCAT_FATBACKUP_H

#include "FatSystem.h"

/**
 * Handle backup of the FAT tables
 */
class FatBackup
{
    public:
        FatBackup(FatSystem &system);

        void backup(string backupFile);
        void patch(string backupFile);

    protected:
        FatSystem &system;
};

#endif // _FATCAT_FATBACKUP_H
