#ifndef _FATCAT_FATSYSTEM_H
#define _FATCAT_FATSYSTEM_H

#include <vector>
#include <string>
#include "FatEntry.h"
#include "FatPath.h"

using namespace std;

// Last cluster
#define FAT_LAST (-1)

// Header offsets
#define FAT_BYTES_PER_SECTOR        0x0b
#define FAT_SECTORS_PER_CLUSTER     0x0d
#define FAT_RESERVED_SECTORS        0x0e
#define FAT_FATS                    0x10
#define FAT_SECTORS_PER_FAT         0x24
#define FAT_ROOT_DIRECTORY          0x2C

// Prefix used for erased files
#define FAT_ERASED                  0xe5

/**
 * A FAT fileSystem
 */
class FatSystem
{
    public:
        FatSystem(string filename);
        ~FatSystem();

        // Initializing the system
        bool init();

        void list(int cluster);
        void list(FatPath &path);
        void infos();

    protected:
        int fd;
        int bytesPerSector;
        int sectorsPerCluster;
        int reservedSectors;
        int fats;
        int sectorsPerFat;
        int rootDirectory;
        int reserved;
        int strange;
        int fatStart;
        int dataStart;
        int bytesPerCluster;

        void parseHeader();

        /**
         * Returns the next cluster number
         */
        int nextCluster(int cluster);

        /**
         * Returns the cluster offset in the filesystem
         */
        int clusterAddress(int cluster);

        void readData(int address, char *buffer, int size);

        /**
         * Get directory entries for a given cluster
         */
        vector<FatEntry> getEntries(int cluster);

        void readFile(int cluster, int size);
};

#endif // _FATCAT_FATSYSTEM_H
