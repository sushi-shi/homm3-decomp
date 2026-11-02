// Reconstructed Victor lock cleanups. The original library object name is
// unknown. Retail places the eight bodies together after the PCX kernels and
// before the bit helpers, LINK's pull order of separate library members, so
// they are their own member; the locks and the cleanup table stay with the
// allocation code in victor.cpp.
#include "va.h"

#include <stdlib.h>
#include <string.h>

#include "victor.h"

// The eight lock cleanups, one per VictorLock in address order; each
// releases g_victorLock<n> (g_victorModuleCleanups[8-n]): test the
// initialized flag at +0x18, DeleteCriticalSection, clear the flag. Both
// editors keep the same order; their cleanup tables list them from lock 7
// down to lock 0, as the game's does.
VA(0x00604620, 0x1f)  // g_victorModuleCleanups[8]; external Victor library
VA_AT(h3maped, 0x004fce60, 0x1f)
VA_AT(h3ccmped, 0x00463bc0, 0x1f)
void __cdecl victorDestroyLock0()
{
    if (g_victorLock0.m_initialized) {
        DeleteCriticalSection(&g_victorLock0.m_section);
        g_victorLock0.m_initialized = 0;
    }
}

VA(0x00604640, 0x1f)  // g_victorModuleCleanups[7]; external Victor library
VA_AT(h3maped, 0x004fce80, 0x1f)
VA_AT(h3ccmped, 0x00463be0, 0x1f)
void __cdecl victorDestroyLock1()
{
    if (g_victorLock1.m_initialized) {
        DeleteCriticalSection(&g_victorLock1.m_section);
        g_victorLock1.m_initialized = 0;
    }
}

VA(0x00604660, 0x1f)  // g_victorModuleCleanups[6]; external Victor library
VA_AT(h3maped, 0x004fcea0, 0x1f)
VA_AT(h3ccmped, 0x00463c00, 0x1f)
void __cdecl victorDestroyLock2()
{
    if (g_victorLock2.m_initialized) {
        DeleteCriticalSection(&g_victorLock2.m_section);
        g_victorLock2.m_initialized = 0;
    }
}

VA(0x00604680, 0x1f)  // g_victorModuleCleanups[5]; external Victor library
VA_AT(h3maped, 0x004fcec0, 0x1f)
VA_AT(h3ccmped, 0x00463c20, 0x1f)
void __cdecl victorDestroyLock3()
{
    if (g_victorLock3.m_initialized) {
        DeleteCriticalSection(&g_victorLock3.m_section);
        g_victorLock3.m_initialized = 0;
    }
}

VA(0x006046a0, 0x1f)  // g_victorModuleCleanups[4]; external Victor library
VA_AT(h3maped, 0x004fcee0, 0x1f)
VA_AT(h3ccmped, 0x00463c40, 0x1f)
void __cdecl victorDestroyLock4()
{
    if (g_victorLock4.m_initialized) {
        DeleteCriticalSection(&g_victorLock4.m_section);
        g_victorLock4.m_initialized = 0;
    }
}

VA(0x006046c0, 0x1f)  // g_victorModuleCleanups[3]; external Victor library
VA_AT(h3maped, 0x004fcf00, 0x1f)
VA_AT(h3ccmped, 0x00463c60, 0x1f)
void __cdecl victorDestroyLock5()
{
    if (g_victorLock5.m_initialized) {
        DeleteCriticalSection(&g_victorLock5.m_section);
        g_victorLock5.m_initialized = 0;
    }
}

VA(0x006046e0, 0x1f)  // g_victorModuleCleanups[2]; external Victor library
VA_AT(h3maped, 0x004fcf20, 0x1f)
VA_AT(h3ccmped, 0x00463c80, 0x1f)
void __cdecl victorDestroyLock6()
{
    if (g_victorLock6.m_initialized) {
        DeleteCriticalSection(&g_victorLock6.m_section);
        g_victorLock6.m_initialized = 0;
    }
}

VA(0x00604700, 0x1f)  // g_victorModuleCleanups[1]; external Victor library
VA_AT(h3maped, 0x004fcf40, 0x1f)
VA_AT(h3ccmped, 0x00463ca0, 0x1f)
void __cdecl victorDestroyLock7()
{
    if (g_victorLock7.m_initialized) {
        DeleteCriticalSection(&g_victorLock7.m_section);
        g_victorLock7.m_initialized = 0;
    }
}
