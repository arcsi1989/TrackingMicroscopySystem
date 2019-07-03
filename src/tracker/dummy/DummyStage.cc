/*
 Copyright (c) 2009-2012, Reto Grieder
 Copyright (c) 2014, Tobias Klauser

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

/**
@file
@brief
    Implementation of the Stage class for Linux.
*/

#include "DummyStage.h"

namespace tracker
{
    /*static*/ Stage* Stage::makeStage(QObject* parent)
    {
        return new DummyStage(parent);
    }

    DummyStage::DummyStage(QObject* parent)
        : Stage(parent)
        , mPosXY(0.0, 0.0)
        , mPosZ(0.0)
    {
    }

    DummyStage::~DummyStage()
    {
    }

    bool DummyStage::move(QPointF distance, int)
    {
        mPosXY += distance;
        emit stageMovedXY(distance);
        return true;
    }

    bool DummyStage::moveZ(double distance, int)
    {
        mPosZ += distance;
        emit stageMovedZ(distance);
        return true;
    }

    bool DummyStage::moveToZ(double zPos, int)
    {
        double distance = zPos - mPosZ;
        mPosZ = zPos;
        emit stageMovedZ(distance);
        return true;
    }

    void DummyStage::stopAll()
    {
    }

    void DummyStage::clearZlimits()
    {
    }

    void DummyStage::setZlimits(double position)
    {
    }
}
