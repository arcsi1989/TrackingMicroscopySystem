/*
 Copyright (c) 2009-2012, Reto Grieder & Benjamin Beyeler
 Copyright (c) 2014, Tobias Klauser

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#include "CorrelationImage.h"

#include <cmath>
#include <ctime>
#include <cassert>
#include <algorithm>
#include <QPainter>

#include "Exception.h"
#include "Logger.h"
#include "TMath.h"
#include "Timing.h"
#include <iostream>
//#include "PathConfig.h" //Temporal 20190130

namespace tracker
{
    BaseImage::BaseImage(QSize size)
        : mSize(size)
        , mArea(size.width() * size.height())
        , mSpatialData(NULL)
        , mSpatialDataZ(NULL)
        , mWindow(NULL)
        , mBrennerRoiPercentage(30)
    {
        // Allocate memory for the spatial data
        mSpatialData = (float*)fftwf_malloc(sizeof(float) * mArea);
        mSpatialDataZ = (float*)fftwf_malloc(sizeof(float) * mArea);
        mWindow      = (float*)fftwf_malloc(sizeof(float) * mArea);

        // Compute spatial window function
        // We multiply this with each input image to reduce edge effects
        for (int row = 0; row < mSize.height(); ++row)
        {
            for (int col = 0; col < mSize.width(); ++col)
            {
                int   i = row * mSize.width() + col;
                float x = (col - mSize.width()  * 0.5f) / mSize.width();
                float y = (row - mSize.height() * 0.5f) / mSize.height();
                // Hamming window
                //mWindow[i]  = 0.54f + 0.46f * std::cos(2.0f * math::pi * x);
                //mWindow[i] *= 0.54f + 0.46f * std::cos(2.0f * math::pi * y);
                // Hann window (preferred over Hamming because it reaches 0 at the edges)
                mWindow[i]  = 0.5f * (1 +  std::cos(2.0f * math::pi * x));
                mWindow[i] *= 0.5f * (1 +  std::cos(2.0f * math::pi * y));
                // Blackman window, seems to have more unwanted edge effects than the Hamming window
                //mWindow[i]  = 0.42f + 0.5f * std::cos(2.0f * math::pi * x) + 0.08f * std::cos(4.0f * math::pi * x);
                //mWindow[i] *= 0.42f + 0.5f * std::cos(2.0f * math::pi * y) + 0.08f * std::cos(4.0f * math::pi * y);
            }
        }
    }

    BaseImage::~BaseImage()
    {        
        fftwf_free(mSpatialData);
        fftwf_free(mSpatialDataZ);
        fftwf_free(mWindow);
    }

    float BaseImage::assign(const QImage image, float dcValue)
    {
        quint64 start_time = mClock.getTime();

        // Just to be sure we don't enlarge the image...
        assert(mSize.width() <= image.width() && mSize.height() <= image.height());

        // Copy the 8 bit integer values to the float array for the DFT.
        // Place the extract (e.g. 384x256) in the middle of the image (e.g. 400x300) for best results
        // Also multiply each value with a window function (mWindow) to reduce edge effects
        // and (very important!) first subtract the DC component (average pixel intensity)
        // from each pixel value. Since this would require to inspect the DC Value beforehand,
        // we simply use the value from the last image (dcValue) for that purpose.
        int rowStart = (image.height() - mSize.height()) / 2;
        int rowEnd   = (image.height() - mSize.height()) / 2 + mSize.height();
        int columnStart = (image.width() - mSize.width()) / 2;
        int fastWidth = mSize.width() - mSize.width() % 4;
        float* target = mSpatialData;
        float* targetz = mSpatialDataZ;
        float* window = mWindow;
        int newDCValue = 0;
        for (int y = rowStart; y < rowEnd; ++y)
        {
            const uchar* sourceStart = image.scanLine(y) + columnStart;
            const uchar* source = sourceStart;

            // Copy 4 values at a time
            const uchar* sourceEnd = sourceStart + fastWidth;
            while (source < sourceEnd)
            {
                //Z
               // targetz[0] = ((float)source[0]);
               // targetz[1] = ((float)source[1]);
               // targetz[2] = ((float)source[2]);
               // targetz[3] = ((float)source[3]);
               // targetz += 4;
                //XY
                target[0] = ((float)source[0] - dcValue) * window[0];
                newDCValue += source[0];
                target[1] = ((float)source[1] - dcValue) * window[1];
                newDCValue += source[1];
                target[2] = ((float)source[2] - dcValue) * window[2];
                newDCValue += source[2];
                target[3] = ((float)source[3] - dcValue) * window[3];
                newDCValue += source[3];
                source += 4;
                target += 4;
                window += 4;
            }

            // Copy the rest (at most 3, probably none)
            sourceEnd = sourceStart + mSize.width();
            while (source < sourceEnd)
            {
                //z
                //targetz[0] = (float)source[0];
                //++targetz;
                //xy
                target[0] = ((float)source[0] - dcValue) * window[0];
                newDCValue += source[0];
                ++source;
                ++target;
                ++window;
            }
        }

        //std::cout<<"BaseImage::assign (without extractFocusBrenner): "<< (mClock.getTime()-start_time)/1000. << " ms "<<std::endl;

        // extract focus value based on Brenner function
        //extractFocusBrenner();

        return (float)newDCValue / mArea;
    }

    void BaseImage::assignZ(const QImage image){
        quint64 start_time = mClock.getTime();

        // Just to be sure we don't enlarge the image...
        assert(mSize.width() <= image.width() && mSize.height() <= image.height());

        // Copy the 8 bit integer values to the float array for the DFT.
        int rowStart = (image.height() - mSize.height()) / 2;
        int rowEnd   = (image.height() - mSize.height()) / 2 + mSize.height();
        int columnStart = (image.width() - mSize.width()) / 2;
        int fastWidth = mSize.width() - mSize.width() % 4;
        float* targetz = mSpatialDataZ;
        for (int y = rowStart; y < rowEnd; ++y)
        {
            const uchar* sourceStart = image.scanLine(y) + columnStart;
            const uchar* source = sourceStart;

            // Copy 4 values at a time
            const uchar* sourceEnd = sourceStart + fastWidth;
            while (source < sourceEnd)
            {
                //Z
                targetz[0] = ((float)source[0]);
                targetz[1] = ((float)source[1]);
                targetz[2] = ((float)source[2]);
                targetz[3] = ((float)source[3]);
                targetz += 4;
                source += 4;
            }

            // Copy the rest (at most 3, probably none)
            sourceEnd = sourceStart + mSize.width();
            while (source < sourceEnd)
            {
                //z
                targetz[0] = (float)source[0];
                ++targetz ;
                ++source;
            }
        }

        // extract focus value based on Brenner function
        /*double sum = 0.0;
        unsigned int n = 2;
        const int H = mSize.height(), W = mSize.width();
        const double factor = ((100 - mBrennerRoiPercentage) / 100.0 / 2.0);
        const int X_MIN = W * factor;
        const int X_MAX = (W - X_MIN) - n;
        const int Y_MIN = H * factor;
        const int Y_MAX = H - Y_MIN;*/

        //QImage image2= image;
       // QRgb value;
        //value = qRgb(122, 163, 39); // 0xff7aa327
       // image2.setColor(0, value);
        /*
        for (int y = Y_MIN; y < Y_MAX; ++y)
        {
            const uchar* sourceStart = image.scanLine(y) + X_MIN;
           // const uchar* sourceStart2 = image2.scanLine(y) + X_MIN;
            const uchar* source = sourceStart;
            //const uchar* source2 = sourceStart;
            const uchar* sourceEnd = sourceStart + X_MAX;
            while (source < sourceEnd)
            {
                //sum += (double)sqr((double)source[0] - (double)source[2]);
                //sum += (double)sqr((int)source[0] - (int)source[2]);
                int temp = (int)source[0] - (int)source[2];
                double temp2 = (double)temp;
                sum += sqr(temp2);
               // image2.setPixel(0,0,0);
                source +=1;
            }
        }
        for (int y = Y_MIN; y < Y_MAX-2; ++y)
        {
            const uchar* sourceStart = image.scanLine(y) + X_MIN;
            const uchar* sourceStart2 = image.scanLine(y+2) + X_MIN;
            const uchar* source = sourceStart;
            const uchar* source2 = sourceStart2;
            const uchar* sourceEnd = sourceStart + X_MAX;
            while (source < sourceEnd)
            {
                double vertical = (double)source[0] - (double)source2[0]; //% Vertical Brenner
                double horizontal = (double)source[0] - (double)source[2];  //% Horizontal Brenner
                sum += sqr(std::max(vertical, horizontal));
                source  +=1;
                source2 +=1;
            }
        }*/



/*
        //for (int y = Y_MIN; y < Y_MAX; y++)
        //{
         //   for (int x = X_MIN; x < X_MAX;x++)
          //  {
           //     sum += sqr((double)image.pixel(y,x) - (double)image.pixel(y,x+2));
           // }
        // }
        QString path = "C:\Users\Workstation\Desktop\Aron\MicroscopeDebugging\20190130\Test08\test.png";
        path = PathConfig::getDataPath().path() + "/" + path;
        QString format("png");
        bool a = image2.save(path,format.toAscii(), 100);
        const uchar* sourceStart = image.scanLine(Y_MIN) + X_MIN;
        std::cout<<"YMIN: "<<Y_MIN<<" X_MIN: "<<X_MIN<<" value: "<<(double)sourceStart[0]<<" save:" <<a<<std::endl;
        //std::cout<<"BrennerValuefromImagewithout_fftwf_malloc: "<<sum / ((X_MAX - X_MIN) * (Y_MAX - Y_MIN)<<std::endl;
*/
        extractFocusBrenner();
       // mFocus.brennerFocus = sum / ((X_MAX - X_MIN) * (Y_MAX - Y_MIN));
    }

    void BaseImage::extractFocusBrenner(unsigned int n)
    {
        quint64 start_time = mClock.getTime();

        double sum = 0.0;
        const int H = mSize.height(), W = mSize.width();
        const double factor = ((100 - mBrennerRoiPercentage) / 100.0 / 2.0);
        /*
        const int X_MIN = W * factor;
        const int X_MAX = (W - X_MIN) - n;
        const int Y_MIN = H * factor;
        const int Y_MAX = H - Y_MIN;
        //const int Y_MIN = H / 3, Y_MAX = H - (H / 3);
        //const int X_MIN = W / 3, X_MAX = (W - (W / 3)) - n;

        for (int y = Y_MIN; y < Y_MAX; ++y) {
            for (int x = X_MIN; x < X_MAX; ++x) {
                int i = y * mSize.width() + x;
                sum += sqr(mSpatialDataZ[i] - mSpatialDataZ[i + n]);
            }
        }*/

        //Optional upgrade for two directional detection.
            const int X_MIN = W * factor;
            const int X_MAX = (W - X_MIN) - n;
            const int Y_MIN = H * factor;
            const int Y_MAX = (H - Y_MIN) - n;

            for (int y = Y_MIN; y < Y_MAX; ++y) {
                for (int x = X_MIN; x < X_MAX; ++x) {
                    int i = y * W + x;
                    int j = (y + n) * W + x;
                    double vertical = mSpatialDataZ[i] - mSpatialDataZ[i + n]; //% Vertical Brenner
                    double horizontal =  mSpatialDataZ[j] - mSpatialDataZ[i];  //% Horizontal Brenner
                    sum += sqr(std::max(vertical, horizontal));
                }
            }



        // normalize with image size, so the focus values remain comparable
        // across different resolutions and thus simplify finding parameters
        // for the Gaussian fit in FocusTracker
        mFocus.brennerFocus = sum / ((X_MAX - X_MIN) * (Y_MAX - Y_MIN));

        //std::cout<<"BaseImage::extractFocusBrenner: "<< (mClock.getTime()-start_time)/1000. << " ms "<< " and n is " << n << std::endl;
        //std::cout<< "Xmin: "<< X_MIN << " Xmax: "<< X_MAX << " Ymin: "<< Y_MIN << " Ymax: "<< Y_MAX << " H: "<< H << " W: " << W <<std::endl;

    }

    CorrelationImage::CorrelationImage(QSize size)
        : BaseImage(size)
        , mFrequencySize(size.width() / 2 + 1, size.height())
        , mFrequencyArea(mFrequencySize.width() * mFrequencySize.height())
        , mSmallerSize(0)
        , mFilter(NULL)
        , mMagnitude(NULL)
        , mReducedMagnitude(NULL)
        , mFrequencyData(NULL)
        , mDFTPlan(NULL)
        , mInverseDFTPlan(NULL)
        , mOffset(0, 0)
    {
        quint64 start_time = mClock.getTime();
        
        mSmallerSize = std::min(mFrequencySize.width(), mFrequencySize.height());

        // Allocate memory for frequency data
        mFilter           =         (float*)fftwf_malloc(sizeof(float) * mFrequencyArea);
        mMagnitude        =         (float*)fftwf_malloc(sizeof(float) * mFrequencyArea);
        mReducedMagnitude =         (float*)fftwf_malloc(sizeof(float) * mSmallerSize);
        mFrequencyData    = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * mFrequencyArea);

        // Compute frequency window function (Gaussian low pass)
        /* Explanation:
         * Using a Gaussian filter does not produce ringing in either the
         * frequency nor the time domain. The disadvantage is the
         * not so sharply defined crossover frequency, but is no matter here.
         *
         * To make things fast, we use the frequency response of the Gaussian to
         * compute a window function that can simply be multiplied with.
         * The DTFT of a discrete Gaussian kernel (instead of a sampled continuous
         * kernel) is exp(t * (cos(theta) - 1)) where t defines the shape (larger
         * means narrower). This function is purely real.
         *
         * Note: We apply the filter AFTER the space domain window. That is wrong
         * strictly speaking, but shouldn't make much difference and it is
         * massively faster.
         */
        float a = 6.0f;
        float b = 100.0f;
        for (int yi = 0; yi < mFrequencySize.height(); ++yi)
        {
            // This magic maps the pixel coordinate to a meaningful value between
            // 0 and Pi but so that 0 marks the lowest frequency.
            // (Pi is in the middle then)
            float y = (1.0f - qAbs(2.0f * yi / mFrequencySize.height() - 1.0f)) * math::pi;
            for (int xi = 0; xi < mFrequencySize.width(); ++xi)
            {
                float x = (float)xi / mFrequencySize.width() * math::pi;

                float d = std::sqrt(x * x + y * y);
                float p = std::cos(d) - 1.0f;
                float coeff = std::exp(a * p) * (1 - std::exp(b * p));

                int i = yi * mFrequencySize.width() + xi;
                // Apply filter twice: we only filter the image AFTER the multiplication
                // in the frequency domain. That requires only one filter step, but
                // with a double filter.
                mFilter[i] = coeff * coeff;
            }
        }

        //std::cout<<"CorrelationImage::CorrelationImage (data allocation): "<< (mClock.getTime()-start_time)/1000. << " ms "<<std::endl;

        start_time = mClock.getTime();
        
        // Plan forward DFT
        mDFTPlan = fftwf_plan_dft_r2c_2d(mSize.height(), mSize.width(), mSpatialData, mFrequencyData, FFTW_MEASURE);
        // Plan reverse DFT
        mInverseDFTPlan = fftwf_plan_dft_c2r_2d(mSize.height(), mSize.width(), mFrequencyData, mSpatialData, FFTW_MEASURE);

        //std::cout<<"CorrelationImage::CorrelationImage (setup forward and reverse DFD plans): "<< (mClock.getTime()-start_time)/1000. << " ms "<<std::endl;
    }

    CorrelationImage::~CorrelationImage()
    {
        // Clean everything up
        fftwf_destroy_plan(mInverseDFTPlan);
        fftwf_destroy_plan(mDFTPlan);
        fftwf_free(mFrequencyData);
        fftwf_free(mFilter);
        fftwf_free(mMagnitude);
        fftwf_free(mReducedMagnitude);
    }

    float CorrelationImage::assignAndTransform(const QImage image, float dcValue)
    {
        quint64 start_time = mClock.getTime();
        dcValue = this->assign(image, dcValue);
        //std::cout<<"CorrelationImage::assignAndTransform (assign function): "<< (mClock.getTime()-start_time)/1000. << " ms "<<std::endl;

        start_time = mClock.getTime();
        // Transform image to frequency domain
        fftwf_execute(mDFTPlan);
        //std::cout<<"CorrelationImage::assignAndTransform (execute FFT function): "<< (mClock.getTime()-start_time)/1000. << " ms "<<std::endl;

        // extract focus value based on DFT
//        extractFocusDFT();

        // Return DC value to be used for the next image
        return dcValue;
    }

    void CorrelationImage::assignAndTransform(const CorrelationImage* image1, const CorrelationImage* image2)
    {
        quint64 start_time = mClock.getTime();
        // Complex multiply the two images
        const fftwf_complex* data0 = image1->getFrequencyData();
        const fftwf_complex* data1 = image2->getFrequencyData();
        fftwf_complex* target = mFrequencyData;
        fftwf_complex* targetEnd = target + mFrequencyArea;

        while (target < targetEnd)
        {
            // Alternative implementation called 'Phase Correlation'
            // Should in theory neutralize illumination changes because only
            // the phase is considered by first normalising the coefficients.
            // However, illumination might also change the phase and the
            // computation takes a lot longer.
            /*
            float s0 = data0[0][0] * data0[0][0] + data0[0][1] * data0[0][1];
            float s1 = data1[0][0] * data1[0][0] + data1[0][1] * data1[0][1];
            float s = 1.0f / std::sqrt(s0 * s1);
            target[0][0] = (data0[0][0] * data1[0][0] + data0[0][1] * data1[0][1]) * s;
            target[0][1] = (data0[0][1] * data1[0][0] - data0[0][0] * data1[0][1]) * s;
            */

            // Normal 'Cross Correlation' implementation
            target[0][0] = data0[0][0] * data1[0][0] + data0[0][1] * data1[0][1];
            target[0][1] = data0[0][1] * data1[0][0] - data0[0][0] * data1[0][1];

            ++target; ++data0; ++data1;
        }

        //std::cout<<"CorrelationImage::assignAndTransform2Images (complex multiply images): "<< (mClock.getTime()-start_time)/1000. << " ms "<<std::endl;

        start_time = mClock.getTime();
        // Band pass filter
        this->filterImage();
        //std::cout<<"CorrelationImage::assignAndTransform2Images (filter images): "<< (mClock.getTime()-start_time)/1000. << " ms "<<std::endl;

        start_time = mClock.getTime();
        // Inverse Fourier transform
        fftwf_execute(mInverseDFTPlan);
        //std::cout<<"CorrelationImage::assignAndTransform2Images (execute inverse FFT): "<< (mClock.getTime()-start_time)/1000. << " ms "<<std::endl;
    }

    void CorrelationImage::filterImage()
    {
        fftwf_complex* target = mFrequencyData;
        const float* source = mFilter;
        const float* sourceEnd = mFilter + mFrequencyArea / 4 * 4;
        // Use 4x loop unrolling for better performance
        while (source < sourceEnd)
        {
            target[0][0] *= source[0]; target[0][1] *= source[0];
            target[1][0] *= source[1]; target[1][1] *= source[1];
            target[2][0] *= source[2]; target[2][1] *= source[2];
            target[3][0] *= source[3]; target[3][1] *= source[3];
            target += 4; source += 4;
        }
        // Compute the rest (at most 3 values)
        sourceEnd = mFilter + mFrequencyArea;
        while (source < sourceEnd)
        {
            target[0][0] *= source[0]; target[0][1] *= source[0];
            target += 1; source += 1;
        }
    }

    QPoint CorrelationImage::getSpatialMaximum() const
    {
        float max = 0; // all values are positive
        int maxIndex = 0;
        float* data = mSpatialData;
        float* dataEnd = data + mArea;
        while (data < dataEnd)
        {
            if (*data > max)
            {
                maxIndex = data - mSpatialData;
                max = *data;
            }
            ++data;
        }

        return QPoint(maxIndex % mSize.width(), maxIndex / mSize.width());
    }

    QImage CorrelationImage::getSpatialImage()
    {
        /*
        float max = 0; // all values are positive
        float min = 1e36f;
        for (int i = 0; i < mArea; ++i)
        {
            if (mSpatialData[i] > max)
                max = mSpatialData[i];
            if (mSpatialData[i] < min)
                min = mSpatialData[i];
        }

        float scaler = 1.0f / (max - min);
        float scaler = 1.0f / (mSize.width() * mSize.height());
        for (int i = 0; i < length; ++i)
        {
            mSpatialData[i] = mSpatialData[i] * scaler + 128;
        }
        */
        return makeQImage(mSpatialData, mSize);
    }

    QImage CorrelationImage::getSpectrumMagnitudeImage()
    {
        //float* magnitude = new float[mFrequencyArea];
        // Calculate magnitude and ignore phase
        // Note: x and y dimensions are swapped to have a 4 byte aligned row dimension
        //for (int i = 0; i < mFrequencyArea; ++i)
        //{
        //    //int iOut = (i % mFrequencySize.width()) * mFrequencySize.height() + i / mFrequencySize.width();
        //  magnitude[i] = std::log(std::sqrt(mFrequencyData[i][0] * mFrequencyData[i][0] + mFrequencyData[i][1] * mFrequencyData[i][1]) + 1);
        //}

        QImage image = makeQImagegraph(this->mReducedMagnitude);
        //delete[] magnitude;
        return image;
    }

    QImage CorrelationImage::getSpectrumPhaseImage()
    {
        float* magnitude = new float[mFrequencyArea];
        // Calculate phase and ignore magnitude
        for (int i = 0; i < mFrequencyArea; ++i)
        {
            //int iOut = (i % mFrequencySize.width()) * mFrequencySize.height() + i / mFrequencySize.width();
            magnitude[i] = std::atan2(mFrequencyData[i][1], mFrequencyData[i][0]);
        }
        QImage image = makeQImage(magnitude, mFrequencySize);
        delete[] magnitude;
        return image;
    }

    void CorrelationImage::reduce()
    {
        // Calculate 2D magnitude and ignore phase
        // Note: x and y dimensions are swapped to have a 4 byte aligned row dimension
        for (int i = 0; i < mFrequencyArea; ++i)
        {
            //int iOut = (i % mFrequencySize.width()) * mFrequencySize.height() + i / mFrequencySize.width();
            mMagnitude[i] = std::log(std::sqrt(mFrequencyData[i][0] * mFrequencyData[i][0] + mFrequencyData[i][1] * mFrequencyData[i][1]) + 1);
        }

        // reduce data 1dim with square approximation Maximum of indices is sufficient
        for (int i = 0; i < mSmallerSize; ++i)
        {
            mReducedMagnitude[i] = 0.0;
        }

        for (int k = 0; k < mSmallerSize; ++k)
        {
            for (int j = 0; j < mSmallerSize; ++j)
            {
                int q = mSmallerSize * k + j;   // forward index
                int l = mSmallerSize * (mFrequencySize.height() - 1 - k) + j; // backward index
                int i = std::max(j, k);

                mReducedMagnitude[i] += mMagnitude[q] + mMagnitude[l];
            }
        }

        for (int n = 0; n < mSmallerSize; ++n)
        {
            mReducedMagnitude[n] /= (2 * n + 1); // normalize
        }
    }

    void CorrelationImage::extractFocusDFT()
    {
        float rampsum = 0;
        float logsum = 0;
        float logweightsum = 0;
        float gausssum = 0;
        float gaussweightsum = 0;
        const float gausspeakpos = 0;                 // mean (expectation) of normal distribution
        const float gausswidth = mSmallerSize / 4.0;  // standard deviation
        float tanhsum = 0;
        float sum = 0;
        float noisesum = 0;

        mFocus.maxValue = 0;
        for (int i = 0; i < mSmallerSize; ++i)
        {
            sum += mReducedMagnitude[i];
            tanhsum += std::tanh(i / 15.0) * mReducedMagnitude[i];

            if (mReducedMagnitude[i] > mFocus.maxValue)
                mFocus.maxValue = mReducedMagnitude[i];

            if (i > mSmallerSize / 5 * 4)
                noisesum += mReducedMagnitude[i]; // calculate noise level for graph legend
        }

        mFocus.noiseLevel       = noisesum / (mSmallerSize / 5);
        mFocus.tanhFocus        = (tanhsum - (mFocus.noiseLevel * mSmallerSize)) / (mFocus.maxValue * mSmallerSize);
        mFocus.integralFocus    = sum / (mFocus.maxValue * mSmallerSize);//(sum-(mNoiselevel*mSmallerSize))/(mMaxvalue*mSmallerSize);

        int k = 0;
        for (int i = 0; mReducedMagnitude[i] > mFocus.noiseLevel; ++i)
        {
            float x = i;
            // integrate normally
            rampsum += i * (mReducedMagnitude[i] - mFocus.noiseLevel);              // integrate with ramp weight
            if (i != 0)
            {
                logsum += std::log(x) * (mReducedMagnitude[i] - mFocus.noiseLevel); // integrate with logarithmic weight
                logweightsum += std::log(x);
            }

            float gaussian = gausswidth*std::exp(-.5 * (x - gausspeakpos) * (x - gausspeakpos) / (gausswidth * gausswidth));
            // integrate with gaussian weight
            gausssum += 1.0 / gaussian * (mReducedMagnitude[i] - .5 * mFocus.maxValue);
            gaussweightsum += 1.0 / gaussian;

            ++k;

            if (i > mSmallerSize - 2)     // Security measure to avoid endless loop
                break;
        }

        mFocus.rampFocus    = rampsum * 2 / (k * (k + 1)) / mFocus.maxValue;    // use little Gauss' Formula to calculate and
        mFocus.logFocus     = logsum / logweightsum / mFocus.maxValue;          // scale all measures with maximum value
        mFocus.gaussFocus   = gausssum / gaussweightsum / mFocus.maxValue;
    }

    /*static*/ QImage CorrelationImage::makeQImage(float* data, QSize size)
    {
        // Find largest value for normalisation
        float maxValue = 0.0f;
        float minValue = 1e36f;
        QPoint maxPoint;
        for (int y = 0; y < size.height(); ++y)
        {
            for (int x = 0; x < size.width(); ++x)
            {
                int i = y * size.width() + x;
                if (data[i] > maxValue)
                {
                    maxValue = data[i];
                    maxPoint = QPoint(x, y);
                }
                if (data[i] < minValue)
                    minValue = data[i];
            }
        }

        QImage image(size, QImage::Format_RGB32);

        // Copy normalised image (notice the conversion from float to uchar)
        float normaliser = 255.0f / (maxValue - minValue);
        for (int y = 0; y < size.height(); ++y)
        {
            for (int x = 0; x < size.width(); ++x)
            {
                uchar grey = (uchar)((data[y*size.width() + x] - minValue) * normaliser);
                reinterpret_cast<QRgb*>(image.scanLine(y))[x] = qRgb(grey, grey, grey);
            }
        }

        // Mark the highest value with a (large) red square
        QPainter painter(&image);
        QPen pen(Qt::red);
        pen.setWidth(2);
        painter.setPen(pen);
        painter.drawLine(maxPoint - QPoint(8, 0), maxPoint + QPoint(8, 0));
        painter.drawLine(maxPoint - QPoint(0, 8), maxPoint + QPoint(0, 8));

        // Create the snapshot and delete the temporary data
        return image;
    }

    QImage CorrelationImage::makeQImagegraph(float* data)
    {
        int length = this->mSmallerSize;
        // Find largest value for normalisation
        float maxValue = 0.0f;
        float minValue = 1e36f;
        int maxPoint;

        for (int x = 0; x < length; ++x)
        {
            if (data[x] > maxValue)
            {
                maxValue = data[x];
                maxPoint = x;
            }
            if (data[x] < minValue)
                minValue = data[x];
        }

        QImage image(QSize(length, length), QImage::Format_RGB32);

        // plot the "graph" normalized to maximum value
        float normaliser = length / maxValue ;
        int paint=0;
        for (int x = 0; x < length; ++x)
        {
            for (int y = 0; y < length; ++y)
            {
                if(y > (length-data[x]*normaliser))
                    paint= 220;
                else
                    paint =0;
                uchar grey = (uchar)(paint);

                reinterpret_cast<QRgb*>(image.scanLine(y))[x] = qRgb(grey, grey, grey);
            }
        }

        // Create the snapshot and delete the temporary data
        return image;
    }
}
