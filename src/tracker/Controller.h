/*
 Copyright (c) 2009-2012, Reto Grieder & Benjamin Beyeler
 Copyright (c) 2014, Tobias Klauser

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef _Controller_H__
#define _Controller_H__

#include "TrackerPrereqs.h"

#include <QFile>
#include <QFutureWatcher>
#include <QImage>
#include <QMap>
#include <QQueue>
#include <QSize>
#include <QVector>
#include <QTimer>

#include "Thread.h"
#include "Timing.h"

namespace tracker
{
    /** Command centre of the cell tracker.
        This class basically controls the whole image tracking and is driven
        by an external object (like MainWindow) while not running or by a
        Qt event loop in the running state. \n
        You should therefore send an object of this class into it's own thread
        prior to starting it with run().
    @par Caution
        While running, all communication must be done asynchronously with
        signals and slots to prevent multi threading issues. \n
        BUT: for performance reasons, the stage has to live in the very same
        thread. You MUST do this manually either before you call run() or simply
        by making the stage a child object of this object.
        See \ref signalslotspage "here" for a scheme about object communication
        and threads. \n
        \n
        For more information about the internals, the documentation of trackImage()
        is a good start. Details about the algorithms can be found in Correlator
        and CorrelationImage.
    @par Modes
        The Controller can be used in three different modes:
        - \ref Controller::Tracking  "Tracking"
        - \ref Controller::Measuring "Measuring"
        - \ref Controller::Timing    "Timing"
    @par Options
        This class has many settings that can greatly influence its behaviour.
        These options can mostly be configured with slots in order to easily
        connect it with a graphical user interface. \n
        Here is a list of supported options:
        - Options that are separate for each camera \ref Camera::setMode "mode"
         - \ref setFFTImageSize()       "FFT Image Size"
         - \ref setControllerGain()     "Controller Gain"
         - \ref setStageCommandDelay()  "Stage Command Delay"
         - \ref setCorrelatorDepth()    "Correlator Depth"
         - \ref setPredictorSize()      "Predictor Size"
         - \ref setPixelSize()          "Pixel Size"
        - General options
         - \ref setMaxTotalStageMove()            "Max Total Stage Move"
         - \ref setMinOffset()                    "Min Offset"
         - \ref setMaxProcessDelay                "Max Process Delay"
        - Options related to measuring or timing
         - \ref setTuningStep()                   "Tuning Step"
         - \ref setTunerTimeout()                 "Tuner timeout"
         - \ref setTunerTimeToWaitAfterStartup()  "Tuner Time To Wait After Startup"
         - \ref setTunerTimeToWaitForMoveEnd()    "Tuner Time To Wait For Move End"
         - \ref setTunerMinimumPixelsForMeasure() "Tuner Minimum Pixels For Measure"
         - \ref setTunerMeasureMovePercentage()   "Tuner Measure Move Percentage"
    */
    class Controller : public Runnable
    {
        Q_OBJECT;

        /** Collection of camera \ref Camera::Mode "mode" dependent options that
            influence the Controller.
            The actual options currently used change every time the camera mode
            is changed by the user. This collection also simplifies the
            serialisation/deserialisation process to the ini file.
        */
        struct OptionSet
        {
            /** See setFFTImageSize(QSize)
            @par Default value
                Camera resolution
            */
            QSize fftImageSize;

            /** See setControllerGain()
            @par Default value
                0.5
            */
            double controllerGain;

            /** See setStageCommandDelay()
            @par Default value
                \c 0.0
            */
            double stageCommandDelay;

            /** See setCorrelatorDepth()
            @par Default value
                \c 2
            */
            int correlatorDepth;

            /** See setPredictorSize()
            @par Default value
                \c 3
            */
            int predictorSize;

            /** See setPixelSize()
            @par Default value
                \c (1.0, 1.0)
            */
            QPointF pixelSize;
        };

    public:
        enum Mode
        {
            /// Normal mode, tracks an image by controlling the stage.
            Tracking,

            /** Measures the \ref setPixelSize() "size of a pixel" and then
                quits the loop.
            */
            Measuring,

            /** Moves the stage in one direction and back to simulate movement.
                This can be used to calibrate the \ref setStageCommandDelay()
                "stage command delay" by minimising the blur effects. \n
                When finishing, this mode summarises the average delays between
                movement command and detected offset on the image. This may be
                used to tune the \ref setPredictorSize() "size of the
                Smith Predictor". However experiments have shown that this is
                better be done manually by maximising the system stability and
                performance.
            */
            Timing,

            /// Acquire Z stack and fit curve parameters for focus autocorrection
            ZStack,
        };

    public:
        /** Basically just sets up all the options, calls readSettings().
        @param cameraModes
            List of possible camera \ref Camera::getAvailableModes() "modes"
            that can be used. The \c string should uniquely identify a mode
            and the \c size should not change during the course of the program.
        */
        Controller(QVector<QPair<QString, QSize> > cameraModes);

        //! Cleanup with a preceding call to writeSettings()
        ~Controller();

        /// See setMaxTotalStageMove()
        double getMaxTotalStageMove() const
            { return mMaxTotalStageMove; }
        /// See setTuningStep()
        double getTuningStep() const
            { return mTuningStep; }
        /// See setFFTImageSize(QSize)
        QSize getFFTImageSize() const
            { return mCurrentOptions->fftImageSize; }
        /// See setControllerGain()
        double getControllerGain() const
            { return mCurrentOptions->controllerGain; }
        /// See setStageCommandDelay()
        double getStageCommandDelay() const
            { return mCurrentOptions->stageCommandDelay; }
        /// See setCorrelatorDepth()
        int getCorrelatorDepth() const
            { return mCurrentOptions->correlatorDepth; }
        /// See setPredictorSize()
        int getPredictorSize() const
            { return mCurrentOptions->predictorSize; }
        /// See setPixelSize()
        QPointF getPixelSize() const
            { return mCurrentOptions->pixelSize; }

        /** Sets the delay in microseconds that the \ref Controller::Timing
            "Timing function" should wait before actually starting.
        */
        void setTunerTimeToWaitAfterStartup(int value);

        /** Sets the timeout in microseconds that the \ref Controller::Timing
            "Timing function" waits for a movement to be complete.
        */
        void setTunerTimeToWaitForMoveEnd(int value);

        /** Sets the maximum total processing time in microseconds for the
            \ref Controller::Measuring "Measuring function".
        */
        void setTunerTimeout(int value);

        /** Sets the minimum displacement that the \ref Controller::Timing
            "Timing function" considers as movement.
        */
        void setTunerMinimumPixelsForMeasure(int value);

        /** Sets the size of the \ref Controller::Measuring "measuring" move in
            percentage of the whole image.
        */
        void setTunerMeasureMovePercentage(double value);

        /** Sets the discarding threshold for how old an image may be.
            The camera might deliver images faster than the Controller can
            process. Therefore we measure the time elapsed between the image
            sent into the event loop after camera processing and the call to
            this function with the same image. If the elapsed time is too large,
            we simply discard the image.
        @param value
            Delay in microseconds
        */
        void setMaxProcessDelay(int value);

        /// Sets the current \ref Controller::Mode "mode" (ignored if running)
        void setMode(Mode mode)
            { mCurrentMode = mIsRunning ? mCurrentMode : mode; }

        /// Enable/disable adaptive auto focus
        void setAdaptiveAutoFocus(bool enable)
            { mAdaptiveAutoFocusEnabled = enable; }

        void setXYStageEnabledBlocking(bool enable)
            { mXYStageEnabledBlocking = enable; }

        void setZStageEnabledBlocking(bool enable)
            { mZStageEnabledBlocking = enable; }

        void setUseEstimatedWindowSize(bool enable)
            { mUseEstimatedWindowSize = enable; }

        void setUseNoiseLevelAtBrenner(bool enable)
            { mUseNoiseLevelAtBrenner = enable; }

        /// Enable/disable calculation of Brenner focus function
        void setBrennerEnabled(bool enable)
            { mBrennerEnabled = enable; }

        /// Set the offset (n parameter) of the Brenner focus function.
        void setBrennerOffset(int offset)
            { mBrennerOffset = offset; }

        /// Set the satisfaction threshold for the Z stack based autofocus,
        /// above which no correction will be done
        void setUpperBrennerThreshold(double tresh);

        /// Set the threshold for the Z stack based autofocus below which
        /// large steps are performed
        void setLowerBrennerThreshold(double tresh);

        /// Set the size of the large steps (below threshold) in microns.
        void setLargeCorrectionStep(double size);

        /// Enable/disable Z stack based focus correction
        void setZStackEnabled(bool enable)
            { mZFocusTrackingEnabled = enable; }

        /// Enable/disable Z stack acquisition
        void setZStackAcquisitionEnabled(bool enable)
            { mZStackAcquisitionEnabled = enable; }

        /// Enable/disable Z stack based focus correction
        void setXYTrackingEnabled(bool enable)
        { mXYTrackingEnabled = enable; }

        /// Indicating individual pressure change steps to initailize image acquistion
        void setPressureisChanged(bool change);

        /// Indicating individual pressure increasment
        void setPressureisIncreased()
        { mDirection = -1; }

        /// Indicating individual pressure decreasement
        void setPressureisDecreased()
        { mDirection =  1; }

        /// Indicating the last pressure change and waiting for focusing before taking high resolution images
        void setLastPressureChangePassed(bool enable,bool modemultishots);

        /// Set the size of the Z stack (in microns)
        void setZStackMaximumNumber(int MaxNum)
            {  mMaxNumberOfZstackafterLastPressureChange = MaxNum;
               mZstackCounter = 0;
            }

        /// Set the size of the Z stack (in microns)
        void setZStackSize(double stackSize)
            { mZStackSize = stackSize; }

        /// Set the step size for the Z stack (in microns)
        void setZStackStepSize(double stepSize)
            { mZStackStepSize = stepSize; }

        ///Allow Time lapse after stretching at the stretched state
        void setTimeLapse(bool set)
            {mTimeLapse = set;}

        /// Set proportional gain of the Z tracking controller.
        void setPropGainZ(double propGainZ)
            { mPropGainZ = propGainZ; }

        /** Stores the pointer to the \b stage and creates the Correlator
            according to the current options.
            Without this call, nothing is going to happen upon start.
        @note
            Only the first call has an effect.
        @param stage
            Pointer to the microscope stage. To avoid threading issues, it is
            best the make the stage a child object of this object.
        */
        void initialise(Stage* stage);

        /** Returns whether the Controller is ready, or more precisely whether
            there is a valid Correlator or not. \n
            This function only returns false before initialise() was called or
            when the Correlator is currently being updated (might take a few
            seconds).
        */
        bool valid() const
            { return mCorrelator != NULL; }

        /// Transforms image coordinates to stage coordinates by scaling (no inversion)
        QPointF stageCoordinates(QPointF imageCoordinates) const;

        /// Transforms stage coordinates to image coordinates by scaling (no inversion)
        QPointF imageCoordinates(QPointF stageCoordinates) const;

        /// Track a pressure change
        void trackPressure(double step);

        /// Flag for the SingleShotrequest - to take a picture when we are in focus
        bool IsInFocus(){
            return mIsInFocus;
        }

        /// Set storage folder for log files
        void setStorageFolder( QString path ) {
            mCorrelatorFileName = path;
        }

        // Helper function to receive pressure value for logging
        void Controller::writePressureInfoToLogFile(double pressureValue);

        QString                     mCorrelatorFileName;    ///< Path of the set folder to put the log files into this folder

    public slots:
        /** Main tracking function: supply an image and the Controller will do
            something base on the previous images supplied.

            Put simply, the Controller is a simplified P controller that directly
            controls the stage position (output) and receives the measured image
            offset as input. That means that, at least in our model, there is no
            transfer function from actuating variable to the output. \n
            \n
            The more complicated explanation includes various measures to
            counteract problems of the real world. \n
            Here is a list of such measures:
            - Smith Predictor: When issuing a command to the stage, we don't
              see this change directly in the next image because of a delay. The
              Smith Predictor simply takes this into account by assuming the
              requested stage movement was executed correctly.
            - \ref setMaxProcessDelay "Process Delay Monitoring"
            - \ref setStageCommandDelay() "Stage Command Timing"
            - \ref setMinOffset() "Control Continuous Drift"
            - \ref setMaxTotalStageMove() "Limit Total Stage Movement"
            - Limit Local %Stage Movement: Moving for more than a quarter of the
              entire image indicates the something has already gone wrong (too
              much blurring, etc.). Therefore we just limit the movement and
              give the controller a saturation.

        @param image
            Captured image from the camera (probably sent by Camera::imageProcessed())
        @param captureTime
            Absolute time stamp in microseconds at image reception
        @param processTime
            Absolute time stamp in microseconds for the time the image was sent
            into the Qt event queue (see \ref setMaxProcessDelay "Process Delay Monitoring")

        @par Detailed flow of events
            To explain visually how everything works, here is a flow chart for
            normal case where the Controller is \ref Controller::Tracking "Tracking".
            \n \n
            @image html Controller_Flowchart_html.png
            @image latex Controller_Flowchart_latex.png "Main flow chart of the Controller" width=15cm
        */
        void trackImage(const QImage image, quint64 captureTime, quint64 processTime);
       
        /** Moves the stage in the horizontal place (X/Y).
         @param distance
            Distance to move the stage in X and Y.
        */
        void moveStageXY(QPointF distance);

        /** Moves the stage vertically (Z direction).
        @param microns
            Distance to move in micrometers.
        @param block
            Whether to block until the stage finished moving (1) or not (0).
        */
        void moveStageZ(double microns, int block);

        /// Switch on the tracker: X,Y,Z
        void switchOnXYZ();

		/** Changes the set of Camera mode dependent \ref Controller::OptionSet
            "options" currently used.
        @param key
            String identifying one of the modes supplied in the
            \ref Controller() "constructor".
        */
        void setOptionsKey(QString key);

        /** Sets the total stage movement allowed in any direction, relative
            to the start point, during a tracker run. If exceeded,
            the tracker will stop immediately.
        @param value
            Maximum distance in micrometers
        */
        void setMaxTotalStageMove(double value);

        /** Sets a minimum pixel offset under which the displacement is simply
            ignored for the moment (but kept in mind).
            This is only necessary because it can happen that a one pixel offset
            in one direction creates another one pixel offset in the same
            direction, therefore slowly drifting the whole stage.
        @param value
            Offset in pixels (applied for x and y direction independently)
        */
        void setMinOffset(float value);

        /** Sets the step size in micrometers used for Controller::Timing and
            for the first step in Controller::Measuring.
        @note
            This value is completely hand tuned and changes, relatively
            speaking, whenever the microscope magnification changes.
        @param value
            Step size in micrometers
        */
        void setTuningStep(double value);

        /** Sets the size used for the FFT computations. This might differ from
            the camera resolution because FFTs can be done \b much faster if
            the size is a multiple of 2s and 3s. \n
            Note that the FFT image size is always smaller than the camera
            resolution. \n
        @par Example
            Camera horizontal resolution: \c 800 \n
            Optimized FFT size: <tt>2^8 * 3 = 768</tt>
        */
        void setFFTImageSize(QSize value);

        /// Horizontal only version of setFFTImageSize(QSize)
        void setFFTImageSizeX(int value);

        /// Vertical only version of setFFTImageSize(QSize)
        void setFFTImageSizeY(int value);

        /// Sets the gain of the P controller (keep below 1 for stable systems)
        void setControllerGain(double value);

        /** Sets the minimum delay between capturing the image and sending the
            stage command.
            Issuing the movement command at the right moment can place the
            actual movement between the exposure of two images. That is only
            possible if the camera is not limited by the exposure time but
            by the data transfer rate. \n
        @param value
            Delay in seconds
        */
        void setStageCommandDelay(double value);

        /** Controls how many images are compared when computing the offset.
            A correlator depth of 1 simply means that the current image is
            only compared to the last one.
        */
        void setCorrelatorDepth(int value);

        /** Set number of the delay frames for the Smith Predictor.
            On other words, this number tells you how many frames it takes
            for a stage movement to be visible on the image.
        */
        void setPredictorSize(int value);

        /** Sets the horizontal and vertical size of a single pixel.
            This is \ref Mode::Measuring "measured" by issuing a movement
            and then computing the offset seen in the image.
        @param value
            Micrometer values for width and height (in that order)
        */
        void setPixelSize(QPointF value);

        /// Set horizontal part of setPixelSize(QPointF)
        void setPixelSizeX(double value);

        /// Set vertical part of setPixelSize(QPointF)
        void setPixelSizeY(double value);

        /// Set the camera exposure time, see mCameraExposureTime
        void setExposureTime(double exposureTime);

        /// Set the camera image size, mCameraImageSize
        void setCameraImageSize(QSize imageSize);

        /// Set the storage filename
        void setStorageFilename(QString filename);

        /// Set Pressure protocol information
        void setCorrectionFactor(double value);
        void setTimestepDuration(double value);
        void setMaxAmplitude(double value);
        void setNumberOfPressureSteps(double value);
        void setXYTrackerDurationOscillation(quint64 interval);
        void setAutofocusModeAfterLastPressureChange(bool enable);

        /// Set the Brenner ROI percentage, see mBrennerRoiPercentage
        void setBrennerRoiPercentage(int precentage);

        void emitHighResPicture();
        void emitAutoPicture();
        void emitSaveImages();
        void emitTimeLapseUpdate();
        void enableXYTrackingSignal();
        void disableXYforDuration();
        void toggleXYOscillation();
        void startXYOscillation();
        void startZstackAcquisitionOscillation();
        void TimeLapseImaging();
        void QuickFocus();


    signals:
        /// Raised when the valid() status changes
        void validityChanged();

        /** Raised when the horizontal component of the
            \ref setPixelSize(QPointF) "pixel size" has changed
        */
        void pixelSizeXChanged(double);

        /** Raised when the vertical component of the
            \ref setPixelSize(QPointF) "pixel size" has changed
        */
        void pixelSizeYChanged(double);

        /// Raised when a new frame rate was computed (unit is frames per second)
        void frameRateUpdated(double);

        /// Optionally sends debug images (has to be used specifically in the code)
        void debugImage(QImage);
        void focusUpdated(const FocusValue &);
        void zmoved(double);
        void StageisMOVED();
        void takeAutoPicture();
        void SaveImages();
        void TimeLapseCounterUpdated(QString);
        void takeHighResolutionHighIntensityPicture();

        /** Raised when the Timelapse imaging is performed and waiting for the time delay
            \ref MainWindow::on_Channel_0_clicked - Lamp channel 0 selection
         */
        void LightOFF();
        /** Raised when the Timelapse imaging is performed and waiting for the time delay
            \ref MainWindow::Turn_ON -> based on stored mMainChannel, on_Channel_"mMainChannel"_clicked
         */
        void LightON();


        /** Raised when the user wants to store a single shot image.
        @param mode
            String identifier for the Camera mode
        @param exposureTime
            Requested exposure time (-1 for no change)
        @param gain
            Requested gain (-1.0 for no change)
        */
        void singleShotRequested(QString mode, int exposureTime, double gain); // ??? necessity????
        void OffLineZstackImage(QImage imae, quint64 capturetime, quint64 processtime); // For debugging the ZstackOffLine

    private slots:
        /** Called whenever the Correlator used internally was fully initialised.
            This function properly handles the results stored in
            Controller::mFutureCorrelatorWatchers.
        @see updateCorrelator()
        */
        void correlatorInitialised();

        /// Stops the event loop by raising the Runnable::quit() signal
        void stopIntern();

    private:
        Q_DISABLE_COPY(Controller);

        /// Characterises the Controller::Timing mode states
        enum TimingState
        {
            WaitingOnStartup,
            WaitingForMove,
            WaitingForMoveEnd,
            WaitingAfterMove,
        };

        /// Characterises the Controller::Measuring mode states
        enum MeasuringState
        {
            MakeFirstMove,
            WaitingForFirstMove,
            WaitingForSecondMove
        };

        /** Entry function of the thread that also starts the Qt event loop.
            It is called whenever the tracker starts in its own thread.
        @note
            The function confines itself to the first (virtual) CPU core to
            avoid possible issues with the Windows Performance Timer. \n
            Also, before returning, the function pushes the Controller object
            back to the application's main thread (UI thread)!
        @remarks
            Be advised that Controller::mStage (supplied in initialise()) has to
            live in the same thread or the start will fail.
        */
        void run(Thread* thread);

        /** Destroys the old Correlator and asynchronously creates a new one.
            The actual implementation is a little tricky here, the reason being
            the fact that initialising the Correlator might take up to 10s.
            So it is run in another thread (hence no UI blocking) by means of
            \c QtConcurrent::run() and we have to wait for it to finish. That's
            when this slot is called and we can assign the new Correlator
            (updating it always creates a new one) to Controller::mCorrelator.
        @note
            While an asynchronous Correlator initialisation is run,
            the user might change some options again, triggering yet a
            new initialisation. To avoid mixups in the finishing order, we
            restrict Qt to only use one single worker thread.
        @par Warning
            As stated above, only one worker thread is used. However this is a
            global Setting in \c QThreadPool. We assume here that no other part
            of the application uses thread pools! \n
            The setting is changed in the \ref Controller() "constructor".
        @see correlatorFinished()
        */
        void updateCorrelator();

        /** Focus Tracking using the Z stack.
            This makes use of the Z stack (which was acquired off-line) to
            estimate the distance from the current Z position to the Z position
            with optimal focus, based on the Brenner focus value.
        */
        void trackZ(const QImage image, quint64 captureTime);

        /** Track image in XY
        */
        void trackXY(const QImage image, quint64 captureTime, quint64 processTime);

        /** Reads all settings from the ini file.
            For all available settings see detailed documentation of the
            \ref Controller "class"
        */
        void readSettings();

        /** Writes all settings to the ini file.
            For all available settings see detailed documentation of the
            \ref Controller "class"
        */
        void writeSettings();

        /** Helper function for reading options in a Controller::OptionSet
        @param settingsKey
            String identifying a camera mode (and with it the OptionSet)
        @param maxSize
            Maximum allowed FFT image size for the mode
        */
        void readSettings(QString settingsKey, QSize maxSize);

        /// Helper function for writing options in Controller::OptionSet
        void writeSettings(QString settingsKey);

        /// Returns the Euclidean length of a 2D vector represented as QPointF
        static double vectorLength(QPointF vector);

        /** In terms of control theory, this is THE controller.
            It returns a desired stage movement by considering the controller
            gain and the values stored in the \ref mSmithPredictor
            "Smith Predictor".
        */
        QPointF transferFunction(QPointF imageOffset);

        /// Implements the Controller::Timing mode
        QPointF timingFunction(QPointF imageOffset);

        /// Implements the Controller::Measuring mode
        QPointF measuringFunction(QPointF imageOffset, QSize imageSize);

        /** Validates a given \ref setPixelSize() "pixel size" for plausibility.
            The following restrictions are applied:
            - Largest microscope magnification is 100x
            - Smallest microscope magnification is 2x
            - Binning from 1x1 to 8x8 can be applied
            - Camera resolution is known
            - Vertical and horizontal size should be about equal
        @param size
            Pixel size in micrometers
        @param tolerance
            \c x / y - 1 > tolerance returns false
        */
        bool checkPixelSize(QPointF size, double tolerance);

        /** Creates some nice output gathered while the
            \ref Controller::Tuning "Tuner" was running
        */
        void processTunerResults();

        /// Qt event callback for timers (used for frame rate updates)
        void timerEvent(QTimerEvent* event);

        /** Pointer to the stage object used to move the microscope stage.
            Note that this variable only gets assigned in initialise() and that
            \c mStage should always live in the same thread as the Controller
            object. Best solution is to make the stage a child object.
        */
        Stage*                      mStage;

        /*** Object ***/

        /// Pointer to a valid correlator (can be NULL though). @see updateCorrelator()
        Correlator*                 mCorrelator;

        /// Stores future Correlator pointers when initialising it (or them)
        QQueue<QFutureWatcher<Correlator*>*> mFutureCorrelatorWatchers;

        /// High precision clock used to compute the frame rate
        HPClock                     mClock;

        /// Map of all possible option sets, each associated with a string camera mode
        QMap<QString, OptionSet*>   mOptions;

        /// List of all possible keys to options sets (initialised in Controller())
        QVector<QString>            mOptionsKeys;


        /*** Variables ***/

        /// Tells whether initialise() was already called
        bool                        mInitialised;
        /// Marks the currently used options
        OptionSet*                  mCurrentOptions;
        /// Marks key to the currently used options
        QString                     mCurrentOptionsKey;
        /// Marks the current \ref Controller::Mode "mode"
        Mode                        mCurrentMode;
        /// Tells whether the tracker is running or not
        bool                        mIsRunning;
        /// Helps to easily compute the frame rate
        FrameCounter                mFrameCounter;
        /// Stores the intermediate values for the Smith Predictor. See trackImage()
        QList<QPointF>              mSmithPredictor;
        /// Stores the total stage movement during a single tracker run (gets reset when restarting)
        QPointF                     mTotalStageMove;

        /*** General options ***/
        double                      mMinOffset;             ///< See setMinOffset()
        double                      mMaxTotalStageMove;     ///< See setMaxTotalStageMove()
        int                         mMaxProcessDelay;       ///< See setMaxProcessDelay()

        /*** Tuner variables ***/
        TimingState                 mTimingState;
        MeasuringState              mMeasuringState;
        bool                        mTunerMoveDetected;
        int                         mTunerMoveDirection;
        QPointF                     mTunerPreviousPixelSize;
        quint64                     mTunerReferenceTime;
        QPointF                     mTunerReferenceOffset;
        QVector<double>             mTunerCurrentResult;
        QList<QVector<double> >     mTunerResults;


        /*** Tuner options ***/
        double                      mTuningStep;                   ///< See setTuningStep()
        int                         mTunerTimeToWaitAfterStartup;  ///< See setTunerTimeToWaitAfterStartup()
        int                         mTunerTimeToWaitForMoveEnd;    ///< See setTunerTimeToWaitForMoveEnd()
        int                         mTunerTimeout;                 ///< See setTunerTimeout()
        int                         mTunerMinimumPixelsForMeasure; ///< See setTunerMinimumPixelsForMeasure()
        double                      mTunerMeasureMovePercentage;   ///< See setTunerMeasureMovePercentage()

        /*** Tracker ***/
        FocusTracker*               mFocusTracker;          ///< Focus tracker
        bool                        mTrackZ;                ///< Enable/disable Z tracking
        double                      mPropGainZ;             ///< Gain of the proportional controller
        double                      mMaxFocus;              ///< Maximal focus value (setpoint of controller)
        int                         mDirection;             ///< Direction of the vertical movement (-1 or 1)
        double                      mDistance;              ///< Distance to move vertically (in microns)
        QVector<double>             mFocii;                 ///< Vector of focus values
        QVector<double>             mFocusError;            ///< Vector of focus errors
        double                      mFocusThreshold;        ///< Threshold for zMovements
        bool                        mMovedInLastCycle;      ///< Flag if we moved stage in z the last cycle
        bool                        mAdaptiveAutoFocusEnabled;  ///< Adapt current focussing step based on value from previous step
        bool                        mXYStageEnabledBlocking;
        bool                        mZStageEnabledBlocking;
        bool                        mUseEstimatedWindowSize;
        bool                        mUseNoiseLevelAtBrenner;
        quint64                     mEstimatedCompletionOfZStageMovement;
        quint64                     mXYTrackerDurationOscillation; /// frequency for toggle XY tracker after last pressure change

        QPair<quint64, double>      mLastPressureChange;    ///< Pressure change tracker
        bool                        mBrennerEnabled;        ///< Enable/disable brenner focus function.
        int                         mBrennerOffset;         ///< Offset (n parameter) of the Brenner focus function.
        bool                        mIsInFocus;             ///< IsInFocus flag for the Singleshot - image acqustion

        // XY Tracking
        bool                        mXYTrackingEnabled;     /// Enable/Disable XY Tracking
        bool                        mZFocusTrackingEnabled;  /// Enable/Disable Z Tracking
        bool                        mZStackAcquisitionEnabled;  /// Enable/Disable ZStackAcquisition
        bool                        mPressureChanged;           /// Is pressure change occured?
        bool                        mLastPressureChangePassed; /// Has last pressure change occured?
        bool                        mInAutofocusModeAfterLastPressureChange;
        bool                        XYCorrection;                /// Flag to indicate requested stagemove

        //Zstack aquisition for high resolution images
        int                         mMaxNumberOfZstackafterLastPressureChange; /// After last pressure change this is set to be the maximum number of aquired Zstack
        int                         mZstackCounter; /// This counter counts how many Zstack has been acquired already.
        bool                        mTimeLapse;     /// Allows timelapse imaging
        int                         mTimeLapseCounter; /// Counting how many images should be taken with constant 1 min per image. TODO....

        // Z stack
        bool                        mZStackEnabled;         ///< Enable/disable Z stack based tracking
        double                      mZStackStepSize;        ///< Distance between two images in the Z stack (in microns)
        double                      mZStackSize;            ///< Z stack size in microns (distance between lowest and highest image in stack)

        //!< Percentage of the image to take as region-of-interest for the Brenner focus
        int                         mBrennerRoiPercentage;
        //!< Log file for tracker focus values

        quint64                     mStampStageMoved;
        quint64                     mLastPressureChangePassedTimestamp;
        double                      mCameraExposureTime;
        QSize                       mCameraImageSize;
        QString                     mStorageFilename;
        double                      mCorrectionFactor;
        double                      mTimestampDuration;
        double                      mMaxAmplitude;
        double                      mNumberOfPressureSteps;
        QTimer*                     cameraTimer;
        QTimer*                     cameraTimerAuto;
        QTimer*                     trackerTimerXY;
        QTimer*                     trackerTimer;
     //   QTimer*                     trackerTimerBetween; ///< Timer responsible to initiate images during stretching between two intermediate stretch

        /*** Logging files for debugging the controllern ***/
        QFile                       mLogFileTracker;
        //!< Output stream for tracker logging Z
        QTextStream                 mLogFileStreamTracker;
        //!< Log file for capturing timing events
        QFile                       mLogFileTrackerTiming;
        QTextStream                 mLogFileStreamTrackerTiming;
        //!< Log file for capturing timing events of Z autofocus
        QFile                       mLogFileAutofocus;
        QTextStream                 mLogFileStreamAutofocus;
        //!< Log file to track pressure values
        QFile                       mLogFilePressure;
        QTextStream                 mLogFileStreamPressure;
        //!< Log file to track code duration, a dummy profiler
        QFile                       mLogFileDuration;
        QTextStream                 mLogFileStreamDuration;
        //!< Log file to track movement of XY stage
        QFile                       mLogFileXYStageMove;
        QTextStream                 mLogFileStreamXYStageMove;
        //!< Log file to track movement of Z stage
        QFile                       mLogFileZStageMove;
        QTextStream                 mLogFileStreamZStageMove;
        //!< Log file to store tracker logfiles
        QFile                       mLogFileParameters;
        QTextStream                 mLogFileStreamParameters;
        //!< Log file to store continuous values, zpos, BV
        QFile                       mLogFileContinuous;
        QTextStream                 mLogFileStreamContinuous;


    };
}

#endif /* _Controller_H__ */
