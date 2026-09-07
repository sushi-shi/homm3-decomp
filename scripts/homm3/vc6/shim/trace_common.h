/* Shared passive-hook utilities; included after passthru.c logging helpers. */
static void writeHex(HANDLE h, unsigned long value)
{
    char text[8];
    unsigned i;
    for (i = 0; i < 8; ++i)
        text[7-i] = "0123456789abcdef"[(value >> (4*i)) & 15];
    { DWORD n; WriteFile(h, text, 8, &n, 0); }
}

static char g_filter[256];

static int contains(const char *text, const char *part)
{
    const char *start, *a, *b;
    if (!*part) return 1;
    if (!text) return 0;
    for (start = text; *start; ++start) {
        for (a = start, b = part; *a && *b && *a == *b; ++a, ++b) {}
        if (!*b) return 1;
    }
    return 0;
}

static int patchHook(unsigned long rva, void *hook,
    const unsigned char *expected, unsigned count)
{
    unsigned char *code = (unsigned char *)g_real+rva;
    DWORD old, ignored;
    unsigned i;
    for (i = 0; i < count; ++i) if (code[i] != expected[i]) return 0;
    if (!VirtualProtect(code, count, PAGE_EXECUTE_READWRITE, &old)) return 0;
    code[0] = 0xe9;
    *(long *)(code+1) = (char *)hook-(char *)(code+5);
    for (i = 5; i < count; ++i) code[i] = 0x90;
    VirtualProtect(code, count, old, &ignored);
    FlushInstructionCache(GetCurrentProcess(), code, count);
    return 1;
}

