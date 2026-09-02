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
    CTimer(unsigned char _enabled)
        : startTime(0), stopTime(0), elapsedTime(0),
          _IsRunning(0), enabled(_enabled)
    {
    }

    // Dreamcast timer.h:33; oldmain's debug-only startup arm is the retail
    // consumer that proves this trivial header boundary at GlobalTimer+13.
    void enable()
    {
        enabled = 1;
    }

    void start()
    {
        if (enabled) {
            startTime = timeGetTime();
            _IsRunning = 1;
        }
    }

    void stop()
    {
        if (_IsRunning && enabled) {
            stopTime = timeGetTime();
            _IsRunning = 0;
            if (stopTime > startTime)
                elapsedTime = stopTime - startTime;
            else
                elapsedTime = 0;
        }
    }

private:
    unsigned long startTime;
    unsigned long stopTime;
    unsigned long elapsedTime;
    unsigned char _IsRunning;
    unsigned char enabled;
};
SIZE(CTimer, 16);

extern CTimer GlobalTimer;

#endif  // HOMM3_TIMER_H
