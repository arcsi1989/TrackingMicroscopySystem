/*
 Copyright (c) 2009-2012, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#include "DummyMicroscope.h"

#include <cmath>
#include <QApplication>

#include "DummyStage.h"
#include "Logger.h"
#include "TMath.h"

namespace tracker
{
    DummyMicroscope::DummyMicroscope(DummyStage* stage)
        : mStage(stage)
    {
        connect(mStage, SIGNAL(stageMovedXY(QPointF)), SLOT(moveStageXY(QPointF)));
        connect(mStage, SIGNAL(stageMovedZ(double)), SLOT(moveStageZ(double)));
    }

    DummyMicroscope::~DummyMicroscope()
    {
    }

    void DummyMicroscope::run(Thread* thread)
    {
        mVirtualTime     = 0.0;
        mFlexCellOffset  = QPointF(0.0, 0.0);
        mFlexCellZOffset = 0.0;
        mStageOffset     = QPointF(0.0, 0.0);
        mZStageOffset    = 0.0;
        mStageCommands.clear();
        mZStageCommands.clear();

        // Start timer
        this->startTimer(msDeltaTime/*ms*/);

        // Event loop
        thread->exec();

        this->moveToThread(QApplication::instance()->thread());
    }

    void DummyMicroscope::timerEvent(QTimerEvent* event)
    {
        mVirtualTime += msDeltaTime / 1000.0;

        // Apply all stage commands with a delay
        while (!mStageCommands.isEmpty() && mStageCommands.first().second < mVirtualTime)
        {
            mStageOffset += mStageCommands.takeFirst().first;
        }
        //TRACKER_INFO("stage offset: " + QString::number(mStageOffset.x()) + ", " + QString::number(mStageOffset.y()));

        // Apply all Z stage commands with a delay
        while (!mZStageCommands.isEmpty() && mZStageCommands.first().second < mVirtualTime)
        {
            mZStageOffset += mZStageCommands.takeFirst().first;
        }

        const double radius = 40.0;
        const double omega  = math::pi; // 0.5 Hz

        // Move the virtual FlexCell in a circle
        double x = radius * std::cos(omega * mVirtualTime);
        double y = radius * std::sin(omega * mVirtualTime);
        //mFlexCellOffset = QPointF(x, y);

        // Move the virtual FlexCell up and down
        const double step = 20.0;
        const double theta = math::pi * 0.5; // 0.25 Hz
        double z = step * std::cos(theta * mVirtualTime);
        //mFlexCellZOffset = z;

        emit offsetChanged(mStageOffset + mFlexCellOffset, mZStageOffset + mFlexCellZOffset, mClock.getTime());
    }

    void DummyMicroscope::moveStageXY(QPointF distance)
    {
        mStageCommands.append(qMakePair(distance, mVirtualTime + msStageDelay / 1000.0));
    }

    void DummyMicroscope::moveStageZ(double distance)
    {
        mZStageCommands.append(qMakePair(distance, mVirtualTime + msStageDelay / 1000.0));
    }
}
