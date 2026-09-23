#ifndef HOMM3_COMPILER_H
#define HOMM3_COMPILER_H

// Compiler spelling only. Game declarations and helper bodies remain in
// their ordinary headers; each compiler supplies its own standard library.
#if defined(__MWERKS__) && defined(__POWERPC__)
#define __int64 long long
#define __cdecl
#define __stdcall
#define __fastcall
#define __forceinline inline
#endif

#endif
