/*
 Copyright (c) 2009-2012, Reto Grieder & Benjamin Beyeler
 Copyright (c) 2014, Tobias Klauser

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

/**
@file
@brief
    Implementation of the Stage class for Windows.
*/

#include "WindowsStage.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Oasis4i.h>

#include "Exception.h"

#include <iostream>

namespace tracker
{
    /*static*/ Stage* Stage::makeStage(QObject* parent)
    {
        return new WindowsStage(parent);
    }

    WindowsStage::WindowsStage(QObject* parent)
        : Stage(parent)
        , mbDriverOpen(false)
        , mPrevZDirection(1.0)
    {
        // Choose hardware, not simulation
        int test;
        LPDWORD teststatus = 0;
        DWORD delay = 1000;
        if (OIFAILED(OI_SetHardwareMode(OI_OASIS)))
            TRACKER_WARNING("Oasis: Could not set hardware mode");
        else
        {
            // Open the driver
            if (OIFAILED(OI_Open()))
                TRACKER_WARNING("Oasis: Could not open driver");

            // Reset pitch (somehow it resets to 3 for y when starting up)
            OI_SetPitchXY(2.0, 2.0);

            //test = OI_OpenComponent(OI_CFG_FOCUS);
            //test = OI_GetComponentStatus(OI_CFG_FOCUS, teststatus);
            //test = &teststatus;
            test = OI_Configure(OI_CFG_FOCUS,OI_OASIS);
            //test = OI_EnableMotorPower(1, 1);
            test = OI_InitializeZ(400, 400); // allows z movement only in +- 400 microns

            double speed;
            OI_GetSpeedZ(&speed);
            std::cout << __FUNCTION__ << ": old speed " << speed << std::endl;

            if (OI_SelectSpeedZ(.5, 1) != OI_OK)
                std::cout << __FUNCTION__ << ": Failed to set Z speed" << std::endl;
            if (OI_SetPitchZ(.78) != OI_OK)
                std::cout << __FUNCTION__ << ": Failed to set Z pitch" << std::endl;

            OI_GetSpeedZ(&speed);
            std::cout << __FUNCTION__ << ": new speed " << speed << std::endl;

            this->mbDriverOpen = true;
        }
    }

    WindowsStage::~WindowsStage()
    {
        // Close driver
        if (this->mbDriverOpen)
            OI_Close();
    }

    bool WindowsStage::move(QPointF distance, int block)
    {
        if (!this->mbDriverOpen)
            return false;

        OI_StepXY(distance.x(), distance.y(), block);
        return true;
    }

    bool WindowsStage::moveZ(double distance, int block)
    {
        if (!this->mbDriverOpen)
            return false;

        int test = OI_StepZ(distance, block);
        if (test != OI_OK) {
            std::cout << __FUNCTION__ << ": Failed to step by " << distance << std::endl;
            return false;
        }

        double direction = distance < 0.0 ? -1.0 : 1.0;
        if (mPrevZDirection != direction)
            OI_StepZ(0, 0); // this line fixes a bug when direction is changed

        mPrevZDirection = direction;

        return true;
    }

    bool WindowsStage::moveToZ(double zPos, int block)
    {
        if (!this->mbDriverOpen)
            return false;

        if (OI_MoveToZ(zPos, block) != OI_OK) {
            std::cout << __FUNCTION__ << ": Failed to move to " << zPos << std::endl;
            return false;
        }

        return true;
    }

    double WindowsStage::getZpos()
    {
        double zpos;
        if (OI_ReadZ(&zpos) != OI_OK)
            std::cout << __FUNCTION__ << ": Failed to get Z position" << std::endl;
        return zpos;
    }

    double WindowsStage::getXpos()
    {
        double xpos,ypos;
        OI_ReadXY(&xpos,&ypos);
        return xpos;
    }

    double WindowsStage::getYpos()
    {
        double xpos,ypos;
        OI_ReadXY(&xpos,&ypos);
        return ypos;
    }

	void WindowsStage::clearZlimits()
	{
		OI_ClearUserLimitsZ();
	}

	void WindowsStage::setZlimits(double position)
	{
		int test;
		test = OI_SetUserLimitsZ(position - 400, position + 400);
	}

	void WindowsStage::stopAll()
    {
        if (this->mbDriverOpen)
        {
            //OI_HaltXY();
            OI_HaltAllAxes();
        }
    }

    bool WindowsStage::isMovingXY()
    {
        if (this->mbDriverOpen)
        {
            WORD xStatus, yStatus;
            OI_ReadStatusXY(&xStatus, &yStatus);
            if ((xStatus & S_MOVING) != 0 || (yStatus & S_MOVING) != 0)
                return true;
        }
        return false;
    }

    bool WindowsStage::isMovingZ()
    {
        if (this->mbDriverOpen)
        {
            WORD zStatus;
            OI_ReadStatusZ(&zStatus);
            if ((zStatus & S_MOVING) != 0)
                return true;
        }
        return false;
    }

}
