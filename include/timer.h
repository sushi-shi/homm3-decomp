// timer.h - E:\gamedcs\timer.h
// Dreamcast CodeView LF_FIELDLIST 0x4e15 proves the complete 16-byte layout:
// three unsigned dwords at +0/+4/+8, then running/enabled bytes at +12/+13.
// The method bodies follow timer.h lines 39/49/60 and are kept visible so
// retail /O2 can eliminate state that a particular timer never observes.
#ifndef HOMM3_TIMER_H
#define HOMM3_TIMER_H

#include "va.h"

class CTimer
{
public:
    // Before normalization (locals): _enabled.
    CTimer(unsigned char enabled)
        : m_startTime(0), m_stopTime(0), m_elapsedTime(0),
          m_isRunning(0), m_enabled(enabled)
    {
    }

    // Dreamcast timer.h:33; oldmain's debug-only startup arm is the retail
    // consumer that proves this trivial header boundary at GlobalTimer+13.
    void enable()
    {
        m_enabled = 1;
    }

    void start()
    {
        if (m_enabled) {
            m_startTime = timeGetTime();
            m_isRunning = 1;
        }
    }

    void stop()
    {
        if (m_isRunning && m_enabled) {
            m_stopTime = timeGetTime();
            m_isRunning = 0;
            if (m_stopTime > m_startTime)
                m_elapsedTime = m_stopTime - m_startTime;
            else
                m_elapsedTime = 0;
        }
    }

private:
    // Before normalization: startTime.
    unsigned long m_startTime;
    // Before normalization: stopTime.
    unsigned long m_stopTime;
    // Before normalization: elapsedTime.
    unsigned long m_elapsedTime;
    // Before normalization: _IsRunning.
    unsigned char m_isRunning;
    // Before normalization: enabled.
    unsigned char m_enabled;
};
SIZE(CTimer, 16);

// Before normalization: GlobalTimer.
extern CTimer g_globalTimer;

#endif  // HOMM3_TIMER_H
