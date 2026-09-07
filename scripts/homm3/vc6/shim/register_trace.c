/* Passive observations at C2's temporary register-binding stores.
 * These are NOT a complete allocator trace or original source-variable names.
 * See docs/vc6/regalloc.md for the two sites and the identity gate.
 */
#include "trace_common.h"

static void *g_bindReturn;
static void *g_storeReturn;
static void *g_bindingsAddress;
static unsigned long g_lastRoot;

static const char *registerName(unsigned long id)
{
    static const char *names[] = {
        "none", "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"
    };
    return id < 9 ? names[id] : "unknown";
}

static void writeValue(HANDLE h, const unsigned char *value)
{
    if (!value) { writeString(h, "free"); return; }
    if (IsBadReadPtr(value, 0x20)) { writeString(h, "unreadable"); return; }
    /* Snapshot category and handle together: heap pointers are not identities. */
    writeDecimal(h, value[4]); writeString(h, ":");
    writeHex(h, *(const unsigned long *)(value + 0x1c));
}

static void __cdecl traceRegisterStore(unsigned long *regs, unsigned long site)
{
    DWORD saved = GetLastError();
    unsigned long *body = *(unsigned long **)((char *)g_real + 0xac380);
    const char *name;
    HANDLE h;
    unsigned i;
    unsigned char **bindings = (unsigned char **)g_bindingsAddress;
    if (!body || IsBadReadPtr(body, 4) || IsBadReadPtr((void *)body[0], 0x1c)) {
        SetLastError(saved); return;
    }
    name = *(const char **)(body[0] + 0x18);
    if (!contains(name, g_filter)) { SetLastError(saved); return; }
    h = logOpen();
    if (h == INVALID_HANDLE_VALUE) { SetLastError(saved); return; }
    if (g_lastRoot != body[0]) {
        writeString(h, "sym "); writeHex(h, body[0]); writeString(h, " ");
        writeString(h, name); writeString(h, "\n");
        g_lastRoot = body[0];
    }
    writeString(h, "register root="); writeHex(h, body[0]);
    writeString(h, " site="); writeHex(h, site);
    writeString(h, " selected=");
    writeString(h, registerName(site == 0x3356e ? regs[1] : regs[7]));
    writeString(h, " value="); writeValue(h, (unsigned char *)regs[5]);
    for (i = 1; i <= 8; ++i) {
        writeString(h, " "); writeString(h, registerName(i)); writeString(h, "=");
        writeValue(h, bindings[i]);
    }
    writeString(h, "\n"); CloseHandle(h); SetLastError(saved);
}

static void __declspec(naked) bindHook(void)
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push 03356eh
        push eax
        call traceRegisterStore
        add esp, 8
        popad
        popfd
        push ecx
        mov ecx, g_bindingsAddress
        mov [ecx + esi*4], edx
        pop ecx
        jmp dword ptr [g_bindReturn]
    }
}

static void __declspec(naked) storeHook(void)
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push 0323feh
        push eax
        call traceRegisterStore
        add esp, 8
        popad
        popfd
        push ecx
        mov ecx, g_bindingsAddress
        mov [ecx + eax*4], edx
        pop ecx
        jmp dword ptr [g_storeReturn]
    }
}

static int installRegisterTrace(void)
{
    static int installed;
    static const unsigned char bindBytes[] = {0x89,0x14,0xb5,0xec,0xd6,0x79,0x10};
    static const unsigned char storeBytes[] = {0x89,0x14,0x85,0xec,0xd6,0x79,0x10};
    if (installed) return 1;
    if (GetEnvironmentVariableA("HOMM3_VC6_REGISTER_TRACE", g_filter,
        sizeof g_filter) >= sizeof g_filter) return 0;
    g_bindReturn = (char *)g_real + 0x33575;
    g_storeReturn = (char *)g_real + 0x32405;
    g_bindingsAddress = (char *)g_real + 0x9d6ec;
    if (!patchHook(0x3356e, bindHook, bindBytes, sizeof bindBytes)) return 0;
    if (!patchHook(0x323fe, storeHook, storeBytes, sizeof storeBytes)) return 0;
    installed = 1;
    return 1;
}
