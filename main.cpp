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
    cout << "-c [cluster]: list files and directories in the given cluster" << endl;
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    vector<string> arguments;
    char *image = NULL;
    int index;

    // -i, display informations about the disk
    bool infoFlag = false;

    // -l, list directories in the given path
    bool listFlag = false;
    string listPath;

    // -c, listing for a direct cluster
    bool listClusterFlag = false;
    int listCluster;

    // Parsing command line
    while ((index = getopt(argc, argv, "il:c:")) != -1) {
        switch (index) {
            case 'i':
                infoFlag = true;
                break;
            case 'l':
                listFlag = true;
                listPath = string(optarg);
                break;
            case 'c':
                listClusterFlag = true;
                listCluster = atoi(optarg);
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
    if (!(infoFlag || listFlag || listClusterFlag)) {
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
        }
    } else {
        cout << "! Failed to init the FAT filesystem" << endl;
    }

    exit(EXIT_SUCCESS);
}
