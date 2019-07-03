/*
 Copyright (c) 2009-2011, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#include "WindowsCamera.h"

#include <QApplication>
#include <QMetaType>
#include <QSettings>
#include <loki/ScopeGuard.h>

#include "Exception.h"
#include "PathConfig.h"
#include "Timing.h"
#include "TMath.h"
#include <iostream>
#include <QDebug>

namespace tracker
{
    /*static*/ Camera* Camera::makeCamera()
    {
        return new WindowsCamera();
    }

    WindowsCamera::WindowsCamera()
        : mBuffer(NULL)
        , mWaitForSingleShot(false)
        ///, mLastSavingImage(false);
    {
        qRegisterMetaType<HANDLE>("HANDLE");

        // Create colour table for 8 bit grey images
        mMonochromeColourTable.resize(256);
        for (int i = 0; i < 256; ++i)
            mMonochromeColourTable[i] = qRgb(i, i, i);

        assertSuccess(FX_InitLibrary(), "FX Library initialisation failed.");
        // FX library creates two threads of its own. So even in the case of an
        // exception we have to release these with FX_DeInitLibrary.
        Loki::ScopeGuard libraryGuard = Loki::MakeGuard(FX_DeInitLibrary);

        // Might be required, but seems to fail every time...
        FX_DeleteLabelInfo();

        // Create Windows events, its Qt wrappers and connect them
        mAcquisitionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        mAcquisitionNotifier = new QWinEventNotifier(mAcquisitionEvent, this);
        this->connect(mAcquisitionNotifier, SIGNAL(activated(HANDLE)), SLOT(processImage(HANDLE)));
        mMessageEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        mMessageNotifier = new QWinEventNotifier(mMessageEvent, this);
        this->connect(mMessageNotifier, SIGNAL(activated(HANDLE)), SLOT(showCameraMessage(HANDLE)));
        // Connect Windows Event that notifies upon arrival of new messages
        assertSuccess(FX_DefineMessageNotification(mMessageEvent, 10),
                      "Failed to define Windows event for incoming messages.");

        // Check for camera support. Simply use the first camera found.
        int numDevices = 0;
        assertSuccess(FX_EnumerateDevices(&numDevices), "Failed to enumerate cameras.");
        if (numDevices > 0)
        {
            tBOFXCAM_INFO devInformation;
            assertSuccess(FX_DeviceInfo(0, &devInformation), "Failed to camera information.");
            mCamName = devInformation.CamName;

            // Label camera with some arbitrary number (maybe don't use 0)
            mCamID = 0xFF;
            assertSuccess(FX_LabelDevice(0, mCamID), "Failed to label the camera.");

            // Create and open a camera object
            assertSuccess(FX_OpenCamera(mCamID), "Failed to open camera");

            // Connect Windows Event that notifies upon arrival of a new image
            assertSuccess(FX_DefineImageNotificationEvent(mCamID, mAcquisitionEvent),
                          "Failed to define Windows event for incoming images.");

            // Compile a list of all viable camera formats with codings
            int nImageFormats;
            tpBoImgFormat* imageFormats;
            assertSuccess(FX_GetCapability(mCamID, BCAM_QUERYCAP_IMGFORMATS, 0/*UNUSED*/,
                                           (void**)&imageFormats, &nImageFormats),
                          "Failed to list camera formats.");
            for (int iFormat = 0; iFormat < nImageFormats; ++iFormat)
            {
                // Only use 8 Bit B/W or 24 Bit colour images
                if (imageFormats[iFormat]->iPixelBits != 8 && imageFormats[iFormat]->iPixelBits != 24)
                    continue;

                int nImageCodes;
                tpBoImgCode* imageCodes;
                assertSuccess(FX_GetCapability(mCamID, BCAM_QUERYCAP_IMGCODES, imageFormats[iFormat]->iFormat,
                                               (void**)&imageCodes, &nImageCodes),
                              "Failed to list camera format codings.");

                // HACK: The Leica DFC 360 FX camera doesn't seem to show any difference for the codings
                // Only use one for easier user interface
                if (mCamName == "DFC 360 FX")
                    nImageCodes = nImageCodes >= 1 ? 1 : 0;

                for (int iCode = 0; iCode < nImageCodes; ++iCode)
                {
                    QSize size(imageFormats[iFormat]->iSizeX, imageFormats[iFormat]->iSizeY);
                    QString name = imageFormats[iFormat]->aName;
                    name += ", Coding " + QString::number(imageCodes[iCode]->iCode);
                    Mode mode = { imageFormats[iFormat]->iFormat, *(imageCodes[iCode]), size, name, mModes.size() };
                    mModes.append(mode);
                }
            }

            if (mModes.isEmpty())
                TRACKER_EXCEPTION("No usable camera modes found.");
            mCurrentMode = &mModes[0];
        }
        else
            TRACKER_EXCEPTION("No camera found!");

        // Get range for exposure time and gain
        tEXPO exposureTimeValues;
        warnIfError(FX_SetExposureTime(mCamID, NULL, &exposureTimeValues),
                    "Could not get exposure time range");
        mExposureTimeRange = qMakePair(exposureTimeValues.timeMin, exposureTimeValues.timeMax);

        tGAIN gainValues;
        warnIfError(FX_SetGainFactor(mCamID, NULL, &gainValues),
                    "Could not get camera gain range");
        mGainRange = qMakePair(gainValues.gainMin, gainValues.gainMax);

        // Computes size of largest required buffer
        mBufferSize = 0;
        foreach (const Mode& mode, mModes)
        {
            int temp = mode.size.width() * mode.size.height() * mode.code.iCanalBytes;
            mBufferSize = qMax(mBufferSize, temp * mode.code.iCanals * mode.code.iPlanes);
        }
        // Allocate buffer
        mBuffer = new unsigned char[mBufferSize];

        this->readSettings();

        // Select first format for startup
        this->setMode(mModes[0].name);

        libraryGuard.Dismiss();
    }

    WindowsCamera::~WindowsCamera()
    {
        this->writeSettings();

        warnIfError(FX_CloseCamera(mCamID), "Failed to properly close camera");
        warnIfError(FX_DeInitLibrary(), "Failed to properly close FX library");
    }

    void WindowsCamera::run(Thread* thread)
    {
        mIsRunning = true;

        // Just in case single shot mode waiting was still in progress
        finishSingleShot(QImage(), mClock.getTime(), mClock.getTime());

        // Allocate memory (SDK internal for the DMA buffer)
        // (setting the third parameter to 0 seems to be the way to go)
        // Do this here because stopping the data capture seems to delete the DMA resources
        if (!warnIfError(FX_AllocateResources(mCamID, 8, 0), "Camera: Failed to allocate DMA resources"))
        {
            if (!warnIfError(FX_StartDataCapture(mCamID, TRUE), "Camera: Failed to start capturing"))
            {
                int timerID = this->startTimer(500);
                mFrameCounter.reset();

                // Start event loop
                thread->exec();

                warnIfError(FX_StartDataCapture(mCamID, FALSE), "Failed to stop camera capturing");
            }
        }

        mIsRunning = false;

        this->moveToThread(QApplication::instance()->thread());
    }

    void WindowsCamera::timerEvent(QTimerEvent*)
    {
        emit frameRateUpdated(mFrameCounter.getFrameRate());
    }

    void WindowsCamera::processImage(HANDLE eventHandle)
    {
        quint64 captureTime = mClock.getTime();
        mFrameCounter.addFrame(captureTime);
      //  if (mCurrentMode->name=="Fullframe: 08 1392 x 1040, Coding 5")
       //     std::cout<<"CaptureTime: "<<captureTime<<std::endl;
            //qDebug()<<mCurrentMode->name;

        //if (true)
         //   this->Trigger();
       /** if (singleShotInProgress())
        {
            SingleStepIncrementer(); // Increasing the mImageCounter
            if (this->IsThisTheLastOne())
            {
                this->DecisionTree();
            }
            else
            {
                mLastSavingImage = true;
                this->SetBackTo();
            }
        }*/

        if (singleShotInProgress() && singleShotTimedOut(captureTime))
        {
            // Single shot timed out
            finishSingleShot(QImage(), captureTime, captureTime);
            TRACKER_INFO("Single shot acquisition timed out");
        }

        tBoImgDataInfoHeader imageHeader;
        // Important, clear struct because we could theoretically set some flags beforehand
        memset(&imageHeader, 0, sizeof(tBoImgDataInfoHeader));

        // Grab image (failure happens often when changing exposure time, gain or binning)
        if (FX_GetImageData(mCamID, &imageHeader, (void*)mBuffer, mBufferSize) != TRUE)
            return;

        if (!imageHeader.sDataCode.operator ==(mCurrentMode->code) ||
            imageHeader.iSizeX    != mCurrentMode->size.width() ||
            imageHeader.iSizeY    != mCurrentMode->size.height())
        {
            // Unsuitable image, discard (should not happen anyway)
            return;
        }

        // Copy image
        QImage image;
        if (imageHeader.sDataCode.iCanals > 1)
        {
            // Colour image
            TRACKER_EXCEPTION("Colour image decoding not yet implemented!", Logger::High);
        }
        else
        {
            // Monochrome image, stored as 8 bit colour table with grey values
            image = QImage(mCurrentMode->size, QImage::Format_Indexed8);
            image.setColorTable(mMonochromeColourTable);

            int width = image.width();
            int fastWidth = width / 8 / 4 * 4;
            int fastByteWidth = fastWidth * 8;
            for (int y = 0; y < image.height(); ++y)
            {
                // Copy line based to avoid alignment issues (QImage is 32 bit aligned
                // and the source is 'as is')

                const unsigned char* source = mBuffer + y * width;
                const unsigned char* sourceEnd = source + fastByteWidth;
                unsigned char* target = image.scanLine(y);

                // Optimised 4 x 64 bit copying
                while (source < sourceEnd)
                {
                    ((uint64_t*)target)[0] = ((const uint64_t*)source)[0];
                    ((uint64_t*)target)[1] = ((const uint64_t*)source)[1];
                    ((uint64_t*)target)[2] = ((const uint64_t*)source)[2];
                    ((uint64_t*)target)[3] = ((const uint64_t*)source)[3];
                    source += 4 * 8;
                    target += 4 * 8;
                }

                // Copy the rest that is not divisible by 8 * 4
                int restWidth = width - fastByteWidth;
                for (int x = 0; x < restWidth; ++x)
                    target[x] = source[x];
            }
        }
        //This function takes 7ms until here from its beginning
        if (singleShotInProgress())
        {
            this->finishSingleShot(image, captureTime, mClock.getTime());
        }
        else
        {
            //quint64 pixelread = mClock.getTime();
            //std::cout<< image.pixel(0,0)<<std::endl;
            //std::cout<<pixelread<<" - "<<mClock.getTime()<<std::endl;
           // if ((image.pixel(0,0)-4278190080)>1000000) // time requirement 33s
                emit imageProcessed(image, captureTime, mClock.getTime());
        }
    }

    void WindowsCamera::showCameraMessage(HANDLE eventHandle)
    {
        tBOMSG message;
        FX_GetNextMessage(&message);
        TRACKER_INFO(QString("Camera Message: ") + message.caAddText, Logger::Low);
    }

    QString WindowsCamera::getName() const
    {
        return mCamName;
    }

    QStringList WindowsCamera::getAvailableModes() const
    {
        QStringList modeList;
        foreach (const Mode& mode, mModes)
            modeList.append(mode.name);
        return modeList;
    }

    bool WindowsCamera::hasMode(QString mode) const
    {
        return (this->getModeIndex(mode) >= 0);
    }

    QString WindowsCamera::getMode() const
    {
        return mCurrentMode->name;
    }

    void WindowsCamera::setMode(QString modeString, int exposureTime, double gain)
    {
        int i = getModeIndex(modeString);
        if (i >= 0)
        {
            if (exposureTime >= 0)
                this->setExposureTime(exposureTime);
            if (gain >= 0)
                this->setGain(gain);

            mCurrentMode = &mModes[i];
            // Both format AND coding are required. They're coupled in this program though.
            warnIfError(FX_SetImageFormat(mCamID, mCurrentMode->format), "Failed to set the camera format");
            warnIfError(FX_SetImageCode(mCamID, mCurrentMode->code), "Failed to set the camera format code");
        }
    }

    int WindowsCamera::getModeIndex(QString modeString) const
    {
        foreach (const Mode& mode, mModes)
            if (mode.name == modeString)
                return mode.index;
        return -1;
    }

    QSize WindowsCamera::getImageSize(QString mode) const
    {
        if (this->hasMode(mode))
            return mModes[getModeIndex(mode)].size;
        else
            return QSize(0, 0);
    }

    int WindowsCamera::getExposureTime() const
    {
        tEXPO temp;
        warnIfError(FX_SetExposureTime(mCamID, NULL, &temp), "Could not get the exposure time");
        return temp.time2set;
    }

    void WindowsCamera::setExposureTime(int value)
    {
        int clampedValue = clamp(value, mExposureTimeRange.first, mExposureTimeRange.second);
        if (clampedValue != value)
            TRACKER_WARNING("Exposure time was clamped to meet the boundaries.", Logger::Low);
        warnIfError(FX_SetExposureTime(mCamID, clampedValue, NULL), "Could not set the exposure time");
    }

    double WindowsCamera::getGain() const
    {
        tGAIN temp;
        warnIfError(FX_SetGainFactor(mCamID, NULL, &temp), "Could not get the camera gain");
        return temp.gain2set;
    }

    void WindowsCamera::setGain(double value)
    {
        double clampedValue = clamp(value, mGainRange.first, mGainRange.second);
        if (clampedValue != value)
            TRACKER_WARNING("Camera gain was clamped to meet the boundaries.", Logger::Low);
        warnIfError(FX_SetGainFactor(mCamID, clampedValue, NULL), "Could not set the camera gain");
    }

    void WindowsCamera::readSettings()
    {
        Camera::readSettings();

        QSettings settings(PathConfig::getConfigFilename(), QSettings::IniFormat);
        settings.beginGroup("WindowsCamera");

        mNumDMABuffers = qMax(1, settings.value("Num_DMA_Buffers", 8).toInt());

        settings.endGroup();
    }

    void WindowsCamera::writeSettings()
    {
        Camera::writeSettings();

        QSettings settings(PathConfig::getConfigFilename(), QSettings::IniFormat);
        settings.beginGroup("WindowsCamera");

        settings.setValue("Num_DMA_Buffers", mNumDMABuffers);

        settings.endGroup();
    }

    /*static*/ void WindowsCamera::assertSuccess(int result, QString message)
    {
        if (result != TRUE)
            TRACKER_EXCEPTION(message + " Error code: " + getFXError(result));
    }

    /*static*/ bool WindowsCamera::warnIfError(int result, QString message, Logger::LogLevel logLevel)
    {
        if (result != TRUE)
        {
            TRACKER_WARNING(message + " Error code: " + getFXError(result), logLevel);
            return true;
        }
        else
            return false;
    }

    /*static*/ QString WindowsCamera::getFXError(int id)
    {
        if (-id >= sizeof(BoFxLibErrors) / sizeof(BoFxErrList))
            return "FX Error ID not found";
        else
            return FX_GetErrString(id);
    }
}
