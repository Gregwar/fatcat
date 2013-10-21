#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include "fat.h"

#define FAT_SEEK(OFFSET) lseek(fat->file, OFFSET, SEEK_SET)
#define FAT_LAST (-1)

struct fatfile
{
    int file;
    int bytesPerSector;
    int sectorsPerCluster;
    int reservedSectors;
    int fats;
    int sectorsPerFat;
    int rootDirectory;
    int reserved;
    char strange; // Flag set if the fat file don't look good
    int fatStart;
    int dataStart;
};

/**
 * Opens the FAT resource
 */
struct fatfile *fat_open(const char *filename)
{
    struct fatfile *fat = malloc(sizeof(struct fatfile));
    fat->file = open(filename, O_RDONLY);
    fat->strange = 0;

    return fat;
}

/**
 * Destroys the FAT resource
 */
void fat_destroy(struct fatfile *fat)
{
    close(fat->file);
    free(fat);
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
void fat_parse_header(struct fatfile *fat)
{
    char buffer[128];

    FAT_SEEK(0);
    read(fat->file, buffer, sizeof(buffer));
    fat->bytesPerSector = READ_SHORT(buffer, BYTES_PER_SECTOR);
    fat->sectorsPerCluster = buffer[SECTORS_PER_CLUSTER];
    fat->reservedSectors = READ_SHORT(buffer, RESERVED_SECTORS);
    fat->fats = buffer[FATS];
    fat->sectorsPerFat = READ_LONG(buffer, SECTORS_PER_FAT);
    fat->rootDirectory = READ_LONG(buffer, ROOT_DIRECTORY);

    if (fat->bytesPerSector != 512) {
        printf("WARNING: Bytes per sector is not 512 (%d)\n", fat->bytesPerSector);
        fat->strange++;
    }

    if (fat->sectorsPerCluster > 128) {
        printf("WARNING: Sectors per cluster high (%d)\n", fat->sectorsPerCluster);
        fat->strange++;
    }

    if (fat->reservedSectors != 0x20) {
        printf("WARNING: Reserved sectors !=0x20 (0x%08x)\n", fat->reservedSectors);
        fat->strange++;
    }

    if (fat->fats != 2) {
        printf("WARNING: Fats number different of 2 (%d)\n", fat->fats);
        fat->strange++;
    }

    if (fat->rootDirectory != 2) {
        printf("WARNING: Root directory is not 2 (%d)\n", fat->rootDirectory);
        fat->strange++;
    }
}

/**
 * Returns the 32-bit fat value for the given cluster number
 */
int fat_follow(struct fatfile *fat, int cluster)
{
    char buffer[4];

    int offset = lseek(fat->file, 0, SEEK_CUR);
    FAT_SEEK(fat->fatStart+4*cluster);
    read(fat->file, buffer, sizeof(buffer));
    FAT_SEEK(offset);

    int next = READ_LONG(buffer, 0);

    if (next >= 0x0ffffff0) {
        return FAT_LAST;
    } else {
        return next;
    }
}

int fat_cluster_address(struct fatfile *fat, int cluster)
{
    return (fat->dataStart + fat->bytesPerSector*fat->sectorsPerCluster*(cluster-2));
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

char is_zero(char *buffer, int n)
{
    int i;
    for (i=0; i<n; i++) {
        if (buffer[i] != 0) {
            return 0;
        }
    }

    return 1;
}

void fat_list(struct fatfile *fat, int cluster)
{
    do {
        FAT_SEEK(fat_cluster_address(fat, cluster));
        char buffer[FAT_RECORD_SIZE];

        int i;
        for (i=0; i<fat->bytesPerSector; i+=sizeof(buffer)) {
            read(fat->file, buffer, sizeof(buffer));
            if (!is_zero(buffer, sizeof(buffer))) {
                struct fatentry entry;
                memcpy(entry.shortName, buffer, sizeof(entry.shortName)-1);
                entry.shortName[sizeof(entry.shortName)-1] = '\0';
                entry.attributes = buffer[FAT_ATTRIBUTES];
                entry.cluster = READ_SHORT(buffer, FAT_CLUSTER_LOW) | (READ_SHORT(buffer, FAT_CLUSTER_HIGH)<<16);
                entry.size = READ_LONG(buffer, FAT_FILESIZE);

                if (!(entry.attributes & FAT_ATTRIBUTES_HIDE)) {
                    if (entry.attributes&FAT_ATTRIBUTES_DIR || entry.size) {
                        if (entry.attributes & FAT_ATTRIBUTES_DIR) {
                            printf("d");
                        } else {
                            printf("f");
                        }
                        printf(" %s [%08x, %08X, %d, %x]\n", entry.shortName, entry.cluster, fat_follow(fat, entry.cluster), entry.size, entry.attributes);
                    }
                }
            }
        }

        cluster = fat_follow(fat, cluster);
    } while (cluster != FAT_LAST);
}

void fat_scan(struct fatfile *fat)
{
    // Parsing header
    fat_parse_header(fat);

    if (fat->strange > 0) {
        printf("* FAT system too strange, aborting\n");
        return;
    } else {
        printf("* FAT system seems OK\n");
        fat->fatStart = fat->bytesPerSector * fat->reservedSectors;
        printf("FAT startings @%08X\n", fat->fatStart);
        fat->dataStart = fat->fatStart + fat->fats * fat->sectorsPerFat * fat->bytesPerSector;
        printf("Clusters startings @%08X\n", fat->dataStart);

        fat_list(fat, fat->rootDirectory);
    }
}
