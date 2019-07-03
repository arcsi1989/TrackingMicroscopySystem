/*
 Copyright (c) 2014, Tobias Klauser

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef __FOCUSTRACKER_H__
#define __FOCUSTRACKER_H__

#include "TrackerPrereqs.h"

#include <QFile>
#include <QImage>
#include <QMetaType>
#include <QPair>
#include <QSize>
#include <QTextStream>
#include <QVector>

#include "Timing.h"
#include "Utils.h"
#include <iostream>

namespace tracker {

    /** Collection of focus-related values. */
    struct FocusValue {
        FocusValue() :
            brennerFocus(0.0), avgBrennerFocus(0.0),
            rampFocus(0.0), logFocus(0.0), gaussFocus(0.0),
            integralFocus(0.0), tanhFocus(0.0), avgFocus(0.0),
            maxValue(0.0), noiseLevel(0.0) {}

        float brennerFocus;     /*!< Focus value as determined by the Brenner function,
                                     see CorrelationImage::extractFocusBrenner() */
        float avgBrennerFocus;  //!<
        float rampFocus;        //!< DEPRECATED
        float logFocus;         //!< DEPRECATED
        float gaussFocus;       //!< DEPRECATED
        float integralFocus;    //!< DEPRECATED
        float tanhFocus;        //!< DEPRECATED
        float avgFocus;         /*!< DEPRECATED, Focus averaged over several images,
                                     set in Correlator::getLastRampFocus() */
        float maxValue;         //!< Max value of reduced DFT data
        float noiseLevel;       //!< Noise level of DFT data
    };

    /** Container for Z position, focus values and noise levels as acquired
     *  during Z stack collection.
     */
    class ZStack {
    public:
        ZStack()
            : mMaxFocusIndex(0)
        {}

        /** Append a triplet <Z position, Brenner focus, noise level> to the
         *  Z stack.
         */
        void append(double pos, double focus, double noise)
        {
            mZPositions.append(pos);
            mFocusValues.append(focus);
            mNoiseLevels.append(noise);

            if (focus > mFocusValues.at(mMaxFocusIndex)){
                mMaxFocusIndex = size() - 1;
                //  std::cout << __FUNCTION__ << ":   FocusValue: " << focus<< std::endl;
            }
        }

        /** Get the number of entries in the Z stack. */
        int size() const {
            // All sizes are guaranteed to be the same
            return mZPositions.size();
        }

        QVector<double> getZPositions() const {
            return mZPositions;
        }

        QVector<double> getFocusValues() const {
            return mFocusValues;
        }

        QVector<double> getNoiseLevels() const {
            return mNoiseLevels;
        }

        double getFocusAt(double zPos) const {
            int index = binarySearchClosest(mZPositions, zPos);
            if (index < 0)
                return 0.0;
            return mFocusValues[index];
        }

        double getNoiseLevelAt(double zPos) const {
            // find closest z position
            int index = binarySearchClosest(mZPositions, zPos);
            if (index < 0)
                return 0.0;
            return mNoiseLevels[index];
        }

        double getNoiseLevelForFocus(double brennerValue) const {
            // find closest brenner value by linear search
            int index = -1;
            double brennerDiff = getMaxFocus();
            for (int i = mMaxFocusIndex; i < size(); i++) {
                double curDiff = std::abs(brennerValue - mFocusValues.at(i));
                if (curDiff < brennerDiff) {
                    index = i;
                    brennerDiff = curDiff;
                }
            }
            if (index >= 0)
                return mNoiseLevels.at(index);
            else
                return 0.0;
        }

        double getPositionForFocus(double brennerValue) const {
            // find closest brenner value by linear search
            int index = -1;
            double brennerDiff = getMaxFocus();
            for (int i = mMaxFocusIndex; i < size(); i++) {
                double curDiff = std::abs(brennerValue - mFocusValues.at(i));
                if (curDiff < brennerDiff) {
                    index = i;
                    brennerDiff = curDiff;
                }
            }
            if (index >= 0)
                return mZPositions.at(index);
            else
                return 0.0;
        }

        double getMaxFocus() const {
            return mFocusValues.at(mMaxFocusIndex);
        }

        double getMaxFocusPosition() const {
            return mZPositions.at(mMaxFocusIndex);
        }

        void clear() {
            mZPositions.clear();
            mFocusValues.clear();
            mNoiseLevels.clear();
            mMaxFocusIndex = 0;
        }

    private:
        QVector<double>     mZPositions;
        QVector<double>     mFocusValues;
        QVector<double>     mNoiseLevels;
        //!< Z stack position with the highest focus value
        int                 mMaxFocusIndex;
    };

    /** Class handling the vertical stage adjustments based on image focus.
     */
    class FocusTracker : public QObject
    {
        Q_OBJECT
    private:
        Q_DISABLE_COPY(FocusTracker)

    public:
        /** Construct focus tracker.
         *
         * Allocates and initializes all necessary resources.
         *
         * @param stage
         *   Pointer to the Stage instance.
         * @param imgSize
         *   Size of the image as delivered by the camera.
         */
        FocusTracker(Stage* stage, QSize imgSize);
        ~FocusTracker();

        /** Start Z stack acquisition.
         *
         * This currently assumes that the stage has manually been moved to the
         * position where focus is optimal.
         *
         * @param stepSize
         *   Step distance in microns between to successive images in the stack
         * @param stackSize
         *   Size of the Z stack in microns (distance from lowest to highest image)
         */
        void startZStack(double stepSize, double stackSize);

        /** Collect the BrennerValue of the OffLineZstack Imaging for
         *  the offline look up table
         * @param BV (BrennerValue)
         *   The image to process.
         */
        void processZStackNewVersion(double BV, quint64);

        /** Perform image transformations needed to extract focus values.
         *
         * @param image
         *   The image to process.
         */
        void processZStack(QImage image, quint64, quint64);

        /** Finish Z stack acquisition.
         *
         * This triggers the curve fitting algorithm to determine the focus
         * autocorrection parameters based on a previously acquired Z stack.
         */
        void finishZStack();

        /** Check whether the Z stack is ready for online usage.
         * This will return true after a Z stack has been acquired and the
         * curve successfully fitted. Otherwise return false;
         *
         * Also see mZStackReady.
         */
        bool isReady() const
            { return mZStackReady; }

        /// See mUpperBrennerThreshold
        void setUpperBrennerThreshold(double thresh)
            { mUpperBrennerThreshold = thresh; }

        /// See mLowerBrennerThreshold
        void setLowerBrennerThreshold(double thresh)
            { mLowerBrennerThreshold = thresh; }

        /// See mLargeCorrectionStep
        void setLargeCorrectionStep(double size)
            { mLargeCorrectionStep = size; }

        /// See mLargeCorrectionStep
        double getLargeCorrectionStep()
            { return mLargeCorrectionStep; }

        /** Get the focus value of the last acquired image. */
        FocusValue getLastFocus() const;

        /** Get the maximum measured focus value from the Z stack. */
        double getMaxFocus() const;

        /** Get the maximum focus value as predicted by the fitted curve.
         *
         * This is equivalent to the amplitude of the fitted Gaussian
         * function.
         */
        double getMaxPredictedFocus() const;

        /** Get the upper threshold focus value based on the Z stack values. */
        double getUpperThresholdFocus() const;
        double getUpperBrennerThreshold() const;

        /** Get the upper threshold focus value based on the fitted curve. */
        double getUpperThresholdPredictedFocus() const;

        /** Get the focus threshold value below which predicition using the
         *  fitted curve no longer reliably works.
         *
         *  Based on the Z stack values.
         */
        double getLowerThresholdFocus() const;
        double getLowerBrennerThreshold() const;

        /** Get the focus threshold value below which predicition using the
         *  fitted curve no longer reliably works.
         *
         *  Based on the fitted curve.
         */
        double getLowerThresholdPredictedFocus() const;

        /** Get the approximate noise level at the given Z position.
         *
         * Since the noise level of the focus is not constant, we also need
         * to "model" this. This is done by taking multiple images at each
         * position in the Z stack and then calculate the std. deviation which
         * is stored alongside the focus value in the Z stack.
         *
         * @param zPos
         *   Z position for which to get the noise level
         * @return
         *   Noise level (same scale as Brenner focus value) at the given
         *   position
         */
        double getNoiseLevelAt(double zPos) const;

        /** Get the approximate noise level at the given brenner value.
         *
         * @param brennerValue
         *   brenner value for which to get the noise level
         * @return
         *   Noise level (same scale as Brenner focus value) at the given
         *   position
         */
        double getNoiseLevelFromBrenner(double brennerValue) const;

        /**
         * Determine the distance required to correct the focus.
         *
         * This takes the current Brenner focus value and uses the Z stack
         * to look up the amount of correction needed to reach the optimal
         * focus region.
         *
         * @param brennerValue
         *   Current Brenner focus value
         * @return
         *   Distance in microns to correct the Z position
         */
        double correctFocus(double brennerValue);

        /** Resize internal storage for internal image.
         *
         * This needs to be called whenever the resolution of the camera is changed,
         * i.e. in Controller::updateCorrelator()
         *
         * @param imgSize
         *   New image size
         */
        void resizeImageBuffer(QSize imgSize);

        void setBrennerRoiPercentage(unsigned int roiPercentage);

        void setStorageFolder(QString filename);




        /** Enable/disable logging of Z stack acquisition.
         *
         * @param enable
         *   true to enable logging, false to disable
         */
        void setLogging(bool enable)
            { mLogging = enable; }

        void setCameraExposureTime(double exposureTime)
            { mCameraExposureTime = exposureTime; }

        /*
         *   Estimate the window size as the half-width at the lower threshold
         *   Brenner value
         */
        double getHalfWindowSize();
        /*
         *   Twice the half-window size estimated from the Brenner function
         *
         *   TODO: Brenner function is a bit asymmetric which could be taken
         *         into account by measuring the exact distance from left to
         *         right intersection points at the lower threshold.
         */
        double getFullWindowSize();

    signals:
        /** Raised when the Z stack acquisition is finished. */
        void zStackFinished();

    private:
        void run(Thread* thread);

        /*!< Number of images to take per Z stack position. This has to be
             larger than 4, since some samples are removed in processZStack()
             to calculate statistical measures. */
        static const int    ZSTACK_IMAGES_PER_POSITION = 7;

        //!< Stage instance, needed to control Z movements
        Stage*              mStage;
        //!< Buffer for the currently processed image during Z stack acquisition
        BaseImage*          mCurrentImage;
        //!< Currently configured camera image size
        QSize               mImageSize;
        //!< Current Z position
        double              mCurrentPosition;
        //!< Percise start position of the Z stack (from stage directly)
        double              mStartPosition;
        //!< Precise position where the image was in focus before Z stack acqusition.
        double              mFocusedPosition;
        //!< Step size for the Z stack (in microns)
        double              mStepSize;
        //!< Requested Z stack size (in microns)
        double              mStackSize;
        //!< Requested number of images in the stack
        unsigned int        mStackNumImages;
        /*!< Satisfaction threshold for the Z stack based autofocus. Above this
             fraction of the maximum Brenner value, no correction will be
             performed. Default value: 0.85 */
        double              mUpperBrennerThreshold;
        /*!< Threshold for the Z stack based step size lookup. Below this
             fraction of the maximum Brenner value, large steps are performed.
             Default value: 0.02 */
        double              mLowerBrennerThreshold;
        //!< Size of the large steps (below threshold) in microns
        double              mLargeCorrectionStep;
        /*!< Current image DC value, needed for image tranformations in
             CorrelationImage::assignAndTransform() */
        double              mCurrentDCValue;
        //!< The Z position/focus value stack used for auto focus correction
        ZStack              mZStack;
        //!< Used to store intermediate focus values during Z stack acquisition.
        QVector<double>     mBrennerTemp;
        /*!< Current exposure time of the camera. Needed to decide whether to
             use an image for the Z stack or not */
        double              mCameraExposureTime;
        /*!< High resolution timer used to determine timesteps of stage
             movements */
        HPClock             mClock;
        /*!< Timestamp when the stage was last moved */
        quint64             mStampStageMoved;
        //!< Curve fitter to extract function parameters from Z stack values
        CurveFitter*        mFitter;
        /*!< Indicates whether the Z stack has been acquired and the necessary
             focus tracking functions have been calculated. */
        bool                mZStackReady;
        //!< Enable/disable Z position/focus value logging
        bool                mLogging;
        //!< Log file for Z stack positions/focus values
        QFile               mLogFileZStack;
        //!< Output stream for Z stack logging
        QTextStream         mLogFileStreamZStack;
        QString             mStorageFilename;
        QTextStream         mLogFileStreamZStackValues;
        QFile               mLogFileZStackValues;

    };

} // namespace tracker

// Allow FocusValue to be used for signal/slot
Q_DECLARE_METATYPE(tracker::FocusValue)

#endif // __FOCUSTRACKER_H__
