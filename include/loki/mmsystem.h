/* mmsystem.h - the Windows multimedia records and timer calls the shared
 * source names (work/loki-game only). */
#ifndef HOMM3_LOKI_MMSYSTEM_H
#define HOMM3_LOKI_MMSYSTEM_H

#include <windows.h>

typedef UINT MMRESULT;

typedef struct waveformat_tag {
    WORD wFormatTag;
    WORD nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD nBlockAlign;
} WAVEFORMAT;

typedef struct pcmwaveformat_tag {
    WAVEFORMAT wf;
    WORD wBitsPerSample;
} PCMWAVEFORMAT;

typedef struct tWAVEFORMATEX {
    WORD wFormatTag;
    WORD nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD nBlockAlign;
    WORD wBitsPerSample;
    WORD cbSize;
} WAVEFORMATEX;

#define WAVE_FORMAT_PCM 1

extern "C" {
DWORD timeGetTime(void);
MMRESULT timeBeginPeriod(UINT period);
MMRESULT timeEndPeriod(UINT period);
}

#endif
