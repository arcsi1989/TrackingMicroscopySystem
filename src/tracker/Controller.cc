/*
 Copyright (c) 2009-2012, Reto Grieder & Benjamin Beyeler
 Copyright (c) 2014, Tobias Klauser

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#include "Controller.h"

#include <cmath>
#include <QApplication>
#include <QCursor>
#include <QSettings>
#include <QThreadPool>
#include <QtConcurrentRun>

#include <QDate>
#include <QTime>
#include <QDir>
#include <QFileInfo>

#include "Logger.h"
#include "Exception.h"
#include "Stage.h"
#include "Correlator.h"
#include "FocusTracker.h"
#include "PathConfig.h"
#include "TMath.h"
#include "TrackerAssert.h"


#include <iostream>

namespace tracker
{
    Controller::Controller(QVector<QPair<QString, QSize> > cameraModes)
        : mStage(NULL)
        , mFocusTracker(NULL)
        , mCorrelator(NULL)
        , mInitialised(false)
        , mCurrentOptions(NULL)
        , mCurrentMode(Tracking)
        , mIsRunning(false)
        , mFrameCounter(2.0)
        , mTotalStageMove(0.0, 0.0)
        , mTuningStep(0.0)
        , mAdaptiveAutoFocusEnabled(false)
        , mUseEstimatedWindowSize(false)
        , mUseNoiseLevelAtBrenner(true)
        , mXYStageEnabledBlocking(false)
        , mZStageEnabledBlocking(true)
        , mPressureChanged(false)
        , mLastPressureChangePassed(false)
        , mZStackEnabled(true)
        , mBrennerEnabled(false)
        , mBrennerOffset(2)
        , mZStackStepSize(0.1)
        , mZStackSize(100.0)
        , mIsInFocus(false)
        , mCorrectionFactor(0)
        , mTimestampDuration(1600)
        , mMaxAmplitude(60)
        , mInAutofocusModeAfterLastPressureChange(false)
        , mNumberOfPressureSteps(10)
        , mXYTrackerDurationOscillation(500)
        , mMaxNumberOfZstackafterLastPressureChange(1)
        , mZstackCounter(0)
        , mTimeLapse(false)
        , mTimeLapseCounter(0)
    {
        // Prepare asynchronous Correlator initialisation:
        // Always only run with one thread because the FFTW planners are not thread safe
        // Assumption: No other part of the programs uses QThreadPool!
        QThreadPool::globalInstance()->setMaxThreadCount(1);

        // Workaround: reading settings requires mCurrentOptions
        mCurrentOptions = new OptionSet();
        // Read settings from file
        this->readSettings();
        delete mCurrentOptions;

        if (cameraModes.size() == 0)
            TRACKER_EXCEPTION("Controller: No option keys specified");

        typedef QPair<QString, QSize> T;
        foreach (T mode, cameraModes)
        {
            // Create and select entry
            mOptions[mode.first] = new OptionSet();
            mCurrentOptions = mOptions[mode.first];
            mOptionsKeys.append(mode.first);
            this->readSettings(mode.first, mode.second);
        }
        mCurrentOptionsKey = mOptionsKeys[0];
        mCurrentOptions = mOptions[mCurrentOptionsKey];

        // Z tracker
        mDirection = 1;             // 1 or -1
        mDistance = 0.1;            // initial distance in microns
        mFocusError.prepend(1000);  // initialize error
        mFocusError.prepend(1000);
        mFocii.prepend(0);
        mFocusThreshold = 0.01;     // threshold to activate focus correction (default is 0.005)
        mPropGainZ = 0.7;             // proportional gain for focus controller

        trackerTimer = new QTimer(this);
        connect(trackerTimer, SIGNAL(timeout()), this, SLOT(toggleXYOscillation()) );

        // single shot timer after 2*timeduration
        trackerTimerXY = new QTimer(this);

        // single shot timer after assumed
      //  trackerTimerBetween = new QTimer(this);

    }

    Controller::~Controller()
    {
        // Store settings to file
        mOptions[mCurrentOptionsKey] = mCurrentOptions;
        this->writeSettings();
        foreach (QString optionsKey, mOptionsKeys)
            this->writeSettings(optionsKey);
        foreach (OptionSet* options, mOptions)
            delete options;

        delete mCorrelator;
        delete mFocusTracker;
    }

    void Controller::run(Thread* thread)
    {
        mIsRunning = true;
        bool ready = true;

        if (mStage->thread() != this->thread())
        {
            TRACKER_WARNING("Tracking failed: Controller and Stage must live in the same thread!");
            ready = false;
        }

        if (!mCorrelator)
        {
            TRACKER_WARNING("Tracking failed: No Correlator created!");
            ready = false;
        }

        /*if (mZFocusTrackingEnabled && (!mFocusTracker || !mFocusTracker->isReady()))
        {
            TRACKER_WARNING("Tracking failed: Z stack not acquired yet."+mFocusTracker->isReady());
            ready = false;
        }*/

        if (ready)
        {
            // Set initial direction for Z tracking
            mDirection = -1;

            // Reset controller variables
            mCorrelator->reset();
            mTotalStageMove = QPointF(0.0, 0.0);
            for (QList<QPointF>::iterator it = mSmithPredictor.begin();
                 it != mSmithPredictor.end(); ++it)
                 *it = QPointF(0.0, 0.0);

            // Initialise tuner
            mTimingState          = WaitingOnStartup;
            mMeasuringState       = MakeFirstMove;
            mTunerMoveDirection   = 1;
            mTunerMoveDetected    = false;
            mTunerReferenceTime   = mClock.getTime();
            mTunerReferenceOffset = QPointF(0.0, 0.0);
            mTunerCurrentResult.clear();
            mTunerResults.clear();

            // Confine this thread to a single CPU core to avoid problems with the
            // high performance timer on Windows.
            setThreadAffinity(0);

            // Start timer to periodically update the frame rate
            int timerID = this->startTimer(500);
            mFrameCounter.reset();

            switch (mCurrentMode)
            {
            case Tracking:
            {
                // Making a new folder for the actual tracking experiment
                QDir dir = QDir::root();
                QString storageFolder = "";

                // retrieve the path portion of the filename only
                QFileInfo info1(mStorageFilename);
                storageFolder = info1.absolutePath();
                storageFolder += "\\";
                storageFolder += QDate::currentDate().toString("yyyyMMdd");
                storageFolder += QTime::currentTime().toString("hhmmss");
                storageFolder += "\\";

                dir.mkdir(storageFolder);

                // Open log file for Z positions/focus values
                QString fileName = storageFolder + "tracker_zpos_focus_brenner.csv";
                mLogFileTracker.setFileName(fileName);
                mLogFileTracker.open(QIODevice::WriteOnly | QIODevice::Text);
                if (!mLogFileTracker.isOpen())
                    TRACKER_WARNING("Could not open log file");
                mLogFileStreamTracker.setDevice(&mLogFileTracker);

                mLogFileStreamTracker << "mCLock, zPos, brennerFocus, mFociiAt1, noiseLevel\n";
                mLogFileStreamTracker.flush();

                // Open log file for collecting timing information of the tracking process
                QString fileNameTiming = storageFolder + "tracker_timing.csv";
                mLogFileTrackerTiming.setFileName(fileNameTiming);
                mLogFileTrackerTiming.open(QIODevice::WriteOnly | QIODevice::Text);
                if (!mLogFileTrackerTiming.isOpen())
                    TRACKER_WARNING("Could not open log file");
                mLogFileStreamTrackerTiming.setDevice(&mLogFileTrackerTiming);

                // Open log file for collecting timing information of the tracking process
                QString fileNameAutofocus = storageFolder + "tracker_autofocus.csv";
                mLogFileAutofocus.setFileName(fileNameAutofocus);
                mLogFileAutofocus.open(QIODevice::WriteOnly | QIODevice::Text);
                if (!mLogFileAutofocus.isOpen())
                    TRACKER_WARNING("Could not open log file");
                mLogFileStreamAutofocus.setDevice(&mLogFileAutofocus);
                mLogFileStreamAutofocus << "mCLock, zPos, brennerFocus, mFociiAt0, mFociiAt1, mDirmDist, captureTime, estimatedZStageMovementDuration \n";
                mLogFileStreamAutofocus.flush();

                fileNameAutofocus = storageFolder + "tracker_zstage_movement.csv";
                mLogFileZStageMove.setFileName(fileNameAutofocus);
                mLogFileZStageMove.open(QIODevice::WriteOnly | QIODevice::Text);
                if (!mLogFileZStageMove.isOpen())
                    TRACKER_WARNING("Could not open log file");
                mLogFileStreamZStageMove.setDevice(&mLogFileZStageMove);
                mLogFileStreamZStageMove << "mCLock, duration, distance, zpos, block\n";
                mLogFileStreamZStageMove.flush();

                fileNameAutofocus = storageFolder + "tracker_xystage_movement.csv";
                mLogFileXYStageMove.setFileName(fileNameAutofocus);
                mLogFileXYStageMove.open(QIODevice::WriteOnly | QIODevice::Text);
                if (!mLogFileXYStageMove.isOpen())
                    TRACKER_WARNING("Could not open log file");
                mLogFileStreamXYStageMove.setDevice(&mLogFileXYStageMove);
                mLogFileStreamXYStageMove << "mCLock, duration, rx, ry\n";
                mLogFileStreamXYStageMove.flush();

                QString fileNamePressure = storageFolder + "tracker_pressure.csv";
                mLogFilePressure.setFileName(fileNamePressure);
                mLogFilePressure.open(QIODevice::WriteOnly | QIODevice::Text);
                if (!mLogFilePressure.isOpen())
                    TRACKER_WARNING("Could not open log file");
                mLogFileStreamPressure.setDevice(&mLogFilePressure);
                mLogFileStreamPressure << "mCLock, pressureValue\n";
                mLogFileStreamPressure.flush();

                QString fileNameDuration = storageFolder + "tracker_duration.csv";
                mLogFileDuration.setFileName(fileNameDuration);
                mLogFileDuration.open(QIODevice::WriteOnly | QIODevice::Text);
                if (!mLogFileDuration.isOpen())
                    TRACKER_WARNING("Could not open log file");
                mLogFileStreamDuration.setDevice(&mLogFileDuration);

                QString fileNameZStageLogging = storageFolder + "tracker_continuous.csv";
                mLogFileContinuous.setFileName(fileNameZStageLogging);
                mLogFileContinuous.open(QIODevice::WriteOnly | QIODevice::Text);
                if (!mLogFileContinuous.isOpen())
                    TRACKER_WARNING("Could not open log file");
                mLogFileStreamContinuous.setDevice(&mLogFileContinuous);
                mLogFileStreamContinuous << "captureTime,mClock,isMovingXY,isMovingZ,zPos,BV\n";
                mLogFileStreamContinuous.flush();

                QString fileNameParams = storageFolder + "tracker_parametersettings.csv";
                mLogFileParameters.setFileName(fileNameParams);
                mLogFileParameters.open(QIODevice::WriteOnly | QIODevice::Text);
                if (!mLogFileParameters.isOpen())
                    TRACKER_WARNING("Could not open log file");
                mLogFileStreamParameters.setDevice(&mLogFileParameters);

                mLogFileStreamParameters << "mXYTrackingEnabled:" << mXYTrackingEnabled << "\n";
                mLogFileStreamParameters << "mZFocusTrackingEnabled:" << mZFocusTrackingEnabled << "\n";
                mLogFileStreamParameters << "mAdaptiveAutoFocusEnabled:" << mAdaptiveAutoFocusEnabled << "\n";
                mLogFileStreamParameters << "mBrennerEnabled:" << mBrennerEnabled << "\n";
                mLogFileStreamParameters << "mBrennerOffset:" << mBrennerOffset << "\n";
                mLogFileStreamParameters << "mZStageEnabledBlocking:" << mZStageEnabledBlocking << "\n";
                mLogFileStreamParameters << "mXYStageEnabledBlocking:" << mXYStageEnabledBlocking << "\n";
                mLogFileStreamParameters << "mUseNoiseLevelAtBrenner:" << mUseNoiseLevelAtBrenner << "\n";
                mLogFileStreamParameters << "mUseEstimatedWindowSize:" << mUseEstimatedWindowSize << "\n";
                mLogFileStreamParameters << "mCameraExposureTime:" << mCameraExposureTime << "\n";
                if( !mCameraImageSize.isNull() )
                    mLogFileStreamParameters << "mCameraImageSize:" << mCameraImageSize.width() << "," << mCameraImageSize.height() << "\n";
                mLogFileStreamParameters << "mMaxProcessDelay:" << mMaxProcessDelay << "\n";
                mLogFileStreamParameters << "mMaxTotalStageMove:" << mMaxTotalStageMove << "\n";
                mLogFileStreamParameters << "mMinOffset:" << mMinOffset << "\n";
                mLogFileStreamParameters << "mTuningStep:" << mTuningStep << "\n";
                mLogFileStreamParameters << "mCorrectionFactor:" << mCorrectionFactor << "\n";
                mLogFileStreamParameters << "mTimestampDuration:" << mTimestampDuration << "\n";
                mLogFileStreamParameters << "mMaxAmplitude:" << mMaxAmplitude << "\n";
                mLogFileStreamParameters << "mNumberOfPressureSteps:" << mNumberOfPressureSteps << "\n";
                QSize ffimg = getFFTImageSize();
                if( ffimg.isNull() )
                    mLogFileStreamParameters << "FFTImageSize:" << ffimg.width() << "," << ffimg.height() << "\n";
                mLogFileStreamParameters << "controllerGain:" << mCurrentOptions->controllerGain << "\n";
                mLogFileStreamParameters << "stageCommandDelay:" << mCurrentOptions->stageCommandDelay << "\n";
                mLogFileStreamParameters << "correlatorDepth:" << mCurrentOptions->correlatorDepth << "\n";
                mLogFileStreamParameters << "predictorSize:" << mCurrentOptions->predictorSize << "\n";
                mLogFileStreamParameters << "pixelSize:" << mCurrentOptions->pixelSize.x() << "," << mCurrentOptions->pixelSize.y() << "\n";
                mLogFileStreamParameters << "mBrennerRoiPercentage:" << mBrennerRoiPercentage << "\n";
                mLogFileStreamParameters << "mZStackSize:" << mZStackSize << "\n";
                mLogFileStreamParameters << "mZStackStepSize:" << mZStackStepSize << "\n";
                mLogFileStreamParameters << "mLargeCorrectionStep:" << mFocusTracker->getLargeCorrectionStep() << "\n";
                mLogFileStreamParameters << "mLowerBrennerThresholdValue:" <<  mFocusTracker->getLowerThresholdFocus() << "\n";
                mLogFileStreamParameters << "mLowerBrennerThresholdPercentage:" <<  mFocusTracker->getLowerBrennerThreshold() << "\n";
                mLogFileStreamParameters << "mLowerBrennerPredictedThreshold:" << mFocusTracker->getLowerThresholdPredictedFocus() << "\n";
                mLogFileStreamParameters << "mUpperBrennerThresholdValue:" << mFocusTracker->getUpperThresholdFocus() << "\n";
                mLogFileStreamParameters << "mUpperBrennerThresholdPercentage:" << mFocusTracker->getUpperBrennerThreshold() << "\n";
                mLogFileStreamParameters << "mUpperBrennerPredictedThreshold:" << mFocusTracker->getUpperThresholdPredictedFocus() << "\n";
                mLogFileStreamParameters << "getMaxFocus:" << mFocusTracker->getMaxFocus() << "\n";
                mLogFileStreamParameters << "getMaxPredictedFocus:" << mFocusTracker->getMaxPredictedFocus() << "\n";
                mLogFileStreamParameters << "getHalfWindowSize:" << mFocusTracker->getHalfWindowSize() << "\n";
                mLogFileStreamParameters << "getFullWindowSize:" << mFocusTracker->getFullWindowSize() << "\n";

                mLogFileStreamParameters.flush();

                //!/ Set the region-of-interest percentage in the focus tracker
                mCorrelator->setBrennerRoiPercentage(mBrennerRoiPercentage);

                // std::cout<< "The Brenner Value has to be set: " << mBrennerRoiPercentage<<std::endl;
                break;
            }
            case ZStack:
                this->connect(mFocusTracker,    SIGNAL(zStackFinished()),
                              this,             SLOT(stopIntern()));
                // Set the region-of-interest percentage in the focus tracker
                mFocusTracker->setBrennerRoiPercentage(mBrennerRoiPercentage);
                mFocusTracker->setStorageFolder( mStorageFilename );
                mFocusTracker->startZStack(mZStackStepSize, mZStackSize);
                break;
            default:
                break;
            }

            // Start event loop
            thread->exec();

            this->killTimer(timerID);

            trackerTimer->stop();
            trackerTimerXY->stop();
            //Temporal test solution -Set mLastPressureChangePassed to false 20170609
            mLastPressureChangePassed = false;

            // TrackerTimerBetween - stopping the time
          //  trackerTimerBetween->stop();

            // Process tuner results
            switch (mCurrentMode)
            {
            case Tracking:
                if (mLogFileTracker.isOpen()) {
                    mLogFileStreamTracker.flush();
                    mLogFileTracker.close();
                }
                if (mLogFileTrackerTiming.isOpen()) {
                     mLogFileStreamTrackerTiming.flush();
                     mLogFileTrackerTiming.close();
                }
                if (mLogFileAutofocus.isOpen()) {
                     mLogFileStreamAutofocus.flush();
                     mLogFileAutofocus.close();
                }
                if (mLogFilePressure.isOpen()) {
                     mLogFileStreamPressure.flush();
                     mLogFilePressure.close();
                }
                if (mLogFileDuration.isOpen()) {
                     mLogFileStreamDuration.flush();
                     mLogFileDuration.close();
                }
                if (mLogFileXYStageMove.isOpen()) {
                     mLogFileStreamXYStageMove.flush();
                     mLogFileXYStageMove.close();
                }
                if (mLogFileZStageMove.isOpen()) {
                     mLogFileStreamZStageMove.flush();
                     mLogFileZStageMove.close();
                }
                if (mLogFileParameters.isOpen()) {
                     mLogFileStreamParameters.flush();
                     mLogFileParameters.close();
                }
                if (mLogFileContinuous.isOpen()) {
                     mLogFileStreamContinuous.flush();
                     mLogFileContinuous.close();
                }
                break;
            case Measuring: break;
            case Timing:
                this->processTunerResults();
                break;
            case ZStack:
                mFocusTracker->finishZStack();
                mStampStageMoved = mClock.getTime();
                break;
            default: assert(false);
            }
        }

        mIsRunning = false;

        this->moveToThread(QApplication::instance()->thread());
    }

    void Controller::stopIntern()
    {
        mIsRunning = false;
        emit quit();
    }

    void Controller::initialise(Stage* stage)
    {
        if (mInitialised)
            return;

        mStage = stage;
        mFocusTracker = new FocusTracker(mStage, mCurrentOptions->fftImageSize);
        mInitialised = true;
        this->updateCorrelator();
    }

    void Controller::timerEvent(QTimerEvent* event)
    {
        emit frameRateUpdated(mFrameCounter.getFrameRate());
    }

    void Controller::trackImage(const QImage image, quint64 captureTime, quint64 processTime)
    {
        //std::cout<<"trackImage: "<<std::endl;
        mLogFileStreamDuration << "track_image_capture_time," << captureTime << "\n";
        mLogFileStreamDuration << "track_image_process_time," << processTime << "\n";
        mLogFileStreamDuration << "start_track_image," << mClock.getTime() << "\n";

        quint64 trackImage_start_time = mClock.getTime();
        //mLogFileStreamTrackerTiming << "trackImage: start at " << trackImage_start_time << "\n";
        mLogFileStreamTrackerTiming << "trackImage: start " << "\n";

        // No processing anymore if thread is waiting to be stopped
        if (!mIsRunning) {
            mLogFileStreamDuration << "return_track_image_1," << mClock.getTime() << "\n";
           // std::cout<<"trackImage:return_track_image_1 !mIsRunning"<<std::endl;
            return;
        }

        /// Trial for manage only 1 assignment but XY tracker is not stable...
        //mCorrelator->AssignImageToMemory(image);
        // continuous logging of z stage movement, brenner value computation, capture time, system time.        
        mCorrelator->computeBrennerValueForSnapshot(image); //temporal 20190130
        mLogFileStreamContinuous << captureTime << "," << mClock.getTime() << "," << mStage->isMovingXY() <<
                                    "," << mStage->isMovingZ() << "," << mStage->getZpos() << "," << mCorrelator->getLastFocus().brennerFocus << "\n";
        //std::cout<< "BrennerValue: "<< mCorrelator->getLastFocus().brennerFocus<<std::endl;

        // Don't let the image buffer overflow due to missing CPU power
        quint64 currentTime = mClock.getTime();
        if (currentTime > processTime + mMaxProcessDelay)
        {
            // Be sure to update the Smith Predictor (acts frame based)
            mSmithPredictor.append(QPointF(0.0, 0.0));
            mSmithPredictor.removeFirst();
            mLogFileStreamDuration << "return_track_image_2," << mClock.getTime() << "\n";
           // std::cout<<"trackImage:return_track_image_2: currentTime > processTime + mMaxProcessDelay"<<std::endl;
            return;
        }

        // Z Stack acquiring is handled independently
        if (mCurrentMode == ZStack) {
            //Debugging the ZStack - so recording all of the images what we have
            //std::cout<<"test"<<std::endl;
            //emit OffLineZstackImage(image, captureTime, processTime);

            quint64 processZStack_start_time = mClock.getTime();
            // mLogFileStreamTrackerTiming << "processZStack: start at " << processZStack_start_time << "\n";
            mFocusTracker->processZStack(image, captureTime, processTime);
            //mFocusTracker->processZStackNewVersion(mCorrelator->getLastFocus().brennerFocus,captureTime);
            quint64 processZStack_end_time = mClock.getTime();
            // mLogFileStreamTrackerTiming << "processZStack: end at " << processZStack_end_time << "\n";
            mLogFileStreamTrackerTiming << "processZStack: duration " << (processZStack_end_time-processZStack_start_time) << "\n";
            emit focusUpdated(mFocusTracker->getLastFocus());
            mLogFileStreamDuration << "return_track_image_3," << mClock.getTime() << "\n";
            // setting some default value making sure it is in the range (TODO could be omitted)
            mEstimatedCompletionOfZStageMovement = mClock.getTime();
            return;
        }

        // Update frame counter (do this after the overflow protection)
        mFrameCounter.addFrame(currentTime);

        // Image has to be larger than the correlator image size
        if (image.size().width()  < mCurrentOptions->fftImageSize.width() ||
            image.size().height() < mCurrentOptions->fftImageSize.height())
        {
            TRACKER_WARNING("Tracking aborted: Camera image was smaller than the FFT image");
            this->stopIntern();
            mLogFileStreamDuration << "return_track_image_4," << mClock.getTime() << "\n";
            return;
        }

        // Invalid correlator forces stop
        TRACKER_ASSERT(mCorrelator, "Controller: Correlator was not initialised");

        if (mZFocusTrackingEnabled) {
            trackZ(image, captureTime);
        }

        if( mXYTrackingEnabled ) {            
            trackXY(image, captureTime, processTime);
        }

        quint64 trackImage_end_time = mClock.getTime();
        mLogFileStreamTrackerTiming << "trackImage: end at " << trackImage_end_time << " duration is " << (trackImage_end_time - trackImage_start_time) << " and mMaxProcessDelay is " << mMaxProcessDelay << "\n";
        mLogFileStreamDuration << "return_track_image_end," << mClock.getTime() << "\n";
        //std::cout<<"trackImage:return_track_image_end "<<std::endl;

    }

    void Controller::enableXYTrackingSignal(){
        mLogFileStreamDuration << "enable_xy_tracking_signal," << mClock.getTime() << "\n";
        setXYTrackingEnabled(true);
        setAutofocusModeAfterLastPressureChange(false); //Disabling the image acquisition for high resoltuion images and let
    }

    void Controller::disableXYforDuration(){
        mLogFileStreamDuration << "disable_xy_for_duration," << mClock.getTime() << "\n";
        // disable XY tracking
        setXYTrackingEnabled(false);
        setAutofocusModeAfterLastPressureChange(true);
    }

    void Controller::toggleXYOscillation(){
        mLogFileStreamDuration << "toggle_oscillation," << mClock.getTime() << "\n";
        if( mXYTrackingEnabled ) {
            mLogFileStreamDuration << "disable_xy_for_duration," << mClock.getTime() << "\n";
            disableXYforDuration();
        } else {
            mLogFileStreamDuration << "enable_xy_for_duration," << mClock.getTime() << "\n";
            enableXYTrackingSignal();
        }
    }

    void Controller::QuickFocus(){
        setZStackEnabled(false);
        emitAutoPicture(); //
    }

    void Controller::TimeLapseImaging(){
        // Wired 20 time lapse image;
        if (mTimeLapseCounter < 14){
            LightON();
            mTimeLapseCounter++;
            emitTimeLapseUpdate();
            setZStackEnabled(true);
            trackerTimerXY->singleShot(1000,this,SLOT(QuickFocus()));
        }
        else{
            // Light off
            if (mIsRunning){
                stopIntern();
            }
            else{
             ///FinishedTask
             ///
            }
        }
    }

    void Controller::startZstackAcquisitionOscillation(){
        //Set the tracker to oscillating mode for Z stack acquisition
        //Switch of XY
        if (mZstackCounter < mMaxNumberOfZstackafterLastPressureChange){
            //std::cout<<"MaxNumberOfZstack: "<<mMaxNumberOfZstackafterLastPressureChange<<std::endl;
            //std::cout<<"counter: "<<mZstackCounter<<std::endl;
            setXYTrackingEnabled(false);
            //Switch of Z
            setZStackEnabled(false);
            /// moving a few micron back
            ///moveStageZ(-2,1);
            //increase counter of Zstacks
            mZstackCounter = mZstackCounter+1;
            //emitAutoImages
            emitAutoPicture();
        }      
    }

    void Controller::switchOnXYZ(){
        std::cout<<"Are we here"<<std::endl;
        if (mLastPressureChangePassed && (mZstackCounter < mMaxNumberOfZstackafterLastPressureChange)){   
            enableXYTrackingSignal();
            setZStackEnabled(true);
            trackerTimerXY->singleShot(mXYTrackerDurationOscillation, this, SLOT(startZstackAcquisitionOscillation()));
        }else{
            //if (mTimeLapse && mLastPressureChangePassed){
           if (mTimeLapse){
                LightOFF();
                mLastPressureChangePassed = false;
                std::cout<<"does it work"<<std::endl;
                //emitSaveImages();
                if (mTimeLapseCounter < 5)
                    trackerTimerXY->singleShot(30000, this, SLOT(TimeLapseImaging()));
                else if (mTimeLapseCounter <10){
                    trackerTimerXY->singleShot(120000, this, SLOT(TimeLapseImaging()));
                    emitSaveImages();
                }
                else if (mTimeLapseCounter <11){
                    trackerTimerXY->singleShot(180000, this, SLOT(TimeLapseImaging()));
                    emitSaveImages();
                }
                else{
                    trackerTimerXY->singleShot(300000, this, SLOT(TimeLapseImaging()));
                    emitSaveImages();
                }
            }
           else{
               if (mIsRunning){
                   stopIntern();
               }
           }
        }
        //std::cout<<"test"<<std::endl;
    }

    void Controller::startXYOscillation(){
        mLogFileStreamDuration << "start_xy_oscillation_single_shot," << mClock.getTime() << "\n";
        // already start disabled, so if there is a region that is satisfied, just take the picture
        disableXYforDuration();
        trackerTimer->start(mXYTrackerDurationOscillation);
    }

    void Controller::emitHighResPicture() {
        mLogFileStreamDuration << "emit_high_res_image," << mClock.getTime() << "\n";
        emit takeHighResolutionHighIntensityPicture();
    }

    void Controller::emitAutoPicture() {
        mLogFileStreamDuration << "emit_auto_image," << mClock.getTime() << "\n";
        emit takeAutoPicture();
    }

    void Controller::emitTimeLapseUpdate(){
        emit TimeLapseCounterUpdated(QString::number(mTimeLapseCounter));
    }

    void Controller::emitSaveImages(){
        emit SaveImages();
    }

    void Controller::moveStageXY(QPointF distance)
    {
        mStage->move(distance);
        //emit StageisMOVED();
    }

    void Controller::moveStageZ(double microns, int block)
    {
        quint64 moveStage_start_time = mClock.getTime();
        //std::cout << "Z stage move: " << microns << " blocking state " << block << std::endl;

        // mStage->moveZ(microns, block);
        // use the blocking state defined in the GUI
        mStage->moveZ(microns, block);
        mLogFileStreamZStageMove << mClock.getTime() << "," << (mClock.getTime()-moveStage_start_time) << "," << microns << "," << mStage->getZpos() << "," << block << "\n";
        emit zmoved(mStage->getZpos());
        mStampStageMoved = mClock.getTime();
        //emit StageisMOVED();
    }

    void Controller::writePressureInfoToLogFile(double pressureValue)
    {
        quint64 pressureTime = mClock.getTime();
        mLastPressureChange.first = pressureTime;
        mLastPressureChange.second = pressureValue;

        mLogFileStreamPressure << pressureTime << "," << pressureValue << "\n";
        mLogFileStreamPressure.flush();
    }

    void Controller::trackXY(const QImage image, quint64 captureTime, quint64 processTime) {

        // Compute image offset from (0, 0)
        quint64 correlatorTrackImage_start_time = mClock.getTime();
        // mLogFileStreamTrackerTiming << "correlatorTrackImage: start at " << correlatorTrackImage_start_time << "\n";
        mLogFileStreamDuration << "mcorrelator_track_image," << mClock.getTime() << "\n";
        QPointF imageOffset = mCorrelator->track(image);
        quint64 correlatorTrackImage_end_time = mClock.getTime();
        // mLogFileStreamTrackerTiming << "correlatorTrackImage: end at " << correlatorTrackImage_end_time << "\n";
        mLogFileStreamTrackerTiming << "correlatorTrackImage: duration " << (correlatorTrackImage_end_time-correlatorTrackImage_start_time) << "\n";
        mLogFileStreamDuration << "mcorrelator_track_image_end," << mClock.getTime() << "\n";
        // Show the DFT spectrum if not using Brenner focus value
        if (!mBrennerEnabled) {
            // mBrennerEnabled is false, thus emit image. Duration:
            emit debugImage(mCorrelator->getLastAdjustedDFTImage());
        }

        emit focusUpdated(mCorrelator->getLastFocus());

        // Controller transfer function
        QPointF stageMove(0.0, 0.0);
        if (mCorrelator->isReady())
        {
            switch (mCurrentMode)
            {
            case Tracking:
                stageMove = this->transferFunction(imageOffset);
                break;
            case Measuring:
                stageMove = this->measuringFunction(imageOffset, image.size());
                break;
            case Timing:
                stageMove = this->timingFunction(imageOffset);
                break;
            default: assert(false);
            }
        }

        mLogFileStreamTrackerTiming << "mStage: is it moving XY? " << mStage->isMovingXY() << "\n";
        mLogFileStreamTrackerTiming << "mStage: is it moving Z? " << mStage->isMovingZ() << "\n";
        mLogFileStreamTrackerTiming << "stageMove: how much? rx: " <<  stageMove.rx() << ", ry: " << stageMove.ry() << "\n";

        // Do this right here right after the image has been taken to allow for
        // a constant delay (assuming the camera is slower than this loop)
        if (!stageMove.isNull() && !mStage->isMovingXY())
        {
            mLogFileStreamTrackerTiming << "stageMove is not-null and stage is not moving" << "\n";

            // Limit local movements (no more than a quarter of the image)
            QPointF localMax = stageCoordinates(QPointF(image.width() / 4, image.height() / 4));
            stageMove.rx() = qMax(-localMax.x(), qMin(localMax.x(), stageMove.x()));
            stageMove.ry() = qMax(-localMax.y(), qMin(localMax.y(), stageMove.y()));
            //  std::cout << __FUNCTION__ << "X:" <<  stageMove.rx()<<"     Y :" <<  stageMove.ry()<<std::endl;

            if (stageMove.rx() != 0 || stageMove.ry() != 0) {
                if(abs(stageMove.rx()) < 0.01 && abs(stageMove.ry()) < 0.01) {
                    // Question from Stephan: does premature return not require adding 0,0 to smith predictor ring buffer?
                    std::cout<<"trackXY:return_track_image_6: abs(stageMove.rx()) < 0.001 && abs(stageMove.ry()) < 0.001 "<<std::endl;

                    return;
                }
            }

            mTotalStageMove += stageMove;
            // Limit total movement
            if (qAbs(mTotalStageMove.x()) > mMaxTotalStageMove || qAbs(mTotalStageMove.y()) > mMaxTotalStageMove)
            {
                TRACKER_WARNING("Tracking aborted: Total distance moved by the stage was exceeded");
                this->stopIntern();
                mLogFileStreamDuration << "return_track_image_5," << mClock.getTime() << "\n";
                std::cout<<"trackXY:return_track_image_5 "<<std::endl;
                return;
            }
            //if (clock.getTime() > imageAcquireTime + stageCommandDelay)
            //    TRACKER_INFO("Stage command period was too high");

            // Busy wait (mSleep dynamics is too slow) until the timing is just right to move the stage
            // If we wait too long or too short, the Smith Predictor will fail
            // Also note that we're using busyWait() instead of just queriying the time. The reason
            // is that the query is a kernel call and high kernel times can cause the timer to go crazy
            // (no kidding, see Microsoft KB274323)

            quint64 busyWaitBeforeStageMovement_start_time = mClock.getTime();
            // mLogFileStreamTrackerTiming << "busyWaitBeforeStageMovement: start at " << busyWaitBeforeStageMovement_start_time << "\n";
            while (mClock.getTime() < captureTime + mCurrentOptions->stageCommandDelay * 1000)
                busyWait(); // Wait for about 100 microseconds (not very precise at all!)
            quint64 busyWaitBeforeStageMovement_end_time = mClock.getTime();
            //mLogFileStreamTrackerTiming << "busyWaitBeforeStageMovement: end at " << busyWaitBeforeStageMovement_end_time << "\n";
            mLogFileStreamTrackerTiming << "busyWaitBeforeStageMovement: duration " << (busyWaitBeforeStageMovement_end_time-busyWaitBeforeStageMovement_start_time) << "\n";

            // Move
            quint64 moveStage_start_time = mClock.getTime();
            // TODO: test blocking
            // 0 as second parameter calls this functionally blocking, i.e. it waits until the stage move has been completed
            // 1 as second parameter immediately returns
            mLogFileStreamDuration << "call_move_XY_stage" << "," << mClock.getTime() << "\n";
            //std::cout <<"XY stage move: " << stageMove.rx() << "," << stageMove.ry() << " as blocking? " << mXYStageEnabledBlocking << std::endl;

            // use the blocking state defined in the GUI
            mStage->move(stageMove, mXYStageEnabledBlocking);

            quint64 moveStage_end_time = mClock.getTime();
            mLogFileStreamXYStageMove << mClock.getTime() << "," << (mClock.getTime()-moveStage_start_time) << "," << stageMove.rx() << "," << stageMove.ry() << "\n";

            mLogFileStreamTrackerTiming << "moveStageXY: duration " << (moveStage_end_time-moveStage_start_time) << "\n";

            mSmithPredictor.append(stageMove);
        }
        else
        {
            mLogFileStreamTrackerTiming << "stageMove is null or stage is moving" << "\n";
            mSmithPredictor.append(QPointF(0.0, 0.0));
        }

        // Advance in ring buffer for the Smith Predictor
        mSmithPredictor.removeFirst();
    }

    void Controller::trackZ(const QImage image, quint64 captureTime)
    {
        // return if the ZStack Offline lookup table was not generated
        // So there is no reason to run this function even if the gui would
        // allow with the QCheckBox  "Enable Z-tracker"
        if (!mFocusTracker->isReady()){
            return;
        }

        quint64 start_track_z_time = mClock.getTime();
        mLogFileStreamDuration << "called_track_z," << start_track_z_time << "\n";

        // return condition of current image is too old to be still useful in
        // a non-blocking stage movement execution
        // take camera exposure time as max
        // TODO: optimize, i.e. make smaller
        int maxAquisitionTimeDelay = (int) mCameraExposureTime;

        mLogFileStreamTrackerTiming << "trackZ: isMovingZ" << mStage->isMovingZ() << "\n";
        mLogFileStreamTrackerTiming << "trackZ: return condition maxAquisitionTimeDelay " << maxAquisitionTimeDelay << " mEstimatedCompletionOfZStageMovement " << mEstimatedCompletionOfZStageMovement << " start_track_z_time " << start_track_z_time << " difference " << (start_track_z_time-mEstimatedCompletionOfZStageMovement) << "\n";

        if( mStage->isMovingZ() ) {
            mLogFileStreamDuration << "return_track_focus_zstack_moving," << mClock.getTime() << "\n";
            if (!mZStageEnabledBlocking)
                mStampStageMoved = mClock.getTime();
            return;
        }

        // only check this for the non-blocking z stage case
        if( mZStageEnabledBlocking ){
            if( (captureTime < mStampStageMoved + maxAquisitionTimeDelay) ) {
               // std::cout<<"trackZ: return track focus"<<std::endl;
                mLogFileStreamDuration << "return_track_focus_zstack_0," << mClock.getTime() << "\n";
                return;
            }
        } else {
            // TODO: using the estimated time here is not very reliable, it should check the isMovingZ
            if( captureTime < (mStampStageMoved + 2 * maxAquisitionTimeDelay)   ) {
              //  std::cout<<"trackZ: return track focus"<<std::endl;
                mLogFileStreamDuration << "return_track_focus_zstack_1," << mClock.getTime() << "\n";
                return;
            }
        }

        // Disabled this return statement because it is non-functional for
        // non-blocking calls to move the Z stage, i.e. mStampStageMoved has
        // a non-sensible time stamp. We now already do not execute this function
        // when isMovingZ is true.
        /*
        // Image was captured before we finished the previous movement...
        if (captureTime - (quint64) mCameraExposureTime < mStampStageMoved) {
            // ...silently ignore it
            mLogFileStreamTrackerTiming << ": Image was captured before we finished the previous movement. Return." << "\n";
            mLogFileStreamDuration << "return_focus_zstack_2," << mClock.getTime() << "\n";
            return;
        }
        */

        // TOREMOVE: fake a waiting time to try to see a more correct z position value?
        /*while (mClock.getTime() < start_track_z_time + 5000)
            busyWait(); // Wait for about 100 microseconds (not very precise at all!)
        */

        mLogFileStreamDuration << "start_track_focus_zstack," << mClock.getTime() << "\n";

        // compute the Z correction value and call stage Z movement non-blocking
        quint64 trackZ_start_time = mClock.getTime();
        mLogFileStreamTrackerTiming << ": inside function start " << "\n";

        mLogFileStreamDuration << "track_z_compute_brenner_start," << mClock.getTime() << "\n";
        mCorrelator->computeBrennerValueForSnapshot(image);
        mLogFileStreamDuration << "track_z_compute_brenner_end," << mClock.getTime() << "\n";

        static const double PRESSURE_CHANGE_MAX_TIME = 100 * 1000;  // usec
        static const double PRESSURE_CHANGE_MAX_PA = 1500;

        double brennerFocus = mCorrelator->getLastFocus().brennerFocus;

        // TODO: this is absolute, and should later be relative!
        double zPos = mStage->getZpos();

        // Keep track of past focus values
        mFocii.prepend(brennerFocus);
        mDistance = 0.0;

        // Wait until at least two focus values are available
        if (mFocii.size() < 2) {
            mLogFileStreamTrackerTiming << "trackZ: Less than two focus values are available. Return." << "\n";
            mLogFileStreamDuration << "return_focus_zstack_3," << mClock.getTime() << "\n";
            return;
        }

        // The last pressure change was more than 100ms ago or was less
        // than 1.5 kPa in scale (TODO: these values might need to be adjusted
        // based on empirical values)

        // TODO: when there is a pressure change, we exactly know in which direction we have to correct, i.e.
        // we know on which side we are on the brenner function.

        mLogFileStreamDuration << "start_decision_tree," << mClock.getTime() << "\n";

        // use either the predefined large correction step or the window size estimated from the zstack
        double largerCorrectionStepSize = 0;
        if( mUseEstimatedWindowSize ) {
          largerCorrectionStepSize = mFocusTracker->getFullWindowSize();
        } else {
          largerCorrectionStepSize = mFocusTracker->getLargeCorrectionStep();
        }
        double noiseLevel = 0.0;
        if( mUseNoiseLevelAtBrenner ) {
            noiseLevel = mFocusTracker->getNoiseLevelFromBrenner( brennerFocus );
        } else {
            noiseLevel = 0.0;
        }

        // if ((mClock.getTime() > mLastPressureChange.first + PRESSURE_CHANGE_MAX_TIME) || (mLastPressureChange.second < PRESSURE_CHANGE_MAX_PA))
        /*if (mClock.getTime() > mLastPressureChange.first + PRESSURE_CHANGE_MAX_TIME)
        {*/

            //mLogFileStreamTrackerTiming << "trackZ: above 100ms or pressure change smaller than 1500 \n";

        if (mFocii.at(0) > mFocusTracker->getUpperThresholdFocus() - noiseLevel ) {
            mLogFileStreamTrackerTiming << "trackZ: Already in focus region 1 \n";
           // std::cout << __FUNCTION__ << "(): Already in focus region 1" << std::endl;

            // TODO: when is this flag set to false?
            mIsInFocus = true;

            bool last_series_images_in_focus = true;
            int N_IMAGES2 = 3;
            if(mFocii.size() > N_IMAGES2) {
                for(int i=0; i<N_IMAGES2; i++) {
                    if( mFocii.at(i) < mFocusTracker->getUpperThresholdFocus() - noiseLevel ) {
                        last_series_images_in_focus = false;
                    }
                }
            }
            mLogFileStreamDuration << "track_image_in_focus," << mClock.getTime() << "," << mInAutofocusModeAfterLastPressureChange << "," << last_series_images_in_focus << "\n";

            if( mInAutofocusModeAfterLastPressureChange && last_series_images_in_focus){
                mLogFileStreamDuration << "track_image_autofocus_on_and_last_in_focus," << mClock.getTime() << "\n";
                std::cout<<"here we shoot images????"<<std::endl;
                // Implementation of ZStack acquisition at the end of the tracking to be sure to capture a sharp image.
                //if (1) {
                    // TODO: try to emit high intensity picture
                //int NumberOfImagesRequest = 5;
               // std::cout<<"*************"<<std::endl<<"Emitting the 5 images for Zstack"<<std::endl<<"*************"<<std::endl;
                    emitAutoPicture();
                    // enable xy tracking again and disable picture taking
   //temporal                 enableXYTrackingSignal();
                    // restart timer
  //temporal                  trackerTimer->start(mXYTrackerDurationOscillation);
                //} else {
                    // TODO: implementation of high resulition images to acquire a Zstack
                    //mDirection = -1; //! my part
                   // mDistance = 1; //! my part
                   // moveStageZ(mDirection * mDistance, mZStageEnabledBlocking);//! my part
                //    std::cout<<"********************"<<std::endl<<"Second acquistion:"<<std::endl<<"********************"<<std::endl;//! my part
                //    emitAutoPicture();//! my part
                //    emitAutoPicture();
              //     int Resolution = 1;
                //   for (unsigned int i = 0; i < 3; ++i){
                 //        moveStageZ(-mDirection * Resolution, mZStageEnabledBlocking);
                  //          emitAutoPicture(5);
                   // }

                    ///NewTrendCreating
                    /// emitHighResPicture();

                    enableXYTrackingSignal();
                    // restart timer
                    trackerTimer->start(mXYTrackerDurationOscillation);
                //}
            }

            // could return here to never correct anything at a satisfied level.
            //return;
            // Set the IsInFocus flag to true, else do nothing
        } else if (mFocii.at(0) < mFocii.at(1) - noiseLevel) {
            mLogFileStreamTrackerTiming << "trackZ: change direction before " << mDirection << " \n";
            //std::cout << __FUNCTION__ << "(): Change direction 1" << std::endl;
            mDirection = -mDirection;
            mLogFileStreamTrackerTiming << "trackZ: change direction after " << mDirection << " \n";

            double fDiff = mFocii.at(1) - mFocusTracker->getLowerThresholdFocus();
            mLogFileStreamTrackerTiming << "trackZ: fDiff " << fDiff << "\n";
            if (fDiff > 0) {
                //std::cout << __FUNCTION__ << "(): Jump correction 1" << std::endl;
                mDistance = 2.0 * mFocusTracker->correctFocus(mFocii.at(1));
                mLogFileStreamTrackerTiming << "trackZ: correctFocus(mFocii.at(1) " << mFocusTracker->correctFocus(mFocii.at(1)) << "\n";
                mLogFileStreamTrackerTiming << "trackZ: fDiff > 0 jump correction distance " << mDistance << "\n";
            } else {
                //std::cout << __FUNCTION__ << "(): Double correction 1" << std::endl;
                mDistance = 2.0 * largerCorrectionStepSize;
                mLogFileStreamTrackerTiming << "trackZ: fDiff <= 0 jump correction distance " << mDistance << "\n";
            }
        } else if ((mFocii.at(0) - mFocusTracker->getLowerThresholdFocus()) > 0) {
            //std::cout << __FUNCTION__ << "(): Gaussian correction 1" << std::endl;
            mDistance = mFocusTracker->correctFocus(mFocii.at(0));
            mLogFileStreamTrackerTiming << "trackZ: Gaussian correction 1 distance is " << mDistance << "\n";
        } else {
            //std::cout << __FUNCTION__ << "(): Below threshold 1" << std::endl;
            mDistance = largerCorrectionStepSize;
            mLogFileStreamTrackerTiming << "trackZ: Below threshold 1 distance is " << mDistance << "\n";
        }

        /*
        } else {
            mLogFileStreamTrackerTiming << "trackZ: above 100ms and pressure change smaller than 1500 \n";

           // std::cout << __FUNCTION__ << "(): Pressure changed by "         << mLastPressureChange.second << std::endl;

            mLogFileStreamTrackerTiming << "trackZ: Pressure changed by " << mLastPressureChange.second << "\n";

            if (captureTime > mLastPressureChange.first) {
                mLogFileStreamTrackerTiming << "trackZ: captureTime > mLastPressureChange.first is true " << "\n";
                double fDiff = std::abs(mFocusTracker->getMaxFocus() - mFocii.at(0));
                mLogFileStreamTrackerTiming << "trackZ: fDiff " << fDiff << " mFocusTracker->getMaxFocus() " << mFocusTracker->getMaxFocus() << " mFocii.at(0) " << mFocii.at(0) << "\n";
                if (fDiff < mFocusTracker->getLowerThresholdFocus()) {
                    //std::cout << __FUNCTION__ << "(): Below threshold 2" << std::endl;
                    mDistance = largerCorrectionStepSize;
                    mLogFileStreamTrackerTiming << "trackZ: Below threshold 2 distance is " << mDistance << "\n";
                } else if (mFocii.at(0) < mFocusTracker->getUpperThresholdFocus()) {
                    //std::cout << __FUNCTION__ << "(): Gaussian correction 2" << std::endl;
                    mDistance = mFocusTracker->correctFocus(mFocii.at(0));
                    mLogFileStreamTrackerTiming << "trackZ: Gaussian correction 2 distance is " << mDistance << "\n";
                } else {
                   // std::cout << __FUNCTION__ << "(): Already in focus region 2" << std::endl;
                    mLogFileStreamTrackerTiming << "trackZ: Already in focus region 2 distance is " << mDistance << "\n";
                }
            } else {
                //std::cout << "Discarding image before pressure change" << std::endl;
                mLogFileStreamTrackerTiming << "trackZ: Discarding image before pressure change. distance is " << mDistance << "\n";
            }
        }
        */

        while (mFocii.size() > 10)
            mFocii.pop_back();

        mLogFileStreamTrackerTiming << "trackZ: measured window half-width of brenner function " << mFocusTracker->getHalfWindowSize() << "\n";

        quint64 focustime = mClock.getTime();
        mLogFileStreamTracker << focustime << "," << zPos << "," << brennerFocus << "," << mFocii.at(1) << "," << noiseLevel << "\n";
        mLogFileStreamTracker.flush();

        mLogFileStreamTrackerTiming << "trackZ: "
                  << "timestamp: " << focustime
                  << " zPos: " << zPos
                  << ", correction " << mDirection * mDistance
                  << ", focus " << brennerFocus
                  << ", last focus " << mFocii.at(1)
                  << ", satisfaction " <<  mFocusTracker->getUpperThresholdFocus() - noiseLevel
                  << ", max focus z stack " << mFocusTracker->getMaxFocus()
                  << ", lower threshold " << mFocusTracker->getLowerThresholdFocus()
                  << ", upper threshold " << mFocusTracker->getUpperThresholdFocus() << "\n";

        // mLogFileStreamTrackerTiming << mClock.getTime()<< " lastPressureChange first " << mLastPressureChange.first << " pressureChange second " << mLastPressureChange.second << "\n";

        /*std::cout << __FUNCTION__ << "(): "
                  << "zPos " << zPos
                  << ", correction " << mDirection * mDistance
                  << ", focus " << brennerFocus
                  << ", last focus " << mFocii.at(1)
                  << ", satisfaction " <<  mFocusTracker->getUpperThresholdFocus() - mFocusTracker->getNoiseLevelAt(zPos)
                  << ", max focus z stack " << mFocusTracker->getMaxFocus()
                  << ", lower threshold " << mFocusTracker->getLowerThresholdFocus()
                  << ", upper threshold " << mFocusTracker->getUpperThresholdFocus()
                  << std::endl;
         */

        // mLogFileStreamTrackerTiming << "trackZ: mIsInFocus " << mIsInFocus << "\n";

        if (mDistance > 0.01 ) {

            // in the case of a non-blocking call, we would like to know when the stage
            // movement has been completed in order to discard images that are taken during
            // the movement which we do not want to use for another compensation call
            // because they are outdated.
            // we do not want to implement a smith predictor for z tracking, thus we
            // use a linear model of stage movement duration vs. stage movement distance
            // to estimated a predicted finishing time for a given movement distance.
            // adding to this another time offset that includes processing duration, i.e.
            // storing the image in memory (like the maxProcessDelay in the XY tracker)
            // and computing the brenner value. Only after this time, we consider the image
            // for another correction step.
            // Performance (i.e. how quick it converges) should be similar or better than
            // a blocking call. We want to get rid of the blocking call because it delays
            // potentially a new call to correct the XY tracker.

            // Statistics: for images, how many are used to correct either XY, Z, XY and Z together, or none of them.

            // stage_movement_distance = a * stage_movement_time + b
            // a = 3um/5000ms = 5000 ms/um
            // b = -2 um
            int estimatedZStageMovementDuration = (int) ( (mDistance * mPropGainZ) + 1.5 ) / (0.0003708);
            mEstimatedCompletionOfZStageMovement = mClock.getTime() + estimatedZStageMovementDuration;

            mLogFileStreamAutofocus << mClock.getTime() << "," << zPos << "," << brennerFocus << ","
                << mFocii.at(0) << "," << mFocii.at(1) << "," << mDirection * mDistance << "," << captureTime << "," << estimatedZStageMovementDuration << "\n";

            mLogFileStreamTrackerTiming << "trackZ: execute Z stage movement " << mDirection * mDistance << " with gain " << mPropGainZ << " with blocking " << mZStageEnabledBlocking << "\n";
            mLogFileStreamDuration << "call_move_stage_z," << mClock.getTime() << "\n";
            //std::cout <<"Z stage move: " << mDirection * mDistance * mPropGainZ << " as blocking? " << mZStageEnabledBlocking << std::endl;
            moveStageZ(mDirection * mDistance * mPropGainZ, mZStageEnabledBlocking);

        }
        mLogFileStreamDuration << "return_focus_zstack_end," << mClock.getTime() << "\n";

        quint64 trackZ_end_time = mClock.getTime();
        mLogFileStreamTrackerTiming << "trackZ: duration " << (trackZ_end_time-trackZ_start_time) << "\n";

    }

    QPointF Controller::transferFunction(QPointF imageOffset)
    {
        mLogFileStreamTrackerTiming << "transferFunction: start " << "\n";
        QPointF stageMove = -stageCoordinates(imageOffset);

        // Subtract all stage movements that are not yet visible on the image
        for (int i = 0; i < mSmithPredictor.size(); ++i) {
            // mLogFileStreamTrackerTiming << "transferFunction: mSmithPredictor i " << i << " is " <<  mSmithPredictor[i] << "\n";
            stageMove -= mSmithPredictor[i];
        }

        // Apply controller gain
        mLogFileStreamTrackerTiming << "transferFunction: controller gain is " << mCurrentOptions->controllerGain << "\n";
        stageMove *= mCurrentOptions->controllerGain;

        mLogFileStreamTrackerTiming << "transferFunction: final stage movement rx " << stageMove.rx() << " ry " << stageMove.ry() << "\n";

        return stageMove;
    }

    QPointF Controller::timingFunction(QPointF imageOffset)
    {
        switch (mTimingState)
        {
        case WaitingOnStartup:
            if (mClock.getTime() >= mTunerReferenceTime + mTunerTimeToWaitAfterStartup)
            {
                mTunerReferenceTime = 0;
                mTimingState = WaitingForMoveEnd;
            }
            break;

        case WaitingForMove:
        case WaitingForMoveEnd:
            {
                QPointF delta(imageOffset - mTunerReferenceOffset);
                QPointF step = imageCoordinates(QPointF(mTuningStep, mTuningStep));

                // Store percentage of move recorded
                mTunerCurrentResult.append(1.0 - vectorLength(delta) / vectorLength(step));
                // Check if 80% of the movement was recorded
                if (mTimingState != WaitingForMoveEnd)
                {
                    if (vectorLength(delta) < 0.2 * vectorLength(step))
                    {
                        mTimingState = WaitingForMoveEnd;
                        mTunerReferenceTime  = mClock.getTime();
                    }
                }
                else if (mClock.getTime() > mTunerReferenceTime + mTunerTimeToWaitForMoveEnd)
                {
                    // Store gathered data
                    if (mTunerCurrentResult.size() > 1) // discard at startup
                        mTunerResults.append(mTunerCurrentResult);
                    mTunerCurrentResult.clear();

                    mTimingState = WaitingForMove;

                    QPointF tuningStep(mTuningStep, mTuningStep);
                    // Alternate move direction
                    mTunerMoveDirection *= -1;
                    tuningStep *= mTunerMoveDirection;
                    mTunerReferenceOffset = imageOffset + imageCoordinates(tuningStep);
                    return tuningStep;
                }
            }
            break;

        default:
            TRACKER_ASSERT(false, "Investigate!");
        }

        return QPointF(0.0, 0.0);
    }

    QPointF Controller::measuringFunction(QPointF imageOffset, QSize imageSize)
    {
        if (mClock.getTime() > mTunerReferenceTime + mTunerTimeout)
        {
            TRACKER_WARNING("Measuring process stopped: No movement detected");
            this->stopIntern();
        }

        switch (mMeasuringState)
        {
        case MakeFirstMove:
            {
                mMeasuringState = WaitingForFirstMove;
                mTunerMoveDetected = false;
                mTunerReferenceOffset = imageOffset;
                mTunerReferenceTime = mClock.getTime();
                // Store pixel size in case the measuring failed
                mTunerPreviousPixelSize = this->getPixelSize();

                // First move is rather small and user defined
                return QPointF(mTuningStep, mTuningStep);
                break;
            }

        case WaitingForFirstMove:
            {
                QPointF delta(imageOffset - mTunerReferenceOffset);

                if (!mTunerMoveDetected)
                {
                    if (vectorLength(delta) > mTunerMinimumPixelsForMeasure)
                    {
                        mTunerMoveDetected = true;
                        mTunerReferenceTime = mClock.getTime();
                    }
                }
                else
                {
                    // Note: Wait three times as long as in the timing tuner, just to be sure
                    if (mClock.getTime() > mTunerReferenceTime + 3 * mTunerTimeToWaitForMoveEnd)
                    {
                        QPointF tuningStep(mTuningStep, mTuningStep);

                        // Compute a preliminary value for the pixel size
                        QPointF pixelSize(tuningStep.x() / delta.x(), tuningStep.y() / delta.y());
                        if (!checkPixelSize(pixelSize, 0.2))
                        {
                            TRACKER_WARNING(QString("Measuring failed! Measure size: %1, %2").arg(pixelSize.x()).arg(pixelSize.y()));
                            this->setPixelSize(mTunerPreviousPixelSize);
                            this->stopIntern();
                        }
                        else
                        {
                            this->setPixelSize(pixelSize);

                            mMeasuringState = WaitingForSecondMove;
                            mTunerMoveDetected = false;
                            mTunerReferenceOffset = imageOffset;
                            mTunerReferenceTime = mClock.getTime();

                            // Make a move that is 40% of the entire image
                            QPointF temp(mCurrentOptions->fftImageSize.width(), mCurrentOptions->fftImageSize.height());
                            return mTunerMeasureMovePercentage * stageCoordinates(temp);
                        }
                    }
                }
                break;
            }

        case WaitingForSecondMove:
            {
                QPointF delta(imageOffset - mTunerReferenceOffset);

                if (!mTunerMoveDetected)
                {
                    if (vectorLength(delta) > mTunerMinimumPixelsForMeasure)
                    {
                        mTunerMoveDetected = true;
                        mTunerReferenceTime = mClock.getTime();
                    }
                }
                else
                {
                    // Note: Wait five times as long as in the timing tuner because the move
                    //       is very large.
                    if (mClock.getTime() > mTunerReferenceTime + 5 * mTunerTimeToWaitForMoveEnd)
                    {
                        QPointF tuningStep(stageCoordinates(QPointF(mCurrentOptions->fftImageSize.width(), mCurrentOptions->fftImageSize.height())));
                        tuningStep *= mTunerMeasureMovePercentage;

                        // Compute a precise value for the pixel size
                        QPointF pixelSize(tuningStep.x() / delta.x(), tuningStep.y() / delta.y());
                        if (!checkPixelSize(pixelSize, 0.1))
                        {
                            TRACKER_WARNING(QString("Measuring failed! Measure size: %1, %2").arg(pixelSize.x()).arg(pixelSize.y()));
                            pixelSize = mTunerPreviousPixelSize;
                        }

                        this->setPixelSize(pixelSize);
                        this->stopIntern();
                    }
                }
                break;
            }

        default:
            TRACKER_ASSERT(false, "Investigate!");
        }

        return QPointF(0.0, 0.0);
    }

    bool Controller::checkPixelSize(QPointF size, double tolerance)
    {
        // X and Y should be about equal
        if (qAbs(size.x() / size.y() - 1.0) > tolerance)
            return false;
        // Lower limit:
        // Largest zoom is 100x, binning 1x1 (includes mathmagic)
        if (size.x() < 0.05 || size.y() < 0.05)
            return false;
        // Upper bound:
        // Smallest zoom is 2x, binning 8x8 (some more mathmagic)
        if (size.x() > 25.0 || size.y() > 25.0)
            return false;

        return true;
    }

    void Controller::processTunerResults()
    {
        QVector<double> result;
        foreach(QVector<double> values, mTunerResults)
        {
            for (int i = 0; i < values.size(); ++i)
            {
                while (i >= result.size())
                    result.append(0.0);
                result[i] += values[i];
            }
        }
        QString str;
        QTextStream out(&str);
        out << "Result: ";
        for (int i = 0; i < result.size(); ++i)
        {
            out << i << ":" << result[i] / mTunerResults.size() << " | ";
        }
        out << endl;
        TRACKER_INFO(str, Logger::Low);
    }

    void Controller::setOptionsKey(QString key)
    {
        if (!mOptions.contains(key))
        {
            TRACKER_WARNING("Controller: Options key not found!");
            return;
        }

        // Save old options
        mOptions[mCurrentOptionsKey] = mCurrentOptions;
        mCurrentOptionsKey = key;
        mCurrentOptions = mOptions[mCurrentOptionsKey];
        this->updateCorrelator();
    }

    Correlator* updateCorrelatorAsynchronous(QSize fftSize, int depth)
    {
        // Note: max. thread count is 1 for QThreadPool --> this function
        // never runs concurrently. This is also required because the FFTW
        // planners share data.
        try
        {
            return new Correlator(fftSize, depth);
        }
        catch (const Exception& ex)
        {
            TRACKER_WARNING("Updating the correlator failed:" + ex.getDescription());
            return NULL;
        }
    }

    void Controller::updateCorrelator()
    {
        // Don't update anything at startup
        if (!mInitialised)
            return;

        if (mIsRunning)
        {
            TRACKER_WARNING("Cannot update Tracker while running");
            return;
        }

        // It's not correlator related, but since the image size might have
        // changed we need to adjust the focus tracker as well
        mFocusTracker->resizeImageBuffer(mCurrentOptions->fftImageSize);

        delete mCorrelator;
        mCorrelator = NULL;

        emit validityChanged();

        // Set mouse cursor to wait symbol
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        // Note: Since QThreadPool only operates with one thread, we can safely
        // assume that the order in the Queue is preserved when the functions finish
        QFutureWatcher<Correlator*>* watcher = new QFutureWatcher<Correlator*>(this);
        this->connect(watcher, SIGNAL(finished()), SLOT(correlatorInitialised()));
        mFutureCorrelatorWatchers.enqueue(watcher);

        // Run planning asynchronously
        QFuture<Correlator*> fCorrelator = QtConcurrent::run(updateCorrelatorAsynchronous,
            mCurrentOptions->fftImageSize, mCurrentOptions->correlatorDepth);
        watcher->setFuture(fCorrelator);
    }

    void Controller::correlatorInitialised()
    {
        QFutureWatcher<Correlator*>* watcher = mFutureCorrelatorWatchers.dequeue();
        if (mIsRunning)
        {
            TRACKER_WARNING("Correlator was about to change while running the Tracker!");
            delete watcher->result();
        }
        else
        {
            if (!mFutureCorrelatorWatchers.isEmpty())
                delete watcher->result(); // New computations pending
            else
            {
                mCorrelator = watcher->result();
                mCorrelator->setMinimumOffset(mMinOffset);
                emit validityChanged();
            }
        }

        delete watcher;
        // Note: Acts like a stack, so do this for EVERY call
        QApplication::restoreOverrideCursor();
    }

    void Controller::setUpperBrennerThreshold(double thresh)
    {
        mFocusTracker->setUpperBrennerThreshold(thresh);
    }

    void Controller::setLowerBrennerThreshold(double thresh)
    {
        mFocusTracker->setLowerBrennerThreshold(thresh);
    }

    void Controller::setLargeCorrectionStep(double size)
    {
        mFocusTracker->setLargeCorrectionStep(size);
    }

    void Controller::setMaxTotalStageMove(double value)
    {
        mMaxTotalStageMove = qMax(0.0, value);
    }

    void Controller::setMinOffset(float value)
    {
        mMinOffset = value;
    }

    void Controller::setTuningStep(double value)
    {
        mTuningStep = value;
    }

    void Controller::setControllerGain(double value)
    {
        mCurrentOptions->controllerGain = qMax(0.0, value);
    }

    void Controller::setStageCommandDelay(double value)
    {
        mCurrentOptions->stageCommandDelay = qMax(0.0, value);
    }

    void Controller::setPredictorSize(int value)
    {
        mCurrentOptions->predictorSize = qMax(0, value);
        while (mSmithPredictor.size() < mCurrentOptions->predictorSize)
            mSmithPredictor.append(QPointF(0.0, 0.0));
        while (mSmithPredictor.size() > mCurrentOptions->predictorSize)
            mSmithPredictor.removeLast();
    }

    void Controller::setPixelSize(QPointF value)
    {
        this->setPixelSizeX(value.x());
        this->setPixelSizeY(value.y());
    }

    void Controller::setPixelSizeX(double value)
    {
        if (value != mCurrentOptions->pixelSize.x())
        {
            mCurrentOptions->pixelSize.rx() = value;
            emit pixelSizeXChanged(mCurrentOptions->pixelSize.x());
        }
    }

    void Controller::setPixelSizeY(double value)
    {
        if (value != mCurrentOptions->pixelSize.y())
        {
            mCurrentOptions->pixelSize.ry() = value;
            emit pixelSizeYChanged(mCurrentOptions->pixelSize.y());
        }
    }

    void Controller::setFFTImageSize(QSize value)
    {
        this->setFFTImageSizeX(value.width());
        this->setFFTImageSizeY(value.height());
    }

    void Controller::setFFTImageSizeX(int value)
    {
        value = qMax(value, 1);
        if (value != mCurrentOptions->fftImageSize.width())
        {
            mCurrentOptions->fftImageSize.setWidth(value);
            this->updateCorrelator();
        }
    }

    void Controller::setFFTImageSizeY(int value)
    {
        value = qMax(value, 1);
        if (value != mCurrentOptions->fftImageSize.height())
        {
            mCurrentOptions->fftImageSize.setHeight(value);
            this->updateCorrelator();
        }
    }

    void Controller::setCorrelatorDepth(int value)
    {
        value = qMax(value, 1);
        if (value != mCurrentOptions->correlatorDepth)
        {
            mCurrentOptions->correlatorDepth = value;
            this->updateCorrelator();
        }
    }

    void Controller::setTunerTimeToWaitAfterStartup(int value)
    {
        mTunerTimeToWaitAfterStartup = qMax(0, value);
    }

    void Controller::setTunerTimeToWaitForMoveEnd(int value)
    {
        mTunerTimeToWaitForMoveEnd = value == -1 ? INT_MAX : value;
        mTunerTimeToWaitForMoveEnd = qMax(0, mTunerTimeToWaitForMoveEnd);
    }

    void Controller::setTunerTimeout(int value)
    {
        mTunerTimeout = value == -1 ? INT_MAX : value;
        mTunerTimeout = qMax(0, mTunerTimeout);
    }

    void Controller::setTunerMinimumPixelsForMeasure(int value)
    {
        mTunerMinimumPixelsForMeasure = qMax(1, value);
    }

    void Controller::setTunerMeasureMovePercentage(double value)
    {
        mTunerMeasureMovePercentage = qMax(0.0, value);
    }

    void Controller::setMaxProcessDelay(int value)
    {
        mMaxProcessDelay = qMax(0, value);
    }

    void Controller::setExposureTime(double exposureTime)
    {
        mCameraExposureTime = exposureTime;
        mFocusTracker->setCameraExposureTime(exposureTime);
    }

    void Controller::setCameraImageSize(QSize imageSize)
    {
        mCameraImageSize = imageSize;
    }

    void Controller::setPressureisChanged(bool change){
        mPressureChanged = change;
        ///one way approach
        //trackerTimerBetween->singleShot(round(mTimestampDuration/2),this,SLOT(start))
        /// second way approach
    }

    void Controller::setLastPressureChangePassed(bool enable, bool modemultishots) {
        mLastPressureChangePassed = enable;/// not in use
        mLastPressureChangePassedTimestamp = mClock.getTime(); /// not in use

        // Distingiush between Multishoot or single shoot
        if (modemultishots){
        // multishot timer in 1t that toggles XYZ tracking on and off
            trackerTimerXY->singleShot(2*mTimestampDuration, this, SLOT(startZstackAcquisitionOscillation())); // 0.5% strain was okay. 1 sec later
            //trackerTimerXY->singleShot(6*mTimestampDuration, this, SLOT(startZstackAcquisitionOscillation())); // 5% strain was okay. 1 sec later
        }else{
        // single shot timer in 1t that toggles XY tracking on and off
            trackerTimerXY->singleShot(2*mTimestampDuration, this, SLOT(startXYOscillation()));
        }
        /*
        if (Zstack){
            trackerTimerXY->singleShot(mTimestampDuration, this, SLOT(startZstackAcquisitionOscillation()));
        }else{
            trackerTimerXY->singleShot(2*mTimestampDuration, this, SLOT(startXYOscillation()));
        }
        */

    }

    void Controller::setStorageFilename(QString filename) {
        mStorageFilename = filename;
    }

    void Controller::setCorrectionFactor(double value){
        mCorrectionFactor = value;
    }

    void Controller::setTimestepDuration(double value){
        mTimestampDuration = value;
    }

    void Controller::setAutofocusModeAfterLastPressureChange(bool enable){
        mInAutofocusModeAfterLastPressureChange = enable;
    }

    void Controller::setXYTrackerDurationOscillation(quint64 interval){
        mXYTrackerDurationOscillation = interval;
    }

    void Controller::setMaxAmplitude(double value){
        mMaxAmplitude = value;
    }

    void Controller::setNumberOfPressureSteps(double value){
        mNumberOfPressureSteps = value;
    }

    void Controller::setBrennerRoiPercentage(int precentage)
    {
        mBrennerRoiPercentage = precentage;
    }

    QPointF Controller::stageCoordinates(QPointF imageCoordinates) const
    {
        return QPointF(imageCoordinates.x() * mCurrentOptions->pixelSize.x(),
                       imageCoordinates.y() * mCurrentOptions->pixelSize.y());
    }

    QPointF Controller::imageCoordinates(QPointF stageCoordinates) const
    {
        return QPointF(stageCoordinates.x() / mCurrentOptions->pixelSize.x(),
                       stageCoordinates.y() / mCurrentOptions->pixelSize.y());
    }

    /*staic*/ double Controller::vectorLength(QPointF vector)
    {
        return std::sqrt(vector.x() * vector.x() + vector.y() * vector.y());
    }

    void Controller::readSettings()
    {
        QSettings settings(PathConfig::getConfigFilename(), QSettings::IniFormat);
        settings.beginGroup("Controller");

        setMaxTotalStageMove           (settings.value("Maximum_Stage_Move",                5000.0).toDouble());
        setMinOffset                   (settings.value("Minimum_Offset",                       4.0).toDouble());
        setTuningStep                  (settings.value("Tuning_Step",                         40.0).toDouble());
        setTunerTimeToWaitAfterStartup (settings.value("Tuner_Time_To_Wait_After_Startup", 1000000).toInt());
        setTunerTimeToWaitForMoveEnd   (settings.value("Tuner_Time_To_Wait_For_Move_End",   300000).toInt());
        setTunerTimeout                (settings.value("Tuner_Timeout",                    5000000).toInt());
        setTunerMinimumPixelsForMeasure(settings.value("Tuner_Minimum_Pixels_For_Measure",      10).toInt());
        setTunerMeasureMovePercentage  (settings.value("Tuner_Measure_Move_Percentage",        0.2).toDouble());
        setMaxProcessDelay             (settings.value("Max_Process_Delay",                   3000).toInt());
    }

    void Controller::writeSettings()
    {
        QSettings settings(PathConfig::getConfigFilename(), QSettings::IniFormat);
        settings.beginGroup("Controller");

        settings.setValue("Maximum_Stage_Move",               mMaxTotalStageMove);
        settings.setValue("Minimum_Offset",                   mMinOffset);
        settings.setValue("Tuning_Step",                      mTuningStep);
        settings.setValue("Tuner_Time_To_Wait_After_Startup", mTunerTimeToWaitAfterStartup);
        settings.setValue("Tuner_Time_To_Wait_For_Move_End",  mTunerTimeToWaitForMoveEnd);
        settings.setValue("Tuner_Timeout",                    mTunerTimeout);
        settings.setValue("Tuner_Minimum_Pixels_For_Measure", mTunerMinimumPixelsForMeasure);
        settings.setValue("Tuner_Measure_Move_Percentage",    mTunerMeasureMovePercentage);
        settings.setValue("Max_Process_Delay",                mMaxProcessDelay);
    }

    void Controller::readSettings(QString settingsKey, QSize maxSize)
    {
        QSettings settings(PathConfig::getConfigFilename(), QSettings::IniFormat);
        QString revisedKey = settingsKey.replace('\\', "__fwd_sl__").replace('/', "__bwd_sl__");
        settings.beginGroup("Controller/" + revisedKey);

        setFFTImageSize     (settings.value("FFT_Image_Size",  maxSize).toSize());
        setControllerGain   (settings.value("Controller_Gain",     0.5).toDouble());
        setStageCommandDelay(settings.value("Stage_Command_Delay", 0.0).toDouble());
        setCorrelatorDepth  (settings.value("Correlator_Depth",    2  ).toInt());
        setPredictorSize    (settings.value("Predictor_Size",      3  ).toInt());
        setPixelSizeX       (settings.value("Pixel_Size_X",        1.0).toDouble());
        setPixelSizeY       (settings.value("Pixel_Size_Y",        1.0).toDouble());
    }

    void Controller::writeSettings(QString settingsKey)
    {
        OptionSet* options = mOptions[settingsKey];
        QSettings settings(PathConfig::getConfigFilename(), QSettings::IniFormat);
        QString revisedKey = settingsKey.replace('\\', "__fwd_sl__").replace('/', "__bwd_sl__");
        settings.beginGroup("Controller/" + revisedKey);

        settings.setValue("FFT_Image_Size",      options->fftImageSize);
        settings.setValue("Controller_Gain",     options->controllerGain);
        settings.setValue("Stage_Command_Delay", options->stageCommandDelay);
        settings.setValue("Correlator_Depth",    options->correlatorDepth);
        settings.setValue("Predictor_Size",      options->predictorSize);
        settings.setValue("Pixel_Size_X",        options->pixelSize.x());
        settings.setValue("Pixel_Size_Y",        options->pixelSize.y());
    }

}
