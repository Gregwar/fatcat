#ifdef __WIN__
#include <io.h>
#else
#include <unistd.h>
#endif
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

#include <FatUtils.h>
#include "FatFilename.h"
#include "FatEntry.h"
#include "FatDate.h"
#include "FatSystem.h"

using namespace std;

/**
 * Opens the FAT resource
 */
FatSystem::FatSystem(string filename_, unsigned long long globalOffset_, OutputFormatType outputFormat_)
    : strange(0),
      filename(filename_),
      globalOffset(globalOffset_),
      totalSize(-1),
      listDeleted(false),
      statsComputed(false),
      freeClusters(0),
      cacheEnabled(false),
      type(FAT32),
    rootEntries(0)
{
    this->_outputFormat = outputFormat_;
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
    sectorsPerCluster = buffer[FAT_SECTORS_PER_CLUSTER]&0xff;
    reservedSectors = FAT_READ_SHORT(buffer, FAT_RESERVED_SECTORS)&0xffff;
    oemName = string(buffer+FAT_DISK_OEM, FAT_DISK_OEM_SIZE);
    fats = buffer[FAT_FATS];

    sectorsPerFat = FAT_READ_SHORT(buffer, FAT16_SECTORS_PER_FAT)&0xffff;

    if (sectorsPerFat != 0) {
        type = FAT16;
        diskLabel = string(buffer+FAT16_DISK_LABEL, FAT16_DISK_LABEL_SIZE);
        fsType = string(buffer+FAT16_DISK_FS, FAT16_DISK_FS_SIZE);
        rootEntries = FAT_READ_SHORT(buffer, FAT16_ROOT_ENTRIES)&0xffff;
        rootDirectory = 0;

        totalSectors = FAT_READ_SHORT(buffer, FAT16_TOTAL_SECTORS)&0xffff;
        if (!totalSectors) {
            totalSectors = FAT_READ_LONG(buffer, FAT_TOTAL_SECTORS)&0xffffffff;
        }

        unsigned int rootDirSectors =
            (rootEntries*FAT_ENTRY_SIZE + bytesPerSector-1)/bytesPerSector;
        unsigned long dataSectors = totalSectors -
            (reservedSectors + fats*sectorsPerFat + rootDirSectors);
        totalClusters = dataSectors/sectorsPerCluster;
        bits = (totalClusters > MAX_FAT12) ? 16 : 12;
    } else {
        type = FAT32;
        bits = 32;
        sectorsPerFat = FAT_READ_LONG(buffer, FAT_SECTORS_PER_FAT)&0xffffffff;
        totalSectors = FAT_READ_LONG(buffer, FAT_TOTAL_SECTORS)&0xffffffff;
        diskLabel = string(buffer+FAT_DISK_LABEL, FAT_DISK_LABEL_SIZE);
        rootDirectory = FAT_READ_LONG(buffer, FAT_ROOT_DIRECTORY)&0xffffffff;
        fsType = string(buffer+FAT_DISK_FS, FAT_DISK_FS_SIZE);
	unsigned long dataSectors = totalSectors -
            (reservedSectors + fats*sectorsPerFat);
        totalClusters = dataSectors/sectorsPerCluster;
    }

    if (bytesPerSector != 512) {
        printf("WARNING: Bytes per sector is not 512 (%llu)\n", bytesPerSector);
        strange++;
    }

    if (sectorsPerCluster > 128) {
        printf("WARNING: Sectors per cluster high (%llu)\n", sectorsPerCluster);
        strange++;
    }

    if (fats == 0) {
        printf("WARNING: Fats numbner is 0\n");
         strange++;
    }

    if (rootDirectory != 2 && type == FAT32) {
        printf("WARNING: Root directory is not 2 (%llu)\n", rootDirectory);
        strange++;
    }
}

/**
 * Returns the 32-bit fat value for the given cluster number
 */
unsigned int FatSystem::nextCluster(unsigned int cluster, int fat)
{
    int bytes = (bits == 32 ? 4 : 2);
#ifndef __WIN__
    char buffer[bytes];
#else
    char *buffer = new char[bytes];
    __try
    {
#endif

    if (!validCluster(cluster)) {
        return 0;
    }

    if (cacheEnabled) {
        return cache[cluster];
    }

    readData(fatStart+fatSize*fat+(bits*cluster)/8, buffer, sizeof(buffer));

    unsigned int next;

    if (type == FAT32) {
        next = FAT_READ_LONG(buffer, 0)&0x0fffffff;

        if (next >= 0x0ffffff0) {
            return FAT_LAST;
        } else {
            return next;
        }
    } else {
        next = FAT_READ_SHORT(buffer,0)&0xffff;

        if (bits == 12) {
            int bit = cluster*bits;
            if (bit%8 != 0) {
                next = next >> 4;
            }
            next &= 0xfff;
            if (next >= 0xff0) {
                return FAT_LAST;
            } else {
                return next;
            }
        } else {
            if (next >= 0xfff0) {
                return FAT_LAST;
            } else {
                return next;
            }
        }
    }
#ifdef __WIN__
}
__finally
{
    delete[] buffer;
}
#endif
}

/**
 * Changes the next cluster in a file
 */
bool FatSystem::writeNextCluster(unsigned int cluster, unsigned int next, int fat)
{
    int bytes = (bits == 32 ? 4 : 2);
#ifndef __WIN__
    char buffer[bytes];
#else
        char *buffer = new char[bytes];
        __try
        {
#endif

    if (!validCluster(cluster)) {
        throw string("Trying to access a cluster outside bounds");
    }

    int offset = fatStart+fatSize*fat+(bits*cluster)/8;

    if (bits == 12) {
        readData(offset, buffer, bytes);
        int bit = cluster*bits;

        if (bit%8 != 0) {
            buffer[0] = ((next&0x0f)<<4)|(buffer[0]&0x0f);
            buffer[1] = (next>>4)&0xff;
        } else {
            buffer[0] = next&0xff;
            buffer[1] = (buffer[1]&0xf0)|((next>>8)&0x0f);
        }
    } else {
        for (int i=0; i<(bits/8); i++) {
            buffer[i] = (next>>(8*i))&0xff;
        }
    }

    return writeData(offset, buffer, bytes) == bytes;
#ifdef __WIN__
}
__finally
{
    delete[] buffer;
}
#endif
}

bool FatSystem::validCluster(unsigned int cluster)
{
    return cluster < totalClusters;
}

unsigned long long FatSystem::clusterAddress(unsigned int cluster, bool isRoot)
{
    if (type == FAT32 || !isRoot) {
        cluster -= 2;
    }

    unsigned long long addr = (dataStart + bytesPerSector*sectorsPerCluster*cluster);

    if (type == FAT16 && !isRoot) {
        addr += rootEntries * FAT_ENTRY_SIZE;
    }

    return addr;
}

vector<FatEntry> FatSystem::getEntries(unsigned int cluster, int *clusters, bool *hasFree)
{
    bool isRoot = false;
    bool contiguous = false;
    int foundEntries = 0;
    int badEntries = 0;
    bool isValid = false;
    set<unsigned int> visited;
    vector<FatEntry> entries;
    FatFilename filename;

    if (clusters != NULL) {
        *clusters = 0;
    }

    if (hasFree != NULL) {
        *hasFree = false;
    }

    if (cluster == 0 && type == FAT32) {
        cluster = rootDirectory;
    }

    isRoot = (type==FAT16 && cluster==rootDirectory);

    if (cluster == rootDirectory) {
        isValid = true;
    }

    if (!validCluster(cluster)) {
        return vector<FatEntry>();
    }

    do {
        bool localZero = false;
        int localFound = 0;
        int localBadEntries = 0;
        unsigned long long address = clusterAddress(cluster, isRoot);
        char buffer[FAT_ENTRY_SIZE];
        if (visited.find(cluster) != visited.end()) {
            cerr << "! Looping directory" << endl;
            break;
        }
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

                    localZero = false;
                } else {
                    localZero = true;
                }
            }

            address += sizeof(buffer);
        }

        int previousCluster = cluster;

        if (isRoot) {
            if (cluster+1 < rootClusters) {
                cluster++;
            } else {
                cluster = FAT_LAST;
            }
        } else {
            cluster = nextCluster(cluster);
        }

        if (clusters) {
            (*clusters)++;
        }

        if (cluster == 0 || contiguous) {
            contiguous = true;

            if (hasFree != NULL) {
                *hasFree = true;
            }
            if (!localZero && localFound && localBadEntries<localFound) {
                cluster = previousCluster+1;
            } else {
                if (!localFound && clusters) {
                    (*clusters)--;
                }
                break;
            }
        }

        if (!isValid) {
            if (badEntries>foundEntries) {
                cerr << "! Entries don't look good, this is maybe not a directory" << endl;
                return vector<FatEntry>();
          }
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
    bool hasFree = false;
    vector<FatEntry> entries = getEntries(cluster, NULL, &hasFree);
    if(this->_outputFormat == Json)
        cout << "{\"Cluster\":" << cluster << ",";
    else
        printf("Directory cluster: %u\n", cluster);
    if (hasFree && this->_outputFormat != Json) {
        printf("Warning: this directory has free clusters that was read contiguously\n");
    }
    if(this->_outputFormat == Json)
        cout << "\"Entries\":";
    list(entries);
    if(this->_outputFormat == Json)
        cout << "}";
}

void FatSystem::list(vector<FatEntry> &entries)
{
    vector<FatEntry>::iterator it;
    if(this->_outputFormat ==  Json){
        cout << "[";
        it = entries.begin();
        FatEntry &first = *it;
        for (; it!=entries.end(); it++) {
            FatEntry &entry = *it;

            if (entry.isErased() && !listDeleted) {
                continue;
            }
            if(&entry != &first) {
                cout << ",";
            }
            cout << "{\"EntryType\":";
            if (entry.isDirectory()) {
                cout << "\"Directory\",";
            } else {
                cout << "\"File\",";
            }

            string name = entry.getFilename();
            string shrtname = entry.getShortFilename();

            cout << "\"EditDate\":\"" << entry.changeDate.isoFormat() << "\",";
            cout << "\"Name\":\"" << name << "\",";
            cout << "\"8DOT3Name\":\"" << shrtname << "\",";
            cout << "\"Cluster\":" << entry.cluster << ",";

            if (!entry.isDirectory()) {
                string pretty = prettySize(entry.size);
                cout << "\"Size\":" << entry.size << ",";
                cout << "\"PrettySize\":\"" << pretty << "\",";
            }

            if (entry.isHidden()) {
                cout << "\"IsHidden\":true,";
            }
            else {
                cout << "\"IsHidden\":false,";
            }
            if (entry.isErased()) {
                cout << "\"IsDeleted\":true";
            }
            else {
                cout << "\"IsDeleted\":false";
            }
            cout << "}";            
            fflush(stdout);
        }
        cout << "]";
    }
    else {
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

            string shrtname = entry.getShortFilename();
            if (name.compare(shrtname)) {
                name += " (" + shrtname + ")";
            }

            printf(" %s ", entry.changeDate.pretty().c_str());
            printf(" %-50s", name.c_str());

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
}

void FatSystem::readFile(unsigned int cluster, unsigned int size, FILE *f, bool deleted)
{
    bool contiguous = deleted;

    if (f == NULL) {
        f = stdout;
    }

    while ((size!=0) && cluster!=FAT_LAST) {
        int currentCluster = cluster;
        int toRead = size;
        if (toRead > bytesPerCluster || size < 0) {
            toRead = bytesPerCluster;
        }
#ifndef __WIN__
        char buffer[bytesPerCluster];
#else
                char *buffer = new char[bytesPerCluster];
                __try
                {
#endif
        readData(clusterAddress(cluster), buffer, toRead);

        if (size != -1) {
            size -= toRead;
        }

        // Write file data to the given file
        fwrite(buffer, toRead, 1, f);
        fflush(f);

        if (contiguous) {
            if (deleted) {
                do {
                    cluster++;
                } while (!freeCluster(cluster));
            } else {
                if (!freeCluster(cluster)) {
                    fprintf(stderr, "! Contiguous file contains cluster that seems allocated\n");
                    fprintf(stderr, "! Trying to disable contiguous mode\n");
                    contiguous = false;
                    cluster = nextCluster(cluster);
                } else {
                    cluster++;
                }
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
#ifdef __WIN__
    }
    __finally
    {
        delete[] buffer;
    }
#endif
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
    // totalClusters = (fatSize*8)/bits;
    dataSize = totalClusters*bytesPerCluster;

    if (type == FAT16) {
        int rootBytes = rootEntries*32;
        rootClusters = rootBytes/bytesPerCluster + ((rootBytes%bytesPerCluster) ? 1 : 0);
    }

    return strange == 0;
}

void FatSystem::infos()
{
    switch (this->_outputFormat) {
        case Json: {
            cout << "{\"FileSystemType\":\"" << trim(fsType) << "\",";
            cout << "\"OemName\":\"" << oemName  << "\",";
            cout << "\"TotalSectors\":" << totalSectors << ",";
            cout << "\"TotalDataClusters\":" << totalClusters << ",";
            cout << "\"DataSize\":" << dataSize << ",";
            cout << "\"PrettyDataSize\":\"" << prettySize(dataSize) << "\",";
            cout << "\"DiskSize\":" << totalSize << ",";
            cout << "\"PrettyDiskSize\":\"" << prettySize(totalSize) << "\",";
            cout << "\"BytesPerSector\":" << bytesPerSector << ",";
            cout << "\"SectorsPerCluster\":" << sectorsPerCluster << ",";
            cout << "\"BytesPerCluster\":" << bytesPerCluster << ",";
            cout << "\"ReservedSectors\":" << reservedSectors << ",";
            if (type == FAT16) {
                cout << "\"RootEntries\":" << rootEntries << ",";
                cout << "\"RootClusters\":" << rootClusters << ",";
            }
        cout << "\"SectorsPerFat\":" << sectorsPerFat << ",";
            cout << "\"FatSize\":" << fatSize << ",";
            cout << "\"PrettyFatSize\":\"" << prettySize(fatSize) << "\",";
            cout << "\"Fat1StartAddress\":" << fatStart << ",";
            cout << "\"Fat2StartAddress\":" << fatStart+fatSize << ",";
            cout << "\"DataStartAddress\":" << dataStart << ",";
            cout << "\"RootDirectoryCluster\":" << rootDirectory << ",";
            cout << "\"DiskLabel\":\"" << trim(diskLabel) << "\",";

            computeStats();
            cout << "\"FreeClusters\":" << freeClusters << ",\"TotalClusters\":" << totalClusters << ",";
            cout << "\"FreeClustersPercent\":" << (100*freeClusters/(double)totalClusters) << ",";
            cout << "\"FreeSpace\":" << (freeClusters*bytesPerCluster) << ",";
            cout << "\"PrettyFreeSpace\":\"" << prettySize(freeClusters*bytesPerCluster) << "\",";
            cout << "\"UsedSpace\":" << ((totalClusters-freeClusters)*bytesPerCluster) << ",";
            cout << "\"PrettyUsedSpace\":\"" << prettySize((totalClusters-freeClusters)*bytesPerCluster) << "\"}";
            cout << endl;
            break;
        }
        default: {
            cout << "FAT Filesystem information" << endl << endl;

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
            if (type == FAT16) {
                cout << "Root entries: " << rootEntries << endl;
                cout << "Root clusters: " << rootClusters << endl;
            }
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
            break;
        }
    }
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
            fprintf(stderr, "! Trying to read a deleted file, enabling deleted mode\n");
            contiguous = true;
        }
        readFile(entry.cluster, entry.size, f, contiguous);
    }
}

void FatSystem::setListDeleted(bool listDeleted_)
{
    listDeleted = listDeleted_;
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
    return nextCluster(cluster) == 0;
}

void FatSystem::computeStats()
{
    if (statsComputed) {
        return;
    }

    statsComputed = true;

    freeClusters = 0;
    for (unsigned int cluster=0; cluster<totalClusters; cluster++) {
        if (freeCluster(cluster + 2)) {
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
#ifndef __WIN__
            char buffer[bytesPerCluster];
#else
            char *buffer = new char[bytesPerCluster];
            __try
            {
#endif
            for (int i=0; i<sizeof(buffer); i++) {
                if (random) {
                    buffer[i] = rand()&0xff;
                } else {
                    buffer[i] = 0x0;
                }
            }
            writeData(clusterAddress(cluster), buffer, sizeof(buffer));
            total++;
#ifdef __WIN__
            }
            __finally
            {
                delete[] buffer;
            }
#endif
    }
}

cout << "Scrambled " << total << " sectors" << endl;
}
