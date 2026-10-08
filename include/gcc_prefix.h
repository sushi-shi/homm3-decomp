/* gcc_prefix.h - the GCC 2.95.2 (Loki Linux, i386 ELF) prefix file.
 *
 * homm3.loki.cc passes this file with `-include`, so it opens every unit the
 * Loki h3maped image compiles from the shared game source. Like
 * codewarrior_prefix.h it only bridges compiler and library spellings: game
 * declarations, bodies and editor code never belong here. VC6 never sees it.
 */
#ifndef HOMM3_GCC_PREFIX_H
#define HOMM3_GCC_PREFIX_H

#if defined(__GNUC__) && defined(HOMM3_TARGET_LOKI)

/* 1. Microsoft keywords GCC 2.95 does not spell. */
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

#endif
#endif
