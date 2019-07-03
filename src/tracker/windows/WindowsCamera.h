/*
 Copyright (c) 2009-2011, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef _WindowsCamera_H__
#define _WindowsCamera_H__

#include "TrackerPrereqs.h"

#include <QtCore/private/qwineventnotifier_p.h>
#include <FxLibApi.h>
#include <FxError.h>

#include "Camera.h"
#include "Logger.h"

namespace tracker
{
    /** Implementation of an interface to a Baumer SDK compatible 1394 camera.
        Most Firewire cameras should be able to operate with this interface.
    @par Limitations
        - Designed for non colour images only. That can be changed rather
          easily though.
        - One single camera (the first one enumerated if there are multiple)
        - For the Leica DFC 360 FX camera, the second colour codings are ignored
          because there doesn't seem to be any noticeable difference.
    @par Settings
        \c Num_DMA_Buffers: Number of DMA buffers used for capturing (default: 8)
    */
    class WindowsCamera : public Camera
    {
        Q_OBJECT;

    public:
        /** Initialises the SDK interface and sets up the camera and its modes.
            Also calls readSettings().
        */
        WindowsCamera();
        /// Calls writeSettings() and releases the SDK resources.
        ~WindowsCamera();

        QString getName() const;

        QStringList getAvailableModes() const;
        bool hasMode(QString mode) const;

        QString getMode() const;
        void setMode(QString mode, int exposureTime = -1, double gain = -1.0);

        QSize getImageSize(QString mode) const;

        int getExposureTime() const;
        void setExposureTime(int value);

        double getGain() const;
        void setGain(double value);

        QPair<int, int> getExposureTimeRange() const
            { return mExposureTimeRange; }
        QPair<double, double> getGainRange() const
            { return mGainRange; }

    private slots:
        /** Grabs an image from the camera buffer and sends the converted QImage
            via imageProcessed().
            The function is called by WindowsCamera::mAcquisitionNotifier
            automatically when the Windows message arrives. This is being dealt
            with internally by a Qt class though. \n
            In case a single shot image was requested, this function calls
            Camera::finishSingleShot() instead of emitting imageProcessed()
            directly.
        @note
            Colour images are not yet supported.
        */
        void processImage(HANDLE eventHandle);

        /// Displays a camera message received via Windows messages (not useful).
        void showCameraMessage(HANDLE eventHandle);

    private:
        Q_DISABLE_COPY(WindowsCamera);

        struct Mode
        {
            int           format;
            tBoImgCode    code;
            QSize         size;
            QString       name;
            int           index;
        };

        /// Starts the capture and the event loop.
        void run(Thread* thread);

        /** Returns the vector index of a camera mode given as string.
        @param mode
            Camera mode identified by as string
        @return
            mModes index if found, -1 otherwise
        */
        int getModeIndex(QString mode) const;

        /// Periodic event used to update the framerate
        void timerEvent(QTimerEvent*);

        void readSettings();
        void writeSettings();

        /// Throws an exception with text \c message if result != TRUE
        static void assertSuccess(int result, QString message);
        /** Issues a warning with text \c message if result != TRUE.
        @return
            Returns true in case of failure (result != TRUE), false otherwise
        */
        static bool warnIfError  (int result, QString message, Logger::LogLevel logLevel = Logger::Normal);
        /// Returns a string with a error keyword that corresponds to \c id.
        static QString getFXError(int id);

        HANDLE                mAcquisitionEvent;
        QWinEventNotifier*    mAcquisitionNotifier;
        HANDLE                mMessageEvent;
        QWinEventNotifier*    mMessageNotifier;
        QVector<Mode>         mModes;
        QVector<QRgb>         mMonochromeColourTable;
        QPair<int, int>       mExposureTimeRange;
        QPair<double, double> mGainRange;
        QString               mCamName;
        int                   mCamID;
        int                   mNumDMABuffers;

        Mode*                 mCurrentMode;
        int                   mBufferSize;
        unsigned char*        mBuffer;
        bool                  mWaitForSingleShot;
    };
}
#endif /* _WindowsCamera_H__ */
