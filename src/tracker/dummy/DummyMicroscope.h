/*
 Copyright (c) 2009-2012, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef _DummyMicroscope_H__
#define _DummyMicroscope_H__

#include "TrackerPrereqs.h"

#include <QImage>
#include <QList>
#include <QPair>
#include "Thread.h"
#include "Timing.h"

namespace tracker
{
    /** Manages the DummyStage and DummyCamera objects so that the desired
        behaviour can be simulated.
        Two virtual mechanisms are implemented: delay for stage movements and
        some sinusoidal flex cell movements. The noise effect is implemented in
        DummyCamera.
    @note
        Like the DummyCamera, all the configuration values are hard coded
        because this class is meant for deployment!
    */
    class DummyMicroscope : public Runnable
    {
        Q_OBJECT;

    public:
        /// Connects mStage->moved() to moveStage()
        DummyMicroscope(DummyStage* stage);
        /// Does nothing
        ~DummyMicroscope();

    signals:
        /** Signal for the DummyCamera to make a new image.
        @param offset
            Current stage position
        */
        void offsetChanged(QPointF, double, quint64);

    public slots:
        /// Moves the stage with a delay
        void moveStageXY(QPointF distance);
        void moveStageZ(double);

    private slots:
        /// Periodic timer event that drives the image taking process
        void timerEvent(QTimerEvent* event);

    private:
        Q_DISABLE_COPY(DummyMicroscope);

        /// Resets some variables and starts the event loop
        void run(Thread* thread);

        HPClock mClock;
        DummyStage* mStage;         ///< pointer to the virtual stage
        double  mVirtualTime;       ///< Absolute time
        QPointF mFlexCellOffset;    ///< Position of the membrane
        double  mFlexCellZOffset;
        QPointF mStageOffset;       ///< Position of the stage
        double  mZStageOffset;
        QList<QPair<QPointF, double> > mStageCommands; ///< Helper queue for simulated stage delays
        QList<QPair<double, double> >  mZStageCommands;

        static const int msDeltaTime  = 20/*ms*/;   ///< Reciprocal of the update frequency
        static const int msStageDelay = 50/*ms*/;   ///< Virtual stage command delay
        static const int msZStageDelay = 50; /*ms*/
    };
}
#endif /* _DummyMicroscope_H__ */
