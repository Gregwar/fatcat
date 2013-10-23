#include <unistd.h>
#include <time.h>
#include <string>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <set>

#include "FatSystem.h"
#include "FatFilename.h"
#include "FatEntry.h"
#include "utils.h"

using namespace std;

/**
 * Opens the FAT resource
 */
FatSystem::FatSystem(string filename_)
    : strange(0),
    filename(filename_),
    totalSize(-1),
    listDeleted(false),
    contiguous(false),
    statsComputed(false),
    freeClusters(0)
{
    fd = open(filename.c_str(), O_RDONLY|O_LARGEFILE);
    writeMode = false;

    if (fd < 0) {
        ostringstream oss;
        oss << "! Unable to open the input file: " << filename << " for reading";

        throw oss.str();
    }
}

void FatSystem::enableWrite()
{
    close(fd);
    fd = open(filename.c_str(), O_RDWR|O_LARGEFILE);

    if (fd < 0) {
        ostringstream oss;
        oss << "! Unable to open the input file: " << filename << " for writing";

        throw oss.str();
    }
    
    writeMode = true;
}

FatSystem::~FatSystem()
{
    close(fd);
}

/**
 * Reading some data
 */
int FatSystem::readData(unsigned long long address, char *buffer, int size)
{
    if (totalSize != -1 && address+size > totalSize) {
//        cout << "! Trying to read outside the disk" << endl;
    }

    lseek64(fd, address, SEEK_SET);

    return read(fd, buffer, size);
}

int FatSystem::writeData(unsigned long long address, char *buffer, int size)
{
    if (!writeMode) {
        throw string("Trying to write data while write mode is disabled");
    }

    lseek64(fd, address, SEEK_SET);

    return write(fd, buffer, size);
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
        printf("WARNING: Bytes per sector is not 512 (%llu)\n", bytesPerSector);
        strange++;
    }

    if (sectorsPerCluster > 128) {
        printf("WARNING: Sectors per cluster high (%llu)\n", sectorsPerCluster);
        strange++;
    }

    if (fats != 2) {
        printf("WARNING: Fats number different of 2 (%llu)\n", fats);
        strange++;
    }

    if (rootDirectory != 2) {
        printf("WARNING: Root directory is not 2 (%llu)\n", rootDirectory);
        strange++;
    }
}

/**
 * Returns the 32-bit fat value for the given cluster number
 */
unsigned int FatSystem::nextCluster(unsigned int cluster, int fat)
{
    char buffer[4];
    
    if (cluster >= totalClusters || cluster < 0) {
        return 0;
    }

    readData(fatStart+fatSize*fat+4*cluster, buffer, sizeof(buffer));

    unsigned int next = FAT_READ_LONG(buffer, 0);

    if (next >= 0x0ffffff0) {
        return FAT_LAST;
    } else {
        return next;
    }
}
    
/**
 * Changes the next cluster in a file
 */
bool FatSystem::writeNextCluster(unsigned int cluster, unsigned int next, int fat)
{
    char buffer[4];

    if (cluster >= totalClusters || cluster < 0) {
        throw string("Trying to access a cluster outside bounds");
    }

    buffer[0] = (next>>0)&0xff;
    buffer[1] = (next>>8)&0xff;
    buffer[2] = (next>>16)&0xff;
    buffer[3] = (next>>24)&0xff;

    return writeData(fatStart+fatSize*fat+4*cluster, buffer, sizeof(buffer))==4;
}

unsigned long long FatSystem::clusterAddress(unsigned int cluster)
{
    return (dataStart + bytesPerSector*sectorsPerCluster*(cluster-2));
}

vector<FatEntry> FatSystem::getEntries(unsigned int cluster)
{
    set<unsigned int> visited;
    vector<FatEntry> entries;
    FatFilename filename;

    if (cluster == 0) {
        cluster = rootDirectory;
    }

    do {
        unsigned long long address = clusterAddress(cluster);
        char buffer[FAT_ENTRY_SIZE];
        visited.insert(cluster);

        unsigned int i;
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

        if (visited.find(cluster) != visited.end()) {
            cerr << "! Looping directory" << endl;
            break;
        }

    } while (cluster != FAT_LAST && cluster != 0);

    return entries;
}
        
void FatSystem::list(FatPath &path)
{
    unsigned int cluster;

    if (findDirectory(path, &cluster)) {
        list(cluster);
    }
}

void FatSystem::list(unsigned int cluster)
{
    vector<FatEntry> entries = getEntries(cluster);
    vector<FatEntry>::iterator it;

    printf("Cluster: %u\n", cluster);

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

        printf(" c=%u", entry.cluster);
        
        if (!entry.isDirectory()) {
            printf(" s=%llu", entry.size);
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

void FatSystem::readFile(unsigned int cluster, unsigned int size, FILE *f)
{
    bool warning = false;
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

        if (contiguous) {
            if (!warning && !freeCluster(cluster)) {
                warning = true;
                fprintf(stderr, "! Warning: contiguous file contains cluster that seems allocated");
            }
            cluster++;
        } else {
            cluster = nextCluster(cluster);
        }

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
    totalClusters = fatSize/4;

    return strange == 0;
}
        
void FatSystem::infos()
{
    cout << "FAT Filesystem informations" << endl << endl;

    cout << "Filesystem type: " << fsType << endl;
    cout << "OEM name: " << oemName << endl;
    cout << "Total sectors: " << totalSectors << endl;
    cout << "Total clusters: " << totalClusters << endl;
    cout << "Disk size: " << totalSize << " (" << prettySize(totalSize) << ")" << endl;
    cout << "Bytes per sector: " << bytesPerSector << endl;
    cout << "Sectors per cluster: " << sectorsPerCluster << endl;
    cout << "Bytes per cluster: " << bytesPerCluster << endl;
    cout << "Reserved sectors: " << reservedSectors << endl;
    cout << "Sectors per FAT: " << sectorsPerFat << endl;
    cout << "Fat size: " << fatSize << endl;
    printf("FAT1 start address: %08x\n", fatStart);
    printf("FAT2 start address: %08x\n", fatStart+fatSize);
    printf("Data start address: %08x\n", dataStart);
    cout << "Root directory cluster: " << rootDirectory << endl;
    cout << "Disk label: " << diskLabel << endl;
    cout << endl;

    computeStats();
    cout << "Free clusters: " << freeClusters << "/" << totalClusters;
    cout << " (" << (100*freeClusters/(double)totalClusters) << "%)" << endl;
    cout << "Free space: " << (freeClusters*bytesPerCluster) << 
        " (" << prettySize(freeClusters*bytesPerCluster) << ")" << endl;
    cout << "Used space: " << ((totalClusters-freeClusters)*bytesPerCluster) << 
        " (" << prettySize((totalClusters-freeClusters)*bytesPerCluster) << ")" << endl;
    cout << endl;
}
        
bool FatSystem::findDirectory(FatPath &path, unsigned int *cluster)
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
        
bool FatSystem::findFile(FatPath &path, unsigned int *cluster, unsigned int *size, bool *erased)
{
    string dirname = path.getDirname();
    string basename = path.getBasename();

    FatPath parent(dirname);
    unsigned int parentCluster;
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
    unsigned int cluster, size;
    bool erased;
    if (findFile(path, &cluster, &size, &erased)) {
        contiguous = contiguousSave;
        if (erased && freeCluster(cluster)) {
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

        if (listDeleted || (!entry.isErased())) {
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
                    bool contiguousSave = contiguous;
                    if (entry.isErased() && freeCluster(entry.cluster)) {
                        fprintf(stderr, "! Trying to read a deleted file, auto-enabling contiguous mode\n");
                        contiguous = true;
                    }
                    readFile(entry.cluster, entry.size, output);
                    fclose(output);
                    contiguous = contiguousSave;
                } else {
                    fprintf(stderr, "! Unable to open %s\n", fullname.c_str());
                }
            }
        }
    }
}
        
void FatSystem::extract(unsigned int cluster, string directory)
{
    FatEntry entry;
    if (cluster == 0) {
        entry = rootEntry();
    } else {
        entry.longName = "/";
        entry.attributes = FAT_ATTRIBUTES_DIR;
        entry.cluster = cluster;
    }
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

bool FatSystem::freeCluster(unsigned int cluster)
{
    unsigned int next = nextCluster(cluster);

    return next == 0 
        || next == 0xf0000000;
}
        
void FatSystem::computeStats()
{
    if (statsComputed) {
        return;
    }

    statsComputed = true;

    freeClusters = 0;
    for (unsigned int cluster=0; cluster<totalClusters; cluster++) {
        if (freeCluster(cluster)) {
            freeClusters++;
        }
    }
}

bool FatSystem::isDirectory(unsigned int cluster)
{
    bool hasDotDir = false;
    bool hasDotDotDir = false;
    vector<FatEntry> entries;
    vector<FatEntry>::iterator it;

    entries = getEntries(cluster);

    for (it=entries.begin(); it!=entries.end(); it++) {
        FatEntry &entry = (*it);

        if (entry.isDirectory()) {
            string name = entry.getFilename();
            if (name == ".") {
                hasDotDir = true;
            }
            if (name == "..") {
                hasDotDotDir = true;
            }
        }
    }

    return hasDotDir && hasDotDotDir;
}

void FatSystem::rewriteUnallocated(bool random)
{
    int total = 0;
    srand(time(NULL));
    for (int cluster=0; cluster<totalClusters; cluster++) {
        if (freeCluster(cluster)) {
            char buffer[bytesPerCluster];
            for (int i=0; i<sizeof(buffer); i++) {
                if (random) {
                    buffer[i] = rand()&0xff;
                } else {
                    buffer[i] = 0x0;
                }
            }
            writeData(clusterAddress(cluster), buffer, sizeof(buffer));
            total++;
        }
    }

    cout << "Scrambled " << total << " sectors" << endl;
}
