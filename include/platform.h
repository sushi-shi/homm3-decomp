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

#endif
