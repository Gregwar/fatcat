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
#include "FatDate.h"
#include "utils.h"

using namespace std;

/**
 * Opens the FAT resource
 */
FatSystem::FatSystem(string filename_, unsigned long long globalOffset_)
    : strange(0),
    filename(filename_),
    globalOffset(globalOffset_),
    totalSize(-1),
    listDeleted(false),
    statsComputed(false),
    freeClusters(0),
    cacheEnabled(false)
{
    fd = open(filename.c_str(), O_RDONLY|O_LARGEFILE);
    writeMode = false;

    if (fd < 0) {
        ostringstream oss;
        oss << "! Unable to open the input file: " << filename << " for reading";

        throw oss.str();
    }
}

void FatSystem::enableCache()
{
    if (!cacheEnabled) {
        cout << "Computing FAT cache..." << endl;
        for (int cluster=0; cluster<totalClusters; cluster++) {
            cache[cluster] = nextCluster(cluster);
        }
        
        cacheEnabled = true;
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
        cerr << "! Trying to read outside the disk" << endl;
    }

    lseek64(fd, globalOffset+address, SEEK_SET);

    int n;
    int pos = 0;
    do {
        n = read(fd, buffer+pos, size);

        if (n > 0) {
            pos += n;
            size -= n;
        }
    } while ((size>0) && (n>0));

    return n;
}

int FatSystem::writeData(unsigned long long address, const char *buffer, int size)
{
    if (!writeMode) {
        throw string("Trying to write data while write mode is disabled");
    }

    lseek64(fd, globalOffset+address, SEEK_SET);

    int n;
    int pos = 0;
    do {
        n = write(fd, buffer, size);

        if (n > 0) {
            pos += n;
            size -= n;
        }
    } while ((size>0) && (n>0));

    return n;
}

/**
 * Parses FAT header
 */
void FatSystem::parseHeader()
{
    char buffer[128];

    readData(0x0, buffer, sizeof(buffer));
    bytesPerSector = FAT_READ_SHORT(buffer, FAT_BYTES_PER_SECTOR)&0xffff;
    sectorsPerCluster = buffer[FAT_SECTORS_PER_CLUSTER];
    reservedSectors = FAT_READ_SHORT(buffer, FAT_RESERVED_SECTORS)&0xffff;
    fats = buffer[FAT_FATS];
    sectorsPerFat = FAT_READ_LONG(buffer, FAT_SECTORS_PER_FAT)&0xffffffff;
    rootDirectory = FAT_READ_LONG(buffer, FAT_ROOT_DIRECTORY)&0xffffffff;
    totalSectors = FAT_READ_LONG(buffer, FAT_TOTAL_SECTORS)&0xffffffff;
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
    
    if (!validCluster(cluster)) {
        return 0;
    }

    if (cacheEnabled) {
        return cache[cluster];
    }

    readData(fatStart+fatSize*fat+4*cluster, buffer, sizeof(buffer));

    unsigned int next = FAT_READ_LONG(buffer, 0)&0x0fffffff;

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

    if (!validCluster(cluster)) {
        throw string("Trying to access a cluster outside bounds");
    }

    buffer[0] = (next>>0)&0xff;
    buffer[1] = (next>>8)&0xff;
    buffer[2] = (next>>16)&0xff;
    buffer[3] = (next>>24)&0xff;

    return writeData(fatStart+fatSize*fat+4*cluster, buffer, sizeof(buffer))==4;
}
        
bool FatSystem::validCluster(unsigned int cluster)
{
    return cluster < totalClusters;
}

unsigned long long FatSystem::clusterAddress(unsigned int cluster)
{
    return (dataStart + bytesPerSector*sectorsPerCluster*(cluster-2));
}

vector<FatEntry> FatSystem::getEntries(unsigned int cluster, int *clusters)
{
    int foundEntries = 0;
    int badEntries = 0;
    bool isValid = false;
    set<unsigned int> visited;
    vector<FatEntry> entries;
    FatFilename filename;

    if (clusters != NULL) {
        *clusters = 0;
    }

    if (cluster == 0) {
        cluster = rootDirectory;
    }

    if (cluster == rootDirectory) {
        isValid = true;
    }

    if (!validCluster(cluster)) {
        return vector<FatEntry>();
    }

    do {
        int localFound = 0;
        int localBadEntries = 0;
        unsigned long long address = clusterAddress(cluster);
        char buffer[FAT_ENTRY_SIZE];
        visited.insert(cluster);

        unsigned int i;
        for (i=0; i<bytesPerCluster; i+=sizeof(buffer)) {
            // Reading data
            readData(address, buffer, sizeof(buffer));

            // Creating entry
            FatEntry entry;

            entry.attributes = buffer[FAT_ATTRIBUTES];
            entry.address = address;

            if (entry.attributes == FAT_ATTRIBUTES_LONGFILE) {
                // Long file part
                filename.append(buffer);
            } else {
                entry.shortName = string(buffer, 11);
                entry.longName = filename.getFilename();
                entry.size = FAT_READ_LONG(buffer, FAT_FILESIZE)&0xffffffff;
                entry.cluster = (FAT_READ_SHORT(buffer, FAT_CLUSTER_LOW)&0xffff) | (FAT_READ_SHORT(buffer, FAT_CLUSTER_HIGH)<<16);
                entry.setData(string(buffer, sizeof(buffer)));

                if (!entry.isZero()) {
                    if (entry.isCorrect() && validCluster(entry.cluster)) {
                        entry.creationDate = FatDate(&buffer[FAT_CREATION_DATE]);
                        entry.changeDate = FatDate(&buffer[FAT_CHANGE_DATE]);
                        entries.push_back(entry);
                        localFound++;
                        foundEntries++;

                        if (!isValid && entry.getFilename() == "." && entry.cluster == cluster) {
                            isValid = true;
                        }
                    } else {
                        localBadEntries++;
                        badEntries++;
                    }
                }
            }

            address += sizeof(buffer);
        }

        int previousCluster = cluster;
        cluster = nextCluster(cluster);

        if (visited.find(cluster) != visited.end()) {
            cerr << "! Looping directory" << endl;
            break;
        }

        if (cluster == 0) {
            if (localFound && localBadEntries<localFound) {
                cluster = previousCluster+1;
            } else {
                break;
            }
        }

        if (!isValid) {
            if (badEntries>foundEntries) {
                cerr << "! Entries don't look good, this is maybe not a directory" << endl;
                return vector<FatEntry>();
          }
        }
        
        if (clusters) {
            (*clusters)++;
        }

    } while (cluster != FAT_LAST);

    return entries;
}
        
void FatSystem::list(FatPath &path)
{
    FatEntry entry;

    if (findDirectory(path, entry)) {
        list(entry.cluster);
    }
}

void FatSystem::list(unsigned int cluster)
{
    vector<FatEntry> entries = getEntries(cluster);
    printf("Directory cluster: %u\n", cluster);
    list(entries);
}
            
void FatSystem::list(vector<FatEntry> &entries)
{
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

        printf(" %s ", entry.changeDate.pretty().c_str());
        printf(" %-30s", name.c_str());

        printf(" c=%u", entry.cluster);
        
        if (!entry.isDirectory()) {
            string pretty = prettySize(entry.size);
            printf(" s=%llu (%s)", entry.size, pretty.c_str());
        }

        if (entry.isHidden()) {
            printf(" h");
        }
        if (entry.isErased()) {
            printf(" d");
        }

        printf("\n");
        fflush(stdout);
    }
}

void FatSystem::readFile(unsigned int cluster, unsigned int size, FILE *f, bool contiguous)
{
    if (f == NULL) {
        f = stdout;
    }

    while ((size!=0) && cluster!=FAT_LAST) {
        int currentCluster = cluster;
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
        fflush(f);

        if (contiguous) {
            if (!freeCluster(cluster)) {
                fprintf(stderr, "! Contiguous file contains cluster that seems allocated\n");
                fprintf(stderr, "! Trying to disable contiguous mode\n");
                contiguous = false;
                cluster = nextCluster(cluster);
            } else {
                cluster++;
            }
        } else {
            cluster = nextCluster(currentCluster);

            if (cluster == 0) {
                fprintf(stderr, "! One of your file's cluster is 0 (maybe FAT is broken, have a look to -2 and -m)\n");
                fprintf(stderr, "! Trying to enable contigous mode\n");
                contiguous = true;
                cluster = currentCluster+1;
            }
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
    dataSize = totalClusters*bytesPerCluster;

    return strange == 0;
}
        
void FatSystem::infos()
{
    cout << "FAT Filesystem informations" << endl << endl;

    cout << "Filesystem type: " << fsType << endl;
    cout << "OEM name: " << oemName << endl;
    cout << "Total sectors: " << totalSectors << endl;
    cout << "Total data clusters: " << totalClusters << endl;
    cout << "Data size: " << dataSize << " (" << prettySize(dataSize) << ")" << endl;
    cout << "Disk size: " << totalSize << " (" << prettySize(totalSize) << ")" << endl;
    cout << "Bytes per sector: " << bytesPerSector << endl;
    cout << "Sectors per cluster: " << sectorsPerCluster << endl;
    cout << "Bytes per cluster: " << bytesPerCluster << endl;
    cout << "Reserved sectors: " << reservedSectors << endl;
    cout << "Sectors per FAT: " << sectorsPerFat << endl;
    cout << "Fat size: " << fatSize << " (" << prettySize(fatSize) << ")" << endl;
    printf("FAT1 start address: %016llx\n", fatStart);
    printf("FAT2 start address: %016llx\n", fatStart+fatSize);
    printf("Data start address: %016llx\n", dataStart);
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
        
bool FatSystem::findDirectory(FatPath &path, FatEntry &outputEntry)
{
    int cluster;
    vector<string> parts = path.getParts();
    cluster = rootDirectory;
    outputEntry.cluster = cluster;

    for (int i=0; i<parts.size(); i++) {
        if (parts[i] != "") {
            parts[i] = strtolower(parts[i]);
            vector<FatEntry> entries = getEntries(cluster);
            vector<FatEntry>::iterator it;
            bool found = false;

            for (it=entries.begin(); it!=entries.end(); it++) {
                FatEntry &entry = *it;
                string name = entry.getFilename();
                if (entry.isDirectory() && strtolower(name) == parts[i]) {
                    outputEntry = entry;
                    cluster = entry.cluster;
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

bool FatSystem::findFile(FatPath &path, FatEntry &outputEntry)
{
    bool found = false;
    string dirname = path.getDirname();
    string basename = path.getBasename();
    basename = strtolower(basename);

    FatPath parent(dirname);
    FatEntry parentEntry;
    if (findDirectory(parent, parentEntry)) {
        vector<FatEntry> entries = getEntries(parentEntry.cluster);
        vector<FatEntry>::iterator it;

        for (it=entries.begin(); it!=entries.end(); it++) {
            FatEntry &entry = (*it);
            if (strtolower(entry.getFilename()) == basename) {
                outputEntry = entry;

                if (entry.size == 0) {
                    found = true;
                } else {
                    return true;
                }
            }
        }
    }

    return found;
}
        
void FatSystem::readFile(FatPath &path, FILE *f)
{
    FatEntry entry;
    
    if (findFile(path, entry)) {
        bool contiguous = false;
        if (entry.isErased() && freeCluster(entry.cluster)) {
            fprintf(stderr, "! Trying to read a deleted file, auto-enabling contiguous mode\n");
            contiguous = true;
        }
        readFile(entry.cluster, entry.size, f, contiguous);
    }
}
        
void FatSystem::setListDeleted(bool listDeleted_)
{
    listDeleted = listDeleted_;
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

            if (name == "." || name == ".." || entry.cluster == 0) {
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
                    bool contiguous = false;
                    if (entry.isErased() && freeCluster(entry.cluster)) {
                        fprintf(stderr, "! Trying to read a deleted file, auto-enabling contiguous mode\n");
                        contiguous = true;
                    }
                    readFile(entry.cluster, entry.size, output);
                    fclose(output);
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

int FatSystem::chainSize(int cluster, bool *isContiguous)
{
    set<int> visited;
    int length = 0;
    bool stop;

    if (isContiguous != NULL) {
        *isContiguous = true;
    }

    do {
        stop = true;
        int currentCluster = cluster;
        visited.insert(cluster);
        length++;
        cluster = nextCluster(cluster);
        if (validCluster(cluster) && cluster!=FAT_LAST) {
            if (currentCluster+1 != cluster && isContiguous != NULL) {
                *isContiguous = false;
            }
            if (visited.find(cluster) != visited.end()) {
                cerr << "! Loop detected, " << currentCluster << " points to " << cluster << " that I already met" << endl;
            } else {
                stop = false;
            }
        }
    } while (!stop);

    return length;
}
