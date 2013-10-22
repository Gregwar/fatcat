#include <unistd.h>
#include <string>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include "FatSystem.h"
#include "FatFilename.h"
#include "FatEntry.h"
#include "utils.h"

using namespace std;

/**
 * Opens the FAT resource
 */
    FatSystem::FatSystem(string filename)
: strange(0)
{
    fd = open(filename.c_str(), O_RDONLY);
}

FatSystem::~FatSystem()
{
    close(fd);
}

/**
 * Reading some data
 */
void FatSystem::readData(int address, char *buffer, int size)
{
    lseek(fd, address, SEEK_SET);
    read(fd, buffer, size);
}

/**
 * Parses FAT header
 */
void FatSystem::parseHeader()
{
    char buffer[128];

    readData(0x0, buffer, sizeof(buffer));
    bytesPerSector = FAT_READ_SHORT(buffer, FAT_BYTES_PER_SECTOR);
    sectorsPerCluster = buffer[FAT_SECTORS_PER_CLUSTER];
    reservedSectors = FAT_READ_SHORT(buffer, FAT_RESERVED_SECTORS);
    fats = buffer[FAT_FATS];
    sectorsPerFat = FAT_READ_LONG(buffer, FAT_SECTORS_PER_FAT);
    rootDirectory = FAT_READ_LONG(buffer, FAT_ROOT_DIRECTORY);

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

    int next = FAT_READ_LONG(buffer, 0);

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

vector<FatEntry> FatSystem::getEntries(int cluster)
{
    vector<FatEntry> entries;
    FatFilename filename;

    do {
        int address = clusterAddress(cluster);
        char buffer[FAT_ENTRY_SIZE];

        int i;
        for (i=0; i<bytesPerSector*sectorsPerCluster; i+=sizeof(buffer)) {
            // Reading data
            readData(address, buffer, sizeof(buffer));
            address += sizeof(buffer);

            // Creating entry
            FatEntry entry;

            entry.attributes = buffer[FAT_ATTRIBUTES];

            if (entry.attributes & FAT_ATTRIBUTES_LONGFILE) {
                // Long file part
                filename.append(buffer);
            } else {
                entry.shortName = string(buffer, 11);
                entry.longName = filename.getFilename();
                entry.cluster = FAT_READ_SHORT(buffer, FAT_CLUSTER_LOW) | (FAT_READ_SHORT(buffer, FAT_CLUSTER_HIGH)<<16);
                entry.size = FAT_READ_LONG(buffer, FAT_FILESIZE);

                if ((entry.shortName[0]&0xff) != FAT_ERASED) {
                    if (!(entry.attributes & FAT_ATTRIBUTES_HIDE)) {
                        if (entry.attributes&FAT_ATTRIBUTES_DIR || entry.attributes&FAT_ATTRIBUTES_FILE) {
                            entries.push_back(entry);
                        }
                    }
                }
            }
        }

        cluster = nextCluster(cluster);
    } while (cluster != FAT_LAST);

    return entries;
}
        
void FatSystem::list(FatPath &path)
{
    vector<string> parts = path.getParts();
    int cluster = rootDirectory;

    for (int i=0; i<parts.size(); i++) {
        if (parts[i] != "") {
            vector<FatEntry> entries = getEntries(cluster);
            vector<FatEntry>::iterator it;
            bool found = false;

            for (it=entries.begin(); it!=entries.end(); it++) {
                FatEntry &entry = *it;
                string name = entry.getFilename();
                if (name == parts[i]) {
                    cluster = entry.cluster;
                    found = true;
                }
            }

            if (!found) {
                cerr << "Error: directory " << path.getPath() << " not found" << endl;
                return;
            }
        }
    }

    list(cluster);
}

void FatSystem::list(int cluster)
{
    vector<FatEntry> entries = getEntries(cluster);
    vector<FatEntry>::iterator it;

    for (it=entries.begin(); it!=entries.end(); it++) {
        FatEntry &entry = *it;
        if (entry.attributes & FAT_ATTRIBUTES_DIR) {
            printf("d");
        } else {
            printf("f");
        }
        printf(" %s [%s] [%08x, %08X, %d, %x]\n", entry.getFilename().c_str(), entry.shortName.c_str(), entry.cluster, nextCluster(entry.cluster), entry.size, entry.attributes);
    }
}

void FatSystem::readFile(int cluster, int size)
{
    while (size) {
        int toRead = size;
        if (toRead > bytesPerCluster) {
            toRead = bytesPerCluster;
        }
        char buffer[bytesPerCluster];
        readData(clusterAddress(cluster), buffer, toRead);
        size -= toRead;

        int i;
        for (i=0; i<toRead; i++) {
            fprintf(stderr, "%c", buffer[i]);
        }

        cluster = nextCluster(cluster);
    }
}

bool FatSystem::init()
{
    // Parsing header
    parseHeader();

    // Computing values
    fatStart = bytesPerSector * reservedSectors;
    dataStart = fatStart + fats*sectorsPerFat*bytesPerSector;
    bytesPerCluster = bytesPerSector*sectorsPerCluster;

    return strange == 0;
}
        
void FatSystem::infos()
{
    cout << "FAT Filesystem informations" << endl;

    cout << "Bytes per sector: " << bytesPerSector << endl;
    cout << "Sectors per cluster: " << sectorsPerCluster << endl;
    cout << "Bytes per cluster: " << bytesPerCluster << endl;
    cout << "Reserved sectors: " << reservedSectors << endl;
    cout << "Sectors per FAT: " << sectorsPerFat << endl;
    printf("FAT start address: %08x\n", fatStart);
    printf("Data start address: %08x\n", dataStart);
}
