/* direct.h - the Microsoft CRT directory calls under their Linux names
 * (work/loki-game only). */
#ifndef HOMM3_LOKI_DIRECT_H
#define HOMM3_LOKI_DIRECT_H

#include <unistd.h>
#include <sys/stat.h>

#define _getcwd getcwd
#define _chdir chdir
#define _mkdir(path) mkdir((path), 0777)

#endif
