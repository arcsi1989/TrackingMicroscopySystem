/*
 Copyright (c) 2009-2012, Reto Grieder & Benjamin Beyeler

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#include "Correlator.h"

#include <cmath>

#include "PathConfig.h"
#include "Logger.h"
#include "CorrelationImage.h"
#include "Exception.h"

namespace tracker
{
    Correlator::Correlator(QSize computationSize, int trackDepth)
        : mCurrentImage(NULL)
        , mCurrentDCValue(128.0f)
        , mMinimumOffset(2.0f)
        , mTrackDepth(trackDepth)
        , mImageSize(computationSize)
    {
        // Track depth 0 makes no sense
        if (trackDepth == 0)
            TRACKER_EXCEPTION("A track depth of 0 makes absolutely no sense!");

        // Create the CorrelationImages that represent the float images
        mCurrentImage = new CorrelationImage(mImageSize);
        for (int i = 0; i < mTrackDepth; ++i)
            mPreviousImages.push_back(new CorrelationImage(mImageSize));

        // CorrelationImages for the backward DFT of the convolved images
        mConvolution = new CorrelationImage(mImageSize);

        this->reset();
    }

    Correlator::~Correlator()
    {
        delete mCurrentImage;
        for (int i = 0; i < mTrackDepth; ++i)
            delete mPreviousImages[i];
        delete mConvolution;
    }

    void Correlator::reset()
    {
        mImagesTracked = 0;
        for (int i = 0; i < mTrackDepth; ++i)
            mPreviousImages[i]->setOffset(QPointF(0.0, 0.0));
        mCurrentImage->setOffset(QPointF(0.0, 0.0));
    }

    QPointF Correlator::track(const QImage snapshot)
    {
        quint64 start_time = mClock.getTime();
        //std::cout<<"Correlator::track start "<<std::endl;

        // A little assert doesn't hurt. See the c'tor for the exception for the same condition
        assert(mImageSize.width() <= snapshot.width() || mImageSize.height() <= snapshot.height());

        // Remove oldest picture from list and reuse it for the new one
        // Only do this if the last image had significant changes
        if (!this->isReady() || mCurrentImage->getOffset().manhattanLength() > mMinimumOffset)
        {
            mPreviousImages.push_front(mCurrentImage);
            mCurrentImage = mPreviousImages.takeLast();
        }

        // Copy and convert image data and compute DFT
        mCurrentDCValue = mCurrentImage->assignAndTransform(snapshot, mCurrentDCValue);

        // First few images can not be compared to older images, skip
        // Also completely discard the very first image because mCurrentDCValue
        // was not yet configured.
        if (!this->isReady())
        {
            ++mImagesTracked;
            return QPointF(0.0, 0.0);
        }

        // Compare the current image with each of the previous ones and store the results
        QVector<QPoint> localOffsets;
        for (int i = 0; i < mPreviousImages.size(); ++i)
            localOffsets.push_back(this->computeCorrelationMaximum(i));

        // Calculate the first estimate based on the correlation with the last image
        QPointF currentOffset = mPreviousImages[0]->getOffset() + localOffsets[0];
        // Iterate through the other correlation and dispose of it in case it is
        // predicted that the local offset is larger than 13% of the image size
        // or if it is within 10% of the predicted offset
        QPointF temp(currentOffset);
        size_t count = 1;
        for (int i = 1; i < mPreviousImages.size(); ++i)
        {
            QPointF prediction = currentOffset - mPreviousImages[i]->getOffset();
            const float magicValue = 0.13f;
            if (qAbs(prediction.x()) < mImageSize.width() * magicValue && qAbs(prediction.y()) < mImageSize.height() * magicValue)
            {
                // Consider this value into the averaging
                ++count;
                temp += mPreviousImages[i]->getOffset() + localOffsets[i];
            }
            else
            {
                // Second chance: if the local offset is within 10% of its prediction, we consider it after all
                const float magicValue = 0.1f;
                if ((qAbs(prediction.x()) < 2 || qAbs((prediction.x() - localOffsets[i].x()) / prediction.x()) < magicValue)
                 && (qAbs(prediction.y()) < 2 || qAbs((prediction.y() - localOffsets[i].y()) / prediction.y()) < magicValue))
                {
                    ++count;
                    temp += mPreviousImages[i]->getOffset() + localOffsets[i];
                }
            }
        }
        // Average
        mCurrentImage->setOffset(temp / count);

        ++mImagesTracked;

        //std::cout<<"QPointF Correlator::track end "<< (mClock.getTime()-start_time)/1000. << " ms "<<std::endl;

        if (mCurrentImage->getOffset().manhattanLength() > mMinimumOffset)
            return mCurrentImage->getOffset();
        else
            return QPointF(0.0, 0.0);

    }

    void Correlator::AssignImageToMemory(const QImage snapshot){
        // A little assert doesn't hurt. See the c'tor for the exception for the same condition
        assert(mImageSize.width() <= snapshot.width() || mImageSize.height() <= snapshot.height());
        // try to set dc value to zero to prevent oscillations - Solved the Oscillation issue @Aron
        mCurrentDCValue = mCurrentImage->assign(snapshot, mCurrentDCValue);
    }

    void Correlator::computeBrennerValueForSnapshot(const QImage snapshot) {
        quint64 start_time = mClock.getTime();

        // A little assert doesn't hurt. See the c'tor for the exception for the same condition
        assert(mImageSize.width() <= snapshot.width() || mImageSize.height() <= snapshot.height());

        // try to set dc value to zero to prevent oscillations - Solved the Oscillation issue @Aron
        mCurrentImage->assignZ(snapshot);
        //mCurrentDCValue = 0.0;
        // default value for n from the literature
        //mCurrentImage->extractFocusBrenner(2);

        //std::cout<<"Correlator::computeBrennerValueForSnapshot : "<< (mClock.getTime()-start_time)/1000. << " ms "<<std::endl;
    }

    QPoint Correlator::computeCorrelationMaximum(int index)
    {
        // Compute cross correlation in the frequency domain by multiplying the complex values
        mConvolution->assignAndTransform(mCurrentImage, mPreviousImages[index]);

        // Calculate maximum
        QPoint offset = mConvolution->getSpatialMaximum();

        // Correlation function is periodic
        if (offset.x() > mImageSize.width() / 2)
            offset.rx() -= mImageSize.width();
        if (offset.y() > mImageSize.height() / 2)
            offset.ry() -= mImageSize.height();

        return offset;
    }

    QImage Correlator::getLastCorrelationImage()
    {
        return mConvolution->getSpatialImage();
    }

    QImage Correlator::getLastAdjustedDFTImage()
    {
        return mCurrentImage->getSpectrumMagnitudeImage();
    }

    FocusValue Correlator::getLastFocus()
    {
        // add a filtered focus for signal stability
        FocusValue focuses;
        const int registersize = 6;

        focuses = mCurrentImage->getFocus();
        mFocusRegister.prepend(focuses.gaussFocus); // filter gaussFocus or integralFocus
        mFocusRegisterBrenner.prepend(focuses.brennerFocus);    // TEMP

        double registersum = 0;
        for (int i = 0; i < mFocusRegister.size(); i++)
            registersum += mFocusRegister.at(i);

        focuses.avgFocus = registersum / mFocusRegister.size();

        registersum = 0.0;
        for (int i = 0; i < mFocusRegisterBrenner.size(); i++)
            registersum += mFocusRegisterBrenner.at(i);

        focuses.avgBrennerFocus = registersum / mFocusRegisterBrenner.size();

        if (mFocusRegister.size() >= registersize)
            mFocusRegister.remove(registersize-1);

        if (mFocusRegisterBrenner.size() >= registersize)
            mFocusRegisterBrenner.remove(registersize-1);

        return focuses;
    }

    void Correlator::setBrennerRoiPercentage(unsigned int roiPercentage)
    {
        mCurrentImage->setBrennerRoiPercentage(roiPercentage);
        for (int i = 0; i < mTrackDepth; ++i)
            mPreviousImages[i]->setBrennerRoiPercentage(roiPercentage);
    }
}
