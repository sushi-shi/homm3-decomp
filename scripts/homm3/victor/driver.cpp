// Oracle driver: retail CRT initialization precedes this DLL's run entry.
// Candidate Victor implementations are linked from their normal VC6 objects.
#include <windows.h>
#include "victor.h"

typedef int (__stdcall *Info)(const char*, PcxData*);
typedef int (__stdcall *Allocate)(imgdes*, int, int, int);
typedef int (__stdcall *Load)(const char*, imgdes*);
typedef int (__stdcall *Flip)(imgdes*, imgdes*);
typedef void (__stdcall *Release)(imgdes*);
static HANDLE g_output;
static unsigned long g_case, g_stage;
static bool g_failed;

static void emit(const void* data, unsigned long size)
{
    DWORD written;
    if (!WriteFile(g_output, data, size, &written, 0) || written != size)
        g_failed = true;
}
static void word(unsigned long value) { emit(&value, 4); }
static void fill(void* data, unsigned long size, unsigned char value)
{
    unsigned char* bytes = (unsigned char*)data;
    for (unsigned long i = 0; i < size; ++i) bytes[i] = value;
}
static void descriptor(imgdes* image)
{
    // Pointer addresses depend on loader/allocator placement. Compare their
    // existence and ownership relationships, plus every scalar field.
    word(image->m_ibuff != 0);
    emit(&image->m_stx, 5 * 4);
    word(image->m_colors);
    word(image->m_imgtype);
    word(image->m_bmh != 0);
    word(image->m_bitmap != 0);
    word(image->m_palette != 0);
    if (image->m_bmh) {
        word(image->m_palette == (RGBQUAD*)(image->m_bmh + 1));
        word(image->m_ibuff == (unsigned char*)(image->m_palette + image->m_colors));
    }
}
static void snapshot(imgdes* image)
{
    descriptor(image);
    emit(image->m_bmh, sizeof(BITMAPINFOHEADER));
    word(image->m_colors * sizeof(RGBQUAD));
    emit(image->m_palette, image->m_colors * sizeof(RGBQUAD));
    word(image->m_bmh->biSizeImage);
    emit(image->m_ibuff, image->m_bmh->biSizeImage);
}

static int compareInput(const char* input, const char* output, bool candidate)
{
    Info info = candidate ? &pcxinfo : (Info)0x6042a0;
    Allocate allocate = candidate ? &allocimage : (Allocate)0x603590;
    Load load = candidate ? &loadpcx : (Load)0x603e00;
    Flip flip = candidate ? &flipimage : (Flip)0x603b20;
    Release release = candidate ? &freeimage : (Release)0x6037a0;
    g_output = CreateFileA(output, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (g_output == INVALID_HANDLE_VALUE) return 10;
    word(0x31544356); // VCT1 protocol
    PcxData data;
    fill(&data, sizeof(data), 0xa5);
    g_stage = 1;
    int status = info(input, &data);
    word(status);
    emit(&data, sizeof(data));
    if (!status) {
        imgdes image;
        fill(&image, sizeof(image), 0xa5);
        g_stage = 2;
        status = allocate(&image, data.m_width, data.m_length,
                          data.m_bpPixel * data.m_nplanes);
        word(status);
        descriptor(&image);
        if (!status) {
            // Padding and untouched destination bytes are controlled inputs.
            fill(image.m_ibuff, image.m_bmh->biSizeImage, 0xa5);
            snapshot(&image);
            g_stage = 3;
            status = load(input, &image);
            word(status);
            snapshot(&image);
            g_stage = 4;
            status = flip(&image, &image);
            word(status);
            snapshot(&image);
            g_stage = 5;
            release(&image);
            descriptor(&image);
        }
    }
    CloseHandle(g_output);
    return g_failed ? 11 : 0;
}

static int execute()
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    char job[MAX_PATH], mode[32], allocation[16] = "global";
    if (!GetEnvironmentVariableA("VICTOR_JOB", job, sizeof(job)) ||
        !GetEnvironmentVariableA("VICTOR_MODE", mode, sizeof(mode))) return 12;
    HANDLE file = CreateFileA(job, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (file == INVALID_HANDLE_VALUE) return 13;
    DWORD size = GetFileSize(file, 0), read;
    char* bytes = (char*)GlobalAlloc(GMEM_FIXED, size + 1);
    if (!bytes || !ReadFile(file, bytes, size, &read, 0) || read != size) return 14;
    CloseHandle(file);
    bytes[size] = 0;
    char* cursor = bytes;
    bool candidate = mode[0] == 'c';
    GetEnvironmentVariableA("VICTOR_ALLOCATION", allocation, sizeof(allocation));
    unsigned long useDib = allocation[0] == 'd';
    g_victorUseDibSection = useDib;
    *(unsigned long*)0x6abaa4 = useDib;
    g_victorCreateDibSection = &CreateDIBSection;
    g_victorSetDibColorTable = &SetDIBColorTable;
    *(VictorCreateDibSection*)0x6abaa8 = &CreateDIBSection;
    *(VictorSetDibColorTable*)0x6abaac = &SetDIBColorTable;
    for (g_case = 0; cursor < bytes + size; ++g_case) {
        const char* input = cursor;
        while (*cursor) ++cursor;
        ++cursor;
        const char* output = cursor;
        while (*cursor) ++cursor;
        ++cursor;
        if (cursor > bytes + size) return 15;
        int status = compareInput(input, output, candidate);
        if (status) return status;
    }
    GlobalFree(bytes);
    return 0;
}
extern "C" int __stdcall run()
{
    __try { return execute(); }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        HANDLE failure = CreateFileA("failure.bin", GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
        unsigned long record[3] = {GetExceptionCode(), g_case, g_stage};
        DWORD written;
        WriteFile(failure, record, sizeof(record), &written, 0);
        CloseHandle(failure);
        return 16;
    }
}
