#ifndef HOMM3_SOUNDMGR_H
#define HOMM3_SOUNDMGR_H

#include <windows.h>
#include "basemgr.h"
#include "kbwin.h"

void pollSound();

class sample;
class ds_memsample;

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

// Miles 5.0e HSTREAM is an opaque _STREAM pointer.
struct _STREAM;
typedef _STREAM* HSTREAM;

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
    sample* m_resSample;
    ds_memsample* m_playSample;
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
sample* getSample(const char* name);
}
SAMPLE2 loadPlaySample(const char* sampleName);
void waitEndSample(SAMPLE2 sample2, int milliWait);
void launchSample(const char* sampleName, int maxTime, int channel);

// The AIL_sample_status return domain. Only the three members this TU
// compares against are modelled, and each one's ROLE is byte-proven -
// but the NAMES are unattested (the Miles headers' own spellings are
// not in evidence in this tree), so they are role placeholders.
enum EAilSampleStatus {
    AIL_SAMPLE_SLOT_FREE = 2,  // MemorySample takes the slot on this
    AIL_SAMPLE_PLAYING = 4,    // every "is it still running" test
    AIL_SAMPLE_RESUMABLE = 8   // ResumeSamples only resumes on this
};

// The AIL_stream_status twin; a separate function, so a separate
// domain. Name unattested, role byte-proven (StopMP3 only serves and
// re-spawns when the stream reads this).
enum EAilStreamStatus {
    AIL_STREAM_PLAYING = 4
};

// The sample channel domain. PROVEN extent: the range table at
// 0x684ab8 holds exactly four rows, and MemorySample's "no free slot"
// path compares sample::field_28 against 4 - one past the last channel.
enum ESoundChannel {
    SOUND_CHANNEL_COUNT = 4
};

// The PCM sample-width domain fills the wave format and selects
// Open's fallback after a failed driver probe. Values are retail-byte proven;
// names are role placeholders.
enum ESoundBitsPerSample {
    SOUND_BITS_PER_SAMPLE_8 = 8,
    SOUND_BITS_PER_SAMPLE_16 = 16
};

// The soundManager::ConvertVolume iVolumeType domain. Byte-proven:
// 101 selects gUnk698760 and every other value selects gUnk698764;
// SetMusicVolume passes 101, MemorySample and ModifySample pass 100.
// NAMES are unattested ordinal placeholders.
enum EVolumeType {
    VOLUME_TYPE_100 = 100,
    VOLUME_TYPE_101 = 101
};

// The soundManager::ModifySample sFunction domain. Byte-proven arms:
// 1 and 100 share a body (set the sample volume; 100 additionally
// writes the value back into gAilDriverState), 5 starts the sample.
// NAMES are unattested ordinal placeholders.
enum ESampleModifyFunction {
    SAMPLE_MODIFY_1 = 1,
    SAMPLE_MODIFY_5 = 5,
    SAMPLE_MODIFY_100 = 100
};

// soundManager (baseManager base = 0x38, basemgr.h SIZE-asserted).

class soundManager : public baseManager {
public:
    enum ESampleInfoOperation {
        SAMPLE_INFO_VOLUME = 1,
        SAMPLE_INFO_PLAYING = 4
    };

    int m_mssHandle;
    AILDigitalDriver* m_ds;
    int m_samples;
    ds_memsample* m_sampleHandles[14];
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

    soundManager();
    // DC SoundMgr.h:124 (dc 0xe6ebc). Complete's ShutDown (0x4f3690)
    // deletes the manager with this body expanded - the vftable store and the three
    // DeleteCriticalSection calls on +0x90 / +0xa8 / +0xc0 in that order.
    // Non-virtual: the retail vftable 0x63fe54 has only baseManager's
    // three slots.
    ~soundManager()
    {
        DeleteCriticalSection(&m_sectionSoundCall);
        DeleteCriticalSection(&m_sectionMp3Change);
        DeleteCriticalSection(&m_sectionMp3NameChange);
    }
    virtual int open(int newPriority);
    virtual void close();
    // baseManager's third pure slot. Declared so kb's InitMainClasses can
    // `new` this manager; the vftable at 0x63fe54 already carries the slot.
    virtual int main(message& msg);
    ds_memsample* memorySample(sample* samplePointer);
    int getSampleInfo(ds_memsample* inSample, short operation);
    void switchAmbientMusic(int newMusicFileId);
    void stopAllSamples(int stopMusicToo);
    void stopSample(ds_memsample* inSample);
    void waitSample(ds_memsample* sample, int time);
    void modifySample(ds_memsample* inSample, short functionId, long value);
    void adjustSoundVolumes();
    void adjustMusicVolumes();
    int musicPlaying();
    void startMP3(const char* filename, int loopCount, unsigned char stopSamples);
    void stopMP3();
    void resumeStream();          // 0x59ac00
    void resumeSamples();         // 0x599b90, name provisional
    void pauseSamples();          // 0x599c40, name provisional
    // WinCE defines the no-op service_sounds in SoundMgr.h:140. Complete's Miles
    // implementation is defined in soundmgr.cpp; see its platform evidence comment.
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
extern HSTREAM g_mp3Stream;

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
__declspec(dllimport) void __stdcall _AIL_end_sample(ds_memsample* sample);
__declspec(dllimport) int __stdcall _AIL_sample_status(ds_memsample* sample);
__declspec(dllimport) int __stdcall _AIL_sample_volume(ds_memsample* sample);
__declspec(dllimport) void __stdcall _AIL_stop_sample(ds_memsample* sample);
__declspec(dllimport) void __stdcall _AIL_resume_sample(ds_memsample* sample);
__declspec(dllimport) void __stdcall _AIL_init_sample(ds_memsample* sample);
__declspec(dllimport) void __stdcall _AIL_start_sample(ds_memsample* sample);
__declspec(dllimport) int __stdcall _AIL_set_sample_file(ds_memsample* sample,
                                                         const void* start,
                                                         int block);
__declspec(dllimport) void __stdcall _AIL_set_sample_loop_count(ds_memsample* sample,
                                                                int loops);
__declspec(dllimport) void __stdcall _AIL_set_sample_volume(ds_memsample* sample,
                                                            int volume);
// Stream signatures follow Miles 5.0e Mss.h:3074..3104; S32 is long.
__declspec(dllimport) long __stdcall _AIL_stream_status(HSTREAM stream);
__declspec(dllimport) long __stdcall _AIL_stream_position(HSTREAM stream);
__declspec(dllimport) long __stdcall _AIL_stream_volume(HSTREAM stream);
__declspec(dllimport) HSTREAM __stdcall _AIL_open_stream(AILDigitalDriver* driver,
                                                       char* filename,
                                                       long streamMem);
__declspec(dllimport) void __stdcall _AIL_set_stream_loop_count(HSTREAM stream,
                                                              long loops);
__declspec(dllimport) void __stdcall _AIL_start_stream(HSTREAM stream);
__declspec(dllimport) void __stdcall _AIL_set_stream_position(HSTREAM stream,
                                                            long position);
__declspec(dllimport) void __stdcall _AIL_set_stream_volume(HSTREAM stream,
                                                          long volume);
__declspec(dllimport) long __stdcall _AIL_service_stream(HSTREAM stream,
                                                       long fillup);
__declspec(dllimport) void __stdcall _AIL_pause_stream(HSTREAM stream, long pause);
__declspec(dllimport) void __stdcall _AIL_close_stream(HSTREAM stream);
__declspec(dllimport) void __stdcall _AIL_shutdown();
__declspec(dllimport) void __stdcall _AIL_serve();
__declspec(dllimport) void __stdcall _AIL_startup();
__declspec(dllimport) int __stdcall _AIL_set_preference(int preference,
                                                        int value);
__declspec(dllimport) int __stdcall _AIL_get_preference(int preference);
__declspec(dllimport) void __stdcall _AIL_HWND();
__declspec(dllimport) int __stdcall _AIL_waveOutOpen(
    AILDigitalDriver** driver, void* waveOut, int device,
    AILWaveFormat* format);
__declspec(dllimport) void __stdcall _AIL_waveOutClose(
    AILDigitalDriver* driver);
__declspec(dllimport) void __stdcall _AIL_digital_configuration(
    AILDigitalDriver* driver, int* rate, int* format, char* description);
__declspec(dllimport) ds_memsample* __stdcall _AIL_allocate_sample_handle(
    AILDigitalDriver* driver);
__declspec(dllimport) unsigned char __stdcall _SmackSoundUseMSS(
    AILDigitalDriver* driver);
typedef void* (__stdcall* BinkOpenMilesProc)(void*);
__declspec(dllimport) void* __stdcall _BinkOpenMiles(void* soundSystem);
__declspec(dllimport) int __stdcall _BinkSetSoundSystem(
    BinkOpenMilesProc openSound, AILDigitalDriver* driver);
}
#define AIL_end_sample _AIL_end_sample
#define AIL_sample_status _AIL_sample_status
#define AIL_sample_volume _AIL_sample_volume
#define AIL_stop_sample _AIL_stop_sample
#define AIL_resume_sample _AIL_resume_sample
#define AIL_init_sample _AIL_init_sample
#define AIL_start_sample _AIL_start_sample
#define AIL_set_sample_file _AIL_set_sample_file
#define AIL_set_sample_loop_count _AIL_set_sample_loop_count
#define AIL_set_sample_volume _AIL_set_sample_volume
#define AIL_stream_status _AIL_stream_status
#define AIL_stream_position _AIL_stream_position
#define AIL_stream_volume _AIL_stream_volume
#define AIL_open_stream _AIL_open_stream
#define AIL_set_stream_loop_count _AIL_set_stream_loop_count
#define AIL_start_stream _AIL_start_stream
#define AIL_set_stream_position _AIL_set_stream_position
#define AIL_set_stream_volume _AIL_set_stream_volume
#define AIL_service_stream _AIL_service_stream
#define AIL_pause_stream _AIL_pause_stream
#define AIL_close_stream _AIL_close_stream
#define AIL_shutdown _AIL_shutdown
#define AIL_serve _AIL_serve
#define AIL_startup _AIL_startup
#define AIL_set_preference _AIL_set_preference
#define AIL_get_preference _AIL_get_preference
#define AIL_HWND _AIL_HWND
#define AIL_waveOutOpen _AIL_waveOutOpen
#define AIL_waveOutClose _AIL_waveOutClose
#define AIL_digital_configuration _AIL_digital_configuration
#define AIL_allocate_sample_handle _AIL_allocate_sample_handle
#define SmackSoundUseMSS _SmackSoundUseMSS
#define BinkOpenMiles _BinkOpenMiles
#define BinkSetSoundSystem _BinkSetSoundSystem

// The CRT thread spawner retail reaches with a plain `call __beginthread`
// (msvcrt, __cdecl). Declared here rather than via <process.h> so the
// TU's import-call forms stay under this header's control.
extern "C" unsigned long __cdecl _beginthread(void(__cdecl* startAddress)(void*),
                                              unsigned stackSize,
                                              void* arglist);
extern "C" void __cdecl _endthread(void);

// Retail .bss 0x2993c4 (DC ?gpSoundManager@@3PAVsoundManager@@A).
extern soundManager* g_soundManager;


#endif  /* HOMM3_SOUNDMGR_H */
