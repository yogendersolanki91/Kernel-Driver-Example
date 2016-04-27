#ifndef __UNIX_H__
#define __UNIX_H__

#include <windows.h>
#include <process.h>
#include <io.h>
#include <fcntl.h>
#include "getopt.h"

#define getpid _getpid
#define fsync _commit
#define open _open
#define write _write
#define lseek _lseek
#define read _read
#define unlink _unlink

#define srandom srand
#define random rand

#ifdef __cplusplus
extern "C" {
#endif

int
unix_init();

int
gettimeofday(
    struct timeval* tp,
    ...
    );

#ifdef __cplusplus
}
#endif

#endif /* __UNIX_H__ */
