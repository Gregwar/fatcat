#ifndef _UTIMES_H
#define _UTIMES_H

#include <direct.h>
#include <errno.h>
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/*errno==EACCES on read-only devices */
int utimes(const char *filename, const struct timeval times[2]);

#ifdef __cplusplus
}
#endif

#endif
