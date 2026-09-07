/* Passive budget observations for pinned VC6 C2.DLL 12.00.8447.
 * Included only in the temporary SHIM_INLINE_TRACE overlay. Normal builds
 * never select this DLL; the trace command restores the hook-free shim.
 *
 * 0x1995c: ESI -> function body, body[0] -> symbol, symbol+0x18 -> name.
 * 0x19f8c: EDI -> callee symbol; ESP+0x48 budget, +0x34 depth,
 * +0x30 sites remaining, +0x1c owner body. symbol+0x6d is signed cb.
 * The second hook observes the budget comparison, before possible later
 * rejection. It does NOT claim the final inline decision; inspect output.
 *
 * Recovered compiler roles cb / budget are documented in inliner.md.
 * Each hook replays whole original instructions with registers and flags
 * restored, using the loaded DLL base. Hash and instruction guards plus
 * exact object equality are required before interpreting the observations.
 */
static void *g_mainReturn;
static void *g_siteReturn;
static unsigned long g_root;
static unsigned long g_seen[8192];
static unsigned g_seenCount;

#include "trace_common.h"

static int g_selected;

static void traceSymbol(HANDLE h, unsigned char *sym)
{
    unsigned i;
    for (i = 0; i < g_seenCount; ++i)
        if (g_seen[i] == (unsigned long)sym) return;
    if (g_seenCount < 8192) g_seen[g_seenCount++] = (unsigned long)sym;
    writeString(h, "sym "); writeHex(h, (unsigned long)sym); writeString(h, " ");
    writeString(h, *(const char **)(sym+0x18)); writeString(h, "\n");
}

static void __cdecl traceMain(unsigned long *body)
{
    HANDLE h;
    DWORD lastError;
    unsigned char *sym = (unsigned char *)body[0];
    g_root = (unsigned long)sym;
    g_selected = contains(*(const char **)(sym+0x18), g_filter);
    if (!g_selected) return;
    lastError = GetLastError();
    h = logOpen();
    if (h == INVALID_HANDLE_VALUE) { SetLastError(lastError); return; }
    traceSymbol(h, sym);
    writeString(h, "main "); writeHex(h, g_root);
    writeString(h, " cb="); writeSignedDecimal(h, *(short *)(sym+0x6d));
    writeString(h, "\n"); CloseHandle(h);
    SetLastError(lastError);
}

static void __cdecl traceSite(unsigned long *regs)
{
    HANDLE h;
    DWORD lastError;
    unsigned char *sym = (unsigned char *)regs[0]; /* saved EDI */
    unsigned char *sp = (unsigned char *)(regs[3]+4); /* before pushfd */
    unsigned long *body = *(unsigned long **)(sp+0x1c);
    if (!g_selected) return;
    lastError = GetLastError();
    h = logOpen();
    if (h == INVALID_HANDLE_VALUE) { SetLastError(lastError); return; }
    traceSymbol(h, sym);
    writeString(h, "site root="); writeHex(h, g_root);
    writeString(h, " owner="); writeHex(h, body[0]);
    writeString(h, " callee="); writeHex(h, (unsigned long)sym);
    writeString(h, " cb="); writeSignedDecimal(h, *(short *)(sym+0x6d));
    writeString(h, " budget="); writeSignedDecimal(h, *(long *)(sp+0x48));
    writeString(h, " depth="); writeDecimal(h, *(unsigned long *)(sp+0x34));
    writeString(h, " remain="); writeDecimal(h, *(unsigned long *)(sp+0x30));
    writeString(h, " running="); writeSignedDecimal(h, *(long *)((char *)g_real+0x9f234));
    writeString(h, "\n"); CloseHandle(h);
    SetLastError(lastError);
}

static void __declspec(naked) mainHook(void)
{
    __asm {
        pushfd
        pushad
        push esi
        call traceMain
        add esp, 4
        popad
        popfd
        mov eax, [esi]
        movsx eax, word ptr [eax+06dh]
        jmp dword ptr [g_mainReturn]
    }
}

static void __declspec(naked) siteHook(void)
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push eax
        call traceSite
        add esp, 4
        popad
        popfd
        mov ax, word ptr [edi+06dh]
        mov esi, [esp+048h]
        jmp dword ptr [g_siteReturn]
    }
}

static int installInlineTrace(void)
{
    static int installed;
    static const unsigned char mainBytes[] = {0x8b,0x06,0x0f,0xbf,0x40,0x6d};
    static const unsigned char siteBytes[] = {0x66,0x8b,0x47,0x6d,0x8b,0x74,0x24,0x48};
    if (installed) return 1;
    if (GetEnvironmentVariableA("HOMM3_VC6_INLINE_TRACE", g_filter,
        sizeof g_filter) >= sizeof g_filter) return 0;
    g_mainReturn = (char *)g_real+0x19962;
    g_siteReturn = (char *)g_real+0x19f94;
    if (!patchHook(0x1995c, mainHook, mainBytes, sizeof mainBytes)) return 0;
    if (!patchHook(0x19f8c, siteHook, siteBytes, sizeof siteBytes)) return 0;
    installed = 1;
    return 1;
}
