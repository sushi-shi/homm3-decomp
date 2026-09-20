#ifndef BINKH
#define BINKH

#define BINKVERSION "0.5a"
#define BINKDATE    "1999-01-20"


// BINKW32.DLL 0.5a exports the public SDK entry points with a leading
// underscore in the identifier itself, in addition to stdcall decoration.
#define BinkPause _BinkPause
#define BinkOpen _BinkOpen
#define BinkClose _BinkClose
#define BinkDDSurfaceType _BinkDDSurfaceType
#define BinkCopyToBuffer _BinkCopyToBuffer
#define BinkDoFrame _BinkDoFrame
#define BinkGoto _BinkGoto
#define BinkNextFrame _BinkNextFrame
#define BinkGetSummary _BinkGetSummary
#define BinkWait _BinkWait
#define BinkGetRects _BinkGetRects
#define BinkSetSoundSystem _BinkSetSoundSystem
#define BinkOpenMiles _BinkOpenMiles

#ifndef __RADRES__

#include "Rad.h"

RADDEFSTART

typedef struct BINK PTR4* HBINK;

typedef s32  (RADLINK PTR4* BINKIOOPEN)         (struct BINKIO PTR4* Bnkio, const char PTR4 *name, u32 flags);
typedef u32  (RADLINK PTR4* BINKIOREADHEADER)   (struct BINKIO PTR4* Bnkio, s32 Offset, void PTR4* Dest,u32 Size);
typedef u32  (RADLINK PTR4* BINKIOREADFRAME)    (struct BINKIO PTR4* Bnkio, u32 Framenum,s32 origofs,void PTR4* dest,u32 size);
typedef u32  (RADLINK PTR4* BINKIOGETBUFFERSIZE)(struct BINKIO PTR4* Bnkio, u32 Size);
typedef void (RADLINK PTR4* BINKIOSETINFO)      (struct BINKIO PTR4* Bnkio, void PTR4* Buf,u32 Size,u32 FileSize,u32 simulate);
typedef u32  (RADLINK PTR4* BINKIOIDLE)         (struct BINKIO PTR4* Bnkio);
typedef void (RADLINK PTR4* BINKIOCLOSE)        (struct BINKIO PTR4* Bnkio);

typedef struct BINKIO {
  BINKIOREADHEADER ReadHeader;
  BINKIOREADFRAME  ReadFrame;
  BINKIOGETBUFFERSIZE GetBufferSize;
  BINKIOSETINFO SetInfo;
  BINKIOIDLE Idle;
  BINKIOCLOSE Close;
  volatile u32 BytesRead;
  volatile u32 TotalTime;
  volatile u32 ForegroundTime;
  volatile u32 BufSize;
  volatile u32 BufHighUsed;
  volatile u32 CurBufSize;
  volatile u32 CurBufUsed;
  volatile u8 iodata[128];
} BINKIO;

typedef s32  (RADLINK PTR4* BINKSNDOPEN)     (struct BINKSND PTR4* BnkSnd, u32 freq, s32 bits, s32 chans, u32 flags, HBINK bink);
typedef s32  (RADLINK PTR4* BINKSNDREADY)    (struct BINKSND PTR4* BnkSnd);
typedef s32  (RADLINK PTR4* BINKSNDLOCK)     (struct BINKSND PTR4* BnkSnd, u8 PTR4* PTR4* addr, u32 PTR4* len);
typedef s32  (RADLINK PTR4* BINKSNDUNLOCK)   (struct BINKSND PTR4* BnkSnd, u32 filled);
typedef void (RADLINK PTR4* BINKSNDVOLUME)   (struct BINKSND PTR4* BnkSnd, s32 volume);
typedef void (RADLINK PTR4* BINKSNDPAN)      (struct BINKSND PTR4* BnkSnd, s32 pan);
typedef s32  (RADLINK PTR4* BINKSNDONOFF)    (struct BINKSND PTR4* BnkSnd, s32 status);
typedef s32  (RADLINK PTR4* BINKSNDPAUSE)    (struct BINKSND PTR4* BnkSnd, s32 status);
typedef void (RADLINK PTR4* BINKSNDCLOSE)    (struct BINKSND PTR4* BnkSnd);

typedef BINKSNDOPEN  (RADLINK PTR4* BINKSNDSYSOPEN) (u32 param);

typedef struct BINKSND {
  BINKSNDCLOSE SetParam;
  BINKSNDCLOSE Reset;
  BINKSNDREADY Ready;
  BINKSNDLOCK Lock;
  BINKSNDUNLOCK Unlock;
  BINKSNDVOLUME Volume;
  BINKSNDPAUSE Pause;
  BINKSNDPAUSE Off;
  BINKSNDCLOSE Close;
  u32 BestSizeIn16;
  u32 SoundDropOuts;
  u32 freq;
  s32 bits;
  s32 chans;
  u8 snddata[128];
} BINKSND;

typedef struct BINKRECT {
  s32 Left,Top,Width,Height;
} BINKRECT;

#define BINKMAXDIRTYRECTS 8

typedef struct BUNDLEPOINTERS {
  void* typeptr;
  void* type16ptr;
  void* colorptr;
  void* bits2ptr;
  void* motionXptr;
  void* motionYptr;
  void* dctptr;
  void* mdctptr;
  void* patptr;
} BUNDLEPOINTERS;


typedef struct BINK {
  u32 Width;
  u32 Height;
  u32 Frames;
  u32 FrameNum;
  u32 FrameRate;
  u32 FrameRateDiv;
  u32 ReadError;
  u32 OpenFlags;
  u32 BinkType;
  u32 Size;
  u32 FrameSize;
  u32 SndSize;
  BINKRECT FrameRects[BINKMAXDIRTYRECTS];
  s32 NumRects;
  void PTR4* YPlane;
  u32 YWidth;
  u32 YHeight;
  u32 UVWidth;
  u32 UVHeight;
  void PTR4* MaskPlane;
  u32 MaskPitch;
  u32 MaskLength;
  u32 LargestFrameSize;
  u32 InternalFrames;
  s32 NumTracks;
  u32 PTR4* TrackSizes;
  s32 Paused;
  void PTR4* compframe;
  void PTR4* preloadptr;
  u32 PTR4* frameoffsets;
  BINKIO bio;
  u8 PTR4* ioptr;
  u32 iosize;
  s32 trackindex;
  u32 PTR4* tracktypes;
  s32 PTR4* trackids;
  u32 numrects;
  u32 playedframes;
  u32 firstframetime;
  u32 startframetime;
  u32 startblittime;
  u32 startsynctime;
  u32 startsyncframe;
  u32 twoframestime;
  u32 entireframetime;
  u32 slowestframetime;
  u32 slowestframe;
  u32 slowest2frametime;
  u32 slowest2frame;
  u32 soundon;
  u32 videoon;
  u32 totalmem;
  u32 timedecomp;
  u32 timeblit;
  u32 timeopen;
  u32 fileframerate;
  u32 fileframeratediv;
  u32 threadcontrol;
  u32 runtimeframes;
  u32 runtimemoveamt;
  u32 PTR4* rtframetimes;
  u32 PTR4* rtdecomptimes;
  u32 PTR4* rtblittimes;
  u32 PTR4* rtreadtimes;
  u32 highest1secrate;
  u32 highest1secframe;
  u32 lastdecompframe;
  u32 sndbufsize;
  u8 PTR4* sndbuf;
  u8 PTR4* sndend;
  u8 PTR4* sndwritepos;
  u8 PTR4* sndreadpos;
  u32 sndcomp;
  u32 sndamt;
  volatile u32 sndreenter;
  u32 sndconvert8;
  BINKSND bsnd;
} BINK;


typedef struct BINKSUMMARY {
  u32 Width;
  u32 Height;
  u32 TotalTime;
  u32 FileFrameRate;
  u32 FileFrameRateDiv;
  u32 FrameRate;
  u32 FrameRateDiv;
  u32 TotalOpenTime;
  u32 TotalFrames;
  u32 TotalPlayedFrames;
  u32 SkippedFrames;
  u32 SoundSkips;
  u32 TotalBlitTime;
  u32 TotalReadTime;
  u32 TotalDecompTime;
  u32 TotalBackReadTime;
  u32 TotalReadSpeed;
  u32 SlowestFrameTime;
  u32 Slowest2FrameTime;
  u32 SlowestFrameNum;
  u32 Slowest2FrameNum;
  u32 AverageDataRate;
  u32 AverageFrameSize;
  u32 HighestMemAmount;
  u32 TotalIOMemory;
  u32 HighestIOUsed;
  u32 Highest1SecRate;
  u32 Highest1SecFrame;
} BINKSUMMARY;


typedef struct BINKREALTIME {
  u32 FrameNum;               // Current frame number
  u32 FrameRate;              // frame rate
  u32 FrameRateDiv;           // frame rate divisor
  u32 Frames;                 // frames in this sample period
  u32 FramesTime;             // time is ms for these frames
  u32 FramesVideoDecompTime;  // time decompressing these frames
  u32 FramesAudioDecompTime;  // time decompressing these frames
  u32 FramesReadTime;         // time reading these frames
  u32 FramesBlitTime;         // time blitting these frames
  u32 ReadBufferSize;         // size of read buffer
  u32 ReadBufferUsed;         // amount of read buffer currently used
  u32 FramesDataRate;         // data rate for these frames
} BINKREALTIME;

#define BINKMARKER1 'fKIB'
#define BINKMARKER2 'gKIB'    // new Bink files use this tag

typedef struct BINKHDR {
  u32 Marker;                 // Bink marker
  u32 Size;                   // size of the file-8
  u32 Frames;                 // Number of frames (1 based, 100 = 100 frames)
  u32 LargestFrameSize;       // Size in bytes of largest frame
  u32 InternalFrames;         // Number of internal frames

  u32 Width;                  // Width (1 based, 640 for example)
  u32 Height;                 // Height (1 based, 480 for example)
  u32 FrameRate;              // frame rate
  u32 FrameRateDiv;           // frame rate divisor (framerate/frameratediv=fps)

  u32 Flags;                  // height compression options
  u32 NumTracks;              // number of tracks
} BINKHDR;


//=======================================================================
#define BINKFRAMERATE         0x00020000L // Override fr (call BinkFrameRate first)
#define BINKPRELOADALL        0x00040000L // Preload the entire animation
#define BINKSNDTRACK          0x00080000L // Set the track number to play
#define BINKALPHA             0x00100000L // Decompress alpha plane (if present)
#define BINKGRAYSCALE         0x00200000L // Force Bink to use grayscale
#define BINKNOSKIP            0x00400000L // Don't skip frames if falling behind
#define BINKNOFILLIOBUF       0x00800000L // Fill the IO buffer in SmackOpen
#define BINKSIMULATE          0x01000000L // Simulate the speed (call BinkSim first)
#define BINKFILEHANDLE        0x08000000L // Use when passing in a file handle
#define BINKIOSIZE            0x04000000L // Set an io size (call BinkIOSize first)
#define BINKIOPROCESSOR       0x08000000L // Set an io processor (call BinkIO first)
#define BINKFROMMEMORY        0x40000000L // Use when passing in a pointer to the file
#define BINKNOTHREADEDIO      0x80000000L // Don't use a background thread for IO

#define BINKSURFACEYINTERLACE 0x20000000L // Force interleaving height scaling
#define BINKSURFACEYDOUBLE    0x10000000L // Force doubling height scaling
#define BINKSURFACEYFORCENONE 0x30000000L // Force height scaling off

#define BINKSURFACEFAST       0x00000000L
#define BINKSURFACESLOW       0x80000000L
#define BINKSURFACEDIRECT     0x40000000L

#define BINKSURFACECOPYALL    0x08000000L // copy all pixels (not just changed)
#define BINKSURFACECOPY2XWH   0x04000000L // copy the width and height zoomed by two
#define BINKSURFACECOPY2XW    0x02000000L // copy the width and height zoomed by two
#define BINKSURFACECOPY2XH    0x01000000L // copy the width and height zoomed by two
#define BINKSURFACECOPYNOMMX  0x00800000L // don't use MMX
//#define BINKNOSKIP          0x00400000L // don't skip the blit if behind in sound
//#define BINKGRAYSCALE       0x00200000L // force Bink to use grayscale

#define BINKSURFACE24          1
#define BINKSURFACE32          2
#define BINKSURFACE555         3
#define BINKSURFACE565         4
#define BINKSURFACE655         5
#define BINKSURFACE664         6
#define BINKSURFACE8P          7
#define BINKSURFACEYUY2        8
#define BINKSURFACEUYVY        9
#define BINKSURFACEYV12       10
#define BINKSURFACEMASK       15

#define BINKGOTOQUICK          1

#define BINKGETKEYPREVIOUS     0
#define BINKGETKEYNEXT         1
#define BINKGETKEYCLOSEST      2
#define BINKGETKEYNOTEQUAL   128

//=======================================================================

RADEXPFUNC void PTR4* RADEXPLINK BinkLogoAddress(void);

RADEXPFUNC void RADEXPLINK BinkSetError(const char PTR4* err);
RADEXPFUNC char PTR4* RADEXPLINK BinkGetError(void);

RADEXPFUNC HBINK RADEXPLINK BinkOpen(const char PTR4* name,u32 flags);

#ifdef __RADMAC__
  #include <files.h>

  RADEXPFUNC HBINK RADEXPLINK BinkMacOpen(FSSpec* fsp,u32 flags);
#endif

RADEXPFUNC s32  RADEXPLINK BinkDoFrame(HBINK bnk);
RADEXPFUNC void RADEXPLINK BinkNextFrame(HBINK bnk);
RADEXPFUNC s32  RADEXPLINK BinkWait(HBINK bnk);
RADEXPFUNC void RADEXPLINK BinkClose(HBINK bnk);
RADEXPFUNC s32  RADEXPLINK BinkPause(HBINK bnk,s32 pause);
RADEXPFUNC s32  RADEXPLINK BinkCopyToBuffer(HBINK bnk,void* dest,s32 destpitch,u32 destheight,u32 destx,u32 desty,u32 flags);
RADEXPFUNC s32  RADEXPLINK BinkGetRects(HBINK bnk,u32 flags);
RADEXPFUNC void RADEXPLINK BinkGoto(HBINK bnk,u32 frame,s32 flags);  // use 1 for the first frame
RADEXPFUNC u32  RADEXPLINK BinkGetKeyFrame(HBINK bnk,u32 frame,s32 flags);

RADEXPFUNC s32  RADEXPLINK BinkSetVideoOnOff(HBINK bnk,s32 onoff);
RADEXPFUNC s32  RADEXPLINK BinkSetSoundOnOff(HBINK bnk,s32 onoff);
RADEXPFUNC void RADEXPLINK BinkSetVolume(HBINK bnk,s32 volume);
RADEXPFUNC void RADEXPLINK BinkSetPan(HBINK bnk,s32 pan);
RADEXPFUNC void RADEXPLINK BinkService(HBINK bink);

typedef struct BINKTRACK PTR4* HBINKTRACK;

typedef struct BINKTRACK
{
  u32 Frequency;
  u32 Bits;
  u32 Channels;
  u32 MaxSize;

  HBINK bink;
  u32 sndcomp;
  s32 trackindex;
} BINKTRACK;


RADEXPFUNC HBINKTRACK RADEXPLINK BinkOpenTrack(HBINK bnk,u32 trackindex);
RADEXPFUNC void RADEXPLINK BinkCloseTrack(HBINKTRACK bnkt);
RADEXPFUNC u32  RADEXPLINK BinkGetTrackData(HBINKTRACK bnkt,void PTR4* dest);

RADEXPFUNC u32  RADEXPLINK BinkGetTrackType(HBINK bnk,u32 trackindex);
RADEXPFUNC u32  RADEXPLINK BinkGetTrackID(HBINK bnk,u32 trackindex);
RADEXPFUNC u32  RADEXPLINK BinkGetTrackLargest(HBINK bnk,u32 trackindex);

RADEXPFUNC void RADEXPLINK BinkGetSummary(HBINK bnk,BINKSUMMARY PTR4* sum);
RADEXPFUNC void RADEXPLINK BinkGetRealtime(HBINK bink,BINKREALTIME PTR4* run,u32 frames);

#define BINKNOSOUND 0xffffffff

RADEXPFUNC void RADEXPLINK BinkSetSoundTrack(u32 track);
RADEXPFUNC void RADEXPLINK BinkSetIO(BINKIOOPEN io);
RADEXPFUNC void RADEXPLINK BinkSetFrameRate(u32 forcerate,u32 forceratediv);
RADEXPFUNC void RADEXPLINK BinkSetSimulate(u32 sim);
RADEXPFUNC void RADEXPLINK BinkSetIOSize(u32 iosize);

RADEXPFUNC s32  RADEXPLINK BinkSetSoundSystem(BINKSNDSYSOPEN open, u32 param);

#ifdef __RADWIN__

  RADEXPFUNC BINKSNDOPEN RADEXPLINK BinkOpenDirectSound(u32 param); // don't call directly
  #define BinkSoundUseDirectSound(lpDS) BinkSetSoundSystem(BinkOpenDirectSound,(u32)lpDS)

  #define BinkTimerSetup()
  #define BinkTimerDone()
  #define BinkTimerRead timeGetTime

  #define INCLUDE_MMSYSTEM_H
  #include "windows.h"
  #include "windowsx.h"

  #ifdef __RADNT__          // to combat WIN32_LEAN_AND_MEAN
    #include "mmsystem.h"
  #endif

#endif

#ifndef __RADMAC__

  RADEXPFUNC BINKSNDOPEN RADEXPLINK BinkOpenMiles(u32 param); // don't call directly
  #define BinkSoundUseMiles(hdigdriver) BinkSetSoundSystem(BinkOpenMiles,(u32)hdigdriver)

#endif


#ifndef __RADDOS__

//=========================================================================
typedef struct BINKBUFFER * HBINKBUFFER;

typedef struct BINKBUFFER {
  u32 Width;
  u32 Height;
  u32 WindowWidth;
  u32 WindowHeight;
  u32 SurfaceType;
  void* Buffer;
  s32 BufferPitch;
  s32 ClientOffsetX;
  s32 ClientOffsetY;
  u32 ScreenWidth;
  u32 ScreenHeight;
  u32 ScreenDepth;
  u32 ExtraWindowWidth;
  u32 ExtraWindowHeight;
  u32 ScaleFlags;
  u32 StretchWidth;
  u32 StretchHeight;

  s32 surface;
  void* ddsurface;
  void* ddclipper;
  s32 destx,desty;
  s32 wndx,wndy;
  u32 HWND;
  s32 ddoverlay;
  s32 ddoffscreen;
  s32 lastovershow;

  s32 issoftcur;
  u32 cursorcount;
  void* buffertop;
  u32 type;
  s32 noclipping;

  s32 loadeddd;
  s32 loadedwin;

  void* dibh;
  void* dibbuffer;
  s32 dibpitch;
  void* dibinfo;
  u32 dibdc;
  u32 diboldbitmap;

} BINKBUFFER;


#define BINKBUFFERSTRETCHXINT    0x80000000
#define BINKBUFFERSTRETCHX       0x40000000
#define BINKBUFFERSHRINKXINT     0x20000000
#define BINKBUFFERSHRINKX        0x10000000
#define BINKBUFFERSTRETCHYINT    0x08000000
#define BINKBUFFERSTRETCHY       0x04000000
#define BINKBUFFERSHRINKYINT     0x02000000
#define BINKBUFFERSHRINKY        0x01000000
#define BINKBUFFERRESOLUTION     0x00800000

#define BINKBUFFERAUTO                0
#define BINKBUFFERPRIMARY             1
#define BINKBUFFERDIBSECTION          2
#define BINKBUFFERYV12OVERLAY         3
#define BINKBUFFERYUY2OVERLAY         4
#define BINKBUFFERUYVYOVERLAY         5
#define BINKBUFFERYV12OFFSCREEN       6
#define BINKBUFFERYUY2OFFSCREEN       7
#define BINKBUFFERUYVYOFFSCREEN       8
#define BINKBUFFERRGBOFFSCREENVIDEO   9
#define BINKBUFFERRGBOFFSCREENSYSTEM 10
#define BINKBUFFERTYPEMASK           31

RADEXPFUNC HBINKBUFFER RADEXPLINK BinkBufferOpen( HWND wnd, u32 width, u32 height, u32 bufferflags);
RADEXPFUNC void RADEXPLINK BinkBufferClose( HBINKBUFFER buf);
RADEXPFUNC s32 RADEXPLINK BinkBufferLock( HBINKBUFFER buf);
RADEXPFUNC s32 RADEXPLINK BinkBufferUnlock( HBINKBUFFER buf);
RADEXPFUNC void RADEXPLINK BinkBufferSetResolution( s32 w, s32 h, s32 bits);
RADEXPFUNC void RADEXPLINK BinkBufferCheckWinPos( HBINKBUFFER buf, s32 PTR4* NewWindowX, s32 PTR4* NewWindowY);
RADEXPFUNC s32 RADEXPLINK BinkBufferSetOffset( HBINKBUFFER buf, s32 destx, s32 desty);
RADEXPFUNC void RADEXPLINK BinkBufferBlit( HBINKBUFFER buf, BINKRECT PTR4* rects, u32 numrects );
RADEXPFUNC s32 RADEXPLINK BinkBufferSetScale( HBINKBUFFER buf, u32 w, u32 h);
RADEXPFUNC s32 RADEXPLINK BinkBufferSetHWND( HBINKBUFFER buf, HWND newwnd);
RADEXPFUNC char PTR4* RADEXPLINK BinkBufferGetDescription( HBINKBUFFER buf);
RADEXPFUNC char PTR4* RADEXPLINK BinkBufferGetError();
RADEXPFUNC s32 RADEXPLINK BinkBufferSetDirectDraw(void PTR4* lpDirectDraw, void PTR4* lpPrimary);
RADEXPFUNC s32 RADEXPLINK BinkBufferClear(HBINKBUFFER buf, u32 RGB);

RADEXPFUNC s32 RADEXPLINK BinkDDSurfaceType(void PTR4* lpDDS);
RADEXPFUNC s32 RADEXPLINK BinkIsSoftwareCursor(void PTR4* lpDDSP,HCURSOR cur);
RADEXPFUNC s32 RADEXPLINK BinkCheckCursor(HWND wnd,s32 x,s32 y,s32 w,s32 h);
RADEXPFUNC void RADEXPLINK BinkRestoreCursor(s32 checkcount);

#endif

RADDEFEND

#endif

#endif

