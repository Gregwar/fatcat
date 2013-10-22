#include <unistd.h>
#include <string>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include "FatSystem.h"

using namespace std;
 
void FatSystem::readData(int address, char *buffer, int size)
{
    lseek(fd, address, SEEK_SET);
    read(fd, buffer, size);
}

/**
 * Opens the FAT resource
 */
FatSystem::FatSystem(string filename)
    : strange(0)
{
    fd = open(filename.c_str(), O_RDONLY);
    cout << "Hello world!" << endl;
}

FatSystem::~FatSystem()
{
    close(fd);
}

// Offsets
#define BYTES_PER_SECTOR        0x0b
#define SECTORS_PER_CLUSTER     0x0d
#define RESERVED_SECTORS        0x0e
#define FATS                    0x10
#define SECTORS_PER_FAT         0x24
#define ROOT_DIRECTORY          0x2C

#define READ_SHORT(buffer,x) ((buffer[x]&0xff)|((buffer[x+1]&0xff)<<8))
#define READ_LONG(buffer,x) \
        ((buffer[x]&0xff)|((buffer[x+1]&0xff)<<8))| \
        (((buffer[x+2]&0xff)<<16)|((buffer[x+3]&0xff)<<24))

/**
 * Parses FAT header
 */
void FatSystem::parseHeader()
{
    char buffer[128];

    readData(0x0, buffer, sizeof(buffer));
    bytesPerSector = READ_SHORT(buffer, BYTES_PER_SECTOR);
    sectorsPerCluster = buffer[SECTORS_PER_CLUSTER];
    reservedSectors = READ_SHORT(buffer, RESERVED_SECTORS);
    fats = buffer[FATS];
    sectorsPerFat = READ_LONG(buffer, SECTORS_PER_FAT);
    rootDirectory = READ_LONG(buffer, ROOT_DIRECTORY);

    if (bytesPerSector != 512) {
        printf("WARNING: Bytes per sector is not 512 (%d)\n", bytesPerSector);
        strange++;
    }

    if (sectorsPerCluster > 128) {
        printf("WARNING: Sectors per cluster high (%d)\n", sectorsPerCluster);
        strange++;
    }

    if (reservedSectors != 0x20) {
        printf("WARNING: Reserved sectors !=0x20 (0x%08x)\n", reservedSectors);
        strange++;
    }

    if (fats != 2) {
        printf("WARNING: Fats number different of 2 (%d)\n", fats);
        strange++;
    }

    if (rootDirectory != 2) {
        printf("WARNING: Root directory is not 2 (%d)\n", rootDirectory);
        strange++;
    }
}

/**
 * Returns the 32-bit fat value for the given cluster number
 */
int FatSystem::nextCluster(int cluster)
{
    char buffer[4];

    readData(fatStart+4*cluster, buffer, sizeof(buffer));

    int next = READ_LONG(buffer, 0);

    if (next >= 0x0ffffff0) {
        return FAT_LAST;
    } else {
        return next;
    }
}

int FatSystem::clusterAddress(int cluster)
{
    return (dataStart + bytesPerSector*sectorsPerCluster*(cluster-2));
}

#define FAT_RECORD_SIZE         0x20

struct fatentry
{
    char shortName[12];
    char attributes;
    int cluster;
    int size;
};

#define FAT_SHORTNAME           0x00
#define FAT_ATTRIBUTES          0x0b
#define FAT_CLUSTER_LOW         0x1a
#define FAT_CLUSTER_HIGH        0x14
#define FAT_FILESIZE            0x1c
#define FAT_ATTRIBUTES_HIDE     (1<<1)
#define FAT_ATTRIBUTES_DIR      (1<<4)
#define FAT_ATTRIBUTES_LONGFILE (0xf)
#define FAT_ATTRIBUTES_FILE     (0x20)

char longFilePos[] = {
    30, 28, 24, 22, 20, 18, 16, 14, 9, 7, 5, 3, 1
};

void append_file_name(char *buffer, char *fnBuffer, int *pos)
{
    int i;  
    for (i=0; i<13; i++) {
        unsigned char c = buffer[longFilePos[i]];
        if (c != 0 && c != 0xff) {
            (*pos)--;
            fnBuffer[*pos] = c;
        }
    }
}

void FatSystem::list(int cluster)
{
    char longFileName[256];
    int longFilePos = 255;
    longFileName[255] = 0;

    do {
        int address = clusterAddress(cluster);
        char buffer[FAT_RECORD_SIZE];

        int i;
        for (i=0; i<bytesPerSector; i+=sizeof(buffer)) {
            readData(address, buffer, sizeof(buffer));
            address += sizeof(buffer);
            struct fatentry entry;
            entry.attributes = buffer[FAT_ATTRIBUTES];

            if (entry.attributes & FAT_ATTRIBUTES_LONGFILE) {
                append_file_name(buffer, longFileName, &longFilePos);
                // Long file part
            } else {
                memcpy(entry.shortName, buffer, sizeof(entry.shortName)-1);
                entry.shortName[sizeof(entry.shortName)-1] = '\0';
                entry.cluster = READ_SHORT(buffer, FAT_CLUSTER_LOW) | (READ_SHORT(buffer, FAT_CLUSTER_HIGH)<<16);
                entry.size = READ_LONG(buffer, FAT_FILESIZE);

                if (!(entry.attributes & FAT_ATTRIBUTES_HIDE)) {
                    if (entry.attributes&FAT_ATTRIBUTES_DIR || entry.attributes&FAT_ATTRIBUTES_FILE) {
                        if (entry.attributes & FAT_ATTRIBUTES_DIR) {
                            printf("d");
                        } else {
                            printf("f");
                        }
                        printf(" %s [%s] [%08x, %08X, %d, %x]\n", longFileName+longFilePos, entry.shortName, entry.cluster, nextCluster(entry.cluster), entry.size, entry.attributes);
                    }
                }
                longFilePos = 255;
            }
        }

        cluster = nextCluster(cluster);
    } while (cluster != FAT_LAST);
}

void FatSystem::readFile(int cluster, int size)
{
    while (size) {
        int toRead = size;
        if (toRead > bytesPerSector) {
            toRead = bytesPerSector;
        }
        char buffer[bytesPerSector];
        readData(clusterAddress(cluster), buffer, toRead);
        size -= toRead;

        int i;
        for (i=0; i<toRead; i++) {
            fprintf(stderr, "%c", buffer[i]);
        }

        cluster = nextCluster(cluster);
    }
}

void FatSystem::run()
{
    // Parsing header
    parseHeader();

    if (strange > 0) {
        printf("* FAT system too strange, aborting\n");
        return;
    } else {
        printf("* FAT system seems OK\n");
        fatStart = bytesPerSector * reservedSectors;
        printf("FAT startings @%08X\n", fatStart);
        dataStart = fatStart + fats*sectorsPerFat*bytesPerSector;
        printf("Clusters startings @%08X\n", dataStart);

        // fat_list(fat, rootDirectory);
        // fat_list(fat, 3);
        // fat_read_file(fat, 0x1e, 792);
        // fat_list(fat, 0x20);
        // fat_read_file(fat, 0x000009d4, 5);
        list(0x9e6);
        // fat_list(fat, 0x9e6);
        // fat_read_file(fat, 0x000009f0, 1048576);
    }
}
