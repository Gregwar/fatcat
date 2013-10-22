#ifndef _FATSCAN_FAT_H
#define _FATSCAN_FAT_H

#include <string>

using namespace std;

// Last cluster
#define FAT_LAST (-1)

/**
 * A FAT fileSystem
 */
class FatSystem
{
    public:
        FatSystem(string filename);
        ~FatSystem();

        void run();

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

        void list(int cluster);
        void readFile(int cluster, int size);
};

#endif // _FATSCAN_FAT_H
