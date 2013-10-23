#include <string>
#include <sstream>
#include <stdio.h>
#include <iostream>
#include <fcntl.h>
#include "FatBackup.h"

using namespace std;

#define CHUNKS_SIZES 1024
        
FatBackup::FatBackup(FatSystem &system)
    : FatModule(system)
{
}

void FatBackup::backup(string backupFile)
{
    char buffer[CHUNKS_SIZES];
    int size = system.fatSize*2;
    int n = 0;
    FILE *backup = fopen(backupFile.c_str(), "w+");

    if (backup == NULL) {
        ostringstream oss;
        oss << "Unable to open file " << backupFile << " for writing";
        throw oss.str();
    }

    for (int i=0; i<size; i+=n) {
        int toRead = CHUNKS_SIZES;
        if (size-i < toRead) {
            toRead = size-i;
        }
        n = system.readData(system.fatStart+i, buffer, toRead);
        if (n > 0) {
            fwrite(buffer, n, 1, backup);
        }
    }

    fclose(backup);
    cout << "Successfully wrote " << backupFile << " (" << size << ")" << endl;
}

void FatBackup::patch(string backupFile)
{
    char buffer[CHUNKS_SIZES];

    // Opening the file
    FILE *backup = fopen(backupFile.c_str(), "r");
    if (backup == NULL) {
        ostringstream oss;
        oss << "Unable to open file " << backupFile << " for reading";
        throw oss.str();
    }

    // Activating the writing mode on the fat system
    system.enableWrite();

    int n;
    int position;
    int toWrite = 1;
    for (position=0; toWrite; position+=n) {
        toWrite = fread(buffer, 1, CHUNKS_SIZES, backup);
        n = system.writeData(system.fatStart+position, buffer, toWrite);
    }

    fclose(backup);

    cout << "Successfully wrote " << backupFile << " as the FAT table of system (" << position << ")" << endl;
}
