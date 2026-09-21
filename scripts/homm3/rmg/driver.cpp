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

static void logMessage(const char* message)
{
    unsigned long length = 0, written;
    while (message[length]) ++length;
    WriteFile(g_log, message, length, &written, 0);
    FlushFileBuffers(g_log);
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

class OutputFile : public TAbstractFile {
public:
    HANDLE m_file;
    bool m_failed;
    OutputFile(HANDLE file) : m_file(file), m_failed(false) {}
    virtual int read(void*, int) { m_failed = true; return 0; }
    virtual int write(const void* bytes, int size) {
        DWORD written = 0;
        if (size < 0 || !WriteFile(m_file, bytes, size, &written, 0)
            || written != (DWORD)size) m_failed = true;
        return (int)written;
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

static int generate(TRandomMapRequest* request, HANDLE output, bool candidate)
{
    OutputFile stream(output);
    int (TRandomMapRequest::*entry)(TAbstractFile*, void*) =
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

static int execute()
{
    char jobPath[1024], dataPath[1024], mode[32];
    DWORD count;
    unsigned long requestWords[20];
    unsigned char* requestBytes = (unsigned char*)requestWords;
    HANDLE file;
    if (!GetEnvironmentVariableA("RMG_JOB", jobPath, sizeof(jobPath))
        || !GetEnvironmentVariableA("RMG_DATA", dataPath, sizeof(dataPath))
        || !GetEnvironmentVariableA("RMG_MODE", mode, sizeof(mode))) return 120;
    if (lstrcmpA(mode, "candidate") != 0 && lstrcmpA(mode, "retail") != 0) return 120;
    if (GetModuleHandleA("rmg-driver.dll") != (HMODULE)0x30000000) return 133;
    if (!checkRequestConstructor()) return 132;
    file = CreateFileA(jobPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (file == INVALID_HANDLE_VALUE) return 121;
    if (!ReadFile(file, &g_seed, 4, &count, 0) || count != 4) return 122;
    if (!ReadFile(file, &g_stackWord, 4, &count, 0) || count != 4) return 122;
    if (!ReadFile(file, &g_heapByte, 4, &count, 0) || count != 4) return 122;
    if (g_heapByte > 255 && g_heapByte != 0xffffffff) return 122;
    if (!ReadFile(file, requestBytes, 80, &count, 0) || count != 80) return 122;
    CloseHandle(file);
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
    logMessage("generating map\r\n");
    file = CreateFileA("map.raw", GENERIC_WRITE, 0, 0, CREATE_NEW, 0, 0);
    if (file == INVALID_HANDLE_VALUE) return 127;
    unsigned short controlWord;
    __asm fnstcw controlWord
    int result = generate((TRandomMapRequest*)requestBytes, file, mode[0] == 'c');
    unsigned short finalControlWord;
    __asm fnstcw finalControlWord
    if (!CloseHandle(file)) return 128;
    typedef unsigned char* (__cdecl *GetThreadData)();
    unsigned long state = *(unsigned long*)(((GetThreadData)0x61d303)() + 0x14);
    unsigned long header[4] = {0x31474d52, (unsigned long)result, state,
        controlWord | ((unsigned long)finalControlWord << 16)};
    file = CreateFileA("result.bin", GENERIC_WRITE, 0, 0, CREATE_NEW, 0, 0);
    if (file == INVALID_HANDLE_VALUE) return 129;
    if (!WriteFile(file, header, sizeof(header), &count, 0) || count != sizeof(header)) return 130;
    if (!WriteFile(file, requestBytes, 80, &count, 0) || count != 80) return 130;
    CloseHandle(file);
    logMessage("generation complete\r\n");
    return 0;
}

static int reportException(EXCEPTION_POINTERS* exception)
{
    DWORD written;
    unsigned long failure[3] = {exception->ExceptionRecord->ExceptionCode,
        (unsigned long)exception->ExceptionRecord->ExceptionAddress, g_stage};
    HANDLE file = CreateFileA("failure.bin", GENERIC_WRITE, 0, 0, CREATE_NEW, 0, 0);
    if (file != INVALID_HANDLE_VALUE) {
        WriteFile(file, failure, sizeof(failure), &written, 0);
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
