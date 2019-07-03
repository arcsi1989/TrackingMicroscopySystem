/*
 Copyright (c) 2014, Tobias Klauser

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#include <math.h>

#include <QImage>
#include <QDate>
#include <QTime>
#include <QDir>
#include <QFileInfo>

#include "FocusTracker.h"

#include "Camera.h"
#include "CorrelationImage.h"
#include "CurveFitter.h"
#include "Logger.h"
#include "Stage.h"
#include "Timing.h"
#include "TMath.h"

// TEMP
#include <iostream>

namespace tracker
{
    FocusTracker::FocusTracker(Stage* stage, QSize imgSize)
        : mStage(stage)
        , mCurrentImage(NULL), mImageSize(imgSize)
        , mCurrentPosition(0.0), mStartPosition(0.0)
        , mStepSize(0.0), mStackSize(0), mStackNumImages(0)
        , mCurrentDCValue(128.0)
        , mCameraExposureTime(0.0)
        , mFitter(NULL)
        , mZStackReady(false)
        , mLogging(true)
        , mUpperBrennerThreshold(0.85)
        , mLowerBrennerThreshold(0.02)
        , mLargeCorrectionStep(7.0)
    {
        // Curve Fitter for 1 term Gaussian with offset
        mFitter = new CurveFitter(CurveFitter::CurveGaussian1Offset);
        mCurrentImage = new BaseImage(mImageSize);

        // We throw away 3 images, so we need to have at least 2 to consider
        // for statistical measures (mean, stddev)
        assert(ZSTACK_IMAGES_PER_POSITION >= 5);
    }

    FocusTracker::~FocusTracker()
    {
        if (mLogFileZStack.isOpen()) {
            mLogFileStreamZStack.flush();
            mLogFileZStack.close();
        }
        delete mCurrentImage;
    }

    void FocusTracker::startZStack(double stepSize, double stackSize)
    {
        // purge previously acquired Z stack
        mZStack.clear();

        mStepSize = stepSize;
        mStackSize = stackSize;
        mStackNumImages = ceil(mStackSize / mStepSize);

        // We assume the stage to be at optimal focus, so move stage half the
        // stack size down before starting. Block the operation until the
        // stage has finished moving.
        mFocusedPosition = mStage->getZpos();
        std::cout << "FocusTracker: Moving stage from " << mFocusedPosition
                  << " to ";
        quint64 s = mClock.getTime();
        //mStage->moveZ(-(mStackSize / 2.0), 1);
        for (unsigned int i = 0; i < mStackNumImages / 2; ++i)
            mStage->moveZ(-mStepSize, 1);

        quint64 e = mClock.getTime();
        mStartPosition = mCurrentPosition = mStage->getZpos();
        std::cout << mCurrentPosition
                  << ", time " << e - s << std::endl;

        if (mLogging) {
            // shouldn't happen, but close it anyways
            if (mLogFileZStack.isOpen())
                mLogFileZStack.close();

            // Open log file for Z positions/focus values
            QFileInfo info1(mStorageFilename);
            QString storageFilename = "";
            storageFilename = info1.absolutePath();
            storageFilename += "\\";
            storageFilename += QDate::currentDate().toString("yyyyMMdd");
            storageFilename += QTime::currentTime().toString("hhmmss");
            storageFilename += "_";
            QString functionFname = storageFilename + "zstack_zpos_focus_brenner.csv";
            std::cout << "Opening log file " << functionFname.toStdString() << std::endl;
            mLogFileZStack.setFileName(functionFname);
            mLogFileZStack.open(QIODevice::WriteOnly | QIODevice::Text);
            if (!mLogFileZStack.isOpen())
                TRACKER_WARNING("Could not open log file");
            mLogFileStreamZStack.setDevice(&mLogFileZStack);

            functionFname = storageFilename + "zstack_zpos_focus_brenner_values.csv";
            std::cout << "Opening log file " << functionFname.toStdString() << std::endl;
            mLogFileZStackValues.setFileName(functionFname);
            mLogFileZStackValues.open(QIODevice::WriteOnly | QIODevice::Text);
            if (!mLogFileZStackValues.isOpen())
                TRACKER_WARNING("Could not open log file");
            mLogFileStreamZStackValues.setDevice(&mLogFileZStackValues);

            mLogFileStreamZStackValues << "captureTime,mClock,mCurrentPosition,brennerFocus\n";
            mLogFileStreamZStackValues.flush();

        }

        std::cout << "Starting Z stack, step size " << mStepSize
                  << ", stack size " << mStackSize
                  << " (" << mStackNumImages << " images)"
                  << std::endl;

        mStampStageMoved = mClock.getTime();
    }

    void FocusTracker::processZStackNewVersion(double BrennerValue,quint64 captureTime){
        // Image was captured before we finished the previous movement...
        if (captureTime - mCameraExposureTime < mStampStageMoved) {
            // ...silently discard it
            std::cout<< std::endl<<"SkippedImage"<<std::endl;
            return;
        }

        // Make sure we don't acquire more images for the stack than asked for
        if (mZStack.size() == mStackNumImages) {
            emit zStackFinished();
            return;
        }

        std::cout << "processZStack";

        // Check whether we're ready to take the next image
        if (mStage->isMovingZ()) {
            std::cout << ", stage still moving" << std::endl;
            return;
        }

       /* if (image.size().height() != mImageSize.height()
            || image.size().width() != mImageSize.width()) {
            TRACKER_WARNING("Image size doesn't match Z stack size");
            resizeImageBuffer(image.size());
        }*/


        double brennerFocus = BrennerValue;
        mCurrentPosition = mStage->getZpos();

        std::cout << ", image " << mZStack.size() + 1 << "/" << mStackNumImages
                  << ", zpos " << mCurrentPosition
                  << ", focus " << brennerFocus;

        mLogFileStreamZStackValues << captureTime << "," << mClock.getTime() << "," <<
                                      mCurrentPosition << "," << brennerFocus << "\n";

        // have we taken enough images at this particular position?
        if (mBrennerTemp.size() >= ZSTACK_IMAGES_PER_POSITION) {
            // Do not consider the first two and the last focus values for the
            // image noise statistics. Especially the first two values are
            // usually quite far off from the others, possibly stemming from
            // the fact that the according images were taken at a former stage
            // position.

            //! - Brenner Function test Zpos correspondence to Brenner Value
            mBrennerTemp.pop_front();
            mBrennerTemp.pop_front();
            mBrennerTemp.pop_back();

            // Minimum, maximum and mean
            double meanb, minb, maxb;
            meanb = 0.0;
            minb = maxb = mBrennerTemp.at(0);
            for (int i = 0; i < mBrennerTemp.size(); i++) {
                meanb += mBrennerTemp[i];
                if (mBrennerTemp[i] < minb)
                    minb = mBrennerTemp[i];
                if (mBrennerTemp[i] > maxb)
                    maxb = mBrennerTemp[i];
            }
            meanb /= mBrennerTemp.size();

            // Calculate standard deviation
            double stdDev = 0.0;
            for (int i = 0; i < mBrennerTemp.size(); i++) {
                stdDev += sqr(mBrennerTemp[i] - meanb);
            }
            stdDev = sqrt(stdDev / mBrennerTemp.size());

            std::cout << ", mean " << meanb
                      << ", stddev " << stdDev
                      << ", min " << minb
                      << ", max " << maxb;

            // Store <position, F_Brenner, noise> in Z-stack
            mZStack.append(mCurrentPosition, meanb, stdDev);

            // log Z position and Brenner focus value
            if (mLogging) {
                mLogFileStreamZStack << mCurrentPosition << ","
                                     << meanb << ","
                                     << stdDev << "\n";
            }

            // Move the stage up and wait until it did so, so we can take a time stamp.
            mStage->moveZ(mStepSize, 1);
            mStampStageMoved = mClock.getTime();

            mBrennerTemp.clear();

            // Check whether we've taken enough images and if so, emit zStackFinished()
            if (mZStack.size() == mStackNumImages) {
                std::cout << " (last)";
                emit zStackFinished();
            }
        } else {
            mBrennerTemp.append(brennerFocus);
        }

        std::cout << std::endl;
    }


    void FocusTracker::processZStack(QImage image, quint64 captureTime, quint64 processTime)
    {
        // Image was captured before we finished the previous movement...
        if (captureTime - mCameraExposureTime < mStampStageMoved) {
            // ...silently discard it
            std::cout<< std::endl<<"SkippedImage"<<std::endl;
            return;
        }

        // Make sure we don't acquire more images for the stack than asked for
        if (mZStack.size() == mStackNumImages) {
            emit zStackFinished();
            return;
        }

        std::cout << "processZStack";

        // Check whether we're ready to take the next image
        if (mStage->isMovingZ()) {
            std::cout << ", stage still moving" << std::endl;
            return;
        }

        if (image.size().height() != mImageSize.height()
            || image.size().width() != mImageSize.width()) {
            TRACKER_WARNING("Image size doesn't match Z stack size");
            resizeImageBuffer(image.size());
        }

        quint64 timebefore = mClock.getTime();
        mCurrentImage->assignZ(image);
        quint64 timeafter = mClock.getTime();
        std::cout<<"TimeForBrenneCalculation: "<<timeafter- timebefore<<std::endl;
        //mCurrentDCValue = mCurrentImage->assign(image, mCurrentDCValue);
        double brennerFocus = mCurrentImage->getFocus().brennerFocus;
        mCurrentPosition = mStage->getZpos();

        std::cout << ", image " << mZStack.size() + 1 << "/" << mStackNumImages
                  << ", zpos " << mCurrentPosition
                  << ", focus " << brennerFocus;

        mLogFileStreamZStackValues << captureTime << "," << mClock.getTime() << "," <<
                                      mCurrentPosition << "," << brennerFocus << "\n";

        // have we taken enough images at this particular position?
        if (mBrennerTemp.size() >= ZSTACK_IMAGES_PER_POSITION) {
            // Do not consider the first two and the last focus values for the
            // image noise statistics. Especially the first two values are
            // usually quite far off from the others, possibly stemming from
            // the fact that the according images were taken at a former stage
            // position.
            
            //! - Brenner Function test Zpos correspondence to Brenner Value
            mBrennerTemp.pop_front();
            mBrennerTemp.pop_front();
            mBrennerTemp.pop_back();

            // Minimum, maximum and mean
            double meanb, minb, maxb;
            meanb = 0.0;
            minb = maxb = mBrennerTemp.at(0);
            for (int i = 0; i < mBrennerTemp.size(); i++) {
                meanb += mBrennerTemp[i];
                if (mBrennerTemp[i] < minb)
                    minb = mBrennerTemp[i];
                if (mBrennerTemp[i] > maxb)
                    maxb = mBrennerTemp[i];
            }
            meanb /= mBrennerTemp.size();

            // Calculate standard deviation
            double stdDev = 0.0;
            for (int i = 0; i < mBrennerTemp.size(); i++) {
                stdDev += sqr(mBrennerTemp[i] - meanb);
            }
            stdDev = sqrt(stdDev / mBrennerTemp.size());

            std::cout << ", mean " << meanb
                      << ", stddev " << stdDev
                      << ", min " << minb
                      << ", max " << maxb;

            // Store <position, F_Brenner, noise> in Z-stack
            mZStack.append(mCurrentPosition, meanb, stdDev);

            // log Z position and Brenner focus value
            if (mLogging) {
                mLogFileStreamZStack << mCurrentPosition << ","
                                     << meanb << ","
                                     << stdDev << "\n";
            }

            // Move the stage up and wait until it did so, so we can take a time stamp.
            mStage->moveZ(mStepSize, 1);
            mStampStageMoved = mClock.getTime();

            mBrennerTemp.clear();

            // Check whether we've taken enough images and if so, emit zStackFinished()
            if (mZStack.size() == mStackNumImages) {
                std::cout << " (last)";
                emit zStackFinished();
            }
        } else {
            mBrennerTemp.append(brennerFocus);
        }

        std::cout << std::endl;
    }

    void FocusTracker::finishZStack()
    {
        std::cout << "FocusTracker: moving back to " << mFocusedPosition;
        // move the stage back down to initial (focussed) position
        quint64 s = mClock.getTime();
        //mStage->moveToZ(mFocusedPosition, 1);
        for (unsigned int i = 0; i < mStackNumImages / 2; ++i)
            mStage->moveZ(-mStepSize, 1);
        quint64 e = mClock.getTime();
        mCurrentPosition = mStage->getZpos();
        std::cout << ", got to " << mCurrentPosition
                  << ", time " << e - s << std::endl;

        std::cout << "FocusTracker: fitting" << std::endl;

        // Provide expected params to make curve fitting faster and more stable
        QVector<double> params;
        // a: height of Gaussian. Use peak value.
        params << mZStack.getMaxFocus();
        // b: mean of Gaussian. Use the Z position of the peak.
        params << mZStack.getPositionForFocus(mZStack.getMaxFocus());
        // c: standard deviation. Empirical value.
        params << 2.2;
        // d: linear offset. Empirical value.
        params << 0.8;

        // Set initial guessed parameters
        mFitter->setParams(params);
        if (!mFitter->fit(mZStack.getZPositions(), mZStack.getFocusValues())) {
            TRACKER_WARNING("Curve could not be fitted. Please re-run Z stack.");
        }

        // Get back the parameters of the fitted curve
        params = mFitter->getParams();

        std::cout << "Fitted curve with parameters:";
        for (int i = 0; i < params.size(); ++i)
                std::cout << " " << params[i];
        std::cout << std::endl;

        // Perform some validity checks on params (i.e. positive amplitude and offset)
        if (params[0] <= 0.0 || params[3] <= 0.0) {
            TRACKER_WARNING("Fitted parameters invalid. Please re-run Z stack.");
        }

        // make sure the logged data is written to disk
        mLogFileStreamZStack.flush();
        mLogFileZStack.close();

        mLogFileStreamZStackValues.flush();
        mLogFileZStackValues.close();

        mZStackReady = true;
    }

    FocusValue FocusTracker::getLastFocus() const
    {
        return mCurrentImage->getFocus();
    }

    double FocusTracker::getMaxFocus() const
    {
        if (mZStack.size() <= 0)
            return 0.0;
        else
            return mZStack.getMaxFocus();
    }

    double FocusTracker::getMaxPredictedFocus() const
    {
        if (!mFitter || !mZStackReady)
            return 0.0;
        else
            return mFitter->getParams()[0];
    }

    double FocusTracker::getUpperBrennerThreshold() const
    {
        return mUpperBrennerThreshold;
    }

    double FocusTracker::getUpperThresholdFocus() const
    {
        if (!mFitter || !mZStackReady)
            return 0.0;
        else
            return getMaxFocus() * mUpperBrennerThreshold;
    }

    double FocusTracker::getUpperThresholdPredictedFocus() const
    {
        if (!mFitter || !mZStackReady)
            return 0.0;
        else
            return mFitter->getParams()[0] * mUpperBrennerThreshold;
    }

    double FocusTracker::getLowerBrennerThreshold() const
    {
        return mLowerBrennerThreshold;
    }

    double FocusTracker::getLowerThresholdFocus() const
    {
        if (!mFitter || !mZStackReady)
            return 0.0;
        else
            return getMaxFocus() * mLowerBrennerThreshold;
    }

    double FocusTracker::getLowerThresholdPredictedFocus() const
    {
        if (!mFitter || !mZStackReady)
            return 0.0;
        else
            return mFitter->getParams()[0] * mLowerBrennerThreshold;
    }

    double FocusTracker::getNoiseLevelAt(double zPos) const
    {
        if (!mFitter || !mZStackReady)
            return 0.0;
        else
            return mZStack.getNoiseLevelAt(zPos);
    }

    double FocusTracker::getNoiseLevelFromBrenner(double brennerValue) const
    {
        if (!mFitter || !mZStackReady)
            return 0.0;
        else
            return mZStack.getNoiseLevelForFocus(brennerValue);
    }

    double FocusTracker::getHalfWindowSize()
    {
        if (!mFitter || !mZStackReady)
            return 0.0;
        else
            return std::abs( mZStack.getPositionForFocus( getLowerThresholdFocus() ) - mZStack.getMaxFocusPosition() );
    }    

    double FocusTracker::getFullWindowSize()
    {
        return 2 * getHalfWindowSize();
    }

    double FocusTracker::correctFocus(double brennerValue)
    {
        if (!mZStackReady)
            return 0.0;

#if 0
        // use the inverse function of the fitted curve to look up the position
        // for the current focus
        double correctionStep = mFitter->interpolateX(brennerValue);
        // get the correction distance by subtracting the mean, the result will
        // always be positive due to the nature of the functional inverse of
        // the Gaussian
        correctionStep -= mFitter->getParams()[1];
        // the direction of the move will be decided on in the caller
#endif
        double zPos = mZStack.getPositionForFocus(brennerValue);
        return zPos - mZStack.getMaxFocusPosition();
    }

    void FocusTracker::resizeImageBuffer(QSize imgSize)
    {
        if (imgSize == mImageSize)
            return;

        delete mCurrentImage;
        mImageSize = imgSize;
        mCurrentImage = new BaseImage(mImageSize);
    }

    void FocusTracker::setStorageFolder(QString filename){
        mStorageFilename = filename;
    }

    void FocusTracker::setBrennerRoiPercentage(unsigned int roiPercentage)
    {
        mCurrentImage->setBrennerRoiPercentage(roiPercentage);
    }

} // namespace tracker
