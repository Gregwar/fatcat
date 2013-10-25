#ifndef _FATCAT_FATSYSTEM_H
#define _FATCAT_FATSYSTEM_H

#include <map>
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
#define FAT_CREATION_DATE           0x10
#define FAT_CHANGE_DATE             0x16

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
        void list(vector<FatEntry> &entries);
        void list(unsigned int cluster);
        void list(FatPath &path);

        /**
         * Display infos about FAT
         */
        void infos();

        /**
         * Find a directory or a file
         */
        bool findDirectory(FatPath &path, FatEntry &entry);
        bool findFile(FatPath &path, FatEntry &entry);

        /**
         * File reading
         */
        void readFile(FatPath &path, FILE *f = NULL);
        void readFile(unsigned int cluster, unsigned int size, FILE * f = NULL);

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
        void extract(unsigned int cluster, string directory);

        /**
         * Is the n-th cluster free?
         */
        bool freeCluster(unsigned int cluster);

        /**
         * Returns the cluster offset in the filesystem
         */
        unsigned long long clusterAddress(unsigned int cluster);

        /**
         * Enable the FAT caching
         */
        void enableCache();

        // File descriptor
        string filename;
        int fd;
        bool writeMode;

        // Header values
        string diskLabel;
        string oemName;
        string fsType;
        unsigned long long totalSectors;
        unsigned long long bytesPerSector;
        unsigned long long sectorsPerCluster;
        unsigned long long reservedSectors;
        unsigned long long fats;
        unsigned long long sectorsPerFat;
        unsigned long long rootDirectory;
        unsigned long long reserved;
        unsigned long long strange;

        // Computed values
        unsigned long long fatStart;
        unsigned long long dataStart;
        unsigned long long bytesPerCluster;
        unsigned long long totalSize;
        unsigned long long dataSize;
        unsigned long long fatSize;
        unsigned long long totalClusters;

        // FAT Cache
        bool cacheEnabled;
        map<int, int> cache;

        // Stats values
        bool statsComputed;
        unsigned long long freeClusters;

        // Flags
        bool listDeleted;
        bool contiguous;
        
        /**
         * Returns the next cluster number
         * If contiguous mode, this will just return cluster+1
         */
        unsigned int nextCluster(unsigned int cluster, int fat=0);

        /**
         * Write a next cluster value in the file (helper)
         */
        bool writeNextCluster(unsigned int cluster, unsigned int next, int fat=0);

        /**
         * Is this cluster valid?
         */
        bool validCluster(unsigned int cluster);

        /**
         * Enable write mode on the FAT system, the internal file descriptor
         * will be re-opened in write mode
         */
        void enableWrite();

        /**
         * Read some data from the system
         */
        int readData(unsigned long long address, char *buffer, int size);

        /**
         * Write some data to the system, write should be enabled
         */
        int writeData(unsigned long long address, const char *buffer, int size);

        /**
         * Get directory entries for a given cluster
         */
        vector<FatEntry> getEntries(unsigned int cluster);

        /**
         * Write random data in unallocated sectors
         */
        void rewriteUnallocated(bool random=false);

        /**
         * Return a chain size
         */
        int chainSize(int cluster);
    
    protected:
        void parseHeader();

        /**
         * Extract an entry
         */
        void extractEntry(FatEntry &entry, string directory);

        /**
         * Root directory entry
         */
        FatEntry rootEntry();

        /**
         * Compute the free clusters stats
         */
        void computeStats();
};

#endif // _FATCAT_FATSYSTEM_H
