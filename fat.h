#ifndef _FATSCAN_FAT_H
#define _FATSCAN_FAT_H

struct fatfile;

struct fatfile *fat_open(const char *filename);
void fat_destroy(struct fatfile *fat);
void fat_scan(struct fatfile *fat);

#endif // _FATSCAN_H
