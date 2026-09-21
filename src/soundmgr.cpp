#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "soundmgr.h"

// SetMusicVolume's three cross-TU views: the combat manager's status
// (which selects combat music), the adventure manager's terrain field,
// and Random.
#include "advmgr.h"
#include "cmbtmgr.h"
#include "kb.h"
#include "kbwin.h"
#include "misc.h"
#include "sample.h"
#include "smackmgr.h"
#include "terrain.h"
#include "va.h"

// Number of live asynchronous sample waiters. WaitEndSampleThread increments
// and decrements this counter; Close gives them up to one second to drain.
// The role and storage are retail-byte-proven; no surviving name covers it.
DATA(0x006a3254) int g_sampleWorkerCount;
DATA(0x0069fec0) MP3ResumePosition g_mp3ResumePositions[50];
DATA(0x006a3498) int g_mp3ResumePositionCount;
DATA(0x00684aa8) int g_soundSampleRate = 44100;
DATA(0x00684aac) int g_soundBitsPerSample = SOUND_BITS_PER_SAMPLE_16;
DATA(0x00684ab0) int g_soundOutputChannels = 2;
DATA(0x00684ae0) int g_soundMaxSamples = 14;
DATA(0x0069fe80) PCMWAVEFORMAT g_soundWaveFormat;
DATA(0x00698a28) int g_unk698a28;

VA(0x005994b0, 0x210)  // dc 0x14b07c
void soundManager::setMusicVolume()
{
    if (g_noSound)
        return;

    int vol = convertVolume(127, VOLUME_TYPE_101);

    EnterCriticalSection(&g_soundManager->m_sectionMp3Change);
    EnterCriticalSection(&g_soundManager->m_sectionSoundCall);
    if (vol != 0) {
        if (g_mp3Stream) {
            if (m_mp3Playing)
                AIL_set_stream_volume(g_mp3Stream, vol);
            else
                resumeStream();
        } else if (g_combatManager->m_status) {
            char name[100];

            sprintf(name, DATA_COMPGEN(0x0066fedc, combatMusicFormat,
                                        "combat%02d"),
                    random(1, 4));
            g_soundManager->startMP3(name, 0, 1);
        } else {
            int musicFileId = g_terrainMusicIds[g_advManager->m_lastTerrain];
            if (musicFileId >= 2 && musicFileId <= 10)
                startMP3(g_terrainMusic[musicFileId - 2], 0, 0);
        }
    } else {
        stopMP3();
    }
    LeaveCriticalSection(&m_sectionSoundCall);
    LeaveCriticalSection(&m_sectionMp3Change);
}

VA(0x005996c0, 0x97)  // dc 0x14b170
int soundManager::convertVolume(int volumeValue, int volumeType)
{
    int result = 0;
    if (volumeType == VOLUME_TYPE_101) {
        const int& setting = g_unk698760;
        if (setting >= 1 && setting <= 10) {
            result = (setting + 1) * volumeValue / 10;
            if (result < 1)
                result = 1;
        }
    } else {
        const int& setting = g_unk698764;
        if (setting >= 1 && setting <= 10) {
            result = (setting + 1) * volumeValue / 10;
            if (result < 1)
                result = 1;
        }
    }
    if (result < 0)
        result = 0;
    if (result > 127)
        result = 127;
    return result;
}
VA(0x00599760, 0x67)  // dc 0x14b1ec
soundManager::soundManager()
    : m_mp3Playing(0)
{
    m_status = 0;
    memset(g_ailDriverState, 0, sizeof(g_ailDriverState));
    memset(&m_samples, 0, sizeof(m_samples) + sizeof(m_sampleHandles) + sizeof(m_sampleNum));
    m_playSounds = 0;
    m_ds = 0;
    m_changeSounds = 0;
    InitializeCriticalSection(&m_sectionSoundCall);
    InitializeCriticalSection(&m_sectionMp3Change);
    InitializeCriticalSection(&m_sectionMp3NameChange);
}

// E:\gamedcs\soundmgr.cpp:322
// Vtable slot 0 and the unique Device:/Miles setup body independently pin
// this retail expansion of soundManager::Open. The DC body is much smaller
// because it uses ds_engine; Complete performs the PC waveOut preference
// fallback, Smacker/Bink binding and twelve-handle allocation here.

// Residual (84.53%): the best source has retail's 17 branches, one return,
// complete middleware call/data flow and 703-byte target extent. Two retry
// branches still target blocks in the opposite physical order, and C2 keeps
// `this` in EBX while retail keeps it in ESI (homed while ESI carries the
// channel count) and holds AIL_set_preference in EBX. Four grounded shapes
// were exhausted: structured retry plus a post-loop driver test (83.56%, one
// extra branch), explicit-goto retry (67.80%, wrong block order), the direct
// result-carrier loop below (84.53%), and an explicit long-lived
// set-preference pointer (same bytes). The remaining layout/RA choice is not
// source-addressable without distorting the proven retry semantics.
// The pointer probe is removed: five direct AIL_set_preference source calls
// preserve 84.5280% and clear the audit's five unresolved indirect-call gaps.
// Retail's cached import pointer is an optimizer result, not source proof of
// a local function pointer. Further source hypotheses remain possible.
VA(0x005997d0, 0x2BF)  // vtable slot + Device: string, dc 0x14b240
int soundManager::open(int newPriority)
{
    m_currentTerrainMusic = 0xff;
    memset(&m_samples, 0,
           sizeof(m_samples) + sizeof(m_sampleHandles) + sizeof(m_sampleNum));

    if (!g_noSound) {
        AIL_startup();
        if (!g_unk698a28 && !m_ds) {
            AIL_set_preference(15, 0);
            AIL_set_preference(33, 1);
            AIL_set_preference(34, 100);

            HDIGDRIVER driver;
            HDIGDRIVER result;
            for (;;) {
                if (g_soundSampleRate < 11025) {
                    result = 0;
                    break;
                }

                g_soundWaveFormat.wf.wFormatTag = 1;
                g_soundWaveFormat.wf.nChannels =
                    static_cast<unsigned short>(g_soundOutputChannels);
                g_soundWaveFormat.wf.nSamplesPerSec = g_soundSampleRate;
                g_soundWaveFormat.wf.nAvgBytesPerSec =
                    (g_soundBitsPerSample / 8) * g_soundOutputChannels
                    * g_soundSampleRate;
                g_soundWaveFormat.wf.nBlockAlign = static_cast<unsigned short>(
                    (g_soundBitsPerSample / 8) * g_soundOutputChannels);
                g_soundWaveFormat.wBitsPerSample =
                    static_cast<unsigned short>(g_soundBitsPerSample);

                AIL_HWND();
                int openResult = AIL_waveOutOpen(
                    &driver, 0, -1, &g_soundWaveFormat.wf);
                if (!openResult) {
                    char description[128];
                    strcpy(description, DATA_COMPGEN(
                        0x00684b28, soundDevicePrefix, "Device: "));
                    AIL_digital_configuration(
                        driver, 0, 0, description + strlen(description));
                    if (AIL_get_preference(15)) {
                        result = driver;
                        break;
                    }
                    if (!strstr(description, DATA_COMPGEN(
                            0x00684b1c, emulatedDeviceMarker,
                            "Emulated"))) {
                        result = driver;
                        break;
                    }
                    AIL_waveOutClose(driver);
                    AIL_set_preference(15, 1);
                } else if (AIL_get_preference(15)) {
                    g_soundSampleRate /= 2;
                    if (g_soundSampleRate >= 11025)
                        continue;
                    if (g_soundBitsPerSample == SOUND_BITS_PER_SAMPLE_8) {
                        g_soundBitsPerSample = SOUND_BITS_PER_SAMPLE_8;
                        g_soundSampleRate = 22050;
                        continue;
                    }
                    result = 0;
                    break;
                }
                AIL_set_preference(15, 1);
            }
            m_ds = result;
        }

        if (!m_ds) {
            g_unk698764 = 0;
        } else {
            if (g_soundManager->m_ds->lppdsb) {
                LPDIRECTSOUNDBUFFER buffer = static_cast<LPDIRECTSOUNDBUFFER>(
                    g_soundManager->m_ds->lppdsb);
                buffer->SetVolume(0);
            }
            SmackSoundUseMSS(g_soundManager->m_ds);
            BinkSoundUseMiles(g_soundManager->m_ds);
        }
        m_playSounds = 1;

        if (!g_noSound && m_ds) {
            int count;
            for (count = 0; count < 12; ++count) {
                m_sampleHandles[count] = AIL_allocate_sample_handle(m_ds);
                if (!m_sampleHandles[count])
                    break;
            }
            m_sampleNum = count;
            g_soundMaxSamples = count;
        }
        m_samples = 1;
    }

    m_id = 16;
    m_priority = -1;
    m_status = STATUS_ACTIVE;
    strcpy(m_mgrName, DATA_COMPGEN(
        0x00684b0c, soundManagerName, "soundManager"));
    return 0;
}

VA(0x00599a90, 0xF1)  // dc 0x14b270
void soundManager::close()
{
    if (m_status == STATUS_ACTIVE) {
        g_soundManager->m_playSounds = 1;
        g_unk691209 = 0;
        videoShutDown();

        if (!g_noSound) {
            for (int waits = 0; waits < 20; ++waits) {
                if (g_sampleWorkerCount <= 0)
                    break;
                Sleep(50);
            }

            EnterCriticalSection(&m_sectionMp3Change);
            EnterCriticalSection(&m_sectionSoundCall);
            if (g_mp3Stream) {
                AIL_pause_stream(g_mp3Stream, 1);
                AIL_close_stream(g_mp3Stream);
                g_mp3Stream = 0;
            }
            for (int i = 0; i < m_sampleNum; ++i)
                AIL_end_sample(m_sampleHandles[i]);
            m_samples = 0;
            AIL_serve();
            Sleep(1);
            AIL_shutdown();
            LeaveCriticalSection(&m_sectionSoundCall);
            LeaveCriticalSection(&m_sectionMp3Change);
        }

        m_status = 0;
        g_noSound = 1;
    }
}

// Complete's desktop activation pair: AppWndProc's WM_ACTIVATEAPP
// calls PauseSamples on

VA(0x00599b90, 0xAB)
void soundManager::resumeSamples()
{
    if (g_noSound)
        return;
    if (!m_ds)
        return;
    if (m_playSounds == 0 && !g_unk691209)
        return;
    EnterCriticalSection(&m_sectionSoundCall);
    for (int i = 0; i < m_sampleNum; i++)
        if (g_sampleWasPlaying[i] && AIL_sample_status(m_sampleHandles[i]) == AIL_SAMPLE_RESUMABLE)
            AIL_resume_sample(m_sampleHandles[i]);
    LeaveCriticalSection(&m_sectionSoundCall);
    memset(g_sampleWasPlaying, 0, sizeof(g_sampleWasPlaying));
}

VA(0x00599c40, 0x14B)
void soundManager::pauseSamples()
{
    if (g_noSound)
        return;
    if (!m_ds)
        return;
    if (m_playSounds == 0 && !g_unk691209)
        return;
    EnterCriticalSection(&m_sectionSoundCall);
    for (int i = 0; i < m_sampleNum; i++) {
        g_sampleWasPlaying[i] = getSampleInfo(m_sampleHandles[i], SAMPLE_INFO_PLAYING);
        AIL_stop_sample(m_sampleHandles[i]);
    }
    LeaveCriticalSection(&m_sectionSoundCall);
    stopMP3();
}

// Original: soundManager::Main; soundmgr.cpp:464, dc 0x14b2a4.
// The slot at retail vftable 0x63fe54 likewise uses the shared zero return.
int soundManager::main(message& msg)
{
    return 0;
}

VA(0x00599d90, 0xEA)  // dc 0x14b2a8
void soundManager::stopAllSamples(int stopMusicToo)
{
    if (g_noSound)
        return;
    if (!m_ds)
        return;
    if (m_playSounds == 0 && !g_unk691209)
        return;
    memset(g_sampleWasPlaying, 0, sizeof(g_sampleWasPlaying));
    EnterCriticalSection(&m_sectionSoundCall);
    for (int i = 0; i < m_sampleNum; i++)
        AIL_end_sample(m_sampleHandles[i]);
    LeaveCriticalSection(&m_sectionSoundCall);
    if (stopMusicToo)
        stopMP3();
}

VA(0x00599e80, 0x3D)  // dc 0x14b2c0
void soundManager::stopSample(ds_memsample* inSample)
{
    if (g_noSound)
        return;
    if (!m_ds)
        return;
    if (!inSample)
        return;
    EnterCriticalSection(&m_sectionSoundCall);
    AIL_end_sample(inSample);
    LeaveCriticalSection(&m_sectionSoundCall);
}

VA(0x00599ec0, 0x7F)  // dc 0x14b2d8
void soundManager::waitSample(ds_memsample* sample, int time)
{
    if (time < 0)
        time = 4000;
    unsigned long deadline = GameTime::get() + time;
    while (g_soundManager->getSampleInfo(sample, SAMPLE_INFO_PLAYING)) {
        if (static_cast<long>(GameTime::get() - deadline) >= 0)
            return;
        process1WindowsMessage();
        pollSound();
    }
}

VA(0x00599f40, 0xE1)  // dc 0x14b37c
void soundManager::modifySample(ds_memsample* inSample, short functionId, long value)
{
    if (g_noSound)
        return;
    if (!m_ds)
        return;
    if (m_playSounds == 0 && !g_unk691209)
        return;
    if (!m_samples)
        return;
    int found = -1;
    EnterCriticalSection(&m_sectionSoundCall);
    for (int i = 0; i < m_sampleNum; i++) {
        if (inSample == m_sampleHandles[i])
            found = i;
        switch (functionId) {
        case SAMPLE_MODIFY_1:
        case SAMPLE_MODIFY_100:
            AIL_set_sample_volume(inSample, convertVolume(value, 100));
            if (found >= 0)
                g_ailDriverState[found] = static_cast<short>(value);
            break;
        case SAMPLE_MODIFY_5:
            AIL_start_sample(inSample);
            break;
        }
    }
    LeaveCriticalSection(&m_sectionSoundCall);
}

// PC-only query used by the remote chat sample path.  Operation 1 returns
// the Miles volume and operation 4 reduces the status to the playing bit;
// every other operation retains the initialized zero result.
VA(0x0059a030, 0x87)
int soundManager::getSampleInfo(ds_memsample* inSample, short operation)
{
    if (g_noSound)
        return 0;
    if (!m_ds)
        return 0;
    if (!inSample)
        return 0;

    int result = 0;
    EnterCriticalSection(&m_sectionSoundCall);
    switch (operation) {
    case SAMPLE_INFO_VOLUME:
        result = AIL_sample_volume(inSample);
        break;
    case SAMPLE_INFO_PLAYING:
        result = AIL_sample_status(inSample) == AIL_SAMPLE_PLAYING;
        break;
    }
    LeaveCriticalSection(&m_sectionSoundCall);
    return result;
}

VA(0x0059a0c0, 0xEB)  // dc 0x14b42c
void soundManager::adjustSoundVolumes()
{
    if (g_noSound)
        return;
    if (!m_ds)
        return;
    if (m_playSounds == 0 && !g_unk691209)
        return;
    for (int i = 1; i < m_sampleNum; i++) {
        ds_memsample* handle = m_sampleHandles[i];
        // GetSampleInfo playing query and sinks the `push 0; push 1` arm below
        if (g_unk698764) {
            if (getSampleInfo(handle, SAMPLE_INFO_PLAYING))
                modifySample(handle, SAMPLE_MODIFY_100, g_ailDriverState[i]);
        } else {
            modifySample(handle, SAMPLE_MODIFY_1, 0);
        }
    }
}

VA(0x0059a1b0, 0x22)  // dc 0x14b4d4
void soundManager::adjustMusicVolumes()
{
    if (g_noSound)
        return;
    if (m_playSounds == 0 && !g_unk691209)
        return;
    setMusicVolume();
}

VA(0x0059a1e0, 0x25)  // dc 0x14b500
void soundManager::switchAmbientMusic(int newMusicFileId)
{
    if (newMusicFileId >= 2 && newMusicFileId <= 10)
        startMP3(g_terrainMusic[newMusicFileId - 2], 0, 0);
}

VA(0x0059a210, 0x1DB)  // dc 0x14b528
ds_memsample* soundManager::memorySample(sample* samplePointer)
{
    if (!g_noSound && m_ds && (m_playSounds || g_unk691209) && g_unk698764 && samplePointer
        && m_samples && samplePointer->m_memSample.m_memVolume) {
        SoundChannelRange* range = &g_soundChannels[samplePointer->m_memSample.m_memCindex];
        EnterCriticalSection(&m_sectionSoundCall);
        int slot = range->m_first;
        while (slot < range->m_last) {
            if (AIL_sample_status(m_sampleHandles[slot]) == AIL_SAMPLE_SLOT_FREE)
                break;
            slot++;
        }
        if (slot == range->m_last) {
            if (samplePointer->m_memSample.m_memCindex == SOUND_CHANNEL_COUNT) {
                LeaveCriticalSection(&m_sectionSoundCall);
                return 0;
            }
            slot = range->m_next++;
            if (range->m_next >= range->m_last) {
                slot = range->m_next = range->m_first;
            }
            stopSample(m_sampleHandles[slot]);
        }

        ds_memsample* handle = m_sampleHandles[slot];
        g_ailDriverState[slot] = static_cast<short>(samplePointer->m_memSample.m_memVolume);
        AIL_init_sample(handle);
        AIL_set_sample_file(handle, samplePointer->m_memSample.m_data, 0);
        AIL_set_sample_loop_count(handle, samplePointer->m_memSample.m_memLooping);
        if (g_unk698764)
            AIL_set_sample_volume(handle, convertVolume(samplePointer->m_memSample.m_memVolume, 100));
        else
            AIL_set_sample_volume(handle, 0);
        AIL_start_sample(handle);
        samplePointer->m_memSample.m_memSampleHandle = handle;
        LeaveCriticalSection(&m_sectionSoundCall);

        g_soundManager->serviceSounds();
        return handle;
    }
    return 0;
}

VA(0x0059a3f0, 0x15)  // dc 0x14b644
int soundManager::musicPlaying()
{
    if (g_noSound)
        return 0;
    return m_mp3Playing;
}

VA(0x0059a410, 0x5C)  // dc 0x14b65c
void clearMemSample(SAMPLE2 sample2)
{
    if (!sample2.m_resSample)
        return;
    if (g_shutDownDone)
        return;
    // This is soundManager::StopSample on the global, inlined by /Ob2 -
    // guard trio, section, AIL_end_sample, section, store for store.
    g_soundManager->stopSample(sample2.m_playSample);
    sample2.m_resSample->dispose();
}

VA(0x0059a470, 0x43)  // dc 0x14b698
SAMPLE2 loadPlaySample(const char* sampleName)
{
    if (!sampleName)
        return g_nullSample2;
    sample* loaded = ResourceManager::getSample(sampleName);
    if (!loaded)
        return g_nullSample2;
    loaded->m_memSample.m_memCindex = 2;
    SAMPLE2 played;
    played.m_resSample = loaded;
    played.m_playSample = g_soundManager->memorySample(loaded);
    return played;
}

VA(0x0059a4c0, 0xCE)  // dc 0x14b6ec
void waitEndSample(SAMPLE2 sample2, int milliWait)
{
    if (milliWait < 0)
        milliWait = 4000;
    unsigned long deadline = GameTime::get() + milliWait;
    while (sample2.m_playSample && g_soundManager->getSampleInfo(
               sample2.m_playSample, soundManager::SAMPLE_INFO_PLAYING)) {
        if (static_cast<long>(GameTime::get() - deadline) >= 0)
            break;
        process1WindowsMessage();
        pollSound();
    }
    clearMemSample(sample2);
}

VA(0x0059a590, 0x112)  // dc 0x14b780
void launchSample(const char* sampleName, int maxTime, int channel)
{
    if (g_noSound)
        return;
    if (!g_soundManager->m_ds)
        return;
    if (g_soundManager->m_playSounds == 0 && !g_unk691209)
        return;
    if (!g_unk698764)
        return;
    if (!sampleName)
        return;
    if (maxTime < 0)
        maxTime = 10000;
    LaunchedSample* launched = new LaunchedSample;
    launched->m_sample2.m_resSample = ResourceManager::getSample(sampleName);
    launched->m_maxTime = maxTime;
    if (!launched->m_sample2.m_resSample) {
        delete launched;
        return;
    }
    launched->m_sample2.m_resSample->m_memSample.m_memCindex = channel;
    launched->m_sample2.m_playSample =
        g_soundManager->memorySample(launched->m_sample2.m_resSample);
    g_soundManager->serviceSounds();
    if (!g_shutDownDone)
        _beginthread(waitEndSampleThread, 0, launched);
}

// E:\gamedcs\soundmgr.cpp:911
VA(0x0059a6b0, 0x113)  // address-taken + packet layout, retail-only
void __cdecl waitEndSampleThread(void* arglist)
{
    ++g_sampleWorkerCount;
    LaunchedSample* launched = static_cast<LaunchedSample*>(arglist);
    int elapsed = 0;
    if (launched->m_sample2.m_playSample && !g_shutDownDone) {
        while (g_soundManager->getSampleInfo(
                   launched->m_sample2.m_playSample, soundManager::SAMPLE_INFO_PLAYING)
               && elapsed < launched->m_maxTime) {
            Sleep(100);
            elapsed += 100;
            if (g_shutDownDone)
                break;
        }
    }
    clearMemSample(launched->m_sample2);
    delete launched;
    --g_sampleWorkerCount;
    _endthread();
}

// Windows Miles service operation. The WinCE counterpart service_sounds is
// a four-byte no-op attributed to SoundMgr.h:140 (dc 0xe6ef4); its records do
// not establish the nonempty Windows definition's inline spelling or owner.
// Retail expands the complete operation only in memorySample and launchSample,
// both in this TU; external consumers call the retained 0x59a7d0 body. All 13
// retail AIL_serve references are in this TU, including ten different sound
// operations. A source-local ordinary body recovers that visibility boundary
// and retained emission. Its Windows ownership is a platform inference; the
// CE header attribution remains recorded separately in dc_only.tsv.
VA(0x0059a7d0, 0x51)
void soundManager::serviceSounds()
{
    EnterCriticalSection(&m_sectionSoundCall);
    AIL_serve();
    HSTREAM stream = g_mp3Stream;
    if (stream) {
        if (g_soundManager->m_mp3Playing) {
            if (!g_shutDownDone)
                AIL_service_stream(stream, 1);
        }
    }
    Sleep(1);
    LeaveCriticalSection(&m_sectionSoundCall);
}

VA(0x0059a830, 0x10)  // dc 0x14b7e0
void __cdecl processMP3Stop(void* nothing)
{
    g_soundManager->threadStopMP3();
    _endthread();
}

VA(0x0059a840, 0x3BB)  // dc 0x14b7f4
void __cdecl processStopAndPlayMP3(void* arglist)
{
    EnterCriticalSection(&g_soundManager->m_sectionMp3Change);
    int volume = g_soundManager->convertVolume(127, VOLUME_TYPE_101);
    EnterCriticalSection(&g_soundManager->m_sectionMp3NameChange);
    if (!g_mp3NamePlaying[0]) {
        LeaveCriticalSection(&g_soundManager->m_sectionMp3NameChange);
        LeaveCriticalSection(&g_soundManager->m_sectionMp3Change);
        _endthread();
        return;
    }
    LeaveCriticalSection(&g_soundManager->m_sectionMp3NameChange);

    g_soundManager->threadStopMP3();
    EnterCriticalSection(&g_soundManager->m_sectionSoundCall);
    if (!g_shutDownDone && g_mp3Stream)
        AIL_close_stream(g_mp3Stream);
    g_mp3Stream = 0;
    LeaveCriticalSection(&g_soundManager->m_sectionSoundCall);

    EnterCriticalSection(&g_soundManager->m_sectionMp3NameChange);
    if (!g_mp3NamePlaying[0]) {
        LeaveCriticalSection(&g_soundManager->m_sectionMp3NameChange);
        LeaveCriticalSection(&g_soundManager->m_sectionMp3Change);
        _endthread();
        return;
    }

    strcpy(g_mp3Name, g_mp3NamePlaying);
    g_unk69fe90 = g_unk69fe9c;
    char filename[100];
    sprintf(filename, DATA_COMPGEN(
        0x00684b34, mp3PathFormat, "mp3\\%s.mp3"), g_mp3NamePlaying);
    g_mp3NamePlaying[0] = 0;
    LeaveCriticalSection(&g_soundManager->m_sectionMp3NameChange);

    if (volume && !g_shutDownDone) {
        g_soundManager->m_mp3Playing = 1;
        EnterCriticalSection(&g_soundManager->m_sectionSoundCall);
        g_mp3Stream = AIL_open_stream(g_soundManager->m_ds, filename, 0);
        if (g_mp3Stream && !g_shutDownDone && g_foregroundApp) {
            AIL_set_stream_volume(
                g_mp3Stream, g_unk69fe90 ? volume : 0);
            AIL_set_stream_loop_count(g_mp3Stream, g_unk69fe90);
            AIL_service_stream(g_mp3Stream, 1);

            if (!g_unk69fe90) {
                EnterCriticalSection(&g_soundManager->m_sectionMp3Change);
                EnterCriticalSection(&g_soundManager->m_sectionSoundCall);
                for (int slot = 0; slot < g_mp3ResumePositionCount; ++slot) {
                    if (strcmp(g_mp3ResumePositions[slot].m_name, g_mp3Name) == 0) {
                        AIL_set_stream_position(
                            g_mp3Stream, g_mp3ResumePositions[slot].m_position);
                        break;
                    }
                }
                LeaveCriticalSection(&g_soundManager->m_sectionSoundCall);
                LeaveCriticalSection(&g_soundManager->m_sectionMp3Change);
            }

            if (!g_shutDownDone)
                AIL_start_stream(g_mp3Stream);

            if (!g_unk69fe90) {
                float currentVolume = 0.0f;
                for (int step = 1; step < 10; ++step) {
                    if (g_shutDownDone)
                        break;
                    currentVolume += static_cast<float>(volume) / 10.0f;
                    AIL_set_stream_volume(
                        g_mp3Stream, static_cast<int>(currentVolume));
                    LeaveCriticalSection(&g_soundManager->m_sectionSoundCall);
                    Sleep(100);
                    EnterCriticalSection(&g_soundManager->m_sectionSoundCall);
                }
                if (!g_shutDownDone)
                    AIL_set_stream_volume(g_mp3Stream, volume);
            }
        }
        LeaveCriticalSection(&g_soundManager->m_sectionSoundCall);
    }
    LeaveCriticalSection(&g_soundManager->m_sectionMp3Change);
    _endthread();
}

VA(0x0059ac00, 0xA9)  // dc 0x14b8e8
void soundManager::resumeStream()
{
    EnterCriticalSection(&m_sectionMp3NameChange);
    if (g_mp3Name[0] == 0 || g_shutDownDone) {
        LeaveCriticalSection(&m_sectionMp3NameChange);
        return;
    }
    strcpy(g_mp3NamePlaying, g_mp3Name);
    g_unk69fe9c = g_unk69fe90;
    LeaveCriticalSection(&m_sectionMp3NameChange);
    EnterCriticalSection(&m_sectionSoundCall);
    AIL_serve();
    LeaveCriticalSection(&m_sectionSoundCall);
    if (g_shutDownDone)
        return;
    _beginthread(processStopAndPlayMP3, 0, 0);
}

VA(0x0059acb0, 0x355)  // dc 0x14b924
void soundManager::startMP3(const char* filename, int loopCount, unsigned char stopSamples)
{
    if (g_noSound)
        return;
    if (!m_ds)
        return;
    if (m_playSounds == 0 && !g_unk691209)
        return;
    if (!g_unk698760)
        return;

    EnterCriticalSection(&m_sectionMp3NameChange);
    if (strcmp(filename, g_mp3NamePlaying) != 0) {
        if (strcmp(filename, g_mp3Name) == 0) {
            LeaveCriticalSection(&m_sectionMp3NameChange);
            EnterCriticalSection(&m_sectionSoundCall);
            int streamStatus = AIL_stream_status(g_mp3Stream);
            LeaveCriticalSection(&m_sectionSoundCall);
            if (streamStatus == AIL_STREAM_PLAYING)
                return;
            if (stopSamples)
                stopAllSamples(1);
            resumeStream();
        } else {
            strcpy(g_mp3NamePlaying, filename);
            g_unk69fe9c = loopCount;
            LeaveCriticalSection(&m_sectionMp3NameChange);
            if (stopSamples)
                stopAllSamples(0);
            EnterCriticalSection(&m_sectionSoundCall);
            AIL_serve();
            Sleep(1);
            LeaveCriticalSection(&m_sectionSoundCall);
            if (g_shutDownDone)
                return;
            _beginthread(processStopAndPlayMP3, 0, 0);
        }
        return;
    }
    LeaveCriticalSection(&m_sectionMp3NameChange);
}

VA(0x0059b010, 0x6A)  // dc 0x14b974
void soundManager::stopMP3()
{
    EnterCriticalSection(&m_sectionMp3Change);
    EnterCriticalSection(&m_sectionSoundCall);
    if (g_mp3Stream && AIL_stream_status(g_mp3Stream) == AIL_STREAM_PLAYING) {
        AIL_serve();
        Sleep(5);
        if (!g_shutDownDone)
            _beginthread(processMP3Stop, 0, 0);
    }
    LeaveCriticalSection(&m_sectionSoundCall);
    LeaveCriticalSection(&m_sectionMp3Change);
}

VA(0x0059b080, 0x215)  // dc 0x14b984
void soundManager::threadStopMP3()
{
    EnterCriticalSection(&m_sectionMp3Change);
    EnterCriticalSection(&m_sectionSoundCall);
    if (g_mp3Stream && m_mp3Playing && !g_shutDownDone) {
        EnterCriticalSection(&g_soundManager->m_sectionMp3NameChange);
        EnterCriticalSection(&g_soundManager->m_sectionSoundCall);

        int slot;
        for (slot = 0; slot < g_mp3ResumePositionCount; ++slot) {
            if (strcmp(g_mp3ResumePositions[slot].m_name, g_mp3Name) == 0) {
                g_mp3ResumePositions[slot].m_position =
                    AIL_stream_position(g_mp3Stream);
                break;
            }
        }
        if (slot == g_mp3ResumePositionCount) {
            strcpy(g_mp3ResumePositions[slot].m_name, g_mp3Name);
            g_mp3ResumePositions[slot].m_position =
                AIL_stream_position(g_mp3Stream);
            ++g_mp3ResumePositionCount;
        }

        LeaveCriticalSection(&g_soundManager->m_sectionSoundCall);
        LeaveCriticalSection(&g_soundManager->m_sectionMp3NameChange);

        float originalVolume =
            static_cast<float>(AIL_stream_volume(g_mp3Stream));
        if (originalVolume != 0.0f) {
            float currentVolume = originalVolume;
            for (int step = 1; step < 10; ++step) {
                if (g_shutDownDone)
                    break;
                currentVolume -= originalVolume / 10.0f;
                AIL_set_stream_volume(
                    g_mp3Stream, static_cast<int>(currentVolume));
                LeaveCriticalSection(&m_sectionSoundCall);
                Sleep(100);
                EnterCriticalSection(&m_sectionSoundCall);
            }
        }
        if (!g_shutDownDone)
            AIL_pause_stream(g_mp3Stream, 1);
    }
    m_mp3Playing = 0;
    LeaveCriticalSection(&m_sectionSoundCall);
    LeaveCriticalSection(&m_sectionMp3Change);
}
