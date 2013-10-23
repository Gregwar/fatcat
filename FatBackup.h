#ifndef _FATCAT_FATBACKUP_H
#define _FATCAT_FATBACKUP_H

#include "FatSystem.h"
#include "FatModule.h"

/**
 * Handle backup of the FAT tables
 */
class FatBackup : public FatModule
{
    public:
        FatBackup(FatSystem &system);
        
        void backup(string backupFile);
        void patch(string backupFile);
};

#endif // _FATCAT_FATBACKUP_H
