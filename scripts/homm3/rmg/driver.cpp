// Test driver only: the executable's real CRT has initialized before run().
// No private CRT/heap is linked. Game objects use the existing retail runtime.
#include <windows.h>
#include "abstractfile.h"
#include "rmg_request.h"

typedef char CheckRequestSize[sizeof(TRandomMapRequest) == 80 ? 1 : -1];
typedef void (__cdecl *Initializer)();
#pragma data_seg(".CRT$XCA")
Initializer g_firstInitializer[] = {0};
#pragma data_seg(".CRT$XCZ")
Initializer g_lastInitializer[] = {0};
#pragma data_seg()

static unsigned long g_seed;
static unsigned long g_stackWord;
static unsigned long g_heapByte;
static HANDLE g_log;
static unsigned long g_stage;
// Index of the job being generated; reported with a fault in batch mode.
static unsigned long g_job;
static bool g_batch;

static void logMessage(const char* message)
{
    unsigned long length = 0, written;
    while (message[length]) ++length;
    WriteFile(g_log, message, length, &written, 0);
    ++g_stage;
}

static long __cdecl fixedTime(long* destination)
{
    if (destination) *destination = (long)g_seed;
    return (long)g_seed;
}

static bool replaceTime()
{
    unsigned char* target = (unsigned char*)0x6198e0;
    DWORD oldProtection, ignored;
    if (!VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &oldProtection))
        return false;
    target[0] = 0xe9;
    *(unsigned long*)(target + 1) = (unsigned long)&fixedTime - (unsigned long)target - 5;
    FlushInstructionCache(GetCurrentProcess(), target, 5);
    return VirtualProtect(target, 5, oldProtection, &ignored) != 0;
}

// Retail operator new at 0x616ed2 forwards (size, 1) to this CRT allocator.
// Keep its allocation/failure policy, but give both generators the same fresh
// storage contents. In particular, added water-zone slots leave their town
// flags uninitialized; recycled heap contents otherwise change RNG consumption.
static void* __cdecl filledAllocation(unsigned int size, int flags)
{
    typedef void* (__cdecl *Allocate)(unsigned int, int);
    unsigned char* memory = (unsigned char*)((Allocate)0x61a417)(size, flags);
    if (memory)
        for (unsigned int i = 0; i < size; ++i)
            memory[i] = (unsigned char)g_heapByte;
    return memory;
}

static bool prepareHeap()
{
    if (g_heapByte == 0xffffffff) return true; // Explicit native-heap diagnostic.
    unsigned char* target = (unsigned char*)0x616ed8;
    if (target[0] != 0xe8 || *(unsigned long*)(target + 1) != 0x353a)
        return false;
    DWORD oldProtection, ignored;
    if (!VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &oldProtection))
        return false;
    *(unsigned long*)(target + 1) = (unsigned long)&filledAllocation - (unsigned long)target - 5;
    FlushInstructionCache(GetCurrentProcess(), target, 5);
    return VirtualProtect(target, 5, oldProtection, &ignored) != 0;
}

// The map is collected in memory and written once: the map writer emits most
// fields with separate small writes, and each WriteFile is costly under Wine.
// write() accepts every byte exactly as a successful WriteFile did.
static unsigned char* g_outputBuffer;
static const unsigned long OUTPUT_CAPACITY = 64 << 20;

class OutputFile : public TAbstractFile {
public:
    unsigned long m_size;
    bool m_failed;
    OutputFile() : m_size(0), m_failed(false) {}
    virtual int read(void*, int) { m_failed = true; return 0; }
    virtual int write(const void* bytes, int size) {
        if (size < 0 || (unsigned long)size > OUTPUT_CAPACITY - m_size) {
            m_failed = true;
            return 0;
        }
        const unsigned char* source = (const unsigned char*)bytes;
        for (int i = 0; i < size; ++i)
            g_outputBuffer[m_size + i] = source[i];
        m_size += size;
        return size;
    }
};

// Both sides enter through this exact call site. An extra wrapper on only
// one side changes the initial storage of the stack-allocated generator.
static int callGenerator(TRandomMapRequest* request, TAbstractFile* output,
    unsigned long address)
{
    int result;
    __asm {
        // Commit and fill the future stack one word at a time, respecting
        // Windows guard pages. Retail reads an uninitialized generator field
        // (next key-tent color at +0xf5c); it is part of the starting state.
        mov eax, g_stackWord
        mov ecx, 04000h
fillStack:
        push eax
        dec ecx
        jnz fillStack
        add esp, 010000h
        push 0
        push output
        mov ecx, request
        mov eax, address
        call eax
        mov result, eax
    }
    return result;
}

static int generate(TRandomMapRequest* request, OutputFile& stream, bool candidate)
{
    int (TRandomMapRequest::*entry)(TAbstractFile*, TProgressSink*) =
        &TRandomMapRequest::generateToFile;
    typedef char CheckMemberPointerSize[sizeof(entry) == sizeof(unsigned long) ? 1 : -1];
    unsigned long address = candidate ? *(unsigned long*)&entry : 0x54bf60;
    int result = callGenerator(request, &stream, address);
    return stream.m_failed ? -10 : result;
}

// Exact, small ABI/link control: independently enter the retained retail and
// candidate request constructors and compare their complete 80-byte records.
static bool checkRequestConstructor()
{
    TRandomMapRequest candidate(36, 36, 1);
    unsigned long retail[20];
    __asm {
        push 1
        push 36
        push 36
        lea ecx, retail
        mov eax, 054bf00h
        call eax
    }
    for (unsigned int i = 0; i < sizeof(candidate); ++i)
        if (((unsigned char*)&candidate)[i] != ((unsigned char*)retail)[i])
            return false;
    return true;
}

// "name" or, in batch mode, "name-<job>" followed by the extension.
static void outputName(char* buffer, const char* name, const char* extension)
{
    char digits[12];
    int count = 0;
    while (*name) *buffer++ = *name++;
    if (g_batch) {
        unsigned long value = g_job;
        do { digits[count++] = (char)('0' + value % 10); value /= 10; } while (value);
        *buffer++ = '-';
        while (count) *buffer++ = digits[--count];
    }
    while (*extension) *buffer++ = *extension++;
    *buffer = 0;
}

static bool writeWhole(const char* name, const void* bytes, unsigned long size)
{
    DWORD count = 0;
    HANDLE file = CreateFileA(name, GENERIC_WRITE, 0, 0, CREATE_NEW, 0, 0);
    if (file == INVALID_HANDLE_VALUE) return false;
    bool ok = WriteFile(file, bytes, size, &count, 0) && count == size;
    return CloseHandle(file) && ok;
}

typedef unsigned char* (__cdecl *GetThreadData)();

static unsigned long& rngState()
{
    return *(unsigned long*)(((GetThreadData)0x61d303)() + 0x14);
}

static int execute()
{
    char jobPath[1024], dataPath[1024], mode[32], name[64];
    DWORD count;
    HANDLE file;
    if (!GetEnvironmentVariableA("RMG_JOB", jobPath, sizeof(jobPath))
        || !GetEnvironmentVariableA("RMG_DATA", dataPath, sizeof(dataPath))
        || !GetEnvironmentVariableA("RMG_MODE", mode, sizeof(mode))) return 120;
    if (lstrcmpA(mode, "candidate") != 0 && lstrcmpA(mode, "retail") != 0) return 120;
    if (GetModuleHandleA("rmg-driver.dll") != (HMODULE)0x30000000) return 133;
    if (!checkRequestConstructor()) return 132;
    // A job is seed, stack word, heap byte and the 80-byte request. A file
    // holding several jobs runs them in order in this one process.
    const unsigned long JOB_SIZE = 12 + 80;
    file = CreateFileA(jobPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (file == INVALID_HANDLE_VALUE) return 121;
    unsigned long jobBytes = GetFileSize(file, 0);
    if (jobBytes == INVALID_FILE_SIZE || !jobBytes || jobBytes % JOB_SIZE) return 122;
    unsigned char* jobs = (unsigned char*)VirtualAlloc(0, jobBytes, MEM_COMMIT, PAGE_READWRITE);
    g_outputBuffer = (unsigned char*)VirtualAlloc(0, OUTPUT_CAPACITY, MEM_COMMIT, PAGE_READWRITE);
    if (!jobs || !g_outputBuffer) return 122;
    if (!ReadFile(file, jobs, jobBytes, &count, 0) || count != jobBytes) return 122;
    CloseHandle(file);
    unsigned long jobCount = jobBytes / JOB_SIZE;
    g_batch = jobCount > 1;
    for (unsigned long check = 0; check < jobCount; ++check) {
        unsigned long heapByte = *(unsigned long*)(jobs + check * JOB_SIZE + 8);
        if (heapByte > 255 && heapByte != 0xffffffff) return 122;
        if ((heapByte == 0xffffffff) != (*(unsigned long*)(jobs + 8) == 0xffffffff))
            return 122; // the native-heap diagnostic cannot be mixed in a batch
    }
    g_heapByte = *(unsigned long*)(jobs + 8);
    if (!replaceTime()) return 123;
    logMessage("retail CRT ready; initializing candidate globals\r\n");
    for (Initializer* init = g_firstInitializer + 1; init < g_lastInitializer; ++init)
        if (*init) (*init)();
    logMessage("opening retail resources\r\n");
    typedef void (__fastcall *SetPath)(const char*);
    typedef bool (__fastcall *OpenResources)(bool, bool, int*);
    ((SetPath)0x55a5c0)(dataPath);
    int error = 0;
    if (!((OpenResources)0x55a250)(true, true, &error)) return 124;
    typedef unsigned char (__fastcall *LoadTable)();
    logMessage("loading creature traits\r\n");
    if (!((LoadTable)0x47b290)()) return 125;
    logMessage("initializing adventure object traits\r\n");
    ((void (__fastcall *)())0x41b500)();
    logMessage("loading artifact traits\r\n");
    if (!((LoadTable)0x44cd50)()) return 125;
    logMessage("loading spell traits\r\n");
    if (!((LoadTable)0x59e090)()) return 125;
    logMessage("loading hero traits\r\n");
    if (!((LoadTable)0x4e67a0)()) return 125;
    if (!prepareHeap()) return 134;
    // Every job starts from the state a fresh process has here: the same
    // x87 control word and CRT rand() state. Stack and heap storage are
    // refilled per job by callGenerator and filledAllocation.
    unsigned short initialControlWord;
    __asm fnstcw initialControlWord
    unsigned long initialRngState = rngState();
    for (g_job = 0; g_job < jobCount; ++g_job) {
        const unsigned char* job = jobs + g_job * JOB_SIZE;
        unsigned long requestWords[20];
        unsigned char* requestBytes = (unsigned char*)requestWords;
        g_seed = *(const unsigned long*)job;
        g_stackWord = *(const unsigned long*)(job + 4);
        g_heapByte = *(const unsigned long*)(job + 8);
        for (unsigned int i = 0; i < 80; ++i) requestBytes[i] = job[12 + i];
        rngState() = initialRngState;
        __asm fldcw initialControlWord
        g_stage = 7;
        logMessage("generating map\r\n");
        OutputFile stream;
        unsigned short controlWord;
        __asm fnstcw controlWord
        int result = generate((TRandomMapRequest*)requestBytes, stream, mode[0] == 'c');
        unsigned short finalControlWord;
        __asm fnstcw finalControlWord
        outputName(name, "map", ".raw");
        if (!writeWhole(name, g_outputBuffer, stream.m_size)) return 127;
        unsigned long header[4] = {0x31474d52, (unsigned long)result, rngState(),
            controlWord | ((unsigned long)finalControlWord << 16)};
        unsigned char record[96];
        for (unsigned int h = 0; h < 16; ++h) record[h] = ((unsigned char*)header)[h];
        for (unsigned int r = 0; r < 80; ++r) record[16 + r] = requestBytes[r];
        outputName(name, "result", ".bin");
        if (!writeWhole(name, record, sizeof(record))) return 130;
        logMessage("generation complete\r\n");
    }
    return 0;
}

static int reportException(EXCEPTION_POINTERS* exception)
{
    DWORD written;
    // Batch faults also name the job; a single job keeps the 12-byte record.
    unsigned long failure[4] = {exception->ExceptionRecord->ExceptionCode,
        (unsigned long)exception->ExceptionRecord->ExceptionAddress, g_stage, g_job};
    HANDLE file = CreateFileA("failure.bin", GENERIC_WRITE, 0, 0, CREATE_NEW, 0, 0);
    if (file != INVALID_HANDLE_VALUE) {
        WriteFile(file, failure, g_batch ? 16 : 12, &written, 0);
        CloseHandle(file);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

extern "C" int __stdcall run()
{
    // Faults are batch results reported by the SEH handler. Do not let the
    // host display a modal error box during unattended comparison campaigns.
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX
        | SEM_NOOPENFILEERRORBOX);
    g_log = CreateFileA("driver.log", GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_NEW, 0, 0);
    if (g_log == INVALID_HANDLE_VALUE) return 119;
    __try {
        int result = execute();
        CloseHandle(g_log);
        return result;
    } __except(reportException(GetExceptionInformation())) {
        CloseHandle(g_log);
        return 131;
    }
}
