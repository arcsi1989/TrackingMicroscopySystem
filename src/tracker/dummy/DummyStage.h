/*
 Copyright (c) 2009-2012, Reto Grieder
 Copyright (c) 2014, Tobias Klauser

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef _DummyStage_H__
#define _DummyStage_H__

#include "TrackerPrereqs.h"

#include "Stage.h"

namespace tracker
{
    /// Empty virtual Stage class that simply redirects move() to moved().
    class DummyStage : public Stage
    {
        Q_OBJECT;

    public:
        /// Does nothing
        DummyStage(QObject* parent);
        /// Does nothing
        ~DummyStage();

        bool isMovingXY()
            { return false; }
        bool isMovingZ()
            { return false; }

    public slots:
        bool move(QPointF distance, int block = 0);
        bool moveZ(double distance, int block = 0);
        bool moveToZ(double zPos, int block = 0);

        double getZpos()
            { return mPosZ; }
        double getXpos()
            { return mPosXY.x(); }
        double getYpos()
            { return mPosXY.y(); }

        void clearZlimits();

        void setZlimits(double position);

        void stopAll();

    signals:
        /** Since this stage is just virtual, we need to tell the
            DummyMicroscope about the movements.
            Any call to move() will automatically raise this signal.
        */
        void stageMovedXY(QPointF distance);

        void stageMovedZ(double distance);

    private:
        Q_DISABLE_COPY(DummyStage);

        QPointF mPosXY;
        double  mPosZ;
    };
}

#endif /* _DummyStage_H__ */
