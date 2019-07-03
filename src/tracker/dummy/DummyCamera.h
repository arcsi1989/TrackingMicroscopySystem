/*
 Copyright (c) 2009-2012, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef _DummyCamera_H__
#define _DummyCamera_H__

#include "TrackerPrereqs.h"

#include <QMap>

#include "Camera.h"
#include "Timing.h"

namespace tracker
{
    /** Virtual camera designed to allow program testing without connection to
        the actual device.
        This class also implements the virtual stage shift by extracting a
        different sub image of a very large image.
    @note
        Most of the functionality has to be configured in the code directly
        because this class is only useful for testing while developing anyway.
        That explains all the hard coded values.
    */
    class DummyCamera : public Camera
    {
        Q_OBJECT;

    public:
        /// Loads the base image and defines the \ref Camera::Mode "modes".
        DummyCamera();
        ~DummyCamera();

        QString getName() const
            { return "DummyCamera"; }

        QStringList getAvailableModes() const;
        bool hasMode(QString mode) const
            { return mModes.contains(mode); }

        QString getMode() const;
        void setMode(QString modeText, int exposureTime = -1, double gain = -1.0);

        QSize getImageSize(QString mode) const;

        int getExposureTime() const
            { return mExposureTime; }
        void setExposureTime(int value)
            { mExposureTime = value; }

        double getGain() const
            { return mGain; }
        void setGain(double value)
            { mGain = value; }

        QPair<int, int> getExposureTimeRange() const
            { return qMakePair(4, 600000000); }
        QPair<double, double> getGainRange() const
            { return qMakePair(1.0, 9.80); }

    private slots:
        /** Creates a new image using gaussian blurring, depending on the stage position.
            This function also adds some noise to the image.
        @param offset
            Current absolute X/Y position of the stage
        @param zOffset
            Current absolute Z position of the stage
        @param processTime

        */
        void makeImage(QPointF offset, double zOffset, quint64 processTime);

        /** Create a new image using the image stack. */
        void makeStackImage(QPointF offset, double zOffset, quint64 processTime);

    private:
        Q_DISABLE_COPY(DummyCamera);

        void run(Thread* thread);

        /// Periodic callback used for framerate updates
        void timerEvent(QTimerEvent*);

        /// Add noise to an image
        void addImageNoise(QImage&);

        HPClock              mClock;
        QImage               mBaseImage;    ///< Large image depicting the entire scene
        QVector<QImage>      mImageStack;   ///< Stack of images
        QVector<double>      mZPosStack;    ///< Stack of Z positions
        QVector<QRgb>        mColourTable;  ///< Grayscale images require this

        QMap<QString, QSize> mModes;        ///< Available camera modes
        int                  mExposureTime; ///< Current exposure time (has no effect on the image)
        double               mGain;         ///< Current gain (has no effect on the images)
        QString              mCurrentMode;  ///< Currently set camera mode
    };
}
#endif /* _DummyCamera_H__ */
