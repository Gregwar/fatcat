#include <stdlib.h>
#include <stdio.h>
#include "fat.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: ./fatscan <file>\n");
        exit(EXIT_FAILURE);
    }

    struct fatfile *fat = fat_open(argv[1]);
    fat_scan(fat);
    fat_destroy(fat);

    exit(EXIT_SUCCESS);
}
