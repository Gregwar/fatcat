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

void FatBackup::backup(string backupFile, int fat)
{
    char buffer[CHUNKS_SIZES];
    int size = system.fatSize;
    int n = 0;
    int offset = 0;
    FILE *backup = fopen(backupFile.c_str(), "w+");

    if (fat == 0) {
        size *= 2;
    } 
    if (fat == 2) {
        offset = system.fatSize;
    }

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
        n = system.readData(system.fatStart+i+offset, buffer, toRead);
        if (n > 0) {
            fwrite(buffer, n, 1, backup);
        }
    }

    fclose(backup);
    cout << "Successfully wrote " << backupFile << " (" << size << ")" << endl;
}

void FatBackup::patch(string backupFile, int fat)
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
    int offset = 0;
    int size = system.fatSize;
    int position;
    int toWrite = 1;
    if (fat == 0) {
        size *= 2;
    }
    if (fat == 2) {
        offset = system.fatSize;
    }
    for (position=0; toWrite>0; position+=n) {
        toWrite = fread(buffer, 1, CHUNKS_SIZES, backup);
        if (position+toWrite > size) {
            toWrite = size-position;
        }
        if (toWrite > 0) {
            n = system.writeData(system.fatStart+offset+position, buffer, toWrite);
        }
    }

    fclose(backup);

    cout << "Successfully wrote " << backupFile << " as the FAT table of system (" << position << ")" << endl;
}
