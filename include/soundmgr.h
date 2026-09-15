// soundmgr.h - prototypes of soundmgr.cpp (compiland soundmgr.obj)
#ifndef HOMM3_SOUNDMGR_H
#define HOMM3_SOUNDMGR_H

#include <windows.h>
#include "basemgr.h"
#include "kbwin.h"

void pollSound();

class Sample;
class DsMemsample;

struct AILPrimaryBufferVtable {
    void* m_methods[15];
    long (__stdcall* m_setVolume)(void* self, long volume);
};
struct AILPrimaryBuffer {
    AILPrimaryBufferVtable* m_vtable;
};
// Only pointers cross this header; the SDK definition is included by its consumer.
struct _DIG_DRIVER;
typedef _DIG_DRIVER AILDigitalDriver;

struct AILWaveFormat {
    unsigned short m_formatTag;
    unsigned short m_channels;
    unsigned long m_samplesPerSec;
    unsigned long m_avgBytesPerSec;
    unsigned short m_blockAlign;
    unsigned short m_bitsPerSample;
};

// DC-attested verbatim (LF_FIELDLIST 0x1c9c, Size = 8): the pair a
// loaded-and-playing sample travels as. `playSample` is `void*` in the
// Dreamcast record; retail hands it straight to AIL_sample_status /
// AIL_end_sample, so it is typed as the handle here.
struct SAMPLE2 {
    Sample* m_resSample;
    DsMemsample* m_playSample;
};

// The 12-byte packet launch_sample heap-allocates (`new`, push 0xc) and
// hands to its wait thread: a SAMPLE2 followed by the wait time, laid
// out at +0/+4/+8 exactly as retail stores them. Name unattested.
struct LaunchedSample {
    SAMPLE2 m_sample2;
    int m_maxTime;
};

// The thread entry launch_sample hands to _beginthread, retail
// 0x59a6b0, reached by the _beginthread address-take at 0x59a68e.
// Its retained body is claimed in soundmgr.cpp; the name is provisional.
// The delinker's function grouping does not establish source linkage.
void __cdecl waitEndSampleThread(void* arglist);

// Retail .bss 0x699258, DC-attested name (?NULL_SAMPLE2@@3USAMPLE2@@A):
// the empty pair LoadPlaySample returns on either failure path.
extern SAMPLE2 g_nullSample2;

SAMPLE2 loadPlaySample(const char* sampleName);
void clearMemSample(SAMPLE2 sample2);
void waitEndSample(SAMPLE2 sample2, int milliWait);

namespace ResourceManager {
Sample* getSample(const char* name);
}
SAMPLE2 loadPlaySample(const char* sampleName);
void waitEndSample(SAMPLE2 sample2, int milliWait);
void launchSample(const char* sampleName, int maxTime, int channel);

// The AIL_sample_status return domain. Only the three members this TU
// compares against are modelled, and each one's ROLE is byte-proven -
// but the NAMES are unattested (the Miles headers' own spellings are
// not in evidence in this tree), so they are role placeholders.
// Before normalization (type): EAilSampleStatus.
enum AilSampleStatus {
    AIL_SAMPLE_SLOT_FREE = 2,  // MemorySample takes the slot on this
    AIL_SAMPLE_PLAYING = 4,    // every "is it still running" test
    AIL_SAMPLE_RESUMABLE = 8   // ResumeSamples only resumes on this
};

// The AIL_stream_status twin; a separate function, so a separate
// domain. Name unattested, role byte-proven (StopMP3 only serves and
// re-spawns when the stream reads this).
// Before normalization (type): EAilStreamStatus.
enum AilStreamStatus {
    AIL_STREAM_PLAYING = 4
};

// The sample channel domain. PROVEN extent: the range table at
// 0x684ab8 holds exactly four rows, and MemorySample's "no free slot"
// path compares sample::field_28 against 4 - one past the last channel.
// Before normalization (type): ESoundChannel.
enum SoundChannel {
    SOUND_CHANNEL_COUNT = 4
};

// The PCM sample-width domain fills the wave format and selects
// Open's fallback after a failed driver probe. Values are retail-byte proven;
// names are role placeholders.
// Before normalization (type): ESoundBitsPerSample.
enum SoundBitsPerSample {
    SOUND_BITS_PER_SAMPLE_8 = 8,
    SOUND_BITS_PER_SAMPLE_16 = 16
};

// The soundManager::ConvertVolume iVolumeType domain. Byte-proven:
// 101 selects gUnk698760 and every other value selects gUnk698764;
// SetMusicVolume passes 101, MemorySample and ModifySample pass 100.
// NAMES are unattested ordinal placeholders.
// Before normalization (type): EVolumeType.
enum VolumeType {
    VOLUME_TYPE_100 = 100,
    VOLUME_TYPE_101 = 101
};

// The soundManager::ModifySample sFunction domain. Byte-proven arms:
// 1 and 100 share a body (set the sample volume; 100 additionally
// writes the value back into gAilDriverState), 5 starts the sample.
// NAMES are unattested ordinal placeholders.
// Before normalization (type): ESampleModifyFunction.
enum SampleModifyFunction {
    SAMPLE_MODIFY_1 = 1,
    SAMPLE_MODIFY_5 = 5,
    SAMPLE_MODIFY_100 = 100
};

// soundManager (baseManager base = 0x38, basemgr.h SIZE-asserted).

// Before normalization (type): soundManager.
class SoundManager : public BaseManager {
public:
// Before normalization (type): soundManager::ESampleInfoOperation.
    enum SampleInfoOperation {
        SAMPLE_INFO_VOLUME = 1,
        SAMPLE_INFO_PLAYING = 4
    };

    int m_mssHandle;
    AILDigitalDriver* m_ds;
    int m_samples;
    DsMemsample* m_sampleHandles[14];
    int m_sampleNum;
    // NH3API currentTerrainMusic; retail receiveSaveGame 0x4cbdb7 reads
    // +80 with MOVSX byte and later restores it through switchAmbientMusic.
    // Keep the retail signed-byte width: NH3API's int32 declaration differs.
    signed char m_currentTerrainMusic;
    int m_playSounds;
    int m_changeSounds;
    unsigned char m_mp3Playing;
    CRITICAL_SECTION m_sectionSoundCall;
    CRITICAL_SECTION m_sectionMp3Change;
    CRITICAL_SECTION m_sectionMp3NameChange;

    SoundManager();
    // DC SoundMgr.h:124 (dc 0xe6ebc). Complete's ShutDown (0x4f3690)
    // deletes the manager with this body expanded - the vftable store and the three
    // DeleteCriticalSection calls on +0x90 / +0xa8 / +0xc0 in that order.
    // Non-virtual: the retail vftable 0x63fe54 has only baseManager's
    // three slots.
    ~SoundManager()
    {
        DeleteCriticalSection(&m_sectionSoundCall);
        DeleteCriticalSection(&m_sectionMp3Change);
        DeleteCriticalSection(&m_sectionMp3NameChange);
    }
    virtual int open(int newPriority);
    virtual void close();
    // baseManager's third pure slot. Declared so kb's InitMainClasses can
    // `new` this manager; the vftable at 0x63fe54 already carries the slot.
    virtual int main(Message& msg);
    DsMemsample* memorySample(Sample* samplePointer);
    int getSampleInfo(DsMemsample* inSample, short operation);
    void switchAmbientMusic(int newMusicFileId);
    void stopAllSamples(int stopMusicToo);
    void stopSample(DsMemsample* inSample);
    void waitSample(DsMemsample* sample, int time);
    void modifySample(DsMemsample* inSample, short functionId, long value);
    void adjustSoundVolumes();
    void adjustMusicVolumes();
    int musicPlaying();
    void startMP3(const char* filename, int loopCount, unsigned char stopSamples);
    void stopMP3();
    void resumeStream();          // 0x59ac00
    void resumeSamples();         // 0x599b90, name provisional
    void pauseSamples();          // 0x599c40, name provisional
    void serviceSounds();

    void setMusicVolume();                              // 0x5994b0
    int convertVolume(int volumeValue, int volumeType);  // 0x5996c0
    void threadStopMP3();                               // 0x59b080
};

// Retail .bss 0x699290: non-zero suppresses every sound path (a
// no-sound / silent-mode latch; name provisional).
extern int g_noSound;

extern short g_ailDriverState[14];

// Retail .data 0x691209, a byte that is zero in the image. Read only as
// the second half of the sound-is-on guard `field_84 || gbUnk691209`
// (AdjustMusicVolumes, ResumeSamples, StopAllSamples, PauseSamples,
// MemorySample). Ordinal placeholder - the role is proven, the NAME is
// unattested by any source.
extern unsigned char g_unk691209;

// Retail .bss 0x698760 / 0x698764: the two volume settings
// ConvertVolume selects between - 0x698760 for VOLUME_TYPE_101 (music,
// what SetMusicVolume asks for), 0x698764 otherwise (samples, what
// MemorySample and ModifySample ask for). Each is a 1..10 step that
// ConvertVolume scales by (setting + 1) / 10; outside that range it
// yields 0, which is also why MemorySample and launch_sample can use
// 0x698764 as a plain "sound is configured on" gate. Ordinal
// placeholders - names unattested.
extern int g_unk698760;
extern int g_unk698764;
extern int g_unk698a28;

// Retail PC Miles initialization state used only by Open. The three .data
// configuration dwords begin at 0x684aa8; the 16-byte PCM descriptor is at
// 0x69fe80; and the successful sample-handle count occupies 0x684ae0.
extern int g_soundSampleRate;
extern int g_soundBitsPerSample;
extern int g_soundOutputChannels;
extern int g_soundMaxSamples;
extern AILWaveFormat g_soundWaveFormat;

// Retail .bss 0x69fe78: the Miles stream handle. Named from the import
// contract - it is the sole argument to AIL_stream_status and
// AIL_service_stream everywhere it appears.
extern void* g_mp3Stream;

// Retail .bss 0x6a3258: a 14-entry side table parallel to
// soundManager::sampleHandles. PauseSamples writes
// `AIL_sample_status(h) == 4` into it before stopping each sample,
// ResumeSamples resumes the flagged slots, and both StopAllSamples and
// ResumeSamples clear all 14 entries with one `rep stosd`.
extern int g_sampleWasPlaying[14];

// The per-channel slot ranges MemorySample allocates out of. PROVEN
// (2026-08-07): MemorySample indexes this table with sample::field_28
// through `lea eax,[ch+2*ch]; lea edi,[4*eax + 0x684ab8]`, i.e. a
// 12-byte stride, and reads three ints from it. The retail image holds
// exactly four rows - {0,1,0} {1,2,1} {2,6,2} {6,14,6} - after which
// 0x684ae8 starts the terrain-music name table, so the channel domain
// is 0..3 and the slot domain is [0,14). Member and type names are
// unattested; the roles are byte-proven.
struct SoundChannelRange {
    int m_first;
    int m_last;
    int m_next;
};
extern SoundChannelRange g_soundChannels[4];  // 0x684ab8

// Retail .bss 0x6a3290 and 0x6a3394, 0x104 bytes apart: the pending MP3
// name and the copy ResumeStream promotes it to under
// section_MP3_name_change. Names provisional.
extern char g_mp3Name[260];
extern char g_mp3NamePlaying[260];

// Retail .bss 0x69fe90 / 0x69fe9c: a dword ResumeStream copies from the
// first to the second alongside the name promotion. Ordinal
// placeholders - names unattested.
extern int g_unk69fe90;
extern int g_unk69fe9c;

// Retail .bss 0x69fec0: fifty 0x108-byte playback-position records. The
// 260-byte name and trailing dword are forced by ThreadStopMP3's stride,
// inline strcmp/strcpy loops and AIL_stream_position store. The count is the
// dword at 0x6a3498. Names are provisional; the PC cache has no DC twin.
struct MP3ResumePosition {
    char m_name[260];
    int m_position;
};
extern MP3ResumePosition g_mp3ResumePositions[50];
extern int g_mp3ResumePositionCount;

void __cdecl processMP3Stop(void* nothing);

void __cdecl processStopAndPlayMP3(void* arglist);

// Retail .rdata/.data 0x684ae8: the nine terrain music base names
// SwitchAmbientMusic hands to StartMP3, indexed [id - 2] for ids 2..10
// ("Water", "Grass", "Snow", "Swamp", "Lava", "Sand", "Dirt", "Rough",
// "Underground"). The folded base retail encodes is 0x684ae0 = the
// array minus the two-dword bias, which is why the delinker invented a
// data symbol there.
extern const char* const g_terrainMusic[9];

// Retail .data 0x678330: terrain -> music id, the nine bytes
// {8,7,3,4,5,9,10,6,2} read straight from the image. SetMusicVolume
// indexes it with advManager::field_58 and hands the result to the same
// [id - 2] terrain-name lookup SwitchAmbientMusic uses, which is what
// bounds the id domain to 2..10. Name provisional.
extern unsigned char g_terrainMusicIds[9];

// Miles Sound System imports. The DLL exports carry their own leading
// underscore (retail IAT: __imp___AIL_end_sample@4), which is Miles'
// own header convention: `_AIL_*` dllimports behind `AIL_*` aliases.
extern "C" {
// Before normalization (function): _AIL_end_sample.
__declspec(dllimport) void __stdcall ailEndSample(DsMemsample* sample);
// Before normalization (function): _AIL_sample_status.
__declspec(dllimport) int __stdcall ailSampleStatus(DsMemsample* sample);
// Before normalization (function): _AIL_sample_volume.
__declspec(dllimport) int __stdcall ailSampleVolume(DsMemsample* sample);
// Before normalization (function): _AIL_stop_sample.
__declspec(dllimport) void __stdcall ailStopSample(DsMemsample* sample);
// Before normalization (function): _AIL_resume_sample.
__declspec(dllimport) void __stdcall ailResumeSample(DsMemsample* sample);
// Before normalization (function): _AIL_init_sample.
__declspec(dllimport) void __stdcall ailInitSample(DsMemsample* sample);
// Before normalization (function): _AIL_start_sample.
__declspec(dllimport) void __stdcall ailStartSample(DsMemsample* sample);
// Before normalization (function): _AIL_set_sample_file.
__declspec(dllimport) int __stdcall ailSetSampleFile(DsMemsample* sample,
                                                         const void* start,
                                                         int block);
// Before normalization (function): _AIL_set_sample_loop_count.
__declspec(dllimport) void __stdcall ailSetSampleLoopCount(DsMemsample* sample,
                                                                int loops);
// Before normalization (function): _AIL_set_sample_volume.
__declspec(dllimport) void __stdcall ailSetSampleVolume(DsMemsample* sample,
                                                            int volume);
// Before normalization (function): _AIL_stream_status.
__declspec(dllimport) int __stdcall ailStreamStatus(void* stream);
// Before normalization (function): _AIL_stream_position.
__declspec(dllimport) int __stdcall ailStreamPosition(void* stream);
// Before normalization (function): _AIL_stream_volume.
__declspec(dllimport) int __stdcall ailStreamVolume(void* stream);
// Before normalization (function): _AIL_open_stream.
__declspec(dllimport) void* __stdcall ailOpenStream(void* driver,
                                                       const char* filename,
                                                       int streamMem);
// Before normalization (function): _AIL_set_stream_loop_count.
__declspec(dllimport) void __stdcall ailSetStreamLoopCount(void* stream,
                                                                int loops);
// Before normalization (function): _AIL_start_stream.
__declspec(dllimport) void __stdcall ailStartStream(void* stream);
// Before normalization (function): _AIL_set_stream_position.
__declspec(dllimport) void __stdcall ailSetStreamPosition(void* stream,
                                                              int position);
// Before normalization (function): _AIL_set_stream_volume.
__declspec(dllimport) void __stdcall ailSetStreamVolume(void* stream, int volume);
// Before normalization (function): _AIL_service_stream.
__declspec(dllimport) void __stdcall ailServiceStream(void* stream, int fillup);
// Before normalization (function): _AIL_pause_stream.
__declspec(dllimport) void __stdcall ailPauseStream(void* stream, int pause);
// Before normalization (function): _AIL_close_stream.
__declspec(dllimport) void __stdcall ailCloseStream(void* stream);
// Before normalization (function): _AIL_shutdown.
__declspec(dllimport) void __stdcall ailShutdown();
// Before normalization (function): _AIL_serve.
__declspec(dllimport) void __stdcall ailServe();
// Before normalization (function): _AIL_startup.
__declspec(dllimport) void __stdcall ailStartup();
// Before normalization (function): _AIL_set_preference.
__declspec(dllimport) int __stdcall ailSetPreference(int preference,
                                                        int value);
// Before normalization (function): _AIL_get_preference.
__declspec(dllimport) int __stdcall ailGetPreference(int preference);
// Before normalization (function): _AIL_HWND.
__declspec(dllimport) void __stdcall ailHWND();
// Before normalization (function): _AIL_waveOutOpen.
__declspec(dllimport) int __stdcall ailWaveOutOpen(
    AILDigitalDriver** driver, void* waveOut, int device,
    AILWaveFormat* format);
// Before normalization (function): _AIL_waveOutClose.
__declspec(dllimport) void __stdcall ailWaveOutClose(
    AILDigitalDriver* driver);
// Before normalization (function): _AIL_digital_configuration.
__declspec(dllimport) void __stdcall ailDigitalConfiguration(
    AILDigitalDriver* driver, int* rate, int* format, char* description);
// Before normalization (function): _AIL_allocate_sample_handle.
__declspec(dllimport) DsMemsample* __stdcall ailAllocateSampleHandle(
    AILDigitalDriver* driver);
// Before normalization (function): _SmackSoundUseMSS.
__declspec(dllimport) unsigned char __stdcall smacksoundusemss(
    AILDigitalDriver* driver);
typedef void* (__stdcall* BinkOpenMilesProc)(void*);
// Before normalization (function): _BinkOpenMiles.
__declspec(dllimport) void* __stdcall binkopenmiles(void* soundSystem);
// Before normalization (function): _BinkSetSoundSystem.
__declspec(dllimport) int __stdcall binksetsoundsystem(
    BinkOpenMilesProc openSound, AILDigitalDriver* driver);
}
#define AIL_end_sample ailEndSample
#define AIL_sample_status ailSampleStatus
#define AIL_sample_volume ailSampleVolume
#define AIL_stop_sample ailStopSample
#define AIL_resume_sample ailResumeSample
#define AIL_init_sample ailInitSample
#define AIL_start_sample ailStartSample
#define AIL_set_sample_file ailSetSampleFile
#define AIL_set_sample_loop_count ailSetSampleLoopCount
#define AIL_set_sample_volume ailSetSampleVolume
#define AIL_stream_status ailStreamStatus
#define AIL_stream_position ailStreamPosition
#define AIL_stream_volume ailStreamVolume
#define AIL_open_stream ailOpenStream
#define AIL_set_stream_loop_count ailSetStreamLoopCount
#define AIL_start_stream ailStartStream
#define AIL_set_stream_position ailSetStreamPosition
#define AIL_set_stream_volume ailSetStreamVolume
#define AIL_service_stream ailServiceStream
#define AIL_pause_stream ailPauseStream
#define AIL_close_stream ailCloseStream
#define AIL_shutdown ailShutdown
#define AIL_serve ailServe
#define AIL_startup ailStartup
#define AIL_set_preference ailSetPreference
#define AIL_get_preference ailGetPreference
#define AIL_HWND ailHWND
#define AIL_waveOutOpen ailWaveOutOpen
#define AIL_waveOutClose ailWaveOutClose
#define AIL_digital_configuration ailDigitalConfiguration
#define AIL_allocate_sample_handle ailAllocateSampleHandle
#define SmackSoundUseMSS smacksoundusemss
#define BinkOpenMiles binkopenmiles
#define BinkSetSoundSystem binksetsoundsystem

// The CRT thread spawner retail reaches with a plain `call __beginthread`
// (msvcrt, __cdecl). Declared here rather than via <process.h> so the
// TU's import-call forms stay under this header's control.
// Before normalization (function): _beginthread.
extern "C" unsigned long __cdecl beginthread(void(__cdecl* startAddress)(void*),
                                              unsigned stackSize,
                                              void* arglist);
// Before normalization (function): _endthread.
extern "C" void __cdecl endthread(void);

// Retail .bss 0x2993c4 (DC ?gpSoundManager@@3PAVsoundManager@@A).
extern SoundManager* g_soundManager;

// E:\gamedcs\SoundMgr.h:140, dc 0xe6ef4
VA(0x0059a7d0, 0x51)  // dc 0xe6ef4
inline void SoundManager::serviceSounds()
{
    EnterCriticalSection(&m_sectionSoundCall);
    AIL_serve();
    void* stream = g_mp3Stream;
    if (stream) {
        if (g_soundManager->m_mp3Playing) {
            if (!g_shutDownDone)
                AIL_service_stream(stream, 1);
        }
    }
    Sleep(1);
    LeaveCriticalSection(&m_sectionSoundCall);
}

// --- globals ---

// --- soundManager ---
// CODEVIEW(E:\gamedcs\soundmgr.cpp:322, dc 0x14b240) int soundManager::Open(int newPriority);
// CODEVIEW(E:\gamedcs\soundmgr.cpp:464, dc 0x14b2a4) int soundManager::Main(message* msg);

#endif  /* HOMM3_SOUNDMGR_H */
