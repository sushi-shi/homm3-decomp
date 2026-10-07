/* Passive candidate-state and budget observations for pinned VC6 C2.DLL 12.00.8447.
 * Included only in the temporary SHIM_INLINE_TRACE overlay. Normal builds
 * never select this DLL; the trace command restores the hook-free shim.
 *
 * 0x1995c: ESI -> function body, body[0] -> symbol, symbol+0x18 -> name.
 * 0x19f8c: EDI -> callee symbol; ESP+0x48 budget, +0x34 depth,
 * +0x30 sites remaining, +0x1c owner body. symbol+0x6d is signed cb.
 * 0x1a412: EDI -> callee symbol before the caller-state eligibility gate.
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
static void *g_candidateReturn;
static unsigned char **g_currentBodySlot;
static unsigned long g_root;
static unsigned long g_seen[8192];
static unsigned g_seenCount;

#include "trace_common.h"

static int g_selected;

/* Decision forcing (docs/vc6/decision-forcing.md). Active only when
 * HOMM3_VC6_INLINE_FORCE names a rule file; otherwise every site takes the
 * original path and this variant equals the passive trace. A rule is one
 * line "owner<TAB>callee<TAB>occurrence<TAB>E|K": substrings of the owner
 * body's and callee's compiler names ("*" = any), and the 1-based count of
 * matching budget tests within the selected root (0 = every one). E jumps
 * to the admitted path at 0x19faf, K to the rejection path at 0x19a94.
 * Only sites that reach the budget comparison can be forced: arity, depth
 * and forceinline sites never enter this hook. */
#define FORCE_RULES 256
static struct {
    char owner[96];
    char callee[160];
    unsigned long occurrence;
    unsigned long seen;
    char action;
} g_rules[FORCE_RULES];
static unsigned g_ruleCount;
static unsigned long g_decision;
static void *g_expandTarget;
static void *g_keepTarget;

static unsigned copyField(char *dst, unsigned size, const char *src, unsigned n)
{
    unsigned i;
    for (i = 0; i < n && src[i] != '\t' && src[i] != '\n' && src[i] != '\r'; ++i)
        if (i + 1 < size) dst[i] = src[i];
    dst[i < size ? i : size - 1] = 0;
    return i;
}

static void loadForceRules(void)
{
    static char path[520];
    static char text[65536];
    HANDLE h;
    DWORD n = 0;
    unsigned at = 0;
    if (!GetEnvironmentVariableA("HOMM3_VC6_INLINE_FORCE", path, sizeof path)) return;
    h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, 0);
    if (h == INVALID_HANDLE_VALUE) return;
    ReadFile(h, text, sizeof text - 1, &n, 0);
    CloseHandle(h);
    text[n] = 0;
    while (at < n && g_ruleCount < FORCE_RULES) {
        unsigned start = at, k;
        unsigned long occurrence = 0;
        char action;
        if (text[at] == '#' || text[at] == '\n' || text[at] == '\r') {
            while (at < n && text[at] != '\n') ++at;
            ++at; continue;
        }
        at += copyField(g_rules[g_ruleCount].owner, sizeof g_rules[0].owner, text+at, n-at);
        if (text[at] != '\t') break;
        ++at;
        at += copyField(g_rules[g_ruleCount].callee, sizeof g_rules[0].callee, text+at, n-at);
        if (text[at] != '\t') break;
        ++at;
        for (k = at; k < n && text[k] >= '0' && text[k] <= '9'; ++k)
            occurrence = occurrence * 10 + (unsigned long)(text[k] - '0');
        at = k;
        if (text[at] != '\t') break;
        action = text[at+1];
        if (action != 'E' && action != 'K') break;
        g_rules[g_ruleCount].occurrence = occurrence;
        g_rules[g_ruleCount].action = action;
        if (g_rules[g_ruleCount].owner[0] == '*' && !g_rules[g_ruleCount].owner[1])
            g_rules[g_ruleCount].owner[0] = 0;
        if (g_rules[g_ruleCount].callee[0] == '*' && !g_rules[g_ruleCount].callee[1])
            g_rules[g_ruleCount].callee[0] = 0;
        ++g_ruleCount;
        while (at < n && text[at] != '\n') ++at;
        ++at;
        (void)start;
    }
}

static char forceDecision(const char *owner, const char *callee)
{
    unsigned i;
    char result = 0;
    for (i = 0; i < g_ruleCount; ++i) {
        if (!contains(owner, g_rules[i].owner) || !contains(callee, g_rules[i].callee))
            continue;
        ++g_rules[i].seen;
        if (!result && (g_rules[i].occurrence == 0
                        || g_rules[i].seen == g_rules[i].occurrence))
            result = g_rules[i].action;
    }
    return result;
}

static void traceSymbol(HANDLE h, unsigned char *sym)
{
    unsigned i;
    for (i = 0; i < g_seenCount; ++i)
        if (g_seen[i] == (unsigned long)sym) return;
    if (g_seenCount < 8192) g_seen[g_seenCount++] = (unsigned long)sym;
    writeString(h, "sym "); writeHex(h, (unsigned long)sym); writeString(h, " ");
    writeString(h, *(const char **)(sym+0x18)); writeString(h, "\n");
}

/* Register-assignment forcing at C2's global coloring choice (regalloc.md
 * section 3b). 0x245c3 picks the lowest-cost eligible register for each
 * live-range group in priority order; at 0x24748 EDI holds that choice and
 * EBX the group (candidate set at group+0x20). HOMM3_VC6_REG_FORCE lists
 * "k:reg" pairs (1-based decision index within the selected function, C2
 * register number 1=EAX..8=EDI). A forced register must be a member of the
 * group's candidate set; otherwise C2's choice stands. Every decision of the
 * selected function is logged as "color k= chosen= eligible= forced=". */
typedef int (__fastcall *setHasFunction)(void *set, unsigned long member);
static setHasFunction g_setHas;
static void *g_colorReturn;
static unsigned long g_colorIndex;
static unsigned long g_mergeIndex;
static struct { unsigned long a, b; } g_vetoPairs[1024];
static unsigned g_vetoPairCount;
static unsigned char g_regForce[1024];
static int g_regForceActive;

static void loadRegisterForce(void)
{
    static char text[8192];
    DWORD n = GetEnvironmentVariableA("HOMM3_VC6_REG_FORCE", text, sizeof text);
    unsigned long k = 0, reg = 0, i;
    int seenColon = 0;
    if (!n || n >= sizeof text) return;
    for (i = 0; i <= n; ++i) {
        char c = i < n ? text[i] : ',';
        if (c >= '0' && c <= '9') {
            if (seenColon) reg = reg * 10 + (unsigned long)(c - '0');
            else k = k * 10 + (unsigned long)(c - '0');
        } else if (c == ':') {
            seenColon = 1;
        } else if (c == ',' || c == ' ') {
            if (seenColon && k && k < sizeof g_regForce && reg && reg < 9 && reg != 5) {
                g_regForce[k] = (unsigned char)reg;
                g_regForceActive = 1;
            }
            k = reg = 0; seenColon = 0;
        }
    }
}

static void __cdecl colorDecision(unsigned long *regs)
{
    /* pushad layout: EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX */
    unsigned char *group = (unsigned char *)regs[4];
    void *set;
    unsigned long chosen = regs[0], eligible = 0, r, forced = 0;
    HANDLE h;
    DWORD lastError;
    if (!g_selected) return;
    lastError = GetLastError();
    ++g_colorIndex;
    set = *(void **)(group + 0x20);
    for (r = 1; r <= 8; ++r)
        if (r != 5 && set && g_setHas(set, r)) eligible |= 1ul << r;
    if (chosen && g_regForceActive && g_colorIndex < sizeof g_regForce
            && g_regForce[g_colorIndex]
            && (eligible & (1ul << g_regForce[g_colorIndex]))) {
        forced = g_regForce[g_colorIndex];
        regs[0] = forced;
    }
    h = logOpen();
    if (h != INVALID_HANDLE_VALUE) {
        writeString(h, "color root="); writeHex(h, g_root);
        writeString(h, " k="); writeDecimal(h, g_colorIndex);
        writeString(h, " chosen="); writeDecimal(h, chosen);
        writeString(h, " eligible="); writeHex(h, eligible);
        writeString(h, " priority="); writeSignedDecimal(h, *(long *)(group + 0x0c));
        if (forced) { writeString(h, " forced="); writeDecimal(h, forced); }
        writeString(h, "\n"); CloseHandle(h);
    }
    SetLastError(lastError);
}

static void __declspec(naked) colorHook(void)
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push eax
        call colorDecision
        add esp, 4
        popad
        popfd
        lea eax, [edi*8]
        sub eax, edi
        jmp dword ptr [g_colorReturn]
    }
}

/* Carried-over back-end state at the start of the selected function
 * (context-variants.md). HOMM3_VC6_SNAPSHOT names a file that receives, on
 * the first selected root only, raw copies of C2's writable image sections
 * (.bssbe, .data, .databe) and the root symbol record, each preceded by a
 * 12-byte header: tag, rva, size. Observation only: nothing is changed. */
static int g_snapshotDone;

static void writeBlock(HANDLE h, unsigned long tag, unsigned long rva,
    const void *data, unsigned long size)
{
    DWORD n;
    unsigned long header[3];
    header[0] = tag; header[1] = rva; header[2] = size;
    WriteFile(h, header, sizeof header, &n, 0);
    WriteFile(h, data, size, &n, 0);
}

static void snapshotState(unsigned char *sym)
{
    char path[MAX_PATH];
    HANDLE h;
    unsigned char *base = (unsigned char *)g_real;
    if (g_snapshotDone) return;
    if (!GetEnvironmentVariableA("HOMM3_VC6_SNAPSHOT", path, sizeof path)) return;
    g_snapshotDone = 1;
    h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, 0);
    if (h == INVALID_HANDLE_VALUE) return;
    writeBlock(h, 1, 0x99000, base + 0x99000, 0x66d4);
    writeBlock(h, 2, 0xab000, base + 0xab000, 0xe0);
    writeBlock(h, 3, 0xac000, base + 0xac000, 0x2470);
    writeBlock(h, 4, (unsigned long)sym, sym, 0x80);
    CloseHandle(h);
}

/* Diagnostic state transplant (context-variants.md): HOMM3_VC6_STATE_PATCH
 * names a binary file of (rva, value) u32 pairs written into C2's image at
 * the start of the first selected root, after the snapshot. Used only to
 * attribute a CUR/MAX difference to carried state; never a variant source. */
static int g_patchDone;

static void patchState(void)
{
    static unsigned long pairs[4096];
    char path[MAX_PATH];
    HANDLE h;
    DWORD n = 0, i;
    unsigned char *base = (unsigned char *)g_real;
    if (g_patchDone) return;
    if (!GetEnvironmentVariableA("HOMM3_VC6_STATE_PATCH", path, sizeof path)) return;
    g_patchDone = 1;
    h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (h == INVALID_HANDLE_VALUE) return;
    ReadFile(h, pairs, sizeof pairs, &n, 0);
    CloseHandle(h);
    for (i = 0; i + 1 < n / 4; i += 2) {
        unsigned long rva = pairs[i];
        if ((rva >= 0x99000 && rva + 4 <= 0x9f6d4) || (rva >= 0xab000 && rva + 4 <= 0xae470))
            *(unsigned long *)(base + rva) = pairs[i + 1];
    }
}

/* Cost-record channel (context-variants.md). HOMM3_VC6_CB_SET is a list
 * "name=cb;name=cb": each symbol whose compiler name contains `name` gets
 * that IL cost in its record (sym+0x6d) when the selected root first meets
 * it (the root itself in traceMain, a callee at its budget test). This
 * emulates a context whose source gives that symbol another cost; C2 then
 * makes every decision itself. "*root*" names the selected root. */
#define CB_SET_MAX 16
static struct { char name[160]; short cb; } g_cbSet[CB_SET_MAX];
static unsigned g_cbSetCount;
static int g_cbSetLoaded;

static void loadCbSet(void)
{
    static char text[4096];
    DWORD n, i;
    unsigned k = 0, j = 0;
    int value = 0, sign = 1, inValue = 0;
    g_cbSetLoaded = 1;
    n = GetEnvironmentVariableA("HOMM3_VC6_CB_SET", text, sizeof text);
    if (!n || n >= sizeof text) return;
    for (i = 0; i <= n && k < CB_SET_MAX; ++i) {
        char c = i < n ? text[i] : ';';
        if (c == ';') {
            if (inValue && j) { g_cbSet[k].name[j] = 0; g_cbSet[k].cb = (short)(sign * value); ++k; }
            j = 0; value = 0; sign = 1; inValue = 0;
        } else if (!inValue && c == '=') {
            inValue = 1;
        } else if (inValue) {
            if (c == '-') sign = -1; else if (c >= '0' && c <= '9') value = value * 10 + (c - '0');
        } else if (j + 1 < sizeof g_cbSet[0].name) {
            g_cbSet[k].name[j++] = c;
        }
    }
    g_cbSetCount = k;
}

static void applyCbSet(unsigned char *sym, int isRoot)
{
    unsigned i;
    const char *name;
    if (!g_cbSetLoaded) loadCbSet();
    if (!g_cbSetCount) return;
    name = *(const char **)(sym + 0x18);
    for (i = 0; i < g_cbSetCount; ++i) {
        if (isRoot ? lstrcmpA(g_cbSet[i].name, "*root*") == 0
                   : (lstrcmpA(g_cbSet[i].name, "*root*") != 0 && contains(name, g_cbSet[i].name)))
            *(short *)(sym + 0x6d) = g_cbSet[i].cb;
    }
}

static void __cdecl traceMain(unsigned long *body)
{
    HANDLE h;
    DWORD lastError;
    unsigned char *sym = (unsigned char *)body[0];
    g_root = (unsigned long)sym;
    g_selected = contains(*(const char **)(sym+0x18), g_filter);
    g_colorIndex = 0;
    g_mergeIndex = 0;
    g_vetoPairCount = 0;
    if (!g_selected) return;
    lastError = GetLastError();
    snapshotState(sym);
    patchState();
    applyCbSet(sym, 1);
    h = logOpen();
    if (h == INVALID_HANDLE_VALUE) { SetLastError(lastError); return; }
    traceSymbol(h, sym);
    writeString(h, "main "); writeHex(h, g_root);
    writeString(h, " cb="); writeSignedDecimal(h, *(short *)(sym+0x6d));
    writeString(h, " phase="); writeDecimal(h, *(unsigned long *)((char *)g_real+0x9f120));
    writeString(h, " key="); writeHex(h, *(unsigned long *)(sym+0x1c));
    writeString(h, "\n"); CloseHandle(h);
    SetLastError(lastError);
}

static unsigned long __cdecl traceSite(unsigned long *regs)
{
    HANDLE h;
    DWORD lastError;
    unsigned char *sym = (unsigned char *)regs[0]; /* saved EDI */
    unsigned char *sp = (unsigned char *)(regs[3]+4); /* before pushfd */
    unsigned long *body = *(unsigned long **)(sp+0x1c);
    char action;
    if (!g_selected) return 0;
    lastError = GetLastError();
    applyCbSet(sym, 0);
    action = g_ruleCount ? forceDecision(*(const char **)((unsigned char *)body[0]+0x18),
                                         *(const char **)(sym+0x18)) : 0;
    h = logOpen();
    if (h == INVALID_HANDLE_VALUE) { SetLastError(lastError); return action == 'E' ? 1 : action == 'K' ? 2 : 0; }
    traceSymbol(h, sym);
    writeString(h, "site root="); writeHex(h, g_root);
    writeString(h, " owner="); writeHex(h, body[0]);
    writeString(h, " callee="); writeHex(h, (unsigned long)sym);
    writeString(h, " cb="); writeSignedDecimal(h, *(short *)(sym+0x6d));
    writeString(h, " budget="); writeSignedDecimal(h, *(long *)(sp+0x48));
    writeString(h, " depth="); writeDecimal(h, *(unsigned long *)(sp+0x34));
    writeString(h, " remain="); writeDecimal(h, *(unsigned long *)(sp+0x30));
    writeString(h, " running="); writeSignedDecimal(h, *(long *)((char *)g_real+0x9f234));
    if (action) { writeString(h, " force="); writeString(h, action == 'E' ? "E" : "K"); }
    writeString(h, "\n"); CloseHandle(h);
    SetLastError(lastError);
    return action == 'E' ? 1 : action == 'K' ? 2 : 0;
}

/* Collector gate at 0x1a418..0x1a427 precedes size/budget checks.
 * Observe its inputs; passing this one gate does not imply admission. */
static void __cdecl traceCandidate(unsigned char *sym)
{
    HANDLE h;
    DWORD lastError;
    unsigned char *body;
    if (!g_selected) return;
    lastError = GetLastError();
    h = logOpen();
    if (h == INVALID_HANDLE_VALUE) { SetLastError(lastError); return; }
    body = *g_currentBodySlot;
    traceSymbol(h, sym);
    writeString(h, "candidate root="); writeHex(h, g_root);
    writeString(h, " callee="); writeHex(h, (unsigned long)sym);
    writeString(h, " body_flags="); writeHex(h, *(unsigned long *)(body+0x34));
    writeString(h, " callee_flags="); writeHex(h, *(unsigned long *)(sym+0x73));
    writeString(h, "\n"); CloseHandle(h);
    SetLastError(lastError);
}

static void __declspec(naked) candidateHook(void)
{
    __asm {
        pushfd
        pushad
        push edi
        call traceCandidate
        add esp, 4
        popad
        popfd
        mov ecx, dword ptr [g_currentBodySlot]
        mov ecx, [ecx]
        jmp dword ptr [g_candidateReturn]
    }
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
        mov dword ptr [g_decision], eax
        popad
        popfd
        mov ax, word ptr [edi+06dh]
        mov esi, [esp+048h]
        cmp dword ptr [g_decision], 1
        je forceExpand
        cmp dword ptr [g_decision], 2
        je forceKeep
        jmp dword ptr [g_siteReturn]
    forceExpand:
        jmp dword ptr [g_expandTarget]
    forceKeep:
        jmp dword ptr [g_keepTarget]
    }
}

/* Tail merging (decision-forcing.md, stage D). Both C2 tail mergers count
 * matching trailing instructions of two blocks and merge when the count is
 * nonzero: 0x36aa0 tests [esp+0x10] at 0x36afa, 0x3e30b tests [esp+0x18]
 * at 0x3e3e0. Each nonzero test in the selected root is decision k and is
 * logged; HOMM3_VC6_MERGE_VETO="k,..." turns those into C2's own no-merge
 * path (count 0). A veto never creates a merge C2 did not find. */
static void *g_mergeReturnA;
static void *g_mergeReturnB;

static unsigned char g_mergeVeto[1024];

static void loadMergeVeto(void)
{
    static char text[8192];
    DWORD n = GetEnvironmentVariableA("HOMM3_VC6_MERGE_VETO", text, sizeof text);
    unsigned long k = 0, i;
    if (!n || n >= sizeof text) return;
    for (i = 0; i <= n; ++i) {
        char c = i < n ? text[i] : ',';
        if (c >= '0' && c <= '9') k = k * 10 + (unsigned long)(c - '0');
        else { if (k && k < sizeof g_mergeVeto) g_mergeVeto[k] = 1; k = 0; }
    }
}

/* regs: pushad layout (EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX). In both
 * mergers EBP and EBX hold the two blocks under comparison at the test. */
static unsigned long __cdecl mergeDecision(unsigned long kind, unsigned long *regs)
{
    HANDLE h;
    DWORD lastError;
    unsigned long count = regs[7], a = regs[2], b = regs[4];
    unsigned i;
    int veto;
    if (!count || !g_selected) return count;
    /* C2 retries a declined pair; a vetoed pair stays declined, so the
     * numbering counts each pair's first decision only. */
    for (i = 0; i < g_vetoPairCount; ++i)
        if (g_vetoPairs[i].a == a && g_vetoPairs[i].b == b) return 0;
    lastError = GetLastError();
    ++g_mergeIndex;
    veto = g_mergeIndex < sizeof g_mergeVeto && g_mergeVeto[g_mergeIndex];
    if (veto && g_vetoPairCount < 1024) {
        g_vetoPairs[g_vetoPairCount].a = a;
        g_vetoPairs[g_vetoPairCount].b = b;
        ++g_vetoPairCount;
    }
    h = logOpen();
    if (h != INVALID_HANDLE_VALUE) {
        writeString(h, "merge root="); writeHex(h, g_root);
        writeString(h, " k="); writeDecimal(h, g_mergeIndex);
        writeString(h, " kind="); writeDecimal(h, kind);
        writeString(h, " count="); writeDecimal(h, count);
        if (veto) writeString(h, " veto=1");
        writeString(h, "\n"); CloseHandle(h);
    }
    SetLastError(lastError);
    return veto ? 0 : count;
}

static void __declspec(naked) mergeHookA(void)
{
    __asm {
        mov eax, [esp+10h]
        pushad
        mov ecx, esp
        push ecx
        push 1
        call mergeDecision
        add esp, 8
        mov [esp+1Ch], eax
        popad
        test eax, eax
        jmp dword ptr [g_mergeReturnA]
    }
}

static void __declspec(naked) mergeHookB(void)
{
    __asm {
        mov eax, [esp+18h]
        pushad
        mov ecx, esp
        push ecx
        push 2
        call mergeDecision
        add esp, 8
        mov [esp+1Ch], eax
        popad
        test eax, eax
        jmp dword ptr [g_mergeReturnB]
    }
}

/* Symbol-hash placement channel (context-variants.md). C2 files back-end
 * symbols in 1024 buckets by key (sym+0x1c) & 0x3ff: insert 0x21267,
 * lookup 0x232ee, unlink 0x213dc; bucket-ordered walks (0x2450d, 0x2df43)
 * enumerate them. k declarations placed before a definition raise every
 * later key by k. HOMM3_VC6_HASH_SHIFT="h0:k" files every key >= h0 as
 * key + k (all three sites agree), which is that placement; the keys
 * themselves and their order are unchanged. HOMM3_VC6_KEY_LOG logs the
 * keys inserted while the selected root is current. */
static unsigned long g_shiftH0, g_shiftK;
static int g_keyLog;
static void *g_lookupReturn, *g_insertReturn, *g_unlinkReturn;

static void loadHashShift(void)
{
    char text[64];
    DWORD n = GetEnvironmentVariableA("HOMM3_VC6_HASH_SHIFT", text, sizeof text);
    unsigned long a = 0, b = 0, i;
    int second = 0;
    for (i = 0; i < n && i < sizeof text; ++i) {
        char c = text[i];
        unsigned long d = (c >= '0' && c <= '9') ? (unsigned long)(c - '0')
            : (c >= 'a' && c <= 'f') ? (unsigned long)(c - 'a' + 10) : 99;
        if (c == ':') { second = 1; continue; }
        if (d == 99) continue;
        if (second) b = b * 10 + d; else a = a * 16 + d;  /* h0 hex, k decimal */
    }
    if (n && second) { g_shiftH0 = a; g_shiftK = b; }
    g_keyLog = GetEnvironmentVariableA("HOMM3_VC6_KEY_LOG", text, sizeof text) != 0;
}

static unsigned long __cdecl shiftKey(unsigned long key, unsigned long site)
{
    if (g_keyLog && site == 1 && g_selected) {
        DWORD lastError = GetLastError();
        HANDLE h = logOpen();
        if (h != INVALID_HANDLE_VALUE) {
            writeString(h, "key "); writeHex(h, key); writeString(h, "\n"); CloseHandle(h);
        }
        SetLastError(lastError);
    }
    if (g_shiftK && key >= g_shiftH0) key += g_shiftK;
    return key & 0x3ff;
}

static void __declspec(naked) lookupHook(void)
{
    __asm {
        push ecx
        push edx
        push 0
        push eax
        call shiftKey
        add esp, 8
        pop edx
        pop ecx
        jmp dword ptr [g_lookupReturn]
    }
}

static void __declspec(naked) insertHook(void)
{
    __asm {
        push ecx
        push edx
        push 1
        push eax
        call shiftKey
        add esp, 8
        pop edx
        pop ecx
        jmp dword ptr [g_insertReturn]
    }
}

static void __declspec(naked) unlinkHook(void)
{
    __asm {
        push eax
        push ecx
        push edx
        push 2
        push edi
        call shiftKey
        add esp, 8
        mov edi, eax
        pop edx
        pop ecx
        pop eax
        jmp dword ptr [g_unlinkReturn]
    }
}

/* Function-entry coverage, used to locate passes (decision-forcing.md,
 * stage D). HOMM3_VC6_COVER names a file of ascending hexadecimal C2 RVAs,
 * one per line. Each gets an INT3; a vectored handler counts the hit,
 * restores the byte, single-steps it and re-arms it, until COVER_CAP hits
 * (then the original byte stays, bounding the cost). After the pass every
 * hit entry is logged as "cover <rva> <count>". Inactive without the file. */
#define COVER_MAX 4096
#define COVER_CAP 4000
static unsigned long g_coverRva[COVER_MAX];
static unsigned long g_coverHits[COVER_MAX];
static unsigned char g_coverByte[COVER_MAX];
static unsigned g_coverCount;
static unsigned char *g_coverRearm;

static int coverFind(unsigned long rva)
{
    int lo = 0, hi = (int)g_coverCount - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (g_coverRva[mid] == rva) return mid;
        if (g_coverRva[mid] < rva) lo = mid + 1; else hi = mid - 1;
    }
    return -1;
}

static LONG __stdcall coverHandler(EXCEPTION_POINTERS *info)
{
    EXCEPTION_RECORD *record = info->ExceptionRecord;
    CONTEXT *context = info->ContextRecord;
    if (record->ExceptionCode == EXCEPTION_BREAKPOINT) {
        unsigned char *at = (unsigned char *)record->ExceptionAddress;
        int k = coverFind((unsigned long)(at - (unsigned char *)g_real));
        if (k < 0) {
            --at;
            k = coverFind((unsigned long)(at - (unsigned char *)g_real));
            if (k < 0) return EXCEPTION_CONTINUE_SEARCH;
        }
        at[0] = g_coverByte[k];
        ++g_coverHits[k];
        context->Eip = (DWORD)at;
        if (g_coverHits[k] < COVER_CAP) {
            g_coverRearm = at;
            context->EFlags |= 0x100;
        }
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (record->ExceptionCode == EXCEPTION_SINGLE_STEP && g_coverRearm) {
        g_coverRearm[0] = 0xcc;
        g_coverRearm = 0;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

typedef PVOID (__stdcall *addHandlerFunction)(ULONG, PVOID);

static void loadCover(void)
{
    static char text[65536];
    char path[MAX_PATH];
    HANDLE h;
    DWORD n = 0, old;
    unsigned i, low = 0xffffffff, high = 0;
    unsigned long value = 0;
    int digits = 0;
    addHandlerFunction add;
    unsigned char *code = (unsigned char *)g_real;
    if (!GetEnvironmentVariableA("HOMM3_VC6_COVER", path, sizeof path)) return;
    h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (h == INVALID_HANDLE_VALUE) return;
    ReadFile(h, text, sizeof text - 1, &n, 0);
    CloseHandle(h);
    for (i = 0; i <= n; ++i) {
        char c = i < n ? text[i] : '\n';
        if (c >= '0' && c <= '9') { value = value*16 + (c-'0'); ++digits; }
        else if (c >= 'a' && c <= 'f') { value = value*16 + (c-'a'+10); ++digits; }
        else if (c == 'x' || c == 'X') { value = 0; digits = 0; }
        else {
            if (digits && g_coverCount < COVER_MAX
                && (!g_coverCount || value > g_coverRva[g_coverCount-1]))
                g_coverRva[g_coverCount++] = value;
            value = 0; digits = 0;
        }
    }
    if (!g_coverCount) return;
    add = (addHandlerFunction)GetProcAddress(GetModuleHandleA("kernel32.dll"),
        "AddVectoredExceptionHandler");
    if (!add || !add(1, (PVOID)coverHandler)) { g_coverCount = 0; return; }
    low = g_coverRva[0];
    high = g_coverRva[g_coverCount-1] + 1;
    VirtualProtect(code+low, high-low, PAGE_EXECUTE_READWRITE, &old);
    for (i = 0; i < g_coverCount; ++i) {
        g_coverByte[i] = code[g_coverRva[i]];
        code[g_coverRva[i]] = 0xcc;
    }
    FlushInstructionCache(GetCurrentProcess(), code+low, high-low);
}

static void writeCover(void)
{
    HANDLE h;
    unsigned i;
    if (!g_coverCount) return;
    h = logOpen();
    if (h == INVALID_HANDLE_VALUE) return;
    for (i = 0; i < g_coverCount; ++i) {
        if (!g_coverHits[i]) continue;
        writeString(h, "cover "); writeHex(h, g_coverRva[i]);
        writeString(h, " "); writeDecimal(h, g_coverHits[i]);
        writeString(h, "\n");
    }
    CloseHandle(h);
}

static int installInlineTrace(void)
{
    static int installed;
    static const unsigned char mainBytes[] = {0x8b,0x06,0x0f,0xbf,0x40,0x6d};
    static const unsigned char siteBytes[] = {0x66,0x8b,0x47,0x6d,0x8b,0x74,0x24,0x48};
    unsigned char candidateBytes[] = {0x8b,0x0d,0,0,0,0};
    static const unsigned char colorBytes[] = {0x8d,0x04,0xfd,0,0,0,0,0x2b,0xc7};
    static const unsigned char mergeBytesA[] = {0x8b,0x44,0x24,0x10,0x85,0xc0,0x77,0x47};
    static const unsigned char mergeBytesB[] = {0x8b,0x44,0x24,0x18,0x85,0xc0,0xc7,0x02};
    if (installed) return 1;
    if (GetEnvironmentVariableA("HOMM3_VC6_INLINE_TRACE", g_filter,
        sizeof g_filter) >= sizeof g_filter) return 0;
    g_currentBodySlot = (unsigned char **)((char *)g_real+0xac380);
    *(unsigned long *)(candidateBytes+2) = (unsigned long)g_currentBodySlot;
    g_candidateReturn = (char *)g_real+0x1a418;
    g_mainReturn = (char *)g_real+0x19962;
    g_siteReturn = (char *)g_real+0x19f94;
    g_expandTarget = (char *)g_real+0x19faf;
    g_keepTarget = (char *)g_real+0x19a94;
    loadForceRules();
    loadRegisterForce();
    g_setHas = (setHasFunction)((char *)g_real+0x19b5);
    g_colorReturn = (char *)g_real+0x24751;
    if (!patchHook(0x1995c, mainHook, mainBytes, sizeof mainBytes)) return 0;
    if (!patchHook(0x19f8c, siteHook, siteBytes, sizeof siteBytes)) return 0;
    if (!patchHook(0x1a412, candidateHook, candidateBytes, sizeof candidateBytes)) return 0;
    if (!patchHook(0x24748, colorHook, colorBytes, sizeof colorBytes)) return 0;
    loadMergeVeto();
    loadHashShift();
    if (g_shiftK || g_keyLog) {
        static const unsigned char lookupBytes[] = {0x25,0xff,0x03,0x00,0x00};
        static const unsigned char insertBytes[] = {0x25,0xff,0x03,0x00,0x00};
        static const unsigned char unlinkBytes[] = {0x81,0xe7,0xff,0x03,0x00,0x00};
        g_lookupReturn = (char *)g_real+0x232f3;
        g_insertReturn = (char *)g_real+0x2126c;
        g_unlinkReturn = (char *)g_real+0x213e2;
        if (!patchHook(0x232ee, lookupHook, lookupBytes, 5)) return 0;
        if (!patchHook(0x21267, insertHook, insertBytes, 5)) return 0;
        if (!patchHook(0x213dc, unlinkHook, unlinkBytes, 6)) return 0;
    }
    g_mergeReturnA = (char *)g_real+0x36b00;
    g_mergeReturnB = (char *)g_real+0x3e3e6;
    if (!patchHook(0x36afa, mergeHookA, mergeBytesA, 6)) return 0;
    if (!patchHook(0x3e3e0, mergeHookB, mergeBytesB, 6)) return 0;
    loadCover();
    installed = 1;
    return 1;
}
