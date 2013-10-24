#ifndef _FATCAT_FATENTRY_H
#define _FATCAT_FATENTRY_H

#include <string>
#include "FatDate.h"

using namespace std;

// Size of a FAT entry
#define FAT_ENTRY_SIZE         0x20

// Offsets
#define FAT_SHORTNAME           0x00
#define FAT_ATTRIBUTES          0x0b
#define FAT_CLUSTER_LOW         0x1a
#define FAT_CLUSTER_HIGH        0x14
#define FAT_FILESIZE            0x1c

// Attributes
#define FAT_ATTRIBUTES_HIDE     (1<<1)
#define FAT_ATTRIBUTES_DIR      (1<<4)
#define FAT_ATTRIBUTES_LONGFILE (0xf)
#define FAT_ATTRIBUTES_FILE     (0x20)

// Prefix used for erased files
#define FAT_ERASED                  0xe5

class FatEntry
{
    public:
        FatEntry();

        string getFilename();
        bool isDirectory();
        bool isHidden();
        bool isErased();

        string shortName;
        string longName;
        char attributes;
        int cluster;
        long long size;

        FatDate creationDate;
        FatDate changeDate;

        void updateData();
        void setData(string data);
        long long address;
        bool hasData;
        string data;
};

#endif // _FATCAT_FATENTRY_H
