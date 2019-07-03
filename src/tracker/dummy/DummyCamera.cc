/*
 Copyright (c) 2009-2012, Reto Grieder
 Copyright (c) 2014, Tobias Klauser

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#include "DummyCamera.h"

#include <QApplication>
#include <QSettings>
#include <iostream>

#include "gaussianblur.h"

#include "Exception.h"
#include "Logger.h"
#include "PathConfig.h"
#include "Timing.h"
#include "TrackerAssert.h"
#include "TMath.h"
#include "Utils.h"

#include "CurveFitter.h"

#include <iostream>

namespace tracker
{
    /*static*/ Camera* Camera::makeCamera()
    {
        return new DummyCamera();
    }

    DummyCamera::DummyCamera()
    {
        // Create colour table for 8 bit grey images
        mColourTable.resize(256);
        for (int i = 0; i < 256; ++i)
            mColourTable[i] = qRgb(i, i, i);

        // Load base image
        mBaseImage.load(PathConfig::getDataPath().path() + "/dummy/s01_20140320_165148.png");
        mBaseImage = mBaseImage.convertToFormat(QImage::Format_Indexed8, mColourTable);

        // Load image stack
        const double stepSize = 0.1;  // distance between 2 successive images (in microns)
        QDir stackDir(PathConfig::getDataPath().path() + "/dummy/zstack");
        if (stackDir.exists()) {
            QStringList imageFiles = stackDir.entryList(QStringList("*.png"),
                                                        QDir::Files | QDir::NoSymLinks);
            if (!imageFiles.isEmpty()) {
                std::cout << "Loading " << imageFiles.size() << " images to stack" << std::endl;
                for (int i = 0; i < imageFiles.size(); i++) {
                    QImage image = QImage(stackDir.absolutePath() + "/" + imageFiles[i]);

                    if (!image.isNull()) {
                        image = image.convertToFormat(QImage::Format_Indexed8, mColourTable);
                        mImageStack.append(image);
/*
                        QFileInfo info(stackDir.absolutePath() + "/" + imageFiles[i]);
                        QFile metaData(stackDir.absolutePath() + "/" + info.baseName() + ".txt");
                        if (!metaData.open(QIODevice::ReadOnly | QIODevice::Text)) {
                            TRACKER_EXCEPTION("Failed to load meta data for image " + imageFiles[i]);
                        } else {
                            QTextStream in(&metaData);
                            bool foundZPos = false;
                            while (!in.atEnd() && !foundZPos) {
                                QString line = in.readLine();

                                // read Z position from metadata
                                if (line.startsWith("z:")) {
                                    bool ok = false;
                                    double zPos = line.mid(2, -1).toDouble(&ok);

                                    if (ok) {
                                        if (mZPosStack.size() > 0 && mZPosStack.last() < zPos) {
                                            TRACKER_EXCEPTION("Image stack Z positions not successive: "
                                                              + QString::number(mZPosStack.last()) + ", " + QString::number(zPos)
                                                              + "(" + QString::number(mZPosStack.size() - 1) + ", " + QString::number(mZPosStack.size()) + ")");
                                        } else
                                            mZPosStack.append(zPos);
                                    } else
                                        TRACKER_EXCEPTION("Failed to get Z position for image " + imageFiles[i]);
                                    foundZPos = true;
                                }
                            }

                            if (!foundZPos)
                                TRACKER_EXCEPTION("Failed to get Z position for image " + imageFiles[i]);
                            metaData.close();
                        }
*/
                    } else
                        TRACKER_EXCEPTION("Failed to load image " + imageFiles[i]);
                }

                std::cout << "Successfully loaded " << mImageStack.size() << " images" << std::endl;

                // Assign Z positions to images, use the middle image as position 0
                for (int i = 0; i < mImageStack.size(); ++i) {
                    mZPosStack.append((i - (mImageStack.size() / 2)) * stepSize);
                }
            } else
                TRACKER_WARNING("No images found in stack directory");
        } else
            TRACKER_WARNING("Image stack directory " + stackDir.absolutePath() + " not found. Not using image stack.");

        // Define video modes
        mModes["640x480"] = QSize(640, 480);
        mModes["720x540"] = QSize(720, 540);
        mModes["800x600"] = QSize(800, 600);
        mExposureTime = 10000;
        mGain = 1.0;

        mCurrentMode = "640x480";

        this->readSettings();
    }

    DummyCamera::~DummyCamera()
    {
        this->writeSettings();
    }

    void DummyCamera::run(Thread* thread)
    {
        mIsRunning = true;

        int timerID = this->startTimer(500);
        mFrameCounter.reset();

        // Start event loop
        thread->exec();

        this->killTimer(timerID);

        mIsRunning = false;

        this->moveToThread(QApplication::instance()->thread());
    }

    void DummyCamera::timerEvent(QTimerEvent*)
    {
        emit frameRateUpdated(mFrameCounter.getFrameRate());
    }

    void DummyCamera::makeImage(QPointF offset, double zOffset, quint64 processTime)
    {
        // Don't let the image buffer overflow due to missing CPU power
        if (mClock.getTime() > processTime + 3000)
            return;

        // Could be that there are still pending events (just a few) even though
        // the thread has already finished
        if (!mIsRunning)
            return;

        quint64 captureTime = mClock.getTime();
        mFrameCounter.addFrame(captureTime);

        QRect rect;
        rect = QRect(QPoint(0, 0), mModes[mCurrentMode]);

        // Move the rectangle to the center but with offset
        // Note: The minus sign is required because moveCenter() moves the camera
        //       instead of the picture itself
        rect.moveCenter(mBaseImage.rect().center() - offset.toPoint());
        QImage image = mBaseImage.copy(rect);

        // TODO: Apply proper out-of-focus simulation using a convolution with a disc
        TGaussianBlur<uint8_t> blur;
        int blurSize = std::abs(zOffset) * 2 + 1;
        blur.Filter(image.scanLine(0), NULL, image.width(), image.height(), blurSize);

        // Add some noise
        addImageNoise(image);

        // Send partial image
        if (this->singleShotInProgress())
            this->finishSingleShot(image, captureTime, mClock.getTime());
        else
            emit imageProcessed(image, captureTime, mClock.getTime());
    }

    void DummyCamera::makeStackImage(QPointF offset, double zOffset, quint64 processTime)
    {
        // Don't let the image buffer overflow due to missing CPU power
        if (mClock.getTime() > processTime + 3000)
            return;

        // Could be that there are still pending events (just a few) even though
        // the thread has already finished
        if (!mIsRunning)
            return;

        // We can't get any image with an empty stack
        if (mImageStack.empty())
            return;

        quint64 captureTime = mClock.getTime();
        mFrameCounter.addFrame(captureTime);

        QRect rect;
        rect = QRect(QPoint(0, 0), mModes[mCurrentMode]);

        // Retreive the image closest to zOffset from the stack, if the value
        // is beyond the range value of the array it is clamped
        int i = binarySearchClosest(mZPosStack, zOffset);
        /*
        std::cout << "makeStackImage(): zOffset " << zOffset
                  << ", index " << i
                  << " (" << mZPosStack.first() << "," << mZPosStack.last() << ")"
                  << std::endl;
        */
        // Move the rectangle to the center but with offset
        // Note: The minus sign is required because moveCenter() moves the camera
        //       instead of the picture itself
        rect.moveCenter(mImageStack[i].rect().center() - offset.toPoint());
        QImage image = mImageStack[i].copy(rect);

        // Add some noise
        addImageNoise(image);

        // Send partial image
        if (this->singleShotInProgress())
            this->finishSingleShot(image, captureTime, mClock.getTime());
        else
            emit imageProcessed(image, captureTime, mClock.getTime());
    }

    QStringList DummyCamera::getAvailableModes() const
    {
        return mModes.keys();
    }

    QString DummyCamera::getMode() const
    {
        return mCurrentMode;
    }

    void DummyCamera::setMode(QString modeText, int exposureTime, double gain)
    {
        if (mModes.contains(modeText))
        {
            mCurrentMode = modeText;
            if (exposureTime >= 0)
                mExposureTime = exposureTime;
            if (gain >= 0)
                mGain = gain;
        }
    }

    QSize DummyCamera::getImageSize(QString mode) const
    {
        if (mModes.contains(mode))
            return mModes[mode];
        else
            return QSize(0, 0);
    }

    void DummyCamera::addImageNoise(QImage& image)
    {
        for (int y = 0; y < image.height(); ++y) {
            uchar* ptr = image.scanLine(y);
            for (int x = 0; x < image.width(); ++x)
            {
                ptr[x] = qMax(0, qMin(ptr[x] + (qrand() & 15) - 8, 255));
            }
        }
    }
}
