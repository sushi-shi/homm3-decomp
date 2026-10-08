// Reconstructed Victor lock cleanups. The original library object name is
// unknown. Retail places the eight bodies together after the PCX kernels and
// before the bit helpers, LINK's pull order of separate library members, so
// they are their own member; the locks and the cleanup table stay with the
// allocation code in victor.cpp.
#include "va.h"

#include <stdlib.h>
#include <string.h>

#include "victor.h"

// The eight lock cleanups, one per VictorLock in address order: 0x604620 +
// 0x20*n releases g_victorLock<n> (g_victorModuleCleanups[8-n]). Each is 31
// bytes: test the initialized flag at +0x18, DeleteCriticalSection through
// the IAT, clear the flag.
VA(0x00604620, 0x1f)  // g_victorModuleCleanups[8]; external Victor library
void __cdecl victorDestroyLock0()
{
    if (g_victorLock0.m_initialized) {
        DeleteCriticalSection(&g_victorLock0.m_section);
        g_victorLock0.m_initialized = 0;
    }
}

VA(0x00604640, 0x1f)  // g_victorModuleCleanups[7]; external Victor library
void __cdecl victorDestroyLock1()
{
    if (g_victorLock1.m_initialized) {
        DeleteCriticalSection(&g_victorLock1.m_section);
        g_victorLock1.m_initialized = 0;
    }
}

VA(0x00604660, 0x1f)  // g_victorModuleCleanups[6]; external Victor library
void __cdecl victorDestroyLock2()
{
    if (g_victorLock2.m_initialized) {
        DeleteCriticalSection(&g_victorLock2.m_section);
        g_victorLock2.m_initialized = 0;
    }
}

VA(0x00604680, 0x1f)  // g_victorModuleCleanups[5]; external Victor library
void __cdecl victorDestroyLock3()
{
    if (g_victorLock3.m_initialized) {
        DeleteCriticalSection(&g_victorLock3.m_section);
        g_victorLock3.m_initialized = 0;
    }
}

VA(0x006046a0, 0x1f)  // g_victorModuleCleanups[4]; external Victor library
void __cdecl victorDestroyLock4()
{
    if (g_victorLock4.m_initialized) {
        DeleteCriticalSection(&g_victorLock4.m_section);
        g_victorLock4.m_initialized = 0;
    }
}

VA(0x006046c0, 0x1f)  // g_victorModuleCleanups[3]; external Victor library
void __cdecl victorDestroyLock5()
{
    if (g_victorLock5.m_initialized) {
        DeleteCriticalSection(&g_victorLock5.m_section);
        g_victorLock5.m_initialized = 0;
    }
}

VA(0x006046e0, 0x1f)  // g_victorModuleCleanups[2]; external Victor library
void __cdecl victorDestroyLock6()
{
    if (g_victorLock6.m_initialized) {
        DeleteCriticalSection(&g_victorLock6.m_section);
        g_victorLock6.m_initialized = 0;
    }
}

VA(0x00604700, 0x1f)  // g_victorModuleCleanups[1]; external Victor library
void __cdecl victorDestroyLock7()
{
    if (g_victorLock7.m_initialized) {
        DeleteCriticalSection(&g_victorLock7.m_section);
        g_victorLock7.m_initialized = 0;
    }
}
