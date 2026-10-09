/* gcc_prefix.h - the GCC 2.95.2 (Loki Linux, i386 ELF) prefix file.
 *
 * Loki evidence builds pass this file with `-include` (work/loki-game). Like
 * codewarrior_prefix.h it only bridges compiler and library spellings: game
 * declarations, bodies and port code never belong here. VC6 never sees it.
 */
#ifndef HOMM3_GCC_PREFIX_H
#define HOMM3_GCC_PREFIX_H

#if defined(__GNUC__) && defined(HOMM3_TARGET_LOKI)

/* Microsoft keywords GCC 2.95 does not spell. */
#define __int64 long long
#define __cdecl
#define _cdecl
#define __stdcall
#define _stdcall
#define __fastcall
#define _fastcall
#define __forceinline inline
#define __inline inline
#define __declspec(x)
#define __unaligned

/* Microsoft C library spellings. */
#define _MAX_PATH 260
#define strcmpi strcasecmp
#define stricmp strcasecmp
#define _stricmp strcasecmp
#define strnicmp strncasecmp
#define _strnicmp strncasecmp
#if defined(__cplusplus)
extern "C" {
#endif
char* _fullpath(char* absolute, const char* relative, unsigned int length);
char* strupr(char* text);
char* strrev(char* text);
char* itoa(int value, char* text, int radix);
#if defined(__cplusplus)
}
#endif

/* The game is built without exceptions (no project .eh_frame), and g++
 * rejects try and throw under -fno-exceptions: guarded blocks run unguarded,
 * handlers are dead and throw expressions are discarded. The library's
 * exception specifications are read first; glibc's empty ones are dropped. */
#if defined(__cplusplus)
#include <sys/cdefs.h>
#include <new>
#undef __THROW
#define __THROW
#define try if (true)
#define catch(declaration) else if (false)
#define throw
#endif

#endif
#endif
