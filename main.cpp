#include <stdlib.h>
#include <string>
#include <iostream>
#include "FatSystem.h"
#include "FatPath.h"

using namespace std;

void usage()
{
    cout << "fatcat v1.0, Gregwar <g.passault@gmail.com>" << endl;
    cout << endl;
    cout << "Usage: fatcat [-i] disk.img" << endl;
    cout << "-i: display information about disk" << endl;
    cout << "-l [dir]: list files and directories in the given path" << endl;
    cout << "-L [cluster]: list files and directories in the given cluster" << endl;
    cout << "-r [path]: reads the file given by the path" << endl;
    cout << "-R [cluster]: reads the data from given cluster" << endl;
    cout << "-s [size]: specify the size of data to read from the cluster" << endl;
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
    int cluster;

    // Parsing command line
    while ((index = getopt(argc, argv, "il:L:r:R:s:")) != -1) {
        switch (index) {
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
    if (!(infoFlag || listFlag || listClusterFlag || readFlag || clusterRead)) {
        usage();
    }

    // Openning the image
    FatSystem fat(image);
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
        }
    } else {
        cout << "! Failed to init the FAT filesystem" << endl;
    }

    exit(EXIT_SUCCESS);
}
