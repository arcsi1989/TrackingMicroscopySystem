/*
 Copyright (c) 2009-2012, Reto Grieder & Benjamin Beyeler

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef _Correlator_H__
#define _Correlator_H__

#include "TrackerPrereqs.h"

#include <QImage>
#include <QList>
#include <QVector>

#include "Timing.h"

namespace tracker
{
    /** The image tracking unit calculates the offset of displaced images that
        are fed in a continuous stream. The first image defines the origin. \n
        In order to compensate for possible motion blur, you can specify that
        the current image is compared not only to the last image, but to the
        last N images. N can be specified in the constructor via \c trackDepth.
        \n\n
        For detailed information about the image analysis algorithms used, see
        CorrelationImage.
    @note
        The correlation simply sums up the individual offsets of succeeding
        images. That inevitably results in a summation error. But empirical
        tests have shown that the problem is negligible.
    */
    class Correlator
    {
    public:
        //! Initializes all the required CorrelationImages.
        Correlator(QSize computationSize, int trackDepth);

        ~Correlator();

        //! Resets the origin back to zero and forgets about past images.
        void reset();

        /** This contains the high level tracking algorithm.
            For every call it computes the offset to the origin defined by the
            first image. \n
            There are measures in place that prevent taking into account images
            with offsets that are clearly wrong when looking at the offsets of
            preceding/succeeding images.
        @note
            The first few images (about as many as \c trackDepth) do not yet
            yield any useful result (just 0|0).
        @param snapshot
            The current image for which the displacement should be computed.
        */
        QPointF track(const QImage snapshot);

        /** Sets a minimum offset under which an image is simply ignored
            and doesn't affect any succeeding images.
        */
        void setMinimumOffset(float value)
            { mMinimumOffset = value; }
        //! Returns the value described in setMinimumOffset().
        float getMinimumOffset() const
            { return mMinimumOffset; }

        /** Returns whether enough images have been submitted to fill the queue
            and therefore start with the tracking. See note in track().
        */
        bool isReady() const
            { return mImagesTracked > mTrackDepth + 1; }

        //! Debug image: returns the last inverse Fourier transform of the cross spectrum.
        QImage getLastCorrelationImage();
        //! Debug image: returns the last spectrum magnitude image of the input image.
        QImage getLastAdjustedDFTImage();
        //! extracts the last focus values
        FocusValue getLastFocus();
        //! Set the Brenner focus region percentage
        void setBrennerRoiPercentage(unsigned int roiPercentage);
        //! Assign the Image to memory array for later BrennerValueExtraction + DFT
        void AssignImageToMemory(const QImage snapshot);
        //! Compute the Brenner focus value for a the allocated tracked Image
        void computeBrennerValueForSnapshot(const QImage snapshot);

    private:
        //! Computes the offset of the current image with one from the queue with index \c index.
        QPoint computeCorrelationMaximum(int index);

        QList<CorrelationImage*>    mPreviousImages;    //!< Array of last N images delivered to the algorithm
        CorrelationImage*           mCurrentImage;      //!< Image currently being processed
        float                       mCurrentDCValue;    //!< DC value (average pixel intensity) of the last image
        CorrelationImage*           mConvolution;       //!< Temporary image used for the offset calculation
        float                       mMinimumOffset;     //!< See setMinimumOffset()
        const int                   mTrackDepth;        //!< Specifies how many images are to be compared
        QSize                       mImageSize;         //!< Easy access to image dimensions
        int                         mImagesTracked;     //!< Number of images already tracked (can be reset)
        QVector<double>             mFocusRegister;     //!< Register of saved focus values, used for filtering of focus signal

        // TEMP
        QVector<double>             mFocusRegisterBrenner;
        HPClock                     mClock;
    };
}

#endif /* _Correlator_H__ */
