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
#include <extras.h>
// VC6's <math.h> declares only the C double forms; MSL's C++ float/integral
// overloads (ansi_prefix.mac.h) make mixed float/double calls ambiguous.
#undef __ANSI_OVERLOAD__
#undef _MSL_INTEGRAL_MATH
// VC6's standard headers reach <cstdio>/<cstring> transitively (<string>
// through <xlocale>); MSL's do not, so the prefix supplies them.
#include <stdio.h>
#include <string.h>
// VC6 keeps a for-init declaration in the enclosing scope (pre-ISO rule).
#pragma ARM_scoping on
// Native MSL provides the equivalent CRT entries under these spellings.
#define _strcmpi _stricmp
#define stricmp _stricmp
#define strnicmp _strnicmp
// The file API keeps Microsoft's names in shared source; MSL supplies the
// corresponding open flags and owner-write permission under POSIX spellings.
#define _O_BINARY O_BINARY
#define _O_CREAT O_CREAT
#define _O_TRUNC O_TRUNC
#define _O_WRONLY O_WRONLY
#define _S_IWRITE S_IWUSR
#endif

#endif
