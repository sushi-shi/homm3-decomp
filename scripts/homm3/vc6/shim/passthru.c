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

typedef int (__stdcall *PFN_INVOKE)(int, char **, int);
typedef int (__stdcall *PFN_ABORT)(int);

static HMODULE    s_real;    /* C2_real.dll, loaded once per process */
static PFN_INVOKE s_invoke;
static PFN_ABORT  s_abort;
static LONG       s_calls;

/* ---- tiny CRT-free logging helpers ------------------------------------ */

static HANDLE log_open(void)
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

static void wr_str(HANDLE h, const char *s)
{
    DWORD n;
    if (h == INVALID_HANDLE_VALUE)
        return;
    if (s == 0)
        s = "(null)";
    WriteFile(h, s, (DWORD)lstrlenA(s), &n, 0);
}

static void wr_dec(HANDLE h, unsigned long v)
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

static void wr_sdec(HANDLE h, long v)
{
    if (v < 0) {
        wr_str(h, "-");
        wr_dec(h, (unsigned long)(-v));
    } else {
        wr_dec(h, (unsigned long)v);
    }
}

static void wr_pad2(HANDLE h, unsigned v)
{
    char b[2];
    DWORD n;
    if (h == INVALID_HANDLE_VALUE)
        return;
    b[0] = (char)('0' + (v / 10) % 10);
    b[1] = (char)('0' + v % 10);
    WriteFile(h, b, 2, &n, 0);
}

static void wr_stamp(HANDLE h)
{
    SYSTEMTIME st;
    GetSystemTime(&st);
    wr_str(h, " utc=");
    wr_dec(h, st.wYear);  wr_str(h, "-");
    wr_pad2(h, st.wMonth); wr_str(h, "-");
    wr_pad2(h, st.wDay);   wr_str(h, "T");
    wr_pad2(h, st.wHour);  wr_str(h, ":");
    wr_pad2(h, st.wMinute); wr_str(h, ":");
    wr_pad2(h, st.wSecond);
    wr_str(h, " tick=");
    wr_dec(h, GetTickCount());
}

/* ---- locate the real back end ----------------------------------------- */

static int resolve_real(void)
{
    static char path[MAX_PATH + 16];
    DWORD n;
    int i, cut;
    HMODULE self;

    if (s_invoke != 0 && s_abort != 0)
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
        s_real = LoadLibraryA(path);
    }
    if (s_real == 0)  /* fall back to the normal search order (exe dir first) */
        s_real = LoadLibraryA("C2_real.dll");
    if (s_real == 0)
        return 0;
    s_invoke = (PFN_INVOKE)GetProcAddress(s_real, "_InvokeCompilerPass@12");
    s_abort  = (PFN_ABORT)GetProcAddress(s_real, "_AbortCompilerPass@4");
    return (s_invoke != 0 && s_abort != 0);
}

/* ---- the two exported entry points (names via passthru.def) ----------- */

int __stdcall InvokeCompilerPass(int argc, char **argv, int fLastTU)
{
    HANDLE h;
    LONG call;
    int i, ret;

    call = InterlockedIncrement(&s_calls);
    h = log_open();
    if (h != INVALID_HANDLE_VALUE) {
        wr_str(h, "# c2shim call=");
        wr_dec(h, (unsigned long)call);
        wr_str(h, " export=InvokeCompilerPass fLastTU=");
        wr_sdec(h, fLastTU);
        wr_str(h, " argc=");
        wr_sdec(h, argc);
        wr_stamp(h);
#ifdef SHIM_NEGATIVE_CONTROL
        wr_str(h, " NEGATIVE-CONTROL(drops -Gy)");
#endif
        wr_str(h, "\n");
        for (i = 0; i < argc; ++i) {
            if (i)
                wr_str(h, " ");
            wr_str(h, argv[i]);
        }
        wr_str(h, "\n");
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

    if (!resolve_real()) {
        h = log_open();
        if (h != INVALID_HANDLE_VALUE) {
            wr_str(h, "# c2shim call=");
            wr_dec(h, (unsigned long)call);
            wr_str(h, " ERROR C2_real.dll or its exports unresolved\n");
            CloseHandle(h);
        }
        return 2;  /* driver treats nonzero as pass failure */
    }
    ret = s_invoke(argc, argv, fLastTU);
    h = log_open();
    if (h != INVALID_HANDLE_VALUE) {
        wr_str(h, "# c2shim call=");
        wr_dec(h, (unsigned long)call);
        wr_str(h, " ret=");
        wr_sdec(h, ret);
        wr_str(h, "\n");
        CloseHandle(h);
    }
    return ret;
}

int __stdcall AbortCompilerPass(int code)
{
    HANDLE h;
    LONG call;
    int ret;

    call = InterlockedIncrement(&s_calls);
    h = log_open();
    if (h != INVALID_HANDLE_VALUE) {
        wr_str(h, "# c2shim call=");
        wr_dec(h, (unsigned long)call);
        wr_str(h, " export=AbortCompilerPass code=");
        wr_sdec(h, code);
        wr_stamp(h);
        wr_str(h, "\n");
        CloseHandle(h);
    }
    if (!resolve_real())
        return 0;
    ret = s_abort(code);
    h = log_open();
    if (h != INVALID_HANDLE_VALUE) {
        wr_str(h, "# c2shim call=");
        wr_dec(h, (unsigned long)call);
        wr_str(h, " ret=");
        wr_sdec(h, ret);
        wr_str(h, "\n");
        CloseHandle(h);
    }
    return ret;
}
