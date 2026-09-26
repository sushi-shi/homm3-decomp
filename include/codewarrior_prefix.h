/* codewarrior_prefix.h - the CodeWarrior (Classic Mac PowerPC) prefix file.
 *
 * homm3.mac.cc_wrap passes this file to MWCPPC with `-prefix`, so it opens
 * every Mac translation unit. It exists so that CodeWarrior Pro 6 with MSL
 * compiles the same VC6-era source unchanged. Every entry is a compiler or
 * library difference, grouped below; game declarations, bodies and Windows
 * API declarations never belong here.
 *
 * For VC6 the file is empty. platform.h still includes it: dropping that
 * include changes VC6's include set, which renumbers its inline-assembly
 * labels and reorders extern symbols in ten objects.
 */
#ifndef HOMM3_CODEWARRIOR_PREFIX_H
#define HOMM3_CODEWARRIOR_PREFIX_H

#if defined(__MWERKS__)

/* 1. Microsoft keywords CodeWarrior does not spell. */
#define __int64 long long
#define __cdecl
#define __stdcall
#define __fastcall
#define __forceinline inline

/* 2. VC6 language rules CodeWarrior implements differently. */
// A for-init declaration stays in the enclosing scope (pre-ISO rule).
#pragma ARM_scoping on
// String literals are char arrays, so a conditional of literals is char*.
#pragma const_strings off

/* 3. VC6 standard-library behaviour MSL differs in. */
#include <extras.h>
// VC6 <math.h> has only the double forms; MSL's overloads make fmod(float, double) ambiguous.
#undef __ANSI_OVERLOAD__
#undef _MSL_INTEGRAL_MATH
// VC6's standard headers include these transitively; MSL's do not.
#include <stdio.h>
#include <string.h>
#if defined(__cplusplus)
#include <string>
#endif

/* 4. Microsoft CRT spellings with MSL equivalents. */
#define _strcmpi _stricmp
#define stricmp _stricmp
#define strnicmp _strnicmp
// The file API keeps Microsoft's names in shared source; MSL supplies the
// open flags and owner-write permission under their POSIX spellings.
#define _O_BINARY O_BINARY
#define _O_CREAT O_CREAT
#define _O_TRUNC O_TRUNC
#define _O_WRONLY O_WRONLY
#define _S_IWRITE S_IWUSR
// MSL declares the <io.h>/<direct.h> CRT subset; the SDK copies #error off Win32.
#include <fcntl.h>
#include <unistd.h>
#define _INC_IO
#define _INC_DIRECT

#endif /* __MWERKS__ */

#endif
