#include <string>
#include <stdio.h>
#include <iostream>
#include <fcntl.h>
#include "FatBackup.h"

using namespace std;

#define CHUNK_SIZES 1024
        
FatBackup::FatBackup(FatSystem &system_)
    : system(system_)
{
}

void FatBackup::backup(string backupFile)
{
    char buffer[CHUNK_SIZES];
    int size = system.fatSize*2;
    int n = 0;
    FILE *backup = fopen(backupFile.c_str(), "w+");

    for (int i=0; i<size; i+=n) {
        int toRead = CHUNK_SIZES;
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
    throw string("Not implemented!");
}
