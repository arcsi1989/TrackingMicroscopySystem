/*
 Copyright (c) 2009-2012, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

/**
@file
@brief
    Implementation of facilities related to timing.
*/

#include "Timing.h"

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#undef min
#undef max

namespace tracker
{
    void msleep(unsigned long milliseconds)
    {
        Sleep(milliseconds);
    }

    void sleep(unsigned long seconds)
    {
        Sleep(seconds * 1000);
    }

    static int dummy = 0;
    void busyWait()
    {
        // This is not very exact, but 100 microseconds is in the order
        // of what is being done here
        for (int i = 1; i < 6000; ++i)
            dummy += dummy % i;
    }

    void setThreadAffinity(unsigned long coreNr)
    {
        if (coreNr < 0)
            return;

        // Get the current process core mask
        DWORD procMask;
        DWORD sysMask;
#  if _MSC_VER >= 1400 && defined (_M_X64)
        GetProcessAffinityMask(GetCurrentProcess(), (PDWORD_PTR)&procMask, (PDWORD_PTR)&sysMask);
#  else
        GetProcessAffinityMask(GetCurrentProcess(), &procMask, &sysMask);
#  endif

        // If procMask is 0, consider there is only one core available
        // (using 0 as procMask will cause an infinite loop below)
        if (procMask == 0)
            procMask = 1;

        // if the core specified with coreNr is not available, take the lowest one
        if (!(procMask & (1 << coreNr)))
            coreNr = 0;

        // Find the lowest core that this process uses and that coreNr suggests
        DWORD threadMask = 1;
        while ((threadMask & procMask) == 0 || (threadMask < (1u << coreNr)))
            threadMask <<= 1;

        // Set affinity
        SetThreadAffinityMask(GetCurrentThread(), threadMask);
    }

    HPClock::HPClock()
    {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        mFrequency = frequency.QuadPart;
    }

    quint64 HPClock::getTime() const
    {
        LARGE_INTEGER count;
        QueryPerformanceCounter(&count);
        return (1000000 * count.QuadPart / mFrequency);
    }


    FrameCounter::FrameCounter(double updatePeriod)
        : mUpdatePeriod(updatePeriod * 1000000)
    {
    }

    void FrameCounter::addFrame(quint64 time)
    {
        mFrameList.prepend(time);
    }

    double FrameCounter::getFrameRate()
    {
        if (mFrameList.isEmpty())
            return 0.0;

        quint64 currentTime = mFrameList.front();
        quint64 pastTime = 0;
        int frameCount = 0;
        for (QList<quint64>::iterator it = mFrameList.begin(); it != mFrameList.end();)
        {
            if (*it >= currentTime - mUpdatePeriod)
            {
                ++frameCount;
                ++it;
            }
            else
            {
                if (pastTime == 0)
                    pastTime = *it;
                it = mFrameList.erase(it);
            }
        }
        if (pastTime == 0)
            pastTime = mFrameList.back();

        if (currentTime == pastTime)
            return 0.0;
        else
            return (double)frameCount / (currentTime - pastTime) * 1000000;
    }
}