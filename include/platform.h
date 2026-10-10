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
// MWCPPC 2.4 defines all three platform built-ins as 1.
#undef powerc
#undef macintosh
#undef __powerc
// RAD's declaration branch avoids its unused x86 inline-assembly intrinsics.
#define __WATCOMC__ 1
#define __far
#include <bink.h>
#include <SMACK.H>
#undef __far
#undef __WATCOMC__
#define powerc 1
#define macintosh 1
#define __powerc 1
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

// Mac retail zero filling uses its two-argument byte-zero routine.
// Keep the Windows SDK zeroing operation's semantics; no nonzero fill is lost.
extern "C" void bzero(void* destination, unsigned long count);
#undef ZeroMemory
#define ZeroMemory(destination, count) bzero(destination, count)

// Mac retail error dialogs take only the message and title strings.
extern "C" void showMacPlatformMessage(const char* message, const char* title);
#define MessageBoxA(window, message, title, flags) \
    showMacPlatformMessage(message, title)
#undef MessageBox
#define MessageBox(window, message, title, flags) \
    showMacPlatformMessage(message, title)
#else
#include <windows.h>
#endif

// Keyboard scan codes differ between the Windows and Classic Mac input APIs.
#if defined(HOMM3_TARGET_MAC)
#define H3_NATIVE_KEY_CODE(windowsCode, macCode) (macCode)
#else
#define H3_NATIVE_KEY_CODE(windowsCode, macCode) (windowsCode)
#endif

// Game data files are little endian; PowerPC loads them byte-reversed.
// The in-place forms decode a just-read buffer; on x86 they are no statement.
#if defined(__POWERPC__)
#define LITTLE_ENDIAN_LONG(value) __lwbrx(&(value), 0)
#define LITTLE_ENDIAN_SHORT(value) __lhbrx(&(value), 0)
#define DECODE_LITTLE_ENDIAN_LONG(value) ((value) = __lwbrx(&(value), 0))
#define DECODE_LITTLE_ENDIAN_SHORT(value) ((value) = __lhbrx(&(value), 0))
#else
#define LITTLE_ENDIAN_LONG(value) (value)
#define LITTLE_ENDIAN_SHORT(value) (value)
#define DECODE_LITTLE_ENDIAN_LONG(value)
#define DECODE_LITTLE_ENDIAN_SHORT(value)
#endif

#endif
