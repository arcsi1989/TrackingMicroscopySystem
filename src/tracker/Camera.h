/*
 Copyright (c) 2009-2011, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef _Camera_H__
#define _Camera_H__

#include "TrackerPrereqs.h"

#include <QImage>
#include <QPair>
#include <QVector>

#include "Thread.h"
#include "Timing.h"

namespace tracker
{
    /** Abstract class defining the interface to a Camera.
        This includes setting and getting several values like exposure time as
        well as taking single shots in a different mode than normal streaming. \n
        You can use \ref signalslotspage "Signals and Slots" for multi
        threading, or just to make life easier. \n
    @par Modes
        Every camera has certain modes of operation (mostly related to resolution
        or colour). These are identified by their display string (what you
        actually see on the screen in the GUI).
    @par Camera parameters
        In this program, only two settings are taken into account:
         - Exposure time in microseconds
         - Internal gain (signal is simply multiplied by some function of the gain)
    @par Settings
         - \c Single_Shot_Min_Timeout: Minimum time to wait for a single shot
           image to be taken. Depending on the exposure time, this timeout
           can be significantly higher.
    */
    class Camera : public Runnable
    {
        Q_OBJECT

    public:
        //! Returns a new camera (implemented in Linux/Windows/Dummy source file)
        static Camera* makeCamera();

        //! Empty constructor (only assigns the clock) @param clock Reference to a time keeper
        Camera();

        //! Empty
        virtual ~Camera()
            { }

        //! Returns the name of the camera in an undefined form
        virtual QString getName() const = 0;
        //! Returns all possible camera modes as strings
        virtual QStringList getAvailableModes() const = 0;
        //! Checks whether a camera mode exists
        virtual bool hasMode(QString mode) const = 0;

        //! Returns the mode currently used for streaming
        virtual QString getMode() const = 0;

        //! Returns the image size of a specific camera mode
        virtual QSize getImageSize(QString mode) const = 0;

        //! Returns the currently applied exposure time in microseconds
        virtual int getExposureTime() const = 0;
        //! Returns the currently applied gain (unspecified unit)
        virtual double getGain() const = 0;

        //! Returns the range of the possible exposure time values (min, max)
        virtual QPair<int, int> getExposureTimeRange() const = 0;
        //! Returns the range of the possible gain values (min, max)
        virtual QPair<double, double> getGainRange() const = 0;

    public slots:
        /** Sets the streaming mode of the camera.
        @param modeText
            Camera mode string identifier
        @param exposureTime
            Desired exposure time (negative value for keeping the current value)
        @param gain
            Desired camera gain (negative value for keeping the current value)
        */
        virtual void setMode(QString modeText, int exposureTime = -1, double gain = -1.0) = 0;

        //! Sets the currently applied exposure time in microseconds
        virtual void setExposureTime(int value) = 0;
        //! Sets the currently applied internal camera gain
        virtual void setGain(double value) = 0;

        /** Initiates the single shot procedure which simply captures one image
            with a different mode.
            Use this to get a high quality image while doing an experiment.
        @param modeText
            Camera mode string identifier
        @param exposureTime
            Desired exposure time (negative value for keeping the current value)
        @param gain
            Desired camera gain (negative value for keeping the current value)
        */
        //void takeSingleShot(QString modeText, int exposureTime = -1, double gain = -1.0);
        //void takeMultipleShot(QString modeText, int exposureTime = -1, double gain = -1.0);
        void Record(QString modeText,int ch1,double i1, int ch2, double i2, int exposureTime = -1, double gain = -1.0);

        /// Set the the number of planes requested for multiple imageging
        void setNumOfPlanes(int NumOfPlanes);
        //! Called upon setting the stepsize of the Zstack
        void setZstepsize(double Zstackstepsize);
        //! Called upon allowing multichannelimaging
        void setMultichannel(bool mIsMultiChannel);
        //! Called upon allowing multichannelimaging
        void setOffsetValue(double value);


        /// Set the the number of planes requested for multiple imageging
        void Feedbacktotracker(bool mSendFeedBack);


    signals:
        //! Called upon having captured a streaming image
        void imageProcessed(const QImage, quint64, quint64);
        //! Called upon having captured a single shot image
        void singleShotTaken(const QImage, quint64, quint64);
        //! Called upon having captured a single shot image in the requested multishot and initiate a Zmove
        void ZSTEP(double, int);        
        //! Called upon changing illumination settings
        void SettingFromCameraTheLamp(int,double);
        //! Called upon having captured all of the images from the the requested Zstack        
        void finishedZstack();
        //! Called upon having captured all of the images from the the requested Zstack in order to reset stage for starting position
        void SetStage();
        //! Called upon update of the camera frame rate
        void frameRateUpdated(double);
        //! Called upong receiving an image as a trigger signal
        void AllowLightForTheNextFrame();

    protected:
        /** Reads the settings values from the ini file.
        @remarks
            You must call this function manually from derived classes!
        */
        virtual void readSettings();

        /** Writes the settings values to the ini file.
        @remarks
            You must call this function manually from derived classes!
        */
        virtual void writeSettings();

        //! Common base function to be called upon recieving an image
        void Trigger();

        //! Common base function to be called upon recording images
        void DecisionTree();

        //! Common base function to be called upon finishing a single shot image
        void finishSingleShot(QImage image, quint64 captureTime, quint64 processTime);

        //! Finishing one recording period with camera mode and illumination back setting
        void SetBackTo();

        //! Returns true if waiting for a single shot image
        bool singleShotInProgress()
            { return mMakeSingleShot; }

        //! Returns the number of requistedimages
        int RequestedNumberOfImages()
            { return mNumOfRequestedIm; }
        //! Increasing the number of recorded images
        void SingleStepIncrementer()
            { mImageCounter++; }
        //! Checking for the lastImage
        bool IsThisTheLastOne();

        //! Simply returns whether the single shot timeout has already been passed
        bool singleShotTimedOut(quint64 referenceTime)
            { return referenceTime > mSingleShotEndTime; }

        HPClock              mClock;                      //!< High precision clock used for frame time stamps
        FrameCounter         mFrameCounter;               //!< Frame count used to compute the average framerate
        bool                 mIsRunning;                  //!< Tells whether the streaming is running

    private:
        Q_DISABLE_COPY(Camera)

        bool                 mMakeSingleShot;           //!< Currently in progress of making a single shot image?
        QString              mSavedMode;                //!< Temporary value used while taking a single shot image
        int                  mSavedExposureTime;        //!< Temporary value used while taking a single shot image
        double               mSavedGain;                //!< Temporary value used while taking a single shot image
        int                  mSavedChannel1;           //!< Temporary value used while recording
        double               mSavedIntensity1;         //!< Temporary value used while recording
        int                  mSavedChannel2;           //!< Temporary value used while recording
        double               mSavedIntensity2;         //!< Temporary value used while recording
        quint64              mSingleShotEndTime;        //!< Time stamp at which the single shot timeout is passed

        int                  mSingleShotMinTimeout;     //!< Camera setting. See readSettings()
        bool                 mMultipleShotInprogress;   //!< MutipleImageacquisition in progress or not?
        int                  mImageCounter;             //!< NumberOfPassedImages
        int                  mNumOfRequestedIm;         //!< NumberOfRequestedImages
        double               mStepSizeofZstack;         //!< In um the requested Stepsize of the Zstack acqsuition
        bool                 mSendFeedBack;             //!< Flag to block or initiate communication with Controller via finishedZstack signal/slot
        bool                 mIsMultiChannel;           //!< Flag to allow multichannel imaging and control from ::Camera
        double               mMultiChannelOffset;           //!< Flag to allow multichannel imaging and control from ::Camera
    };
}
#endif /* _Camera_H__ */
