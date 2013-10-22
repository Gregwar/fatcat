#ifndef _FATSCAN_UTILS_H
#define _FATSCAN_UTILS_H

// Utils
#define FAT_READ_SHORT(buffer,x) ((buffer[x]&0xff)|((buffer[x+1]&0xff)<<8))
#define FAT_READ_LONG(buffer,x) \
        ((buffer[x]&0xff)|((buffer[x+1]&0xff)<<8))| \
        (((buffer[x+2]&0xff)<<16)|((buffer[x+3]&0xff)<<24))

#endif // _FATSCAN_UTILS_H
