/*
 Copyright (c) 2009-2011, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#include "Camera.h"

#include <QSettings>
#include <QDebug>

#include "Logger.h"
#include "PathConfig.h"
#include <iostream>

namespace tracker
{
    Camera::Camera()
        : mFrameCounter(1.0)
        , mIsRunning(false)
        , mMakeSingleShot(false)
        , mMultipleShotInprogress(false)
        , mImageCounter(0) // temporal solution, not nice handling it.
        , mNumOfRequestedIm(0) // temporal solutionnot nice handling it.
        , mMultiChannelOffset(0)
    {
    }

    void Camera::Record(QString modeText, int channel1, double intensity1,int channel2, double intensity2, int exposureTime, double gain)
    {
        std::cout<<"This function was called: "<< std::endl;
        // Save current camera settings
        mSavedExposureTime = exposureTime >= 0 ? this->getExposureTime() : -1;
        mSavedGain = gain >= 0 ? this->getGain() : -1.0;
        mSavedMode = this->getMode();

        // Save current illumination mode
        mSavedChannel1 = channel1;
        mSavedIntensity1 = intensity1;
        /// Temporal solution
        emit SettingFromCameraTheLamp(mSavedChannel1,2);

        // Save multichannel illumination mode
        mSavedChannel2 = channel2;
        mSavedIntensity2 = intensity2;

        // Set Camera
        this->setMode(modeText, exposureTime, gain);

        // Allow storing the images
        mMakeSingleShot = true;

        // If Multichannel
        if(mIsMultiChannel)
            mNumOfRequestedIm = mNumOfRequestedIm * 2;

        // Do some quick & dirty math for the timeout 
        int timeout = mSavedExposureTime + exposureTime*mNumOfRequestedIm + mSingleShotMinTimeout;
        mSingleShotEndTime = mClock.getTime() + timeout;

    }

    void Camera::SetBackTo()
    {
        mMakeSingleShot = false;

        // Set back to old mode
        this->setMode(mSavedMode, mSavedExposureTime, mSavedGain);

        // Set back to old illumination setting
        emit(SettingFromCameraTheLamp(mSavedChannel1,mSavedIntensity1));

        // SetBackStage to initial state
        if (mNumOfRequestedIm > 1)
            emit(SetStage());

        // Inform tracker
        if (mSendFeedBack)           
            emit finishedZstack();


        mImageCounter = 0;
    }

    void Camera::Trigger(){
        emit AllowLightForTheNextFrame();
    }

    bool Camera::IsThisTheLastOne()
    {   return mImageCounter<mNumOfRequestedIm; }

    void Camera::DecisionTree()
    {        
        if (mIsMultiChannel)
        {
            if (mImageCounter*2<mNumOfRequestedIm)                
                emit ZSTEP(mStepSizeofZstack,1);
            else if (mImageCounter*2 == mNumOfRequestedIm)
            {
                std::cout<<"Channel has changed: "<<std::endl;
                emit ZSTEP(mMultiChannelOffset,1); // TODO:::OffsetBetweebTwoChannels ~4-5um
                emit SettingFromCameraTheLamp(mSavedChannel2,mSavedIntensity2);
            }
            else
            {
                emit ZSTEP(mStepSizeofZstack,1);
            }
        }
        else
        {
             emit ZSTEP(mStepSizeofZstack,1);
        }
    }

    void Camera::finishSingleShot(QImage image, quint64 captureTime, quint64 processTime)
    {
        // Sending the recorded image to the MainWindow for storing.
        emit singleShotTaken(image, captureTime, processTime);

        if (mMakeSingleShot)
         {
             SingleStepIncrementer(); // Increasing the mImageCounter
             if (this->IsThisTheLastOne())
             {
                 this->DecisionTree();
             }
             else
             {
                 this->SetBackTo();
             }
         }
    }

    void Camera::setNumOfPlanes(int NumOfPlanes)
    {
        mNumOfRequestedIm = NumOfPlanes;
    }

    void Camera::setZstepsize(double Zstackstepsize)
    {
        mStepSizeofZstack = Zstackstepsize;
    }
    void Camera::setMultichannel(bool multichannel)
    {
        mIsMultiChannel = multichannel;
    }
    void Camera:: setOffsetValue(double value)
    {
        mMultiChannelOffset = value;
    }


    void Camera::Feedbacktotracker(bool enable)
    {
        mSendFeedBack = enable;
    }
    /**
    void Camera::takeSingleShot(QString modeText, int exposureTime, double gain)
    {
        // Save values if overridden to restore them later
        mSavedExposureTime = exposureTime >= 0 ? this->getExposureTime() : -1;
        mSavedGain = gain >= 0 ? this->getGain() : -1.0;
        mSavedMode = this->getMode();

        quint64 before= mClock.getTime();
        this->setMode(modeText, exposureTime, gain);
        quint64 after= mClock.getTime();

        std::cout<<"SetMode before: "<<before<<" "<<"SetMode after: "<<after<<std::endl;

        mMakeSingleShot = true;

        //std::cout<<"I took a single shot: "<<std::endl;
        //std::cout<<"Exposure Time: "<<mSavedExposureTime<<std::endl;
        //std::cout<<"Gain: "<<mSavedGain<<std::endl;
        //qDebug() << modeText; //showing the mode text
        // Do some quick & dirty math for the timeout
        int timeout = mSavedExposureTime + exposureTime + mSingleShotMinTimeout;
        mSingleShotEndTime = mClock.getTime() + timeout;
    }

    void Camera::takeMultipleShot(QString modeText, int exposureTime, double gain)
    {
        mSavedExposureTime = exposureTime >= 0 ? this->getExposureTime() : -1;
        mSavedGain = gain >= 0 ? this->getGain() : -1.0;
        mSavedMode = this->getMode();

        this->setMode(modeText, exposureTime, gain);
        mMakeSingleShot = true;
        mMultipleShotInprogress = true;        

        // Do some quick & dirty math for the timeout
        int timeout = (mSavedExposureTime + exposureTime + mSingleShotMinTimeout)*mNumOfRequestedIm;
        mSingleShotEndTime = mClock.getTime() + timeout;
    }

    void Camera::finishSingleShot(QImage image, quint64 captureTime, quint64 processTime)
    {
        if (mMakeSingleShot)
        {
            if (mMultipleShotInprogress)
            {
                // Raise a singnal to move Zstack
                emit ZSTEP(mStepSizeofZstack,0);
                // Send the required image to store in MainWindow::
                emit singleShotTaken(image, captureTime, processTime);
                mImageCounter = mImageCounter+1;
                if (mImageCounter == mNumOfRequestedIm)
                {
                  mMultipleShotInprogress = false;
                  mImageCounter = 0;
                  if (mSendFeedBack)
                    emit finishedZstack();
                }

            }
            else
            {
                mMakeSingleShot = false;                
                // TODO: doing mode change only if it is needed ( in case of series of images (zstack) no change)
                // Set back to old mode
                this->setMode(mSavedMode, mSavedExposureTime, mSavedGain);
                emit singleShotTaken(image, captureTime, processTime);
            }
        }
    }*/

    void Camera::readSettings()
    {
        QSettings settings(PathConfig::getConfigFilename(), QSettings::IniFormat);
        settings.beginGroup("Camera");

        mSingleShotMinTimeout = qMax(0, settings.value("Single_Shot_Min_Timeout", 200000).toInt());

        settings.endGroup();
    }

    void Camera::writeSettings()
    {
        QSettings settings(PathConfig::getConfigFilename(), QSettings::IniFormat);
        settings.beginGroup("Camera");

        settings.setValue("Single_Shot_Min_Timeout", mSingleShotMinTimeout);

        settings.endGroup();
    }
}
