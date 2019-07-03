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
    Declaration of facilities related to timing
 */

#ifndef _Timing_H__
#define _Timing_H__

#include "TrackerPrereqs.h"

#include <QList>

namespace tracker
{
    //! Makes the thread sleep for a few @a milliseconds
    void msleep(unsigned long milliseconds);
    //! Makes the thread sleep for a few @a seconds
    void sleep(unsigned long seconds);
    //! Waits for approximately 100 microseconds (plus minus 50% or so)
    void busyWait();

    //! Confine thread executing this function to a specific CPU core
    void setThreadAffinity(unsigned long coreNr);

    //! Wrapper around the OS high precision timing function.
    class HPClock
    {
    public:
        HPClock();
        //! Returns a time in microseconds with undefined reference value
        quint64 getTime() const;

    private:
        quint64 mFrequency;  //!< Stores the Performance Counter frequency
    };

    //! Utility class that calculates the frame rate if updated regularly.
    class FrameCounter
    {
    public:
        //! @param udpatePeriod Time span in seconds that is used to average the frame rate.
        FrameCounter(double updatePeriod);
        ~FrameCounter() {}

        //! Call this function with a time stamp once a new frame has begun
        void addFrame(quint64 time);
        //! Forget all past frames
        void reset()
            { mFrameList.clear(); }

        //! Returns the average frames per second over the update period specified in the constructor
        double getFrameRate();

    private:
        const quint64  mUpdatePeriod;    //!< Time span in seconds that is used to average the frame rate.
        QList<quint64> mFrameList;       //!< List of past frames with time stamps
    };
}

#endif /* _Timing_H__ */
