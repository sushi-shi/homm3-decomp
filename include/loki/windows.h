/* windows.h - the Win32 platform types the shared source names, as the Loki
 * Linux port spells them (work/loki-game only). Declarations only: Loki
 * replaced the Win32 backends with SDL, so no Windows behaviour is supplied. */
#ifndef HOMM3_LOKI_WINDOWS_H
#define HOMM3_LOKI_WINDOWS_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define WINAPI
#define CALLBACK
#define APIENTRY
#define FAR
#define NEAR
#define far
#define near
#define CONST const
#define VOID void
#define IN
#define OUT

typedef unsigned long DWORD;
typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef long LONG;
typedef unsigned long ULONG;
typedef unsigned int UINT;
typedef int INT;
typedef short SHORT;
typedef unsigned short USHORT;
typedef char CHAR;
typedef unsigned char UCHAR;
typedef float FLOAT;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef char* PSTR;
typedef const char* PCSTR;
typedef char TCHAR;
typedef char* LPTSTR;
typedef const char* LPCTSTR;
typedef unsigned short WCHAR;
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef void* PVOID;
typedef DWORD* LPDWORD;
typedef BYTE* LPBYTE;
typedef BYTE* PBYTE;
typedef WORD* LPWORD;
typedef LONG* LPLONG;
typedef BOOL* LPBOOL;
typedef UINT WPARAM;
typedef LONG LPARAM;
typedef LONG LRESULT;
typedef LONG HRESULT;
typedef WORD ATOM;
typedef DWORD COLORREF;
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;

typedef void* HANDLE;
#define DECLARE_HANDLE(name) struct name##__ { int unused; }; typedef struct name##__* name
DECLARE_HANDLE(HWND);
DECLARE_HANDLE(HINSTANCE);
DECLARE_HANDLE(HMENU);
DECLARE_HANDLE(HDC);
DECLARE_HANDLE(HICON);
DECLARE_HANDLE(HBITMAP);
DECLARE_HANDLE(HBRUSH);
DECLARE_HANDLE(HFONT);
DECLARE_HANDLE(HPALETTE);
DECLARE_HANDLE(HKEY);
DECLARE_HANDLE(HGLOBAL__);
typedef HICON HCURSOR;
typedef HINSTANCE HMODULE;
typedef HANDLE HGLOBAL;

#define TRUE 1
#define FALSE 0
#ifndef NULL
#define NULL 0
#endif
#define INVALID_HANDLE_VALUE ((HANDLE)-1)
#define MAX_PATH 260

#define LOWORD(l) ((WORD)(l))
#define HIWORD(l) ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOBYTE(w) ((BYTE)(w))
#define HIBYTE(w) ((BYTE)(((WORD)(w) >> 8) & 0xFF))
#define MAKELONG(a, b) ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#define RGB(r, g, b) ((COLORREF)(((BYTE)(r) | ((WORD)((BYTE)(g)) << 8)) | (((DWORD)(BYTE)(b)) << 16)))

typedef struct tagRECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT, *PRECT, *LPRECT;
typedef const RECT* LPCRECT;

typedef struct tagPOINT {
    LONG x;
    LONG y;
} POINT, *PPOINT, *LPPOINT;

typedef struct tagSIZE {
    LONG cx;
    LONG cy;
} SIZE, *PSIZE, *LPSIZE;

typedef struct _GUID {
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} GUID;
typedef GUID* LPGUID;
typedef const GUID* LPCGUID;
typedef GUID IID;
typedef GUID CLSID;
#define REFGUID const GUID&
#define REFIID const IID&
#define REFCLSID const CLSID&

typedef struct _RTL_CRITICAL_SECTION {
    void* DebugInfo;
    LONG LockCount;
    LONG RecursionCount;
    HANDLE OwningThread;
    HANDLE LockSemaphore;
    DWORD SpinCount;
} CRITICAL_SECTION, *LPCRITICAL_SECTION;

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *LPFILETIME;

typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *LPSYSTEMTIME;

typedef struct tagPALETTEENTRY {
    BYTE peRed;
    BYTE peGreen;
    BYTE peBlue;
    BYTE peFlags;
} PALETTEENTRY, *LPPALETTEENTRY;

typedef struct tagMSG {
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD time;
    POINT pt;
} MSG, *LPMSG;

typedef union _LARGE_INTEGER {
    struct {
        DWORD LowPart;
        LONG HighPart;
    } u;
    LONGLONG QuadPart;
} LARGE_INTEGER;

typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef DWORD (*LPTHREAD_START_ROUTINE)(LPVOID);

#define SUCCEEDED(hr) ((HRESULT)(hr) >= 0)
#define FAILED(hr) ((HRESULT)(hr) < 0)
#define S_OK ((HRESULT)0)
#define S_FALSE ((HRESULT)1)
#define E_FAIL ((HRESULT)0x80004005L)
#define E_OUTOFMEMORY ((HRESULT)0x8007000EL)

#define ZeroMemory(destination, length) memset((destination), 0, (length))
#define CopyMemory(destination, source, length) memcpy((destination), (source), (length))
#define FillMemory(destination, length, fill) memset((destination), (fill), (length))

#define INFINITE 0xFFFFFFFF
#define WAIT_OBJECT_0 0x00000000L
#define WAIT_TIMEOUT 258L
#define ERROR_SUCCESS 0L
#define GENERIC_READ 0x80000000L
#define GENERIC_WRITE 0x40000000L
#define FILE_SHARE_READ 0x00000001
#define FILE_SHARE_WRITE 0x00000002
#define CREATE_NEW 1
#define CREATE_ALWAYS 2
#define OPEN_EXISTING 3
#define OPEN_ALWAYS 4
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FILE_FLAG_SEQUENTIAL_SCAN 0x08000000
#define FILE_BEGIN 0
#define FILE_CURRENT 1
#define FILE_END 2
#define KEY_READ 0x20019
#define KEY_WRITE 0x20006
#define REG_SZ 1
#define REG_DWORD 4
#define HKEY_CURRENT_USER ((HKEY)0x80000001)
#define HKEY_LOCAL_MACHINE ((HKEY)0x80000002)
#define VER_PLATFORM_WIN32s 0
#define VER_PLATFORM_WIN32_WINDOWS 1
#define VER_PLATFORM_WIN32_NT 2
#define MB_OK 0x00000000L
#define MB_OKCANCEL 0x00000001L
#define MB_YESNO 0x00000004L
#define MB_ICONHAND 0x00000010L
#define MB_ICONSTOP MB_ICONHAND
#define MB_ICONERROR MB_ICONHAND
#define MB_ICONEXCLAMATION 0x00000030L
#define MB_ICONINFORMATION 0x00000040L
#define MB_SYSTEMMODAL 0x00001000L
#define MB_TASKMODAL 0x00002000L
#define IDOK 1
#define IDCANCEL 2
#define IDYES 6
#define IDNO 7
#define WM_CLOSE 0x0010
#define WM_QUIT 0x0012
#define WM_USER 0x0400
#define MF_UNCHECKED 0x00000000L
#define MF_CHECKED 0x00000008L
#define MF_BYCOMMAND 0x00000000L
#define SW_HIDE 0
#define SW_SHOWNORMAL 1
#define SW_SHOW 5
#define SW_MINIMIZE 6
#define SW_RESTORE 9
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define MAKEINTRESOURCEA(i) ((LPSTR)((DWORD)((WORD)(i))))
#define MAKEINTRESOURCE MAKEINTRESOURCEA

typedef struct _WIN32_FIND_DATAA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    CHAR cFileName[MAX_PATH];
    CHAR cAlternateFileName[14];
} WIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;
typedef WIN32_FIND_DATAA WIN32_FIND_DATA;

typedef struct _OSVERSIONINFOA {
    DWORD dwOSVersionInfoSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;
    CHAR szCSDVersion[128];
} OSVERSIONINFOA, *LPOSVERSIONINFOA;
typedef OSVERSIONINFOA OSVERSIONINFO;

typedef struct tagRGBQUAD {
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER;

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[1];
} BITMAPINFO, *LPBITMAPINFO;

typedef struct _SECURITY_ATTRIBUTES {
    DWORD nLength;
    LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

typedef struct _OVERLAPPED {
    DWORD Internal;
    DWORD InternalHigh;
    DWORD Offset;
    DWORD OffsetHigh;
    HANDLE hEvent;
} OVERLAPPED, *LPOVERLAPPED;

extern "C" {
void InitializeCriticalSection(LPCRITICAL_SECTION section);
void EnterCriticalSection(LPCRITICAL_SECTION section);
void LeaveCriticalSection(LPCRITICAL_SECTION section);
void DeleteCriticalSection(LPCRITICAL_SECTION section);
HANDLE CreateEventA(LPSECURITY_ATTRIBUTES attributes, BOOL manualReset, BOOL initialState, LPCSTR name);
BOOL SetEvent(HANDLE event);
BOOL ResetEvent(HANDLE event);
DWORD WaitForSingleObject(HANDLE handle, DWORD milliseconds);
BOOL CloseHandle(HANDLE handle);
void Sleep(DWORD milliseconds);
DWORD GetTickCount(void);
HANDLE CreateFileA(LPCSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES attributes,
                   DWORD disposition, DWORD flags, HANDLE templateFile);
BOOL ReadFile(HANDLE file, LPVOID buffer, DWORD count, LPDWORD read, LPOVERLAPPED overlapped);
BOOL WriteFile(HANDLE file, LPCVOID buffer, DWORD count, LPDWORD written, LPOVERLAPPED overlapped);
DWORD SetFilePointer(HANDLE file, LONG distance, LONG* distanceHigh, DWORD method);
DWORD GetFileSize(HANDLE file, LPDWORD sizeHigh);
BOOL DeleteFileA(LPCSTR name);
BOOL GetFileTime(HANDLE file, LPFILETIME creation, LPFILETIME access, LPFILETIME write);
BOOL FileTimeToLocalFileTime(const FILETIME* fileTime, LPFILETIME localFileTime);
BOOL FileTimeToSystemTime(const FILETIME* fileTime, LPSYSTEMTIME systemTime);
HANDLE FindFirstFileA(LPCSTR name, LPWIN32_FIND_DATAA data);
BOOL FindNextFileA(HANDLE find, LPWIN32_FIND_DATAA data);
BOOL FindClose(HANDLE find);
DWORD GetModuleFileNameA(HMODULE module, LPSTR name, DWORD size);
BOOL GetVersionExA(LPOSVERSIONINFOA info);
LONG RegOpenKeyExA(HKEY key, LPCSTR subKey, DWORD options, DWORD desired, HKEY* result);
LONG RegCreateKeyExA(HKEY key, LPCSTR subKey, DWORD reserved, LPSTR keyClass, DWORD options,
                     DWORD desired, LPSECURITY_ATTRIBUTES attributes, HKEY* result, LPDWORD disposition);
LONG RegQueryValueExA(HKEY key, LPCSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD size);
LONG RegSetValueExA(HKEY key, LPCSTR name, DWORD reserved, DWORD type, const BYTE* data, DWORD size);
LONG RegCloseKey(HKEY key);
int MessageBoxA(HWND window, LPCSTR text, LPCSTR caption, UINT type);
BOOL MessageBeep(UINT type);
BOOL PostMessageA(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT SendMessageA(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
SHORT GetKeyState(int key);
SHORT GetAsyncKeyState(int key);
int ShowCursor(BOOL show);
BOOL ShowWindow(HWND window, int command);
BOOL IsIconic(HWND window);
HWND GetForegroundWindow(void);
BOOL SetForegroundWindow(HWND window);
HMENU LoadMenuA(HINSTANCE instance, LPCSTR name);
BOOL SetMenu(HWND window, HMENU menu);
BOOL DestroyMenu(HMENU menu);
DWORD CheckMenuItem(HMENU menu, UINT item, UINT check);
int FillRect(HDC dc, const RECT* rect, HBRUSH brush);
int FrameRect(HDC dc, const RECT* rect, HBRUSH brush);
BOOL IntersectRect(LPRECT destination, const RECT* first, const RECT* second);
BOOL OffsetRect(LPRECT rect, int dx, int dy);
BOOL SetRect(LPRECT rect, int left, int top, int right, int bottom);
BOOL PtInRect(const RECT* rect, POINT point);
COLORREF GetPixel(HDC dc, int x, int y);
BOOL SetPixelFormat(HDC dc, int format, const void* descriptor);
}
#define CreateEvent CreateEventA
#define CreateFile CreateFileA
#define DeleteFile DeleteFileA
#define FindFirstFile FindFirstFileA
#define FindNextFile FindNextFileA
#define GetModuleFileName GetModuleFileNameA
#define GetVersionEx GetVersionExA
#define RegOpenKeyEx RegOpenKeyExA
#define RegCreateKeyEx RegCreateKeyExA
#define RegQueryValueEx RegQueryValueExA
#define RegSetValueEx RegSetValueExA
#define MessageBox MessageBoxA
#define PostMessage PostMessageA
#define SendMessage SendMessageA
#define LoadMenu LoadMenuA

#endif
