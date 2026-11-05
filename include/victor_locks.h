#ifndef HOMM3_VICTOR_LOCKS_H
#define HOMM3_VICTOR_LOCKS_H

#include "victor.h"

// The eight Victor module locks, in cleanup order. Defined in src/victor.cpp
// and released by the cleanups in src/victor_locks.cpp. The bit helpers and
// other Victor members do not use them.
extern VictorLock g_victorLock0;
extern VictorLock g_victorLock1;
extern VictorLock g_victorLock2;
extern VictorLock g_victorLock3;
extern VictorLock g_victorLock4;
extern VictorLock g_victorLock5;
extern VictorLock g_victorLock6;
extern VictorLock g_victorLock7;

#endif /* HOMM3_VICTOR_LOCKS_H */
