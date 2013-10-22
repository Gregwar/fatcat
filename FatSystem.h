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
#define FAT_TOTAL_SECTORS           0x20
#define FAT_SECTORS_PER_FAT         0x24
#define FAT_ROOT_DIRECTORY          0x2c
#define FAT_DISK_LABEL              0x47
#define FAT_DISK_LABEL_SIZE         11
#define FAT_DISK_OEM                0x3
#define FAT_DISK_OEM_SIZE           8
#define FAT_DISK_FS                 0x52
#define FAT_DISK_FS_SIZE            8

/**
 * A FAT fileSystem
 */
class FatSystem
{
    public:
        FatSystem(string filename);
        ~FatSystem();

        /**
         * Initializes the system
         */
        bool init();

        /**
         * Directory listing
         */
        void list(int cluster);
        void list(FatPath &path);

        /**
         * Display infos about FAT
         */
        void infos();

        /**
         * Find a directory or a file
         */
        bool findDirectory(FatPath &path, int *cluster);
        bool findFile(FatPath &path, int *cluster, int *size, bool *erased);

        /**
         * File reading
         */
        void readFile(FatPath &path, FILE *f = NULL);
        void readFile(int cluster, int size, FILE * f = NULL);

        /**
         * Showing deleted file in listing
         */
        void setListDeleted(bool listDeleted);

        /**
         * Contiguous mode
         */
        void setContiguous(bool setContiguous);

        /**
         * Extract all the files to the given directory
         */
        void extract(string directory);

        /**
         * Compare the 2 FATs
         */
        bool compare();

        /**
         * Returns the cluster offset in the filesystem
         */
        int clusterAddress(int cluster);

    protected:
        // File descriptor
        int fd;

        // Header values
        string diskLabel;
        string oemName;
        string fsType;
        int totalSectors;
        int bytesPerSector;
        int sectorsPerCluster;
        int reservedSectors;
        int fats;
        int sectorsPerFat;
        int rootDirectory;
        int reserved;
        int strange;

        // Computed values
        int fatStart;
        int dataStart;
        int bytesPerCluster;
        int totalSize;
        int fatSize;
        int totalClusters;

        // Stats values
        bool statsComputed;
        int freeClusters;

        // Flags
        bool listDeleted;
        bool contiguous;

        void parseHeader();

        /**
         * Returns the next cluster number
         * If contiguous mode, this will just return cluster+1
         */
        int nextCluster(int cluster, int fat=0);

        /**
         * Read some data from the system
         */
        void readData(int address, char *buffer, int size);

        /**
         * Extract an entry
         */
        void extractEntry(FatEntry &entry, string directory);

        /**
         * Root directory entry
         */
        FatEntry rootEntry();

        /**
         * Is the n-th cluster free?
         */
        bool freeCluster(int cluster);

        /**
         * Compute the free clusters stats
         */
        void computeStats();

        /**
         * Get directory entries for a given cluster
         */
        vector<FatEntry> getEntries(int cluster);
};

#endif // _FATCAT_FATSYSTEM_H
