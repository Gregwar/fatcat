#include <stdlib.h>
#include <string>
#include <iostream>
#include "FatSystem.h"

using namespace std;

int main(int argc, char *argv[])
{
    if (argc != 2) {
        cout << "Usage: ./fatscan <file>" << endl;
        exit(EXIT_FAILURE);
    }

    FatSystem fat(argv[1]);
    fat.run();

    exit(EXIT_SUCCESS);
}
