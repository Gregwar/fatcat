#include <stdlib.h>
#include <string>
#include <iostream>
#include "FatSystem.h"
#include "FatPath.h"

using namespace std;

void usage()
{
    cout << "FatScan V1" << endl;
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{


    FatSystem fat(argv[1]);
    FatPath path(argv[2]);
    fat.run();
    fat.list(path);

    exit(EXIT_SUCCESS);
}
