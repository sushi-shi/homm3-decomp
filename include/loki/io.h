/* io.h - the Microsoft CRT low-level file calls under their Linux names
 * (work/loki-game only). */
#ifndef HOMM3_LOKI_IO_H
#define HOMM3_LOKI_IO_H

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define _open open
#define _close close
#define _read read
#define _write write
#define _lseek lseek
#define _access access
#define _unlink unlink
#define _O_BINARY 0
#define _O_RDONLY O_RDONLY
#define _O_WRONLY O_WRONLY
#define _O_RDWR O_RDWR
#define _O_CREAT O_CREAT
#define _O_TRUNC O_TRUNC
#define _S_IREAD S_IRUSR
#define _S_IWRITE S_IWUSR

struct _finddata_t {
    unsigned attrib;
    long time_create;
    long time_access;
    long time_write;
    unsigned long size;
    char name[260];
};
long _findfirst(const char*, struct _finddata_t*);
int _findnext(long, struct _finddata_t*);
int _findclose(long);

#endif
