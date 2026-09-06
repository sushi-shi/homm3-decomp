/* Optional in-process trace of C2 12.00.8447's inline budget tests.
 * Entry RVAs and replaced instruction bytes are from docs/vc6/inliner.md.
 * Root ECX points to the body; body[0] is the function symbol. Symbol
 * +0x18 is the name, +0x28 the IL handle, +0x6d signed cb, +0x73 flags.
 * The site hook runs after argument/depth/force-inline checks, before the
 * budget/running-size checks and later substitution veto. These are tests,
 * not final expansion verdicts. See inline_trace.py's object-identity gate.
 * GPRs, EFLAGS, x87 and Win32 last-error state survive both hooks. */
static void *trace_root_trampoline;
static void *trace_site_trampoline;
static unsigned long trace_root_index;
static int trace_installed;
static int trace_active;
static char *trace_filter;

static void wr_hex(HANDLE h, unsigned long v)
{
    static const char digits[] = "0123456789abcdef";
    char buf[8];
    int i;
    DWORD n;
    if (h == INVALID_HANDLE_VALUE)
        return;
    for (i = 7; i >= 0; --i) {
        buf[i] = digits[v & 15];
        v >>= 4;
    }
    WriteFile(h, buf, 8, &n, 0);
}

static void trace_symbol(HANDLE h, unsigned char *sym)
{
    wr_str(h, " sym=");    wr_hex(h, (unsigned long)sym);
    wr_str(h, " cb=");     wr_sdec(h, *(short *)(sym + 0x6d));
    wr_str(h, " handle="); wr_hex(h, *(unsigned long *)(sym + 0x28));
    wr_str(h, " flags=");  wr_hex(h, *(unsigned long *)(sym + 0x73));
    wr_str(h, " name=");   wr_str(h, *(char **)(sym + 0x18));
}

static void __cdecl trace_root(unsigned long *regs)
{
    DWORD error = GetLastError();
    HANDLE h;
    unsigned char *sym = *(unsigned char **)regs[6];
    char *name = *(char **)(sym + 0x18);
    ++trace_root_index;
    trace_active = !trace_filter || !trace_filter[0]
        || (name && !lstrcmpA(trace_filter, name));
    if (trace_active) {
        h = log_open();
        wr_str(h, "# inline ROOT "); wr_dec(h, trace_root_index);
        trace_symbol(h, sym);
        wr_str(h, "\n");
        if (h != INVALID_HANDLE_VALUE)
            CloseHandle(h);
    }
    SetLastError(error);
}

static void __cdecl trace_site(unsigned long *regs)
{
    unsigned long sp = regs[3] + 4; /* PUSHAD saved ESP after PUSHFD. */
    DWORD error = GetLastError();
    HANDLE h;
    if (trace_active) {
        h = log_open();
        wr_str(h, "# inline SITE "); wr_dec(h, trace_root_index);
        wr_str(h, " depth=");     wr_dec(h, *(unsigned long *)(sp + 0x34));
        wr_str(h, " budget=");    wr_sdec(h, *(long *)(sp + 0x48));
        wr_str(h, " remaining="); wr_dec(h, *(unsigned long *)(sp + 0x30));
        trace_symbol(h, (unsigned char *)regs[0]);
        wr_str(h, "\n");
        if (h != INVALID_HANDLE_VALUE)
            CloseHandle(h);
    }
    SetLastError(error);
}

static void __declspec(naked) trace_root_hook(void)
{
    __asm {
        pushfd
        pushad
        mov eax,esp
        sub esp,108
        fnsave [esp]
        push eax
        call trace_root
        add esp,4
        frstor [esp]
        add esp,108
        popad
        popfd
        jmp dword ptr [trace_root_trampoline]
    }
}

static void __declspec(naked) trace_site_hook(void)
{
    __asm {
        pushfd
        pushad
        mov eax,esp
        sub esp,108
        fnsave [esp]
        push eax
        call trace_site
        add esp,4
        frstor [esp]
        add esp,108
        popad
        popfd
        jmp dword ptr [trace_site_trampoline]
    }
}

static int trace_patch(unsigned long rva, const unsigned char *expected,
                       void *hook, void **trampoline)
{
    unsigned char *site = (unsigned char *)s_real + rva;
    unsigned char *code;
    unsigned i;
    DWORD old;
    for (i = 0; i < 8; ++i)
        if (site[i] != expected[i])
            return 0;
    code = (unsigned char *)VirtualAlloc(0, 13, MEM_COMMIT | MEM_RESERVE,
                                        PAGE_EXECUTE_READWRITE);
    if (!code)
        return 0;
    for (i = 0; i < 8; ++i)
        code[i] = site[i];
    code[8] = 0xe9;
    *(long *)(code + 9) = (long)(site + 8) - (long)(code + 13);
    if (!FlushInstructionCache(GetCurrentProcess(), code, 13))
        return 0;
    *trampoline = code;
    if (!VirtualProtect(site, 8, PAGE_EXECUTE_READWRITE, &old))
        return 0;
    site[0] = 0xe9;
    *(long *)(site + 1) = (long)hook - (long)(site + 5);
    for (i = 5; i < 8; ++i)
        site[i] = 0x90;
    if (!FlushInstructionCache(GetCurrentProcess(), site, 8))
        return 0;
    return VirtualProtect(site, 8, old, &old) != 0;
}

static int trace_install(void)
{
    static const unsigned char root_bytes[8] = {0x56,0x8b,0xf1,0xb9,4,0,0,0};
    static const unsigned char site_bytes[8] = {0x66,0x8b,0x47,0x6d,0x8b,0x74,0x24,0x48};
    DWORD size, copied;
    if (trace_installed)
        return 1;
    size = GetEnvironmentVariableA("HOMM3_VC6_TRACE_FN", 0, 0);
    if (size) {
        trace_filter = (char *)VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE,
                                           PAGE_READWRITE);
        if (!trace_filter)
            return 0;
        copied = GetEnvironmentVariableA("HOMM3_VC6_TRACE_FN", trace_filter, size);
        if (!copied || copied >= size)
            return 0;
    }
    if (!trace_patch(0x1994f, root_bytes, trace_root_hook, &trace_root_trampoline))
        return 0;
    if (!trace_patch(0x19f8c, site_bytes, trace_site_hook, &trace_site_trampoline))
        return 0;
    trace_installed = 1;
    return 1;
}

static int trace_requested(void)
{
    char enabled[8] = {0};
    DWORD size = GetEnvironmentVariableA("HOMM3_VC6_INLINE_TRACE", enabled, sizeof enabled);
    return size && size < sizeof enabled && enabled[0] == '1';
}
