/* bink.h - RAD Bink as the Loki port sees it: opaque handles only (Loki
 * played video through smpeg; work/loki-game only). */
#ifndef HOMM3_LOKI_BINK_H
#define HOMM3_LOKI_BINK_H

struct BINK;
struct BINKIO;
struct BINKSND;
struct BINKRECT;
typedef BINK* HBINK;

typedef struct BINKSUMMARY {
    unsigned long Width;
    unsigned long Height;
    unsigned long TotalTime;
    unsigned long FileFrameRate;
    unsigned long FileFrameRateDiv;
    unsigned long FrameRate;
    unsigned long FrameRateDiv;
    unsigned long TotalOpenTime;
    unsigned long TotalFrames;
    unsigned long TotalPlayedFrames;
    unsigned long SkippedFrames;
    unsigned long SoundSkips;
    unsigned long TotalBlitTime;
    unsigned long TotalReadTime;
    unsigned long TotalDecompTime;
    unsigned long TotalBackReadTime;
    unsigned long TotalReadSpeed;
    unsigned long SlowestFrameTime;
    unsigned long Slowest2FrameTime;
    unsigned long SlowestFrameNum;
    unsigned long Slowest2FrameNum;
    unsigned long AverageDataRate;
    unsigned long AverageFrameSize;
    unsigned long HighestMemAmount;
    unsigned long TotalIOMemory;
    unsigned long HighestIOUsed;
    unsigned long Highest1SecRate;
    unsigned long Highest1SecFrame;
} BINKSUMMARY;

#endif
