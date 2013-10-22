#include <unistd.h>
#include <string>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "FatSystem.h"
#include "FatFilename.h"
#include "FatEntry.h"
#include "utils.h"

using namespace std;

/**
 * Opens the FAT resource
 */
FatSystem::FatSystem(string filename)
    : strange(0),
    totalSize(-1),
    listDeleted(false),
    contiguous(false)
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
    if (totalSize != -1 && address+size > totalSize) {
        cout << "! Trying to read outside the disk" << endl;
    }

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
    totalSectors = FAT_READ_LONG(buffer, FAT_TOTAL_SECTORS);
    diskLabel = string(buffer+FAT_DISK_LABEL, FAT_DISK_LABEL_SIZE);
    oemName = string(buffer+FAT_DISK_OEM, FAT_DISK_OEM_SIZE);
    fsType = string(buffer+FAT_DISK_FS, FAT_DISK_FS_SIZE);

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

    if (contiguous) {
        return cluster+1;
    }

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

    if (cluster == 0) {
        cluster = rootDirectory;
    }

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

                if (entry.attributes&FAT_ATTRIBUTES_DIR || entry.attributes&FAT_ATTRIBUTES_FILE) {
                    entries.push_back(entry);
                }
            }
        }

        cluster = nextCluster(cluster);
    } while (cluster != FAT_LAST);

    return entries;
}
        
void FatSystem::list(FatPath &path)
{
    int cluster;

    if (findDirectory(path, &cluster)) {
        list(cluster);
    }
}

void FatSystem::list(int cluster)
{
    vector<FatEntry> entries = getEntries(cluster);
    vector<FatEntry>::iterator it;

    for (it=entries.begin(); it!=entries.end(); it++) {
        FatEntry &entry = *it;

        if (entry.isErased() && !listDeleted) {
            continue;
        }

        if (entry.isDirectory()) {
            printf("d");
        } else {
            printf("f");
        }

        string name = entry.getFilename();
        if (entry.isDirectory()) {
            name += "/";
        }
        printf(" %-40s", name.c_str());

        printf(" c=%d", entry.cluster);
        
        if (!entry.isDirectory()) {
            printf(" s=%d", entry.size);
        }

        if (entry.isHidden()) {
            printf(" h");
        }
        if (entry.isErased()) {
            printf(" d");
        }

        printf("\n");
    }
}

void FatSystem::readFile(int cluster, int size, FILE *f)
{
    if (f == NULL) {
        f = stdout;
    }

    while ((size!=0) && cluster!=FAT_LAST) {
        int toRead = size;
        if (toRead > bytesPerCluster || size < 0) {
            toRead = bytesPerCluster;
        }
        char buffer[bytesPerCluster];
        readData(clusterAddress(cluster), buffer, toRead);

        if (size != -1) {
            size -= toRead;
        }

        // Write file data to the given file
        fwrite(buffer, toRead, 1, f);

        cluster = nextCluster(cluster);

        if (cluster == 0) {
            fprintf(stderr, "! One of your file's cluster is 0 (maybe FAT is broken, you should try -c)\n");
        }
    }
}

bool FatSystem::init()
{
    // Parsing header
    parseHeader();

    // Computing values
    fatStart = bytesPerSector*reservedSectors;
    dataStart = fatStart + fats*sectorsPerFat*bytesPerSector;
    bytesPerCluster = bytesPerSector*sectorsPerCluster;
    totalSize = totalSectors*bytesPerSector;
    fatSize = sectorsPerFat*bytesPerSector;

    return strange == 0;
}
        
void FatSystem::infos()
{
    cout << "FAT Filesystem informations" << endl << endl;

    cout << "Filesystem type: " << fsType << endl;
    cout << "OEM name: " << oemName << endl;
    cout << "Total sectors: " << totalSectors << endl;
    cout << "Disk size: " << totalSize << endl;
    cout << "Bytes per sector: " << bytesPerSector << endl;
    cout << "Sectors per cluster: " << sectorsPerCluster << endl;
    cout << "Bytes per cluster: " << bytesPerCluster << endl;
    cout << "Reserved sectors: " << reservedSectors << endl;
    cout << "Sectors per FAT: " << sectorsPerFat << endl;
    cout << "Fat size: " << fatSize << endl;
    printf("FAT start address: %08x\n", fatStart);
    printf("Data start address: %08x\n", dataStart);
    cout << "Root directory cluster: " << rootDirectory << endl;
    cout << "Disk label: " << diskLabel << endl;
}
        
bool FatSystem::findDirectory(FatPath &path, int *cluster)
{
    vector<string> parts = path.getParts();
    *cluster = rootDirectory;

    for (int i=0; i<parts.size(); i++) {
        if (parts[i] != "") {
            vector<FatEntry> entries = getEntries(*cluster);
            vector<FatEntry>::iterator it;
            bool found = false;

            for (it=entries.begin(); it!=entries.end(); it++) {
                FatEntry &entry = *it;
                string name = entry.getFilename();
                if (name == parts[i]) {
                    *cluster = entry.cluster;
                    found = true;
                }
            }

            if (!found) {
                cerr << "Error: directory " << path.getPath() << " not found" << endl;
                return false;
            }
        }
    }

    return true;
}
        
bool FatSystem::findFile(FatPath &path, int *cluster, int *size, bool *erased)
{
    string dirname = path.getDirname();
    string basename = path.getBasename();

    FatPath parent(dirname);
    int parentCluster;
    if (findDirectory(parent, &parentCluster)) {
        vector<FatEntry> entries = getEntries(parentCluster);
        vector<FatEntry>::iterator it;

        for (it=entries.begin(); it!=entries.end(); it++) {
            FatEntry &entry = (*it);
            if (entry.getFilename() == path.getBasename()) {    
                *cluster = entry.cluster;
                *size = entry.size;
                *erased = entry.isErased();
                return true;
            }
        }
    }

    return false;
}
        
void FatSystem::readFile(FatPath &path, FILE *f)
{
    bool contiguousSave = contiguous;
    contiguous = false;
    int cluster, size;
    bool erased;
    if (findFile(path, &cluster, &size, &erased)) {
        contiguous = contiguousSave;
        if (erased && nextCluster(cluster)==0) {
            fprintf(stderr, "! Trying to read a deleted file, auto-enabling contiguous mode\n");
            contiguous = true;
        }
        readFile(cluster, size, f);
    }
}
        
void FatSystem::setListDeleted(bool listDeleted_)
{
    listDeleted = listDeleted_;
}
        
void FatSystem::setContiguous(bool contiguous_)
{
    contiguous = contiguous_;
}
        
void FatSystem::extractEntry(FatEntry &entry, string directory)
{
    vector<FatEntry> entries = getEntries(entry.cluster);
    vector<FatEntry>::iterator it;

    mkdir(directory.c_str(), 0755);

    for (it=entries.begin(); it!=entries.end(); it++) {
        FatEntry &entry = (*it);

        if (!entry.isErased()) {
            string name = entry.getFilename();

            if (name == "." || name == "..") {
                continue;
            }
                
            string fullname = directory + "/" + name;

            if (entry.isDirectory()) {
                cout << "Entering " << fullname << endl;
                extractEntry(entry, fullname);
            } else {
                cout << "Extracting " << fullname << endl;
                FILE *output = fopen(fullname.c_str(), "w+");
                if (output != NULL) {
                    readFile(entry.cluster, entry.size, output);
                    fclose(output);
                } else {
                    fprintf(stderr, "! Unable to open %s\n", fullname.c_str());
                }
            }
        }
    }
}
        
void FatSystem::extract(string directory)
{
    FatEntry entry = rootEntry();
    extractEntry(entry, directory);
}
        
FatEntry FatSystem::rootEntry()
{
    FatEntry entry;
    entry.longName = "/";
    entry.attributes = FAT_ATTRIBUTES_DIR;
    entry.cluster = rootDirectory;

    return entry;
}
