/* homm3.vc6.shim/passthru.c - C2-slot pass-through instrumentation DLL (v1).
 *
 * Drop-in replacement for the pinned back end C2.DLL (12.00.8447, sha256
 * a0cc45f8..., image base 0x10700000).  The CL driver (12.00.8168) resolves
 * the back end by name at run time - LoadLibraryA(<bin>\c2.dll) at CL.EXE
 * .text 0x406130, then GetProcAddress for the two literal strings
 * "_InvokeCompilerPass@12" (0x409a78) and "_AbortCompilerPass@4" (0x409a60)
 * at 0x40613f..0x406156 - so a DLL of the same name exporting the same two
 * decorated stdcall symbols slots in without touching the pinned toolchain.
 *
 * ABI (RE'd from the CL.EXE call site at 0x4061bc..0x4061bf, see
 * docs/vc6/shim.md):
 *
 *     int __stdcall InvokeCompilerPass(int argc, char **argv, int fLastTU);
 *     int __stdcall AbortCompilerPass(int code);
 *
 * v1 behaviour: append the received argv to a log file, forward all three
 * arguments unchanged to the real back end (renamed C2_real.dll in the same
 * directory by shim/build.py), and return its return value.  Inertness is
 * proven by shim/build.py's byte-identity gate, not assumed.
 *
 * Log file: the Windows path in HOMM3_VC6_SHIM_LOG (build.py passes the
 * winepath of build/vc6/shim/argv.log), else c2shim_argv.log in the cwd.
 * Log format (one block per call; the bare line is parseable by
 * homm3.vc6.argv's --verify, which takes the last line containing "c2.dll"):
 *
 *     # c2shim call=1 export=InvokeCompilerPass fLastTU=1 argc=13 utc=... tick=...
 *     Z:\...\c2.dll -il C:\...\a00123 -f sample.cpp ... -EHs
 *     # c2shim call=1 ret=0
 *
 * Build discipline: C89, compiled BY the pinned VC6 CL/LINK (shim/build.py),
 * /NOENTRY with no CRT - kernel32 imports only.  Exports come from
 * passthru.def so the decorated names match the real C2.DLL's exactly.
 *
 * SHIM_NEGATIVE_CONTROL: when defined at compile time this builds the
 * deliberately NON-inert variant for the gate's negative control - it drops
 * every "-Gy" token before forwarding (function-level COMDATs disappear, so
 * the produced .obj must differ and the byte-identity gate must go red).
 * Never install this variant except through `shim/build.py negative`.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef int (__stdcall *invokeCompilerPassFunction)(int, char **, int);
typedef int (__stdcall *abortCompilerPassFunction)(int);

static HMODULE    g_real;    /* C2_real.dll, loaded once per process */
static invokeCompilerPassFunction g_invoke;
static abortCompilerPassFunction  g_abort;
static LONG       g_calls;

/* ---- tiny CRT-free logging helpers ------------------------------------ */

static HANDLE logOpen(void)
{
    static char path[520];
    HANDLE h;

    if (!GetEnvironmentVariableA("HOMM3_VC6_SHIM_LOG", path, sizeof path))
        lstrcpyA(path, "c2shim_argv.log");
    h = CreateFileA(path, GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, 0,
                    OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (h != INVALID_HANDLE_VALUE)
        SetFilePointer(h, 0, 0, FILE_END);
    return h;
}

static void writeString(HANDLE h, const char *s)
{
    DWORD n;
    if (h == INVALID_HANDLE_VALUE)
        return;
    if (s == 0)
        s = "(null)";
    WriteFile(h, s, (DWORD)lstrlenA(s), &n, 0);
}

static void writeDecimal(HANDLE h, unsigned long v)
{
    char buf[16];
    int i = 16;
    DWORD n;
    if (h == INVALID_HANDLE_VALUE)
        return;
    do {
        buf[--i] = (char)('0' + (int)(v % 10));
        v /= 10;
    } while (v != 0 && i > 0);
    WriteFile(h, buf + i, (DWORD)(16 - i), &n, 0);
}

static void writeSignedDecimal(HANDLE h, long v)
{
    if (v < 0) {
        writeString(h, "-");
        writeDecimal(h, (unsigned long)(-v));
    } else {
        writeDecimal(h, (unsigned long)v);
    }
}

static void writePaddedTwo(HANDLE h, unsigned v)
{
    char b[2];
    DWORD n;
    if (h == INVALID_HANDLE_VALUE)
        return;
    b[0] = (char)('0' + (v / 10) % 10);
    b[1] = (char)('0' + v % 10);
    WriteFile(h, b, 2, &n, 0);
}

static void writeTimestamp(HANDLE h)
{
    SYSTEMTIME st;
    GetSystemTime(&st);
    writeString(h, " utc=");
    writeDecimal(h, st.wYear);  writeString(h, "-");
    writePaddedTwo(h, st.wMonth); writeString(h, "-");
    writePaddedTwo(h, st.wDay);   writeString(h, "T");
    writePaddedTwo(h, st.wHour);  writeString(h, ":");
    writePaddedTwo(h, st.wMinute); writeString(h, ":");
    writePaddedTwo(h, st.wSecond);
    writeString(h, " tick=");
    writeDecimal(h, GetTickCount());
}

/* ---- locate the real back end ----------------------------------------- */

static int resolveReal(void)
{
    static char path[MAX_PATH + 16];
    DWORD n;
    int i, cut;
    HMODULE self;

    if (g_invoke != 0 && g_abort != 0)
        return 1;
    /* our own module was loaded as ...\c2.dll; swap the basename */
    self = GetModuleHandleA("C2.DLL");
    n = (self != 0) ? GetModuleFileNameA(self, path, MAX_PATH) : 0;
    cut = -1;
    for (i = 0; i < (int)n; ++i)
        if (path[i] == '\\' || path[i] == '/' || path[i] == ':')
            cut = i;
    if (cut >= 0) {
        lstrcpyA(path + cut + 1, "C2_real.dll");
        g_real = LoadLibraryA(path);
    }
    if (g_real == 0)  /* fall back to the normal search order (exe dir first) */
        g_real = LoadLibraryA("C2_real.dll");
    if (g_real == 0)
        return 0;
    g_invoke = (invokeCompilerPassFunction)GetProcAddress(g_real, "_InvokeCompilerPass@12");
    g_abort  = (abortCompilerPassFunction)GetProcAddress(g_real, "_AbortCompilerPass@4");
    return (g_invoke != 0 && g_abort != 0);
}

#ifdef SHIM_INLINE_TRACE
#include "inline_trace.c"
#endif

/* ---- the two exported entry points (names via passthru.def) ----------- */

int __stdcall InvokeCompilerPass(int argc, char **argv, int fLastTU)
{
    HANDLE h;
    LONG call;
    int i, ret;

    call = InterlockedIncrement(&g_calls);
    h = logOpen();
    if (h != INVALID_HANDLE_VALUE) {
        writeString(h, "# c2shim call=");
        writeDecimal(h, (unsigned long)call);
        writeString(h, " export=InvokeCompilerPass fLastTU=");
        writeSignedDecimal(h, fLastTU);
        writeString(h, " argc=");
        writeSignedDecimal(h, argc);
        writeTimestamp(h);
#ifdef SHIM_NEGATIVE_CONTROL
        writeString(h, " NEGATIVE-CONTROL(drops -Gy)");
#endif
        writeString(h, "\n");
        for (i = 0; i < argc; ++i) {
            if (i)
                writeString(h, " ");
            writeString(h, argv[i]);
        }
        writeString(h, "\n");
        CloseHandle(h);
    }

#ifdef SHIM_NEGATIVE_CONTROL
    {
        static char *mut[512];
        int j = 0;
        for (i = 0; i < argc && j < 511; ++i)
            if (argv[i] == 0 || lstrcmpA(argv[i], "-Gy") != 0)
                mut[j++] = argv[i];
        mut[j] = 0;
        argv = mut;
        argc = j;
    }
#endif

    if (!resolveReal()) {
        h = logOpen();
        if (h != INVALID_HANDLE_VALUE) {
            writeString(h, "# c2shim call=");
            writeDecimal(h, (unsigned long)call);
            writeString(h, " ERROR C2_real.dll or its exports unresolved\n");
            CloseHandle(h);
        }
        return 2;  /* driver treats nonzero as pass failure */
    }
#ifdef SHIM_INLINE_TRACE
    if (!installInlineTrace()) {
        h = logOpen();
        if (h != INVALID_HANDLE_VALUE) {
            writeString(h, "# inline trace instruction guard failed\n");
            CloseHandle(h);
        }
        return 3;
    }
#endif
    ret = g_invoke(argc, argv, fLastTU);
    h = logOpen();
    if (h != INVALID_HANDLE_VALUE) {
        writeString(h, "# c2shim call=");
        writeDecimal(h, (unsigned long)call);
        writeString(h, " ret=");
        writeSignedDecimal(h, ret);
        writeString(h, "\n");
        CloseHandle(h);
    }
    return ret;
}

int __stdcall AbortCompilerPass(int code)
{
    HANDLE h;
    LONG call;
    int ret;

    call = InterlockedIncrement(&g_calls);
    h = logOpen();
    if (h != INVALID_HANDLE_VALUE) {
        writeString(h, "# c2shim call=");
        writeDecimal(h, (unsigned long)call);
        writeString(h, " export=AbortCompilerPass code=");
        writeSignedDecimal(h, code);
        writeTimestamp(h);
        writeString(h, "\n");
        CloseHandle(h);
    }
    if (!resolveReal())
        return 0;
    ret = g_abort(code);
    h = logOpen();
    if (h != INVALID_HANDLE_VALUE) {
        writeString(h, "# c2shim call=");
        writeDecimal(h, (unsigned long)call);
        writeString(h, " ret=");
        writeSignedDecimal(h, ret);
        writeString(h, "\n");
        CloseHandle(h);
    }
    return ret;
}
