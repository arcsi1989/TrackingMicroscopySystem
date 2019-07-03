/*
 Copyright (c) 2009-2012, Reto Grieder & Benjamin Beyeler
 Copyright (c) 2014, Tobias Klauser

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef _CorrelationImage_H__
#define _CorrelationImage_H__

#include "TrackerPrereqs.h"

#include <QImage>
#include <fftw3.h>

#include "FocusTracker.h"
#include "TMath.h"
#include "Timing.h"

namespace tracker
{
    /** Base class to store for image processing.

        To be used if no frequency domain processing is needed.
     */
    class BaseImage
    {
    public:
        BaseImage(QSize size);
        ~BaseImage();

        /** Assigns the image data to the internal spatial data field

        @param image
            Any QImage that represents a spatial image and exceeds both
            dimensions specified in CorrelationImage::CorrelationImage().
        @param dcValue
            The average pixel value of the image.
        @return
            The DC value of the image provided in the \c image parameter.
        */
        float assign(const QImage image, float dcValue);

        /** Assigns the image data to the internal spatial data without
            window filtering and extract the BrennerValue;

        @param image
            Any QImage that represents a spatial image and exceeds both
            dimensions specified in CorrelationImage::CorrelationImage().

        */
        void assignZ(const QImage image);

        /** Extract focus value using the Brenner function (Brenner et al, 1971).

            The Brenner function \f$f_{Brenner}\f$ is calculated as follows:

            \f$F_{Brenner} = \sum_{j} \sum_{i} (G(i, j) - G(i + n, j))^2\f$

            where \f$i+2 < image size\f$ and \f$n\f$ is the distance value
            (usually 2).

        @param n
            The distance between the pixels considered by the Brenner function.
            The default value suggested in the literature is 2.
        */
        void extractFocusBrenner(unsigned int n = 2);

        /** Return the focus values.

            Attention: In BaseImage::assign() only mFocus.brennerFocus is set,
            the other values will be 0.
         */
        FocusValue getFocus() const
            { return mFocus; }

        //! Returns a raw pointer to the spatial domain data (real image)
        float* getSpatialData()
            { return mSpatialData; }
        //! Const overload function of getSpatialData()
        const float* getSpatialData() const
            { return mSpatialData; }

        /*! Set a new region-of-interest percentage to consider for Brenner
            focus value */
        void setBrennerRoiPercentage(int roiPrecentage)
        {
            mBrennerRoiPercentage = clamp(roiPrecentage, 0, 100);
        }

    protected:
        QSize           mSize;          //!< Size of the spatial image
        int             mArea;          //!< Pixel area of the spatial image
        float*          mSpatialData;   //!< Pointer to the spatial image data
        float*          mSpatialDataZ;  //!< Pointer for the spatial image data without filtering
        float*          mWindow;        //!< Pointer to the spatial window function
        FocusValue      mFocus;         //!< Focus related values
        /*! Center percentage of the image to take as region-of-interest for the
            calculation of the Brenner focus value. */
        int             mBrennerRoiPercentage;
        HPClock              mClock;
    };

    /** Wrapper around the various image analysis algorithms used for tracking.

        The general method used to predict the offset of two subsequent images
        is called cross correlation. It basically just computes the correlation
        coefficient for every possible displacement and picks the largest one.
        There is a very fast implementation using the Discrete Fourier Transform
        to do the hard work in the frequency domain. The drawback are the edge
        effects which have to be taken care of. \n
        Documentation of the individual process steps are given in both
        overloads of assignAndTransform().
    @par DFT algorithm
        DFTs are usually implemented using the Fast Fourier Transform method
        that has much better run time complexity. To make things ever better,
        there is an open source library called FFTW (Fastest Fourier Transform
        in the West) that is in fact incredibly fast at its job. The drawback is
        that every transformation (including the memory location of the data)
        has to be 'planned' beforehand. We do this in the constructor.
    @par Filter
        There is also a band pass filter installed that effectively restricts
        the information to the dots (beads) on the image. But using filters
        usually produces 'ringing' artifacts in the spatial domain. The use of
        Gaussian filters eliminates that problem but makes the transitions of
        the band pass a little smoother.
    @par Class usage
        Even though it is just one class, it is used separately for the first
        and second part of the whole process. It might seem strange but it used
        to have advantages...
    @note
        This tracking algorithm uses both a band pass filter and a spatial
        window to reduce various effects. Technically the filter should be
        applied before the window, but this would require almost double the
        computation time. Hence we switch the order and compensate by
        subtracting the DC value from the original image
        (see assignAndTransform(QImage, float)) because the effects are
        otherwise negligible.
    */
    class CorrelationImage : public BaseImage
    {
    public:
        /** Allocates the internal data fields, computes the window functions
            and 'plans' the Fast Fourier Transformations.
        */
        CorrelationImage(QSize size);
        //! Cleans up all the data
        ~CorrelationImage();

        /** Assigns the image data to the internal spatial data field and
            computes the frequency data.

            This function calls BaseImage::assign() to perform the assignment
            of the image data to the internal spatial data field.

            This is a customised function that is used in the first step of the
            cross correlation (computing the Fourier Transformation).
        @par Performed steps
            -# At first the image is placed in the middle of the internal image
               so that it overlaps on all sides.
            -# Then the \c dcValue is subtracted from every pixel. That step is
               very important because we apply a spatial window afterwards.
               Normally \c dcValue would just be added to the correlation factor
               equally for every displacement and therefore not influence the
               result. However the applied spatial window function is different
               at every pixel location and would therefore scale the DC value
               unequally. \n
               Tests have shown that not accounting for the DC value prior to
               the DFT results in significantly lower performance. Took me a
               while to figure it out though ;)
            -# Afterwards the image is multiplied with a \c Hann \c Window to
               reduce edge effects. Bear in mind that the image should be
               imagined to infinitely repeat itself in every direction when
               computing the correlation coefficient. Displacing these infinite
               planes then inherently overlaps unrelated image content at the
               edges.
            -# Finally the DFT of the input image is computed.
        @param image
            See parameter documentation of BaseImage::assign()
        @param dcValue
            The average pixel value of the image. It can be slightly off so we
            can simply use the DC value from the recent most image. We easily
            get that \a after the Fourier Transformation, however we need it
            here \a prior to the transformation.
        @return
            See parameter documentation of BaseImage::assign()
        */
        float assignAndTransform(const QImage image, float dcValue);

        /** Computes the cross power spectrum of two frequency images and
            transforms the image back to the spatial domain.
            This is a customised function that is used in the second step of the
            cross correlation (computing the cross power spectrum and the
            inverse Fourier Transformation).
        @par Performed steps
            -# Both frequency images are filtered with a Gaussian band pass.
               That is a low pass multiplied with a high pass. The Fourier
               Transform of the discrete Gaussian kernel is used because it is
               more accurate (not really important) but mostly just easier to
               compute. Implementation see constructor source code.
            -# The cross spectrum between the two frequency responses is
               computed (basically just a complex multiplication of the two).
            -# The last step is the computation of the inverse DFT of the
               cross spectrum.
        @par Implementation
            Note that the multiplication with the filter can be done \c after
            the computation of the power spectrum because the latter is also
            just a multiplication of complex values (which is commutative). The
            filter then of course has to be applied twice. And that is
            equivalent to using one single filter that can be computed in advance.
        @param image1
            Image with frequency data that represents the DFT of the spatial data.
        @param image2
            Image with frequency data that represents the DFT of the spatial data.
        */
        void assignAndTransform(const CorrelationImage* image1, const CorrelationImage* image2);

        //! Stores the absolute offset of this image (used by Correlator)
        void setOffset(QPointF offset)
            { mOffset = offset; }
        //! Returns the absolute offset of this image (used by Correlator)
        QPointF getOffset() const
            { return mOffset; }

        //! Returns a raw pointer to the frequency domain data
        fftwf_complex* getFrequencyData()
            { return mFrequencyData; }
        //! Const overload function of getFrequencyData()
        const fftwf_complex* getFrequencyData() const
            { return mFrequencyData; }

        /** Returns the point in the spatial image with the highest value.
            Sub pixel accuracy is neither done nor required here.
        */
        QPoint getSpatialMaximum() const;

        //! Debug function: Returns the spatial image (the real image) as normal QImage
        QImage getSpatialImage();

        /** Debug function: Returns the magnitude of each comples frequency domain value.
        @note
            The values are computed in log scale (but not the scale itself).
        @return
            A QImage with half the size in x direction.
        */
        QImage getSpectrumMagnitudeImage();

        /** Debug function: Returns the phase of each complex frequency domain value.
        @note
            The values are computed in normal scale.
        @return
            A QImage with half the size in x direction.
        */
        QImage getSpectrumPhaseImage();

        QSize getSize() const
            { return mSize; }

    private:
        /** Apply band pass filter on the image in the frequency domain. */
        void filterImage();

        /** Extract focus value by integrating over reduced DFT data. */
        void extractFocusDFT();

        /** Reduces 2D DFT data to 1D with square approximation. */
        void reduce();

    protected:
        /** Normalises a float image and converts it to a QImage. Also, the highest
            value is marked with a red cross.
        */
        static QImage makeQImage(float* data, QSize size);

        /** Converts 1D data into a graph like image of sizw length^2,
            normalized to highest value.
        */
        QImage makeQImagegraph(float* data);

        int             mSmallerSize;       //!< The smaller of the two dimensions of the spatial image
        QSize           mFrequencySize;     //!< Size of the complex frequency domain image
        int             mFrequencyArea;     //!< Pixle area of the complex frequency domain image

        /** Pointer to the band pass filter window in the frequency domain.
        @note
            The data would be complex, but using a Gaussian band pass only yields real valued data.
        */
        float*          mFilter;            //!< Frequency domain band pass filter
        float*          mMagnitude;         //!< Magnitude of DFT
        float*          mReducedMagnitude;
        fftwf_complex*  mFrequencyData;     //!< Frequency domain image
        fftwf_plan      mDFTPlan;           //!< Execution plan for the DFT
        fftwf_plan      mInverseDFTPlan;    //!< Execution plan for the inverse DFT
        QPointF         mOffset;            //!< Stored absolute offset
        HPClock              mClock;
    };
}

#endif /* _CorrelationImage_H__ */
