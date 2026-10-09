/* shellapi.h - the shell call the shared source names (work/loki-game only). */
#ifndef HOMM3_LOKI_SHELLAPI_H
#define HOMM3_LOKI_SHELLAPI_H

#include <windows.h>

extern "C" HINSTANCE ShellExecuteA(HWND window, LPCSTR operation, LPCSTR file,
                                   LPCSTR parameters, LPCSTR directory, INT show);
#define ShellExecute ShellExecuteA

#endif
