#include <stdlib.h>
#include <argp.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include "FatSystem.h"
#include "FatPath.h"
#include "FatChains.h"
#include "FatBackup.h"
#include "FatDiff.h"
#include "utils.h"

using namespace std;

void usage()
{
    cout << "fatcat v1.0, Gregwar <g.passault@gmail.com>" << endl;
    cout << endl;
    cout << "Usage: fatcat disk.img [options]" << endl;
    cout << "  -i: display information about disk" << endl;
    cout << endl;
    cout << "Browsing & extracting:" << endl;
    cout << "  -l [dir]: list files and directories in the given path" << endl;
    cout << "  -L [cluster]: list files and directories in the given cluster" << endl;
    cout << "  -r [path]: reads the file given by the path" << endl;
    cout << "  -R [cluster]: reads the data from given cluster" << endl;
    cout << "  -s [size]: specify the size of data to read from the cluster" << endl;
    cout << "  -d: enable listing of deleted files" << endl;
    cout << "  -x [directory]: extract all files to a directory, deleted files included if -d" << endl;
    cout << "                  will start with rootDirectory, unless -f is provided" << endl;
    cout << "  -f [cluster]: define the cluster starting point for the extraction" << endl;
    cout << "* -S: write scamble data in unallocated sectors" << endl;
    cout << "* -z: write scamble data in unallocated sectors" << endl;
    cout << endl;
    cout << "FAT Hacking" << endl;
    cout << "  -@ [cluster]: Get the cluster address and informations" << endl;
    cout << "  -2: analysis & compare the 2 FATs" << endl;
    cout << "  -b [file]: backup the FATs" << endl;
    cout << "* -p [file]: restore (patch) the FATs" << endl;
    cout << "* -w [cluster] -v [value]: write next cluster (see -T)" << endl;
    cout << "  -T [table]: specify which table to write (0:both, 1:first, 2:second)" << endl;
    cout << "* -m: merge the FATs" << endl;
    cout << "  -o: search for orphan files and directories" << endl;
    cout << endl;
    cout << "Entries hacking" << endl;
    cout << "  -e [path]: sets the entry to hack, combined with:" << endl;
    cout << "* -c [cluster]: sets the entry cluster" << endl;
    cout << "* -s [cluster]: sets the entry size" << endl;

    cout << endl;
    cout << "*: These flags writes on the disk, and may damage it, be careful" << endl;
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    vector<string> arguments;
    char *image = NULL;
    int index;

    // -s, specify the size to be read
    int size = -1;

    // -i, display informations about the disk
    bool infoFlag = false;

    // -l, list directories in the given path
    bool listFlag = false;
    string listPath;

    // -c, listing for a direct cluster
    bool listClusterFlag = false;
    int listCluster;

    // -r, reads a file
    bool readFlag = false;
    string readPath;

    // -t, reads from cluster file
    bool clusterRead = false;
    int cluster = 0;

    // -d, lists deleted
    bool listDeleted = false;

    // -x: extract
    bool extract = false;
    string extractDirectory;

    // -2: compare two fats
    bool compare = false;

    // -@: get the cluster address
    bool address = false;

    // -k: analysis the chains
    bool chains = false;

    // -b: backup the fats
    bool backup = false;
    bool patch = false;
    string backupFile;

    // -w: write next cluster
    bool writeNext = false;

    // -m: merge the FATs
    bool merge = false;

    // -v: value
    bool hasValue = false;
    int value;

    // -T: FAT table to write
    int table = 0;

    // -S: write random data in unallocated sectors
    bool scramble = false;
    bool zero = false;

    // -e: entry hacking
    bool entry = false;
    string entryPath;
    bool clusterProvided = false;
    bool sizeProvided = false;

    // Parsing command line
    while ((index = getopt(argc, argv, "il:L:r:R:s:dc:hx:2@:ob:p:w:v:mT:f:Sze:")) != -1) {
        switch (index) {
            case 'e':
                entry = true;
                entryPath = string(optarg);
                break;
            case 'z':
                zero = true;
                break;
            case 'S':
                scramble = true;
                break;
            case 'T':
                table = atoi(optarg);
                break;
            case 'm':
                merge = true;
                break;
            case 'v':
                hasValue = true;
                value = atoi(optarg);
                break;
            case 'f':
                cluster = atoi(optarg);
                break;
            case 'w':
                writeNext = true;
                cluster = atoi(optarg);
                break;
            case '@':
                address = true;
                cluster = atoi(optarg);
                break;
            case 'o':
                chains = true;
                break;
            case 'i':
                infoFlag = true;
                break;
            case 'l':
                listFlag = true;
                listPath = string(optarg);
                break;
            case 'L':
                listClusterFlag = true;
                listCluster = atoi(optarg);
                break;
            case 'r':
                readFlag = true;
                readPath = string(optarg);
                break;
            case 'R':
                clusterRead = true;
                cluster = atoi(optarg);
                break;
            case 's':
                size = atoi(optarg);
                sizeProvided = true;
                break;
            case 'd':
                listDeleted = true;
                break;
            case 'c':
                cluster = atoi(optarg);
                clusterProvided = true;
                break;
            case 'x':
                extract = true;
                extractDirectory = string(optarg);
                break;
            case '2':
                compare = true;
                break;
            case 'b':
                backup = true;
                backupFile = string(optarg);
                break;
            case 'p':
                patch = true;
                backupFile = string(optarg);
                break;
            case 'h':
                usage();
                break;
        }
    }

    // Trying to get the FAT file or device
    if (optind != argc) {
        image = argv[optind];
    }

    if (!image) {
        usage();
    }

    // Getting extra arguments
    for (index=optind+1; index<argc; index++) {
        arguments.push_back(argv[index]);
    }

    // If the user did not required any actions
    if (!(infoFlag || listFlag || listClusterFlag || 
        readFlag || clusterRead || extract || compare || address ||
        chains || backup || patch || writeNext || merge ||
        scramble || zero || entry)) {
        usage();
    }

    try {
        // Openning the image
        FatSystem fat(image);

        fat.setListDeleted(listDeleted);

        if (fat.init()) {
            if (infoFlag) {
                fat.infos();
            } else if (listFlag) {
                cout << "Listing path " << listPath << endl;
                FatPath path(listPath);
                fat.list(path);
            } else if (listClusterFlag) {
                cout << "Listing cluster " << listCluster << endl;
                fat.list(listCluster);
            } else if (readFlag) {
                FatPath path(readPath);
                fat.readFile(path);
            } else if (clusterRead) {
                fat.readFile(cluster, size);
            } else if (extract) {
                fat.extract(cluster, extractDirectory);
            } else if (compare) {
                FatDiff diff(fat);
                diff.compare();
            } else if (address) {
                cout << "Cluster " << cluster << " address:" << endl;
                long long addr = fat.clusterAddress(cluster);
                int next1 = fat.nextCluster(cluster, 0);
                int next2 = fat.nextCluster(cluster, 1);
                printf("%llu (%08x)\n", addr, addr);
                cout << "Next cluster:" << endl;
                printf("FAT1: %u (%08x)\n", next1, next1);
                printf("FAT2: %u (%08x)\n", next2, next2);
            } else if (chains) {
                FatChains chains(fat);
                chains.chainsAnalysis();
            } else if (backup || patch) {
                FatBackup backupSystem(fat);
                
                if (backup) {
                    backupSystem.backup(backupFile);
                } else {
                    backupSystem.patch(backupFile);
                }
            } else if (writeNext) {
                if (!hasValue) {
                    throw string("You should provide a value with -v");
                }

                int prev = fat.nextCluster(cluster);
                printf("Writing next cluster of %u from %u to %u\n", cluster, prev, value);
                fat.enableWrite();

                if (table == 0 || table == 1) {
                    printf("Writing on FAT1\n");
                    fat.writeNextCluster(cluster, value, 0);
                } 
                if (table == 0 || table == 2) {
                    printf("Writing on FAT2\n");
                    fat.writeNextCluster(cluster, value, 1);
                } 
            } else if (merge) {
                FatDiff diff(fat);
                diff.merge();
            } else if (scramble) {
                fat.enableWrite();
                fat.rewriteUnallocated(1);
            } else if (zero) {
                fat.enableWrite();
                fat.rewriteUnallocated();
            } else if (entry) {
                cout << "Searching entry for " << entryPath << endl;
                FatPath path(entryPath);
                FatEntry entry;
                if (fat.findFile(path, entry)) {
                    printf("Entry address %016x\n", entry.address);
                    cout << "Found entry, cluster=" << entry.cluster;
                    if (entry.isDirectory()) {
                        cout << ", directory";
                    } else {
                        cout << ", file with size=" << entry.size << " (" << prettySize(entry.size) << ")";
                    }
                    cout << endl;

                    if (clusterProvided) {
                        cout << "Setting the cluster to " << cluster << endl;
                        entry.cluster = cluster;
                    }
                    if (sizeProvided) {
                        cout << "Setting the size to " << size << endl;
                        entry.size = size;
                    }
                    if (clusterProvided || sizeProvided) {
                        entry.updateData();
                        fat.enableWrite();
                        string data = entry.data;
                        fat.writeData(entry.address, data.c_str(), entry.data.size());
                    }
                } else {
                    cout << "Entry not found." << endl;
                }
            }
        } else {
            cout << "! Failed to init the FAT filesystem" << endl;
        }
    } catch (string error) {
        cerr << "Error: " << error << endl;
    }

    exit(EXIT_SUCCESS);
}
