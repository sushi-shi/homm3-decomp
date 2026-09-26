#ifndef HOMM3_PLATFORM_H
#define HOMM3_PLATFORM_H

#include "codewarrior_prefix.h"

#if defined(HOMM3_TARGET_MAC)
// The Windows reconstruction still mentions Windows SDK types. Import their
// real declarations for compilation; this does not supply a Mac OS backend or
// establish that a Windows platform class has the Mac retail layout.
// The native MSL headers remain first on the standard-library include path.
#include <stddef.h>
#include <stdlib.h>
#pragma push
#pragma bool off
// This SDK predates bool and declares a VARIANT member literally named bool.
// Scope its compiler dialect and platform selection to the SDK import.
#define _WIN32 1
#define _M_PPC 1
#define _STDCALL_SUPPORTED 1
#define _cdecl
#define __unaligned
#define __declspec(x)
#define _SIZE_T_DEFINED
#define _WCHAR_T_DEFINED
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winsock.h>
#include <ddraw.h>
#include <mmsystem.h>
#include <dsound.h>
#include <Mss.h>
// Compile the Windows reconstruction against the actual RAD vendor ABI.
// CodeWarrior otherwise selects the Mac SDK declarations in these headers.
#define __INTEL__ 1
#if defined(powerc)
#define HOMM3_RAD_RESTORE_POWERC
#undef powerc
#endif
#if defined(macintosh)
#define HOMM3_RAD_RESTORE_MACINTOSH
#undef macintosh
#endif
#if defined(__powerc)
#define HOMM3_RAD_RESTORE_POWER_C
#undef __powerc
#endif
// RAD's declaration branch avoids its unused x86 inline-assembly intrinsics.
#define __WATCOMC__ 1
#define __far
#include <bink.h>
#include <SMACK.H>
#undef __far
#undef __WATCOMC__
#ifdef HOMM3_RAD_RESTORE_POWERC
#define powerc 1
#undef HOMM3_RAD_RESTORE_POWERC
#endif
#ifdef HOMM3_RAD_RESTORE_MACINTOSH
#define macintosh 1
#undef HOMM3_RAD_RESTORE_MACINTOSH
#endif
#ifdef HOMM3_RAD_RESTORE_POWER_C
#define __powerc 1
#undef HOMM3_RAD_RESTORE_POWER_C
#endif
#undef __INTEL__
// ShellExecuteA; WIN32_LEAN_AND_MEAN drops it from windows.h.
#include <shellapi.h>
#undef __declspec
#undef __unaligned
#undef _cdecl
#undef _STDCALL_SUPPORTED
#undef _M_PPC
#undef _WIN32
#pragma pop
#else
#include <windows.h>
#endif

// Game data files are little endian; PowerPC loads them byte-reversed.
#if defined(__POWERPC__)
#define LITTLE_ENDIAN_LONG(value) __lwbrx(&(value), 0)
#define LITTLE_ENDIAN_SHORT(value) __lhbrx(&(value), 0)
#else
#define LITTLE_ENDIAN_LONG(value) (value)
#define LITTLE_ENDIAN_SHORT(value) (value)
#endif

#endif
