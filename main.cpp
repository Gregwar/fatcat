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
    cout << "      fatcat [-i] disk.img" << endl;
    cout << "-i: display information about disk" << endl;
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    vector<string> arguments;
    char *image = NULL;
    int index;
    bool infoFlag = false;

    while ((index = getopt(argc, argv, "i")) != -1) {
        switch (index) {
            case 'i':
                infoFlag = true;
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
    if (!infoFlag) {
        usage();
    }

    // Openning the image
    FatSystem fat(image);
    if (fat.init()) {
        if (infoFlag) {
            fat.infos();
        }
    } else {
        cout << "! Failed to init the FAT filesystem" << endl;
    }
    
    //FatPath path(argv[2]);
    //fat.list(path);

    exit(EXIT_SUCCESS);
}
