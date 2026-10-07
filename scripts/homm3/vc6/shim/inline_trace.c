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
static void rearmCover(void);

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

/* HOMM3_VC6_HASH_DUMP: at the selected main, log every symbol filed in the
 * back-end hash (buckets .bssbe 0x9d88c, key sym+0x1c, chain +0x2c). */
static void dumpHash(void)
{
    char flag[8];
    HANDLE h;
    unsigned b;
    unsigned char **buckets = (unsigned char **)((char *)g_real + 0x9d88c);
    if (!GetEnvironmentVariableA("HOMM3_VC6_HASH_DUMP", flag, sizeof flag)) return;
    h = logOpen();
    if (h == INVALID_HANDLE_VALUE) return;
    for (b = 0; b < 1024; ++b) {
        unsigned char *sym = buckets[b];
        unsigned depth = 0;
        while (sym && depth < 4096) {
            writeString(h, "hash "); writeDecimal(h, b);
            writeString(h, " "); writeHex(h, *(unsigned long *)(sym + 0x1c));
            writeString(h, " "); writeHex(h, (unsigned long)sym);
            writeString(h, "\n");
            sym = *(unsigned char **)(sym + 0x2c);
            ++depth;
        }
    }
    CloseHandle(h);
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
    rearmCover();
    dumpHash();
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

/* Compiled-callee state (unstable-state.md): a callee record's +0x14 bit
 * 0x800 is set once C2 has compiled the callee's own body. An inlined body
 * expands differently before and after that (its EH states, for one).
 * HOMM3_VC6_COMPILED=0|1 forces the bit for every callee at every budget
 * test, the order in which every callee body came first or last. */
static long g_compiledValue = -2;

/* Per-root form: HOMM3_VC6_COMPILED_SPEC names a file of lines
 * "root\tcallee\tcallee..." (callees to treat as compiled while that root is
 * being compiled; every other callee of a listed root is treated as not
 * yet compiled). Roots without a line keep C2's own bits. */
#define SPEC_BUCKETS 4096
typedef struct SpecRoot { struct SpecRoot *next; char *root; char **callees; unsigned count; } SpecRoot;
static SpecRoot *g_spec[SPEC_BUCKETS];
static int g_specLoaded;
static SpecRoot *g_specCurrent;
static unsigned long g_specRoot;

static unsigned long nameHash(const char *s)
{
    unsigned long h = 5381;
    while (*s) h = h * 33 + (unsigned char)*s++;
    return h % SPEC_BUCKETS;
}

static void loadCompiledSpec(void)
{
    char path[MAX_PATH];
    HANDLE f;
    DWORD size, n;
    char *text, *line, *end;
    g_specLoaded = 1;
    if (!GetEnvironmentVariableA("HOMM3_VC6_COMPILED_SPEC", path, sizeof path)) return;
    f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (f == INVALID_HANDLE_VALUE) return;
    size = GetFileSize(f, 0);
    text = (char *)LocalAlloc(LMEM_FIXED, size + 1);
    ReadFile(f, text, size, &n, 0);
    CloseHandle(f);
    text[n] = 0;
    for (line = text; *line; line = end) {
        SpecRoot *r;
        unsigned count = 0, i;
        char *p;
        for (end = line; *end && *end != '\n'; ++end) if (*end == '\t') ++count;
        if (*end) *end++ = 0;
        if (!*line) continue;
        r = (SpecRoot *)LocalAlloc(LMEM_FIXED, sizeof *r);
        r->callees = (char **)LocalAlloc(LMEM_FIXED, sizeof(char *) * (count + 1));
        r->count = 0;
        r->root = line;
        for (p = line, i = 0; *p; ++p) {
            if (*p == '\t') { *p = 0; r->callees[r->count++] = p + 1; }
        }
        r->next = g_spec[nameHash(r->root)];
        g_spec[nameHash(r->root)] = r;
    }
}

static SpecRoot *specFor(unsigned long root)
{
    SpecRoot *r;
    const char *name;
    if (root == g_specRoot) return g_specCurrent;
    g_specRoot = root;
    g_specCurrent = 0;
    if (!root) return 0;
    name = *(const char **)((unsigned char *)root + 0x18);
    for (r = g_spec[nameHash(name)]; r; r = r->next)
        if (lstrcmpA(r->root, name) == 0) { g_specCurrent = r; break; }
    return g_specCurrent;
}

static void applyCompiled(unsigned char *sym)
{
    SpecRoot *r;
    if (!g_specLoaded) loadCompiledSpec();
    r = specFor(g_root);
    if (r) {
        unsigned i;
        const char *name = *(const char **)(sym + 0x18);
        int compiled = 0;
        for (i = 0; i < r->count; ++i) if (lstrcmpA(r->callees[i], name) == 0) { compiled = 1; break; }
        if (compiled) *(unsigned long *)(sym + 0x14) |= 0x800;
        else *(unsigned long *)(sym + 0x14) &= ~0x800ul;
        return;
    }
    if (g_compiledValue == -2) {
        char text[8];
        DWORD n = GetEnvironmentVariableA("HOMM3_VC6_COMPILED", text, sizeof text);
        g_compiledValue = (n && n < sizeof text) ? (text[0] == '1') : -1;
    }
    if (g_compiledValue == 1) *(unsigned long *)(sym + 0x14) |= 0x800;
    else if (g_compiledValue == 0) *(unsigned long *)(sym + 0x14) &= ~0x800ul;
}

static unsigned long __cdecl traceSite(unsigned long *regs)
{
    HANDLE h;
    DWORD lastError;
    unsigned char *sym = (unsigned char *)regs[0]; /* saved EDI */
    unsigned char *sp = (unsigned char *)(regs[3]+4); /* before pushfd */
    unsigned long *body = *(unsigned long **)(sp+0x1c);
    char action;
    applyCompiled(sym);
    if (!g_selected) return 0;
    lastError = GetLastError();
    applyCbSet(sym, 0);
    {
        /* Diagnostic: HOMM3_VC6_SITE_OR="name:offhex:maskhex" ORs a mask into
         * the callee record at its budget test. */
        char spec[200];
        DWORD n = GetEnvironmentVariableA("HOMM3_VC6_SITE_OR", spec, sizeof spec);
        if (n && n < sizeof spec) {
            char *a = spec, *b = 0, *c = 0;
            unsigned long off = 0, mask = 0;
            DWORD i;
            for (i = 0; i < n; ++i) if (spec[i] == ':') { spec[i] = 0; if (!b) b = spec + i + 1; else c = spec + i + 1; }
            if (b && c && contains(*(const char **)(sym+0x18), a)) {
                for (; *b; ++b) off = off * 16 + (*b <= '9' ? *b - '0' : (*b | 32) - 'a' + 10);
                for (; *c; ++c) mask = mask * 16 + (*c <= '9' ? *c - '0' : (*c | 32) - 'a' + 10);
                *(unsigned long *)(sym + off) |= mask;
            }
        }
    }
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
    {
        char want[160];
        if (GetEnvironmentVariableA("HOMM3_VC6_SITE_DUMP", want, sizeof want)
                && contains(*(const char **)(sym+0x18), want)) {
            unsigned i;
            writeString(h, " record=");
            for (i = 0; i < 0x80; i += 4) { writeHex(h, *(unsigned long *)(sym + i)); writeString(h, ","); }
        }
    }
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

static unsigned long __cdecl shiftKey(unsigned long key, unsigned long site, unsigned char *sym)
{
    if (g_keyLog && site == 1 && g_selected) {
        DWORD lastError = GetLastError();
        HANDLE h = logOpen();
        if (h != INVALID_HANDLE_VALUE) {
            unsigned i;
            writeString(h, "key "); writeHex(h, key);
            for (i = 0; i < 0x40; i += 4) { writeString(h, " "); writeHex(h, *(unsigned long *)(sym + i)); }
            writeString(h, "\n"); CloseHandle(h);
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
        push 0
        push eax
        call shiftKey
        add esp, 12
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
        push ecx
        push 1
        push eax
        call shiftKey
        add esp, 12
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
        push 0
        push 2
        push edi
        call shiftKey
        add esp, 12
        mov edi, eax
        pop edx
        pop ecx
        pop eax
        jmp dword ptr [g_unlinkReturn]
    }
}

/* Declaration-offset state (unstable-state.md). The IL reader 0x1c92e
 * decodes every symbol handle (u16, or a 31-bit value when bit 15 is set).
 * HOMM3_VC6_HANDLE_SHIFT="h0:k" returns handle + k for every decoded handle
 * >= h0 (hex h0, decimal k): the numbering k extra declarations before
 * handle h0 would have produced. */
static unsigned long g_handleH0, g_handleK;
static void *g_decodeContinue;
static unsigned long *g_decodeFlag;

#define DECODE_SITES 64
static unsigned long g_decodeSite[DECODE_SITES], g_decodeCount[DECODE_SITES],
    g_decodeMin[DECODE_SITES], g_decodeMax[DECODE_SITES];
static int g_decodeLog;

/* Decoder call sites whose values are not symbol handles: measured by
 * comparing real captures with and without leading declarations. */
static int isHandleSite(unsigned long ret)
{
    unsigned long site = ret - (unsigned long)g_real;
    return site != 0x1cedf && site != 0x1d193 && site != 0x1d253;
}

/* scratch: HOMM3_VC6_HANDLE_SET="h1,h2,...:delta[:invert]" (hex handles, decimal delta) */
static unsigned long g_setH[256], g_setN, g_setDelta;
static int g_setInvert;
static void loadHandleSet(void)
{
    static char t[4096];
    DWORD n = GetEnvironmentVariableA("HOMM3_VC6_HANDLE_SET", t, sizeof t);
    unsigned long v = 0, i, part = 0;
    if (!n || n >= sizeof t) return;
    for (i = 0; i <= n; ++i) {
        char c = i < n ? t[i] : 0;
        int d = (c >= '0' && c <= '9') ? c - '0' : (c >= 'a' && c <= 'f') ? c - 'a' + 10 : -1;
        if (part == 0) {
            if (d >= 0) v = v * 16 + d;
            else { if (v && g_setN < 256) g_setH[g_setN++] = v; v = 0; if (c == ':') part = 1; }
        } else if (part == 1) {
            if (c >= '0' && c <= '9') g_setDelta = g_setDelta * 10 + (c - '0');
            else if (c == ':') part = 2;
        } else if (c == '1') g_setInvert = 1;
    }
}
static unsigned long __cdecl adjustHandle(unsigned long value, unsigned long ret)
{
    if (g_decodeLog == 2) {
        static HANDLE seq = INVALID_HANDLE_VALUE;
        unsigned long pair[2];
        DWORD n;
        if (seq == INVALID_HANDLE_VALUE) {
            char path[MAX_PATH];
            if (GetEnvironmentVariableA("HOMM3_VC6_DECODE_SEQ", path, sizeof path))
                seq = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
        }
        pair[0] = ret - (unsigned long)g_real; pair[1] = value;
        if (seq != INVALID_HANDLE_VALUE) WriteFile(seq, pair, 8, &n, 0);
    }
    if (g_decodeLog) {
        unsigned i;
        unsigned long site = ret - (unsigned long)g_real;
        for (i = 0; i < DECODE_SITES && g_decodeSite[i] && g_decodeSite[i] != site; ++i) {}
        if (i < DECODE_SITES) {
            if (!g_decodeSite[i]) { g_decodeSite[i] = site; g_decodeMin[i] = value; }
            ++g_decodeCount[i];
            if (value < g_decodeMin[i]) g_decodeMin[i] = value;
            if (value > g_decodeMax[i]) g_decodeMax[i] = value;
        }
    }
    if (g_handleK && value && value >= g_handleH0 && isHandleSite(ret)) value += g_handleK;
    if (g_setN && value && isHandleSite(ret)) {
        unsigned i;
        int inSet = 0;
        for (i = 0; i < g_setN; ++i) if (g_setH[i] == value) inSet = 1;
        if (inSet != g_setInvert) value += g_setDelta;
    }
    return value;
}

static void writeDecodeLog(void)
{
    HANDLE h;
    unsigned i;
    if (!g_decodeLog) return;
    h = logOpen();
    if (h == INVALID_HANDLE_VALUE) return;
    for (i = 0; i < DECODE_SITES && g_decodeSite[i]; ++i) {
        writeString(h, "decode "); writeHex(h, g_decodeSite[i]);
        writeString(h, " n="); writeDecimal(h, g_decodeCount[i]);
        writeString(h, " min="); writeHex(h, g_decodeMin[i]);
        writeString(h, " max="); writeHex(h, g_decodeMax[i]);
        writeString(h, "\n");
    }
    CloseHandle(h);
}

static void __declspec(naked) decodeTrampoline(void)
{
    __asm {
        sub esp, 8
        mov ecx, dword ptr [g_decodeFlag]
        mov ecx, dword ptr [ecx]
        jmp dword ptr [g_decodeContinue]
    }
}

static void __declspec(naked) decodeHook(void)
{
    __asm {
        call decodeTrampoline
        push ecx
        push edx
        push dword ptr [esp+8]
        push eax
        call adjustHandle
        add esp, 8
        pop edx
        pop ecx
        ret
    }
}

static int loadHandleShift(void)
{
    char text[64];
    DWORD n = GetEnvironmentVariableA("HOMM3_VC6_HANDLE_SHIFT", text, sizeof text);
    unsigned long a = 0, b = 0, i;
    int second = 0;
    static const unsigned char entryBytes[] = {0x83,0xec,0x08,0x8b,0x0d};
    g_decodeLog = GetEnvironmentVariableA("HOMM3_VC6_DECODE_LOG", text + 32, 8) != 0;
    if (GetEnvironmentVariableA("HOMM3_VC6_DECODE_SEQ", text + 40, 8)) g_decodeLog = 2;
    if (!g_decodeLog && !g_setN && (!n || n >= sizeof text)) return 1;
    if (n >= 32) n = 0;
    for (i = 0; i < n; ++i) {
        char c = text[i];
        if (c == ':') { second = 1; continue; }
        if (second) { if (c >= '0' && c <= '9') b = b * 10 + (c - '0'); }
        else if (c >= '0' && c <= '9') a = a * 16 + (c - '0');
        else if (c >= 'a' && c <= 'f') a = a * 16 + (c - 'a' + 10);
    }
    g_handleH0 = a; g_handleK = b;
    g_decodeFlag = *(unsigned long **)((char *)g_real + 0x1c933);
    g_decodeContinue = (char *)g_real + 0x1c937;
    return patchHook(0x1c92e, decodeHook, entryBytes, 5);
}

/* Phase-flag state (unstable-state.md). 0x5b11 in 0x5739 reads the flag
 * 0x9f120 that the previous function's driver (0x13615) left behind.
 * HOMM3_VC6_PHASE=0|1 makes every read see that value. HOMM3_VC6_STATE_LOG
 * logs each read with the current inliner root. */
static long g_phaseValue = -1;
static int g_stateLog;
static void *g_phaseReturn;

static unsigned long g_readIndex;
static char g_readForce[512];
static unsigned long __cdecl phaseRead(unsigned long value, unsigned long esi, unsigned long ebx, unsigned long ret, unsigned long ebp, unsigned long blk)
{
    unsigned long forced = value;
    ++g_readIndex;
    if (g_readForce[0]) {
        /* comma list of k=v; k is the 1-based read index in the whole run */
        const char *p = g_readForce;
        while (*p) {
            unsigned long k = 0, v = 0;
            while (*p >= '0' && *p <= '9') k = k * 10 + (unsigned long)(*p++ - '0');
            if (*p == '=') { ++p; while (*p >= '0' && *p <= '9') v = v * 10 + (unsigned long)(*p++ - '0'); }
            if (k == g_readIndex) forced = v;
            while (*p && *p != ',') ++p;
            if (*p == ',') ++p;
        }
    }
    if (g_stateLog) {
        DWORD lastError = GetLastError();
        HANDLE h = logOpen();
        if (h != INVALID_HANDLE_VALUE) {
            writeString(h, "phase-read k="); writeDecimal(h, g_readIndex);
            writeString(h, " root="); writeHex(h, g_root);
            writeString(h, " value="); writeDecimal(h, value);
            writeString(h, " used="); writeDecimal(h, forced);
            writeString(h, " stage="); writeDecimal(h, *(unsigned long *)((char *)g_real + 0x9f114));
            writeString(h, " ret="); writeHex(h, ret - (unsigned long)g_real);
            writeString(h, " tup="); writeHex(h, esi);
            writeString(h, " op="); writeHex(h, *(unsigned long *)(esi + 4));
            writeString(h, " ty="); writeHex(h, *(unsigned short *)(esi + 0xa));
            writeString(h, " def="); writeHex(h, ebx);
            writeString(h, " defop="); writeHex(h, *(unsigned long *)(ebx + 4));
            writeString(h, " defkind="); writeHex(h, *(unsigned char *)(ebx + 8));
            writeString(h, " defa="); writeHex(h, *(unsigned char *)(ebx + 0xa));
            writeString(h, " ebp="); writeHex(h, ebp);
            {
                unsigned char *o1 = *(unsigned char **)(esi + 0x18);
                unsigned char *o2 = o1 ? *(unsigned char **)o1 : 0;
                writeString(h, " o1k="); writeHex(h, o1 ? o1[8] : 0xff);
                if (o1 && o1[8] == 7) { writeString(h, " o1v="); writeHex(h, *(unsigned long *)(o1 + 0x18)); }
                if (o1 && (o1[8] == 1 || o1[8] == 2 || o1[8] == 3)) { writeString(h, " o1sym="); writeHex(h, *(unsigned long *)(o1 + 0x14)); }
                writeString(h, " o2k="); writeHex(h, o2 ? o2[8] : 0xff);
                if (o2 && o2[8] == 7) { writeString(h, " o2v="); writeHex(h, *(unsigned long *)(o2 + 0x18)); }
                if (o2 && (o2[8] == 1 || o2[8] == 2 || o2[8] == 3)) { writeString(h, " o2sym="); writeHex(h, *(unsigned long *)(o2 + 0x14)); }
                writeString(h, " blk="); writeHex(h, blk);
                writeString(h, " blkno="); writeDecimal(h, (unsigned long)(long)*(short *)(blk + 0x6c));
            }
            writeString(h, "\n"); CloseHandle(h);
        }
        SetLastError(lastError);
    }
    return forced;
}

static void __declspec(naked) phaseHook(void)
{
    __asm {
        push ecx
        push edx
        mov eax, dword ptr [esp + 0x18 + 8]
        push eax
        push ebp
        mov eax, dword ptr [esp + 0x34 + 16]
        push eax
        push ebx
        push esi
        mov eax, dword ptr [g_real]
        mov eax, dword ptr [eax + 0x9f120]
        push eax
        call phaseRead
        add esp, 24
        pop edx
        pop ecx
        jmp dword ptr [g_phaseReturn]
    }
}

/* The driver 0x13615 has one caller; its return lands at 0x683cc. Setting
 * the flag there is the leftover value a different previous function would
 * have left for the next one (and install sets it for the first one). */
static void *g_driverReturn;

static void __cdecl driverDone(unsigned long *regs)
{
    /* pushad layout: EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX */
    if (g_stateLog) {
        DWORD lastError = GetLastError();
        HANDLE h = logOpen();
        unsigned long *body = (unsigned long *)regs[1];
        if (h != INVALID_HANDLE_VALUE) {
            unsigned i;
            unsigned char *sym = (unsigned char *)body[0];
            (void)i;
            writeString(h, "driver left="); writeDecimal(h, *(unsigned long *)((char *)g_real + 0x9f120));
            writeString(h, " base="); writeHex(h, *(unsigned long *)(sym + 0x28));
            writeString(h, " name="); writeString(h, *(const char **)(sym + 0x18));
            writeString(h, "\n"); CloseHandle(h);
        }
        SetLastError(lastError);
    }
    if (g_phaseValue >= 0)
        *(unsigned long *)((char *)g_real + 0x9f120) = (unsigned long)g_phaseValue;
}

static void __declspec(naked) driverHook(void)
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push eax
        call driverDone
        add esp, 4
        popad
        popfd
        mov ecx, esi
        push eax
        mov eax, dword ptr [g_real]
        mov word ptr [eax + 0xac360], di
        pop eax
        jmp dword ptr [g_driverReturn]
    }
}


/* scratch: driver checkpoints (phase-flag RE). tag 0 = entry (body in ECX),
 * 1/2/3 = the three flag writes (body in EBP). */
static void *g_ckReturn[4];
static void __cdecl checkpoint(unsigned long *regs, unsigned long tag)
{
    /* pushad layout: EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX */
    DWORD lastError = GetLastError();
    HANDLE h;
    unsigned char *body = (unsigned char *)(tag == 0 ? regs[6] : regs[2]);
    unsigned char *b;
    unsigned long n = 0, dead = 0;
    if (tag == 0) {
        char t[8];
        if (GetEnvironmentVariableA("HOMM3_VC6_SIBLING_ZERO", t, sizeof t)) {
            *(unsigned long *)((char *)g_real + 0x9f0e8) = 0;
            *(unsigned long *)((char *)g_real + 0x9f104) = 0;
        }
        if (GetEnvironmentVariableA("HOMM3_VC6_SIBLING_ONE", t, sizeof t)) {
            *(unsigned long *)((char *)g_real + 0x9f0fc) = 1;
            *(unsigned long *)((char *)g_real + 0x9f118) = 1;
        }
    }
    if (!g_stateLog) return;
    h = logOpen();
    if (h != INVALID_HANDLE_VALUE) {
        for (b = **(unsigned char ***)(body + 8); b; b = *(unsigned char **)b) {
            ++n;
            if (*(unsigned long *)(b + 0x18) & 0x2000000) ++dead;
        }
        writeString(h, "ck tag="); writeDecimal(h, tag);
        writeString(h, " flag="); writeDecimal(h, *(unsigned long *)((char *)g_real + 0x9f120));
        writeString(h, " blocks="); writeDecimal(h, n);
        writeString(h, " deadmark="); writeDecimal(h, dead);
        writeString(h, " loops="); writeHex(h, *(unsigned long *)(body + 0xc));
        writeString(h, " cfgdirty="); writeDecimal(h, *(unsigned long *)((char *)g_real + 0x9f0e0));
        writeString(h, " name="); writeString(h, *(const char **)(*(unsigned char **)body + 0x18));
        writeString(h, "\n"); CloseHandle(h);
    }
    SetLastError(lastError);
}
static void __declspec(naked) ck0(void)
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push 0
        push eax
        call checkpoint
        add esp, 8
        popad
        popfd
        sub esp, 0x14
        push ebx
        push ebp
        jmp dword ptr [g_ckReturn]
    }
}
static void __declspec(naked) ck1(void)
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push 1
        push eax
        call checkpoint
        add esp, 8
        popad
        popfd
        push eax
        mov eax, dword ptr [g_real]
        mov dword ptr [eax + 0x9f120], esi
        pop eax
        jmp dword ptr [g_ckReturn + 4]
    }
}
static void __declspec(naked) ck2(void)
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push 2
        push eax
        call checkpoint
        add esp, 8
        popad
        popfd
        push eax
        mov eax, dword ptr [g_real]
        mov dword ptr [eax + 0x9f120], edi
        pop eax
        jmp dword ptr [g_ckReturn + 8]
    }
}
static void __declspec(naked) ck3(void)
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push 3
        push eax
        call checkpoint
        add esp, 8
        popad
        popfd
        push eax
        mov eax, dword ptr [g_real]
        mov dword ptr [eax + 0x9f120], ebx
        pop eax
        jmp dword ptr [g_ckReturn + 12]
    }
}
static int installCheckpoints(void)
{
    unsigned char e0[5] = {0x83, 0xec, 0x14, 0x53, 0x55};
    unsigned char w[6];
    unsigned long flag = (unsigned long)g_real + 0x9f120;
    g_ckReturn[0] = (char *)g_real + 0x1361a;
    g_ckReturn[1] = (char *)g_real + 0x136fc;
    g_ckReturn[2] = (char *)g_real + 0x1391c;
    g_ckReturn[3] = (char *)g_real + 0x13994;
    if (!patchHook(0x13615, ck0, e0, 5)) return 0;
    w[0] = 0x89; *(unsigned long *)(w + 2) = flag;
    w[1] = 0x35; if (!patchHook(0x136f6, ck1, w, 6)) return 0;
    w[1] = 0x3d; if (!patchHook(0x13916, ck2, w, 6)) return 0;
    w[1] = 0x1d; if (!patchHook(0x1398e, ck3, w, 6)) return 0;
    return 1;
}


/* scratch (handle-period RE): HOMM3_VC6_PATCH="rva=hexbytes;..." writes raw
 * bytes into the loaded C2 image before compilation (counterfactual probes). */
static int hexv(char c)
{
    return (c >= '0' && c <= '9') ? c - '0' : (c >= 'a' && c <= 'f') ? c - 'a' + 10
        : (c >= 'A' && c <= 'F') ? c - 'A' + 10 : -1;
}
static void loadBytePatches(void)
{
    static char text[2048];
    DWORD n = GetEnvironmentVariableA("HOMM3_VC6_PATCH", text, sizeof text);
    unsigned i = 0;
    if (!n || n >= sizeof text) return;
    while (i < n) {
        unsigned long rva = 0;
        unsigned char buf[256];
        unsigned len = 0;
        DWORD old, ignored;
        while (i < n && text[i] != '=') { if (hexv(text[i]) >= 0 && !(text[i] == 'x' )) rva = rva * 16 + hexv(text[i]); ++i; }
        ++i;
        while (i + 1 < n + 1 && text[i] && text[i] != ';' && len < sizeof buf) {
            int a = hexv(text[i]), b = hexv(text[i + 1]);
            if (a < 0 || b < 0) break;
            buf[len++] = (unsigned char)(a * 16 + b);
            i += 2;
        }
        while (i < n && text[i] != ';') ++i;
        ++i;
        if (!len || !rva) continue;
        if (VirtualProtect((char *)g_real + rva, len, PAGE_EXECUTE_READWRITE, &old)) {
            unsigned j;
            for (j = 0; j < len; ++j) ((unsigned char *)g_real)[rva + j] = buf[j];
            VirtualProtect((char *)g_real + rva, len, old, &ignored);
            FlushInstructionCache(GetCurrentProcess(), (char *)g_real + rva, len);
        }
    }
}


/* scratch (handle-period RE): HOMM3_VC6_TUPLE_DUMP=<winpath> appends, at
 * every phaseCheckpoint (0x9ab4) while the selected root is current, a
 * handle-independent listing of its tuples: per block, each tuple's opcode,
 * kind and type, and its operands' kinds; symbols are named by first
 * appearance in that listing (pointer identity), constants by value. */
static HANDLE g_tupleFile = INVALID_HANDLE_VALUE;
static void *g_ckptReturn;
static unsigned long g_ckptCount;
#define SYMSEEN 8192
static unsigned long g_symSeen[SYMSEEN];
static unsigned g_symSeenN;
static char g_tbuf[1 << 16];
static unsigned g_tlen;
static void tflush(void) { DWORD n; if (g_tlen) WriteFile(g_tupleFile, g_tbuf, g_tlen, &n, 0); g_tlen = 0; }
static void tput(const char *s) { while (*s) { if (g_tlen >= sizeof g_tbuf - 1) tflush(); g_tbuf[g_tlen++] = *s++; } }
static void thex(unsigned long v) { char b[12]; int i = 0, j; char t[12]; do { int d = (int)(v & 15); t[i++] = (char)(d < 10 ? '0' + d : 'a' + d - 10); v >>= 4; } while (v); for (j = 0; j < i; ++j) b[j] = t[i - 1 - j]; b[i] = 0; tput(b); }
static unsigned symId(unsigned long p)
{
    unsigned i;
    for (i = 0; i < g_symSeenN; ++i) if (g_symSeen[i] == p) return i;
    if (g_symSeenN < SYMSEEN) g_symSeen[g_symSeenN++] = p;
    return g_symSeenN - 1;
}
static void dumpOperand(unsigned char *o)
{
    unsigned char k = o[8];
    tput(" k"); thex(k);
    if (k == 7) { tput("="); thex(*(unsigned long *)(o + 0x18)); }
    else if (k >= 1 && k <= 6) {
        unsigned long sym = *(unsigned long *)(o + 0x14);
        if (k <= 3 && sym) { tput("s"); thex(symId(sym)); }
        if (k == 5 || k == 6) { tput("d"); thex(*(unsigned long *)(o + 0x24)); }
    }
}
#define RD(p, n) (!IsBadReadPtr((const void *)(p), (n)))
static void dumpBody(unsigned char *body, unsigned long ret)
{
    unsigned char *blk;
    unsigned char **bl;
    unsigned nb = 0;
    g_symSeenN = 0;
    tput("== ck "); thex(g_ckptCount); tput(" ret "); thex(ret - (unsigned long)g_real); tput("\n");
    if (!RD(body + 8, 4)) { tput("FAULT\n"); return; }
    bl = *(unsigned char ***)(body + 8);
    if (!bl || !RD(bl, 4)) { tput("noblocks\n"); return; }
    for (blk = *bl; blk && nb < 100000; ++nb) {
        unsigned char *t, *last;
        unsigned nt = 0;
        if (!RD(blk, 0x24)) { tput("FAULT\n"); return; }
        t = *(unsigned char **)(blk + 0x1c);
        last = *(unsigned char **)(blk + 0x20);
        tput("B\n");
        while (t && nt < 100000) {
            unsigned char *o;
            if (!RD(t, 0x20)) { tput("FAULT\n"); return; }
            tput(" t"); thex(*(unsigned long *)(t + 4)); tput(" "); thex(t[8]); tput(" "); thex(*(unsigned short *)(t + 0xa));
            if (t[8] >= 0xc) {
                for (o = *(unsigned char **)(t + 0x18); o && RD(o, 0x28); o = *(unsigned char **)o) dumpOperand(o);
                tput(" ->");
                o = *(unsigned char **)(t + 0x1c);
                if (o && RD(o, 0x28)) dumpOperand(o);
            }
            tput("\n");
            if (t == last) break;
            t = *(unsigned char **)t; ++nt;
        }
        blk = *(unsigned char **)blk;
    }
}
static void __cdecl checkpointDump(unsigned long ret)
{
    unsigned char *body;
    DWORD lastError = GetLastError();
    ++g_ckptCount;
    if (g_tupleFile != INVALID_HANDLE_VALUE && g_selected) {
        body = *(unsigned char **)((char *)g_real + 0xac380);
        if (body) { dumpBody(body, ret); tflush(); }
    }
    SetLastError(lastError);
}
static void __declspec(naked) ckptHook(void)
{
    __asm {
        pushfd
        pushad
        mov eax, dword ptr [esp + 36]
        push eax
        call checkpointDump
        add esp, 4
        popad
        popfd
        mov eax, dword ptr [g_real]
        mov eax, dword ptr [eax + 0xac2e8]
        jmp dword ptr [g_ckptReturn]
    }
}
static int loadTupleDump(void)
{
    char path[MAX_PATH];
    unsigned char b[5];
    if (!GetEnvironmentVariableA("HOMM3_VC6_TUPLE_DUMP", path, sizeof path)) return 1;
    g_tupleFile = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (g_tupleFile == INVALID_HANDLE_VALUE) return 1;
    b[0] = 0xa1; *(unsigned long *)(b + 1) = (unsigned long)g_real + 0xac2e8;
    g_ckptReturn = (char *)g_real + 0x9ab9;
    return patchHook(0x9ab4, ckptHook, b, 5);
}


/* scratch: log the 0x2df43 temp walk (record order, key) and its accepted moves */
static void *g_walkRet1, *g_walkRet2;
static int g_walkLog;
static void __cdecl walkLog(unsigned long *regs, unsigned long tag)
{
    /* pushad: EDI ESI EBP ESP EBX EDX ECX EAX */
    DWORD lastError = GetLastError();
    HANDLE h;
    unsigned char *rec = (unsigned char *)regs[1];
    if (!g_walkLog || !g_selected) return;
    h = logOpen();
    if (h != INVALID_HANDLE_VALUE) {
        writeString(h, tag ? "walk-move rec=" : "walk rec="); writeHex(h, (unsigned long)rec);
        writeString(h, " key="); writeHex(h, *(unsigned long *)(rec + 0x1c));
        writeString(h, " f5="); writeHex(h, rec[5]);
        writeString(h, " w4="); writeHex(h, *(unsigned long *)(rec + 4));
        writeString(h, " use="); writeHex(h, *(unsigned long *)(rec + 0x14));
        writeString(h, " def="); writeHex(h, *(unsigned long *)(rec + 0x38));
        writeString(h, "\n"); CloseHandle(h);
    }
    SetLastError(lastError);
}
static void __declspec(naked) walkHook1(void)
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push 0
        push eax
        call walkLog
        add esp, 8
        popad
        popfd
        mov edi, dword ptr [esi + 0x30]
        mov edx, dword ptr [esi + 0x1c]
        jmp dword ptr [g_walkRet1]
    }
}
static void __declspec(naked) walkHook2(void)
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push 1
        push eax
        call walkLog
        add esp, 8
        popad
        popfd
        mov eax, dword ptr [ebx + 0xc]
        test eax, eax
        jmp dword ptr [g_walkRet2]
    }
}
static int loadWalkLog(void)
{
    char t[8];
    static const unsigned char b1[] = {0x8b,0x7e,0x30,0x8b,0x56,0x1c};
    static const unsigned char b2[] = {0x8b,0x43,0x0c,0x85,0xc0};
    if (!GetEnvironmentVariableA("HOMM3_VC6_WALK_LOG", t, sizeof t)) return 1;
    g_walkLog = 1;
    g_walkRet1 = (char *)g_real + 0x2e17c;
    g_walkRet2 = (char *)g_real + 0x2e1ac;
    if (!patchHook(0x2e176, walkHook1, b1, 6)) return 0;
    return patchHook(0x2e1a7, walkHook2, b2, 5);
}


/* scratch: who disqualifies which temp record in 0x2df43's first loop */
static void *g_dqRet[4];
static void __cdecl dqLog(unsigned long *regs, unsigned long tag)
{
    DWORD lastError = GetLastError();
    HANDLE h;
    unsigned char *esi = (unsigned char *)regs[1], *ebp = (unsigned char *)regs[2];
    unsigned char *rec = tag < 2 ? (unsigned char *)regs[0] : *(unsigned char **)(ebp + 0x18);
    if (!g_walkLog || !g_selected) return;
    h = logOpen();
    if (h != INVALID_HANDLE_VALUE) {
        writeString(h, "dq site="); writeDecimal(h, tag);
        writeString(h, " key="); writeHex(h, *(unsigned long *)(rec + 0x1c));
        writeString(h, " tup="); writeHex(h, (unsigned long)esi);
        writeString(h, " op="); writeHex(h, *(unsigned long *)(esi + 4));
        writeString(h, " ofl="); writeHex(h, *(unsigned long *)(ebp + 0x10));
        writeString(h, "\n"); CloseHandle(h);
    }
    SetLastError(lastError);
}
static void __declspec(naked) dq0(void)
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push 0
        push eax
        call dqLog
        add esp, 8
        popad
        popfd
        or byte ptr [edi + 5], 4
        jmp dword ptr [g_dqRet + 0]
    }
}
static void __declspec(naked) dq1(void)
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push 1
        push eax
        call dqLog
        add esp, 8
        popad
        popfd
        or bl, 4
        mov byte ptr [edi + 5], bl
        jmp dword ptr [g_dqRet + 4]
    }
}
static void __declspec(naked) dq2(void)
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push 2
        push eax
        call dqLog
        add esp, 8
        popad
        popfd
        mov eax, dword ptr [ebp + 0x18]
        or byte ptr [eax + 5], 4
        jmp dword ptr [g_dqRet + 8]
    }
}
static void __declspec(naked) dq3(void)
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push 3
        push eax
        call dqLog
        add esp, 8
        popad
        popfd
        mov eax, dword ptr [ebp + 0x18]
        or byte ptr [eax + 5], 4
        jmp dword ptr [g_dqRet + 12]
    }
}
static int loadDqLog(void)
{
    static const unsigned char b0[] = {0x80,0x4f,0x05,0x04,0xe9};
    static const unsigned char b1[] = {0x80,0xcb,0x04,0x88,0x5f,0x05};
    static const unsigned char b2[] = {0x8b,0x45,0x18,0x80,0x48,0x05,0x04};
    if (!g_walkLog) return 1;
    g_dqRet[0] = (char *)g_real + 0x2e094;
    g_dqRet[1] = (char *)g_real + 0x2dfe6;
    g_dqRet[2] = (char *)g_real + 0x2e045;
    g_dqRet[3] = (char *)g_real + 0x2e063;
    if (!patchHook(0x2dfd1, dq0, b0, 5)) return 0;
    if (!patchHook(0x2dfe0, dq1, b1, 6)) return 0;
    if (!patchHook(0x2e03e, dq2, b2, 7)) return 0;
    return patchHook(0x2e05c, dq3, b2, 7);
}


/* scratch: log 0x2c6ce's 64-bucket rematerialization table (bucket computed at 0x2c6fe),
 * the 0x2cdf4 free walk (0x2ce69) and record reuse (0x2125a). */
static void *g_pkRet, *g_pfRet, *g_prRet;
static int g_pkLog;
static void writeName(HANDLE h, unsigned char *sym)
{
    const char *n;
    unsigned i;
    if (!sym || IsBadReadPtr(sym + 0x18, 4)) return;
    n = *(const char **)(sym + 0x18);
    if (!n || IsBadReadPtr(n, 1)) return;
    writeString(h, " name=");
    for (i = 0; i < 80 && !IsBadReadPtr(n + i, 1) && n[i] > 32 && n[i] < 127; ++i) {
        char c[2]; c[0] = n[i]; c[1] = 0; writeString(h, c);
    }
}
static void __cdecl pkLog(unsigned long ebp)
{
    DWORD lastError = GetLastError();
    HANDLE h;
    unsigned char *op = *(unsigned char **)(ebp - 0x10);
    unsigned long bucket = *(unsigned long *)(ebp - 8);
    if (!g_pkLog || !g_selected) return;
    h = logOpen();
    if (h != INVALID_HANDLE_VALUE) {
        unsigned char k = op[8];
        unsigned char *sym = 0;
        writeString(h, "pk kind="); writeHex(h, k);
        writeString(h, " bucket="); writeDecimal(h, bucket);
        if (k == 7) { writeString(h, " val="); writeHex(h, *(unsigned long *)(op + 0x18)); }
        else if (k == 3) sym = **(unsigned char ***)(*(unsigned char **)(op + 0x14) + 8);
        else sym = *(unsigned char **)(op + 0x14);
        if (sym) {
            writeString(h, " key="); writeHex(h, *(unsigned long *)(sym + 0x28));
            writeString(h, " sym="); writeHex(h, (unsigned long)sym);
            writeString(h, " cat="); writeHex(h, sym[4]);
            writeName(h, sym);
        }
        writeString(h, "\n"); CloseHandle(h);
    }
    SetLastError(lastError);
}
static void __declspec(naked) pkHook(void)
{
    __asm {
        pushfd
        pushad
        push ebp
        call pkLog
        add esp, 4
        popad
        popfd
        mov edx, dword ptr [ebp - 8]
        mov eax, dword ptr [g_real]
        lea eax, [eax + edx*4 + 0x9d750]
        mov eax, dword ptr [eax]
        jmp dword ptr [g_pkRet]
    }
}
static void __cdecl pfLog(unsigned long *regs, unsigned long tag)
{
    DWORD lastError = GetLastError();
    HANDLE h;
    if (!g_pkLog || !g_selected) return;
    h = logOpen();
    if (h != INVALID_HANDLE_VALUE) {
        if (tag == 0) {
            /* 0x2ce69: ESI pseudo being freed, EAX its use struct; record at [EAX+0xc] */
            unsigned char *rec = *(unsigned char **)(regs[7] + 0xc);
            writeString(h, "pfree pseudo="); writeHex(h, regs[1]);
            writeString(h, " reckey="); writeHex(h, rec ? *(unsigned long *)(rec + 0x1c) : 0);
            writeString(h, " bucketwalk="); writeHex(h, (regs[4] - ((unsigned long)g_real + 0x9d750)) / 4);
        } else {
            /* 0x2125a: ESI reused record (key kept), EDI the symbol it is for */
            writeString(h, "preuse key="); writeHex(h, *(unsigned long *)(regs[1] + 0x1c));
            writeString(h, " for="); writeHex(h, regs[0]);
            writeString(h, " forkey="); writeHex(h, *(unsigned long *)(regs[0] + 0x28));
            writeName(h, (unsigned char *)regs[0]);
        }
        writeString(h, "\n"); CloseHandle(h);
    }
    SetLastError(lastError);
}
static void __declspec(naked) pfHook(void)
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push 0
        push eax
        call pfLog
        add esp, 8
        popad
        popfd
        mov edi, dword ptr [eax + 0xc]
        xor edx, edx
        jmp dword ptr [g_pfRet]
    }
}
static void __declspec(naked) prHook(void)
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push 1
        push eax
        call pfLog
        add esp, 8
        popad
        popfd
        mov eax, dword ptr [esi + 0x2c]
        push ecx
        mov ecx, dword ptr [g_real]
        mov dword ptr [ecx + 0xae038], eax
        pop ecx
        jmp dword ptr [g_prRet]
    }
}
static int loadPkLog(void)
{
    char t[8];
    unsigned char b[10];
    static const unsigned char bf[] = {0x8b,0x78,0x0c,0x33,0xd2};
    unsigned char br[8];
    if (!GetEnvironmentVariableA("HOMM3_VC6_PK_LOG", t, sizeof t)) return 1;
    g_pkLog = 1;
    b[0] = 0x8b; b[1] = 0x55; b[2] = 0xf8; b[3] = 0x8b; b[4] = 0x04; b[5] = 0x95;
    *(unsigned long *)(b + 6) = (unsigned long)g_real + 0x9d750;
    g_pkRet = (char *)g_real + 0x2c708;
    g_pfRet = (char *)g_real + 0x2ce6e;
    g_prRet = (char *)g_real + 0x21262;
    br[0] = 0x8b; br[1] = 0x46; br[2] = 0x2c; br[3] = 0xa3;
    *(unsigned long *)(br + 4) = (unsigned long)g_real + 0xae038;
    if (!patchHook(0x2c6fe, pkHook, b, 10)) return 0;
    if (!patchHook(0x2ce69, pfHook, bf, 5)) return 0;
    return patchHook(0x2125a, prHook, br, 8);
}

static int loadPhaseState(void)
{
    char text[16];
    unsigned char readBytes[5];
    DWORD n = GetEnvironmentVariableA("HOMM3_VC6_PHASE", text, sizeof text);
    if (n && n < sizeof text) g_phaseValue = text[0] == '1' ? 1 : 0;
    g_stateLog = GetEnvironmentVariableA("HOMM3_VC6_STATE_LOG", text, sizeof text) != 0;
    if (GetEnvironmentVariableA("HOMM3_VC6_PHASE_READS", g_readForce, sizeof g_readForce) >= sizeof g_readForce) g_readForce[0] = 0;
    if (g_phaseValue < 0 && !g_stateLog) return 1;
    {
        unsigned char doneBytes[9];
        doneBytes[0] = 0x8b; doneBytes[1] = 0xce; doneBytes[2] = 0x66; doneBytes[3] = 0x89; doneBytes[4] = 0x3d;
        *(unsigned long *)(doneBytes + 5) = (unsigned long)g_real + 0xac360;
        g_driverReturn = (char *)g_real + 0x683d5;
        if (g_phaseValue >= 0)
            *(unsigned long *)((char *)g_real + 0x9f120) = (unsigned long)g_phaseValue;
        if (!patchHook(0x683cc, driverHook, doneBytes, 9)) return 0;
    }
    if (!g_stateLog) return 1;
    if (!installCheckpoints()) return 0;
    g_phaseReturn = (char *)g_real + 0x5b16;
    readBytes[0] = 0xa1;
    *(unsigned long *)(readBytes + 1) = (unsigned long)g_real + 0x9f120;
    return patchHook(0x5b11, phaseHook, readBytes, 5);
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
/* HOMM3_VC6_COVER_SEQ: also record the ordered entry sequence while the
 * selected root is current (from its main to the next main), uncapped. */
#define COVER_SEQ_MAX (4u << 20)
static unsigned long g_coverSeq[COVER_SEQ_MAX];
static unsigned long g_coverSeqCount;
static int g_coverSeqOn;

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
        if (g_coverSeqOn && g_selected && g_coverSeqCount + 2 <= COVER_SEQ_MAX) {
            g_coverSeq[g_coverSeqCount++] = g_coverRva[k];
            g_coverSeq[g_coverSeqCount++] = *(unsigned long *)context->Esp - (unsigned long)g_real;
        }
        if (g_coverHits[k] < COVER_CAP || (g_coverSeqOn && g_selected)) {
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
    {
        char flag[8];
        g_coverSeqOn = GetEnvironmentVariableA("HOMM3_VC6_COVER_SEQ", flag, sizeof flag) != 0;
    }
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

static void rearmCover(void)
{
    unsigned i;
    unsigned char *code = (unsigned char *)g_real;
    if (!g_coverSeqOn) return;
    for (i = 0; i < g_coverCount; ++i)
        if (code[g_coverRva[i]] != 0xcc && (unsigned char *)(code + g_coverRva[i]) != g_coverRearm)
            code[g_coverRva[i]] = 0xcc;
}

static void writeCover(void)
{
    HANDLE h;
    unsigned i;
    if (!g_coverCount) return;
    if (g_coverSeqOn) {
        char path[MAX_PATH];
        if (GetEnvironmentVariableA("HOMM3_VC6_COVER_SEQ", path, sizeof path)) {
            DWORD n;
            HANDLE f = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
            if (f != INVALID_HANDLE_VALUE) {
                WriteFile(f, g_coverSeq, g_coverSeqCount * 4, &n, 0);
                CloseHandle(f);
            }
        }
    }
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
    loadHandleSet();
    if (!loadHandleShift()) return 0;
    if (!loadPhaseState()) return 0;
    loadBytePatches();
    if (!loadTupleDump()) return 0;
    if (!loadWalkLog()) return 0;
    if (!loadDqLog()) return 0;
    if (!loadPkLog()) return 0;
    {
        /* Diagnostic heap displacement: HOMM3_VC6_HEAP_PAD="malloc,local"
         * byte counts allocated once before C2 runs. */
        char text[64];
        DWORD n = GetEnvironmentVariableA("HOMM3_VC6_HEAP_PAD", text, sizeof text);
        if (n && n < sizeof text) {
            unsigned long a = 0, b = 0, i;
            int second = 0;
            typedef void *(__cdecl *mallocFunction)(unsigned);
            mallocFunction m = (mallocFunction)GetProcAddress(GetModuleHandleA("msvcrt.dll"), "malloc");
            for (i = 0; i < n; ++i) {
                if (text[i] == ',') { second = 1; continue; }
                if (text[i] >= '0' && text[i] <= '9') {
                    if (second) b = b * 10 + (text[i] - '0'); else a = a * 10 + (text[i] - '0');
                }
            }
            if (a && m) m(a);
            if (b) LocalAlloc(LMEM_FIXED, b);
        }
    }
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
