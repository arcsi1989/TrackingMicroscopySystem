/*
 Copyright (c) 2009-2012, Reto Grieder, Benjamin Beyeler and Andreas Ziegler
 Copyright (c) 2014, Tobias Klauser

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#include <QtGui>
#include <QtCore/QCoreApplication>
#include "MainWindow.h"
#include "StoragePath.h"

#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <algorithm>
#include <QDateTime>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QThread>
#include <QLCDNumber>
#include <QMetaType>
#include <QTimer>
#include <QtConcurrentRun>

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
/*
#include "../../dependencies/include/qtserialport/include/QSerialPort"
#include "../../dependencies/include/qtserialport/include/QSerialPortInfo"
*/


#include "gauge/qtsvgdialgauge.h"

#include "Camera.h"
#include "Controller.h"
#include "DraggableLabel.h"
#include "Exception.h"
#include "FocusTracker.h"
#include "Logger.h"
#include "PathConfig.h"
#include "PressureSensor.h"
#include "Stage.h"
#include "TimeSpinBox.h"

#ifdef TRACKER_DUMMY
#include "dummy/DummyCamera.h"
#include "dummy/DummyMicroscope.h"
#include "dummy/DummyStage.h"
#endif

#include <iostream>
#include <QDebug>

namespace tracker
{
  MainWindow::MainWindow()
    : mDiscardStageStepValue(false)
    , mDiscardExposureTimeValue(false)
    , mDiscardGainValue(false)
    , mIgnoreFFTImageSizeChanges(false)
    , mInitialised(false)
    , mStoringImage(false)
    , mPreviousDisplayModeIndex(-1)
    , mCurrentImageArea(0)
    , mMaxSliderExposureTime(10000)
    , timer100ms(0)
    , pictureHeight(300)
    , pictureWidth(500)
    , graphOn(false)
    , mStoragePath(NULL)
    , storageRunSpinBoxValue(0)
    , Counter(0)
    , NumOfImages(0)
    , mMainChannel(4)
    , mAction(0)
  {
    /***************************** Create UI ******************************/

    qRegisterMetaType<FocusValue>("FocusValue");

    // Initialize first "time point"
    mTimeValues.append(0);

    // QMenu's
    //  View menu's
    acdeacGraphAction = new QAction(tr("Graph"), this);
    acdeacGraphAction->setCheckable(true);
    acdeacGraphAction->setChecked(true);

    acdeacLinMotAction = new QAction(tr("Linear Motor"), this);
    acdeacLinMotAction->setCheckable(true);
    acdeacLinMotAction->setChecked(true);

    acdeacLoggerAction = new QAction(tr("Logger"), this);
    acdeacLoggerAction->setCheckable(true);
    acdeacLoggerAction->setChecked(true);

    acdeacCameraAction = new QAction(tr("Camera"), this);
    acdeacCameraAction->setCheckable(true);
    acdeacCameraAction->setChecked(true);

    acdeacTrackerAction = new QAction(tr("Tracker"), this);
    acdeacTrackerAction->setCheckable(true);
    acdeacTrackerAction->setChecked(true);

    acdeacTrackerOptionsAction = new QAction(tr("Tracker Options"), this);
    acdeacTrackerOptionsAction->setCheckable(true);
    acdeacTrackerOptionsAction->setChecked(true);

    acdeacStageAction = new QAction(tr("Stage"), this);
    acdeacStageAction->setCheckable(true);
    acdeacStageAction->setChecked(true);

    acdeacPressureAction = new QAction(tr("Pressure"), this);
    acdeacPressureAction->setCheckable(true);
    acdeacPressureAction->setChecked(true);

    acdeacStorageAction = new QAction(tr("Storage path"), this);
    acdeacStorageAction->setCheckable(true);
    acdeacStorageAction->setChecked(true);

    acdeacPositionListAction = new QAction(tr("Position List"), this);
    acdeacPositionListAction->setCheckable(true);
    acdeacPositionListAction->setChecked(true);

    acdeacFocusInformationAction = new QAction(tr("Focus Information"), this);
    acdeacFocusInformationAction->setCheckable(true);
    acdeacFocusInformationAction->setChecked(true);

    acdeacProtocolsAction = new QAction(tr("Protocols"), this);
    acdeacProtocolsAction->setCheckable(true);
    acdeacProtocolsAction->setChecked(true);

    mViewMenu = menuBar()->addMenu(tr("View"));
    mViewMenu->addAction(acdeacGraphAction);
    mViewMenu->addAction(acdeacLinMotAction);
    mViewMenu->addAction(acdeacLoggerAction);
    mViewMenu->addAction(acdeacCameraAction);
    mViewMenu->addAction(acdeacTrackerAction);
    mViewMenu->addAction(acdeacTrackerOptionsAction);
    mViewMenu->addAction(acdeacStageAction);
    mViewMenu->addAction(acdeacPressureAction);
    mViewMenu->addAction(acdeacStorageAction);
    mViewMenu->addAction(acdeacPositionListAction);
    mViewMenu->addAction(acdeacFocusInformationAction);
    mViewMenu->addAction(acdeacProtocolsAction);

    connect(acdeacGraphAction,  SIGNAL(triggered()),
            this,                SLOT(acdeacGraph()));
    connect(acdeacLinMotAction, SIGNAL(triggered()),
            this,               SLOT(acdeacLinMot()));
    connect(acdeacLoggerAction, SIGNAL(triggered()),
            this,               SLOT(acdeacLogger()));
    connect(acdeacCameraAction, SIGNAL(triggered()),
            this,               SLOT(acdeacCamera()));
    connect(acdeacTrackerAction,  SIGNAL(triggered()),
            this,                 SLOT(acdeacTracker()));
    connect(acdeacTrackerOptionsAction, SIGNAL(triggered()),
            this,                       SLOT(acdeacTrackerOptions()));
    connect(acdeacStageAction,  SIGNAL(triggered()),
            this,               SLOT(acdeacStage()));
    connect(acdeacPressureAction, SIGNAL(triggered()),
            this,                 SLOT(acdeacPressure()));
    connect(acdeacPositionListAction, SIGNAL(triggered()),
            this,                     SLOT(acdeacPositionList()));
    connect(acdeacFocusInformationAction, SIGNAL(triggered()),
            this,                         SLOT(acdeacFocusInformation()));
    connect(acdeacProtocolsAction,  SIGNAL(triggered()),
            this,                   SLOT(acdeacProtocols()));

    //  Settings menu
    mStoragePathAction = new QAction(tr("Storage path"), this);
    mComPortAction = new QAction(tr("Com Ports"), this);
    mScaleFactorAction = new QAction(tr("Scale factor"), this);

    mSettingsMenu = menuBar()->addMenu(tr("Settings"));
    mSettingsMenu->addAction(mStoragePathAction);
    mSettingsMenu->addAction(mComPortAction);
    mSettingsMenu->addAction(mScaleFactorAction);

    connect(mStoragePathAction,  SIGNAL(triggered()),
            this,               SLOT(storagePathWindow()));
    connect(mComPortAction,  SIGNAL(triggered()),
            this,           SLOT(comPortWindow()));
    connect(mScaleFactorAction, SIGNAL(triggered()),
            this,               SLOT(scaleFactorWindow()));

    //  Stretcher menu
    mStretcherAction = menuBar()->addAction("Stretcher");

    connect(mStretcherAction, SIGNAL(triggered()),
            this,             SLOT(stretcherWindow()));

    // Central Image Label
    mImageLabel = new DraggableLabel(this);
    mImageLabel->setObjectName("mImageLabel");
    mImageLabel->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignTop);

    //Graph image label
    mGraphLabel = new DraggableLabel(this);
    mGraphLabel->setObjectName("mGraphLabel");
    mGraphLabel->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignTop);

    // Pressure gauge
    /*
    mPressureGauge = new QtSvgDialGauge(this);
    mPressureGauge->setObjectName("mPressureGauge");
    mPressureGauge->setMinimumSize(QSize(150, 150));
    mPressureGauge->setSkin("Tachometer");
    mPressureGauge->setNeedleOrigin(0.486, 0.466);
    mPressureGauge->setMinimum(0);
    mPressureGauge->setMaximum(600);
    mPressureGauge->setStartAngle(-130);
    mPressureGauge->setEndAngle(133);
    mPressureGauge->setMaximumSize(150, 150);
    mPressureGauge->setShowOverlay(false);
    mPressureGauge->setValue(0);
    */
    // Pressure plot
    /*
    mPressurePlot = new QCustomPlot(this);
    mPressurePlot->setObjectName(QString::fromUtf8("mPressurePlot"));
    mPressurePlot->addGraph();
    mPressurePlot->xAxis->setRange(0, 30);
    mPressurePlot->yAxis->setRange(0, 5);

    mPressurePlot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCross));
    mPressurePlot->xAxis->setLabel("Time");
    mPressurePlot->yAxis->setLabel("Pressure [kPa]");
    mPressurePlot->xAxis->setAutoTickStep(false);
    mPressurePlot->xAxis->setTickStep(1);
    mPressurePlot->setInteraction(QCP::iRangeDrag, true);
    mPressurePlot->setInteraction(QCP::iRangeZoom, true);

    connect(mPressurePlot, SIGNAL(mouseDoubleClick(QMouseEvent*)),
             this,        SLOT(exportPressure()));
    */

    // Exposure Time SpinBox
    mExposureTimeBox = new TimeSpinBox(this);
    mExposureTimeBox->setObjectName("mExposureTimeBox");
    mExposureTimeBox->setMinimumWidth(62);
    mExposureTimeBox->setToolTip("Camera exposure time (use 's', 'ms' and 'us' suffixes)");
    // Status bar labels
    mCameraFrameRateLabel = new QLabel("0", this);
    mCameraFrameRateLabel->setMinimumWidth(40);
    mCameraFrameRateLabel->setAlignment(Qt::AlignRight);
    mCameraFrameRateLabel->setToolTip("Camera framerate");
    mControllerFrameRateLabel = new QLabel("0", this);
    mControllerFrameRateLabel->setMinimumWidth(40);
    mControllerFrameRateLabel->setAlignment(Qt::AlignRight);
    mControllerFrameRateLabel->setToolTip("Tracker framerate");
    mImageZoomLabel = new QLabel("1.00x", this);
    mImageZoomLabel->setMinimumWidth(50);
    mImageZoomLabel->setAlignment(Qt::AlignRight);
    mImageZoomLabel->setToolTip("Image zoom");
    // Signal mapper for single shot buttons
    mSingleShotButtonMapper = new QSignalMapper(this);
    mSingleShotButtonMapper->setObjectName("mSingleShotButtonMapper");
    // SpinBox validators
    mFFTSizeXValidator = new QIntValidator(1, 2048, this);
    mFFTSizeYValidator = new QIntValidator(1, 1536, this);
    // Shortcuts
    mNextRunShortcut        = new QShortcut(QKeySequence("F8"),  this);
    mSetShutterShortcut	= new QShortcut(QKeySequence(Qt::Key_Space),this);
    mSetChannel0Shortcut = new QShortcut(QKeySequence("t"),this);
    mSetChannel1Shortcut = new QShortcut(QKeySequence("q"),this);
    mSetChannel2Shortcut = new QShortcut(QKeySequence("w"),this);
    mSetChannel3Shortcut = new QShortcut(QKeySequence("e"),this);
    mSetChannel4Shortcut = new QShortcut(QKeySequence("r"),this);
    mSetMultiImagesShortcut = new QShortcut(QKeySequence("f"),this);
    mSetMultiChannelsShortcut = new QShortcut(QKeySequence("g"),this);
    // Tracker animation (GIF)
    mTrackerAnimation = new QMovie(":/tracker/icons/misc/busy.gif", QByteArray(), this);

    // Create and configure components from designer
    this->setupUi(this);
    // Add tableau
    mPoslistwidget = new QTableWidget();
    mPoslistwidget->setRowCount(100);
    mPoslistwidget->setColumnCount(21);
    mPoslistwidget->setHorizontalHeaderItem(0,new QTableWidgetItem("Name"));
    mPoslistwidget->setHorizontalHeaderItem(1,new QTableWidgetItem("x;y"));
    mPoslistwidget->setHorizontalHeaderItem(2,new QTableWidgetItem(""));
    mPoslistwidget->setHorizontalHeaderItem(3,new QTableWidgetItem("z"));
    mPoslistwidget->setHorizontalHeaderItem(4,new QTableWidgetItem(""));
    mPoslistwidget->setHorizontalHeaderItem(5,new QTableWidgetItem("Pressure"));
    mPoslistwidget->setHorizontalHeaderItem(6,new QTableWidgetItem(""));
    mPoslistwidget->setHorizontalHeaderItem(7,new QTableWidgetItem("Gain"));
    mPoslistwidget->setHorizontalHeaderItem(8,new QTableWidgetItem(""));
    mPoslistwidget->setHorizontalHeaderItem(9,new QTableWidgetItem("Exposure time(ms)"));
    mPoslistwidget->setHorizontalHeaderItem(10,new QTableWidgetItem(""));
    mPoslistwidget->setHorizontalHeaderItem(11,new QTableWidgetItem("Ilumination"));
    mPoslistwidget->setHorizontalHeaderItem(12,new QTableWidgetItem(""));
    mPoslistwidget->setHorizontalHeaderItem(13,new QTableWidgetItem("Cube"));
    mPoslistwidget->setHorizontalHeaderItem(14,new QTableWidgetItem(""));
    mPoslistwidget->setHorizontalHeaderItem(15,new QTableWidgetItem("Intensity"));
    mPoslistwidget->setHorizontalHeaderItem(16,new QTableWidgetItem(""));
    mPoslistwidget->setHorizontalHeaderItem(17,new QTableWidgetItem("Shutter"));
    mPoslistwidget->setHorizontalHeaderItem(18,new QTableWidgetItem(""));
    mPoslistwidget->setHorizontalHeaderItem(19,new QTableWidgetItem(" "));
    mPoslistwidget->setHorizontalHeaderItem(20,new QTableWidgetItem(" "));
    //mPoslistwidget->setHorizontalHeaderItem(17,new QTableWidgetItem(" "));

    mPoslist = new QList<ExperimentState*>;

    mProtocolstoprequest =false;
    // Add self made widgets
    centralScrollArea->setWidget(mImageLabel);
    GraphScrollArea->setWidget(mGraphLabel);

    /*
    pressureVL->insertWidget(0, mPressureGauge);
    pressureVL->insertWidget(0, mPressurePlot);
    */
    poslistLayout->insertWidget(0,mPoslistwidget);
    cameraGL_1->addWidget(mExposureTimeBox, 1, 2);
    this->statusBar()->addPermanentWidget(mCameraFrameRateLabel);
    this->statusBar()->addPermanentWidget(mControllerFrameRateLabel);
    this->statusBar()->addPermanentWidget(mImageZoomLabel);
    fftImageSizeXBox->setValidator(mFFTSizeXValidator);
    fftImageSizeYBox->setValidator(mFFTSizeYValidator);
    trackerBusyLabel->setMovie(mTrackerAnimation);
    // Fix tab order
    QWidget::setTabOrder(exposureTimeSlider, mExposureTimeBox);

    // Connect Logger output with text window
    // Don't do this only later because important messages could be lost
    this->connect(&Logger::getInstance(), SIGNAL(messageLogged(QString)),
                  logTextBrowser,         SLOT(append(QString)));

    // Liniar motor

    chooseEF->addItem("Force");
    chooseEF->addItem("Expansion");
    chooseMode->addItem("Until");
    chooseMode->addItem("Hold");
    stretchUntilValueBox->setSuffix(" N");

    mAutoStretch = new AutoStretch(this);
    mLinearMotor = new LinearMotor(this, mAutoStretch);
    mLinearMotor->createMessagesHandler();

    graphWidget->addGraph();
    graphWidget->xAxis->setRange(0, 30);
    graphWidget->yAxis->setRange(-5, 0);

    graphWidget->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCross));
    graphWidget->xAxis->setLabel("Length [mm]");
    graphWidget->yAxis->setLabel("Force [N]");
    graphWidget->setInteraction(QCP::iRangeDrag, true);
    graphWidget->setInteraction(QCP::iRangeZoom, true);

    /*connect(chooseMode,   SIGNAL(currentIndexChanged(int)),
            mAutoStretch, SLOT(updateAutoStretchMode(int)));
    */
    connect(this,         SIGNAL(setZeroDistance(double)),
            mLinearMotor, SLOT(setZeroDistance(double)));
    connect(chooseEF,     SIGNAL(currentIndexChanged(int)),
            mAutoStretch, SLOT(updateAutoStretchUnit(int)));
    connect(mAutoStretch, SIGNAL(setStretchUntilValueSuffix(QString)),
            this,         SLOT(setStretchUntilValueSuffix(QString)));
    connect(stretchUntilValueBox, SIGNAL(valueChanged(double)),
            mAutoStretch,         SLOT(updateAutoStretchValue(double)));
    connect(stopStretchButton,  SIGNAL(clicked()),
            mAutoStretch,       SLOT(stop()));

    connect(forceThreshold, SIGNAL(valueChanged(double)),
            mAutoStretch,   SLOT(setForceThreshold(double)));
    connect(amountStdSamples,  SIGNAL(valueChanged(double)),
            mAutoStretch,   SLOT(setAmStdForceSamples(double)));
    connect(amountZeroSamples,  SIGNAL(valueChanged(double)),
            mAutoStretch,       SLOT(setAmZeroForceSamples(double)));
    connect(forceLowThreshold,  SIGNAL(valueChanged(double)),
            mAutoStretch,       SLOT(setChangeForceLowThreshold(double)));
    connect(forceHighThreshold, SIGNAL(valueChanged(double)),
            mAutoStretch,       SLOT(setChangeForceHighThreshold(double)));
    connect(this,           SIGNAL(updateAutoStretchMode(int)),
            mAutoStretch,   SLOT(updateAutoStretchMode(int)));

    connect(mLinearMotor, SIGNAL(speedChanged(int)),
            currentSpeed, SLOT(display(int)));
    connect(mAutoStretch, SIGNAL(updateMeasuredPoints(double, double)),
            this,         SLOT(updateMeasuredPoints(double, double)));
    connect(graphWidget, SIGNAL(mouseDoubleClick(QMouseEvent*)),
             this,        SLOT(exportPng()));


    /************************* Create Components **************************/

    // Note: Camera and Controller do not have parents because they need
    //       to be moved to another thread.
    mCamera.reset(Camera::makeCamera());
    // Get camera mode strings
    QVector<QPair<QString, QSize> > modes;
    foreach (QString modeName, mCamera->getAvailableModes())
        modes.append(qMakePair(mCamera->getName() + "__sep__" + modeName, mCamera->getImageSize(modeName)));

    mController.reset(new Controller(modes));

    // Note: Controller parent is required so that moveToThread works correctly
    mStage = Stage::makeStage(mController.get());

    mPressureSensor = new PressureSensor(this);
    mAhmmicroscope = new ahmMicroscope();
    // Display the shutter state and the illumination mode
    switch(mAhmmicroscope->getFlShutterState()){
      case 0:
        shutterLabel->setText("close");
        break;
      case 1:
        shutterLabel->setText("open");
        break;
    }
    switch(mAhmmicroscope->getContrastingMethod()){
      case ahmMicroscope::ilFluorescence:
        illuminationLabel->setText("incident");
        break;
      case ahmMicroscope::ilWhiteLight:
        illuminationLabel->setText("transmitted");
        break;
    }

    //Creating the lamp
    mLamp = new Lamp();
    channel = 0;
    intensity = 0;

    mDac = Dac::makeDac(this);
    mDac->setvoltage(5);	// initialize to 5V which corresponds to 0kPa
    tickcounter =0;			// initialize flags for pressure oscillation
    rising =true;
    dactimerID = 0;

    mSafety = true;
    mCurrentlistposition=0;
    //mKeepinmemory=false;

    mProtocolwidget=protocollistWidget;

    cameratimer = new QTimer(this);

    // Create threads
    mCameraThread     = new Thread(mCamera.get(), this);
    mControllerThread = new Thread(mController.get(), this);

    /********************* Connect and Configure UI ***********************/
    // Note: Some connections are automatic and therefore not listed here!

    connect(mCameraThread,     SIGNAL(finished()), this, SLOT(cameraFinished()));
    connect(mControllerThread, SIGNAL(finished()), this, SLOT(controllerFinished()));

    // --- Storage ---
/**
    connect(this,              SIGNAL(singleShotRequested(QString, int, double)),
            mCamera.get(),     SLOT(takeSingleShot(QString, int, double)));
    connect(this,              SIGNAL(multipleShotRequested(QString, int, double)),
            mCamera.get(),     SLOT(takeMultipleShot(QString, int, double)));
            */
    connect(this,              SIGNAL(RecordMW(QString, int, double,int, double,int, double)),
            mCamera.get(),     SLOT(Record(QString, int, double,int, double,int, double)));
    connect(mCamera.get(),     SIGNAL(singleShotTaken(QImage, quint64, quint64)),
            this,              SLOT(storeSingleShot(QImage, quint64, quint64)));
    //Temporal testing of the OFFLINE Zstack acquisition
    connect(mController.get(),     SIGNAL(OffLineZstackImage(QImage, quint64, quint64)), //20180322
            this,              SLOT(storeSingleShot(QImage, quint64, quint64)));

   // connect(mController,       SIGNAL(singleShotRequested(QString, int, double)),
   //         mCamera.get(),     SLOT(takeSingleShot(QString, int, double)));

    connect(mNextRunShortcut,  SIGNAL(activated()),
            storageRunSpinBox, SLOT(stepUp()));

    // --- Camera ---

    // Fill combo box with camera modes
    cameraModeBox->addItems(mCamera->getAvailableModes());

    // Set exposure time and gain range
    //mExposureTimeBox->setMinimum(mCamera->getExposureTimeRange().first);
    mExposureTimeBox->setMinimum(1000);
    mExposureTimeBox->setMaximum(mCamera->getExposureTimeRange().second);

    gainBox->setMinimum(mCamera->getGainRange().first);
    gainBox->setMaximum(mCamera->getGainRange().second);

    connect(this,                 SIGNAL(cameraModeChanged(QString, int, double)),
            mCamera.get(),        SLOT(setMode(QString, int, double)));
    connect(mExposureTimeBox,     SIGNAL(valueChanged(int)),
            mCamera.get(),        SLOT(setExposureTime(int)));
   // connect(mExposureTimeBox,     SIGNAL(valueChanged(int)),
   //         this,        SLOT(SetAction()));
    connect(gainBox,              SIGNAL(valueChanged(double)),
            mCamera.get(),        SLOT(setGain(double)));
    //connect(gainBox,     SIGNAL(valueChanged(double)),
    //        this,        SLOT(SetAction()));
    connect(mCamera.get(),        SIGNAL(frameRateUpdated(double)),
            this,                 SLOT(displayCameraFrameRate(double)));

    //connect(cameratimer,        SIGNAL(timeout()),
    //       this,                SLOT(shootautopicture()));

    // --- Tracker & Options ---

    connect(maxStageMoveBox,      SIGNAL(valueChanged(double)),
            mController.get(),    SLOT(setMaxTotalStageMove(double)));
    connect(tuningStepBox,        SIGNAL(valueChanged(double)),
            mController.get(),    SLOT(setTuningStep(double)));
    connect(pixelSizeXBox,        SIGNAL(valueChanged(double)),
            mController.get(),    SLOT(setPixelSizeX(double)));
    connect(pixelSizeYBox,        SIGNAL(valueChanged(double)),
            mController.get(),    SLOT(setPixelSizeY(double)));
    connect(controllerGainBox,    SIGNAL(valueChanged(double)),
            mController.get(),    SLOT(setControllerGain(double)));
    connect(stageCommandDelayBox, SIGNAL(valueChanged(double)),
            mController.get(),    SLOT(setStageCommandDelay(double)));
    connect(correlatorDepthBox,   SIGNAL(valueChanged(int)),
            mController.get(),    SLOT(setCorrelatorDepth(int)));
    connect(predictorSizeBox,     SIGNAL(valueChanged(int)),
            mController.get(),    SLOT(setPredictorSize(int)));

    connect(mController.get(),    SIGNAL(pixelSizeXChanged(double)),
            pixelSizeXBox,        SLOT(setValue(double)));
    connect(mController.get(),    SIGNAL(pixelSizeYChanged(double)),
            pixelSizeYBox,        SLOT(setValue(double)));

    connect(mController.get(),    SIGNAL(validityChanged()),
            this,                 SLOT(updateWidgetActivities()));

    connect(mController.get(),    SIGNAL(frameRateUpdated(double)),
            this,                 SLOT(displayControllerFrameRate(double)));

    connect(mController.get(),    SIGNAL(debugImage(QImage)),
            mGraphLabel,          SLOT(displayImage(QImage)));

    // --- TimeLapseIndicator
    connect(mController.get(),    SIGNAL(TimeLapseCounterUpdated(QString)),
            this,                 SLOT(TimeLapseCounterUpdated(QString)));

    // --- Saving method during time lapse
    connect(mController.get(),    SIGNAL(SaveImages()),
            this,                 SLOT(saveBuffertoHarddisk()));

    // --- Z Tracker ---

    // display current focus values for a newly processed image
    connect(mController.get(),    SIGNAL(focusUpdated(const FocusValue &)),
            this,                 SLOT(displayFocus(const FocusValue &)));
    // display current stage position if the stage was moved vertically
    connect(mController.get(),    SIGNAL(zmoved(double)),
            this,                 SLOT(displayZpos(double)));

    connect(mController.get(),    SIGNAL(takeAutoPicture()),
            this,                 SLOT(shootautopicture()));

    connect(mController.get(),    SIGNAL(takeHighResolutionHighIntensityPicture()),
            this,                 SLOT(shoothighreshighintensitypicture()));

    // move stage vertically on request from one of the UI elements or mouse wheel
    connect(this,                 SIGNAL(ZMoveRequested(double, int)),
            mController.get(),    SLOT(moveStageZ(double, int)));
    // move stage vertically on request from the Camera in order to aquire Zsteck (based on the number of recieved images
    connect(mCamera.get(),        SIGNAL(ZSTEP(double, int)),
            mController.get(),    SLOT(moveStageZ(double, int)));
    // informing tracker about the finished Zstack aquisition
    connect(mCamera.get(),        SIGNAL(finishedZstack()),
            mController.get(),    SLOT(switchOnXYZ()));
    // move stage back to its original position before imaging.
    connect(mCamera.get(),        SIGNAL(SetStage()),
            this,    SLOT(SetStageBack()));
    // move stage vertically if the pressure was changed by the pressure ramp algorithm
    connect(this,                 SIGNAL(pressureStepped(float)),
            this,                 SLOT(adjustStageZ(float)));

    // Update on stage motion
    connect(mController.get(),     SIGNAL(StageisMOVED()),
            this,                   SLOT(SetAction()));


    // --- Pressure Control ---

    connect(this,                 SIGNAL(pressureSetRequested(float)),
            this,                 SLOT(setPressure(float)));

    // --- Time Lapse ---
    connect(TimeLapse,   SIGNAL(clicked(bool)),
            this, SLOT(SetTimeLapse(bool)));

    // Shutter Shortcut
    connect(mSetShutterShortcut, SIGNAL(activated()),
            this, SLOT(on_toggleshutterButton_pressed()));

    // Lamp Shortcut
    connect(mSetChannel0Shortcut, SIGNAL(activated()),
            this, SLOT(on_Channel_0_clicked()));
    connect(mSetChannel1Shortcut, SIGNAL(activated()),
            this, SLOT(on_Channel_1_clicked()));
    connect(mSetChannel2Shortcut, SIGNAL(activated()),
            this, SLOT(on_Channel_2_clicked()));
    connect(mSetChannel3Shortcut, SIGNAL(activated()),
            this, SLOT(on_Channel_3_clicked()));
    connect(mSetChannel4Shortcut, SIGNAL(activated()),
            this, SLOT(on_Channel_4_clicked()));

    //Multichannel-Imaging shortcuts
    connect(mSetMultiImagesShortcut, SIGNAL(activated()),
            this, SLOT(MultiImageToggle()));
    connect(mSetMultiChannelsShortcut, SIGNAL(activated()),
            this, SLOT(MultiChannelToggle()));

    // --- Stage ---

    connect(this,                 SIGNAL(stageMoveRequested(QPointF)),
            mStage,               SLOT(move(QPointF)));

    // --- Pressure Gauge ---

    connect(mPressureSensor,      SIGNAL(pressureUpdated(double)),
            this,                 SLOT(forceUpdated(double)));

    connect(mImageLabel,          SIGNAL(labelrequestszmove(double)),
            this,                 SLOT(wheelzmove(double)));

    mInitialised = true;

    // Read settings from file and apply them
    this->readSettings();

    // Initialise the controller's correlator here because it's quite
    // computationally intensive (and filling the UI triggers updates)
    mController->initialise(mStage);

    // Just in case it hasn't been done yet
    this->updateWidgetActivities();

    // Connect these only later to avoid problems when filling the combo boxes
    this->connect(cameraModeBox,  SIGNAL(currentIndexChanged(int)),
                  SLOT(cameraModeBox_currentIndexChanged(int)));
    this->connect(displayModeBox, SIGNAL(currentIndexChanged(int)),
                  SLOT(displayModeBox_currentIndexChanged(int)));
    // Update values
    displayModeBox_currentIndexChanged(displayModeBox->currentIndex());
    cameraModeBox_currentIndexChanged(cameraModeBox->currentIndex());

    // Connencting the Lamp with MainWindow
    connect(Channel_0, SIGNAL(clicked()),
    this, SLOT(on_Channel_0_clicked()));
    connect(Channel_1, SIGNAL(clicked()),
    this, SLOT(on_Channel_1_clicked()));
    connect(Channel_2, SIGNAL(clicked()),
    this, SLOT(on_Channel_2_clicked()));
    connect(Channel_3, SIGNAL(clicked()),
    this, SLOT(on_Channel_3_clicked()));
    connect(Channel_4, SIGNAL(clicked()),
    this, SLOT(on_Channel_4_clicked()));
    // Connencting the Lamp with Controller
    connect(mController.get(),SIGNAL(LightOFF()),
            this, SLOT(on_Channel_0_clicked()));
    connect(mController.get(),SIGNAL(LightON()),
            this, SLOT(Turn_ON()));
    //connect(SetChannel, SIGNAL(clicked()),
    //this, SLOT(testgrouping()));

    //Connectin camera to Lamp;
    connect(mCamera.get(),SIGNAL(SettingFromCameraTheLamp(int,double)),
            this,SLOT(SetLampFromCameraSignal(int, double)));
    connect(mCamera.get(),SIGNAL(SettingFromCameraTheLamp(int,double)),
            this,SLOT(SetChannelandIntensityValuesforSavedimages(int, double)));
   // connect(mCamera.get(),SIGNAL(AllowLightForTheNextFrame()),
   //        this,SLOT(TriggerFromCameraForFlash()));


    connect(uProgram, SIGNAL(clicked()),
    this, SLOT(on_StartMP_clicked()));


#ifdef TRACKER_DUMMY
    mMicroscope.reset(new DummyMicroscope(static_cast<DummyStage*>(mStage)));

    mMicroscopeThread = new Thread(mMicroscope.get(), this);
    mMicroscope->moveToThread(mMicroscopeThread);
    // Start the microscope in its own thread
    mMicroscopeThread->start();
    // Warning: Exceptions thrown after start() will generate a crash!
#endif
  }

  MainWindow::~MainWindow()
  {
#ifdef TRACKER_DUMMY
    mMicroscopeThread->quit();
    if (!mMicroscopeThread->wait(1000))
      mMicroscopeThread->terminate();
#endif

    mCameraThread->quit();
    if (!mCameraThread->wait(1000))
      mCameraThread->terminate();
    mControllerThread->quit();
    if (!mControllerThread->wait(2000))
      mControllerThread->terminate();

    // Store all settings
    this->writeSettings();
  }

  /**************************************************************************/
  /****************************** Menu's ************************************/
  /**************************************************************************/

  /**
   * @brief Creates and brings up the "Storage path" menu and make the required connections.
   */
  void MainWindow::storagePathWindow() {
    mStoragePath = new Ui::StoragePath();
    mStoragePathDialog = new QDialog;
    mStoragePath->setupUi(mStoragePathDialog);
    mStoragePathDialog->show();

    connect(mStoragePath->settingsStorageFilenameEdit,  SIGNAL(textChanged(QString)),
            this,                                       SLOT(on_settingsStorageFilenameEdit_textChanged(QString)));
    connect(mStoragePath->settingsStorageLocationButton,  SIGNAL(clicked()),
            this,                                         SLOT(on_settingsStorageLocationButton_clicked()));
    connect(mStoragePath->settingsStorageRunSpinBox,  SIGNAL(valueChanged(int)),
            this,                                     SLOT(on_settingsStorageRunSpinBox_valueChanged(int)));
    connect(mStoragePath->storagePathCancelButton,  SIGNAL(clicked()),
            this,                                   SLOT(on_storagePathCancelButton_clicked()));
    connect(mStoragePath->storagePathOKButton,  SIGNAL(clicked()),
            this,                               SLOT(on_storagePathOKButton_clicked()));

    mStoragePath->settingsStorageRunSpinBox->setValue(storageRunSpinBoxValue);
    mStoragePath->settingsStorageFilenameEdit->setText(storageFilename);
  }

  /**
   * @brief Creates and brings up the "Com port" menu and makes the required connections.
   */
  void MainWindow::comPortWindow(){
    mComPort = new Ui::ComPorts;
    mComPortDialog = new QDialog;
    mComPort->setupUi(mComPortDialog);
    mComPortDialog->show();

    // Scan available com ports and save the number and name in the vector mComPortsStrings and add them to the ComboBoxes
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
      QString text = info.portName();
      text += " ";
      text += info.description();
      mComPortStrings.push_back(text);
      mComPort->linMotComPortComboBox->addItem(text);
      mComPort->converterComboBox->addItem(text);
    }

    // Read the com port for the pressure sensor from the tracker.ini file
    QSettings settings(PathConfig::getConfigFilename(), QSettings::IniFormat);
    settings.beginGroup("PressureSensor");
    QString pressurePort = settings.value("Port", "com6").toString();
    settings.endGroup();
    settings.beginGroup("LinearMotor");
    QString linMotPort =  settings.value("Port", "com3").toString();
    settings.endGroup();

    mComPort->linMotComPortComboBox->setCurrentIndex(0);
    mComPort->converterComboBox->setCurrentIndex(1);

    // Set the ConverterComboBox to the right index if a available com ports fits to the configuration file definition
    for(unsigned int i = 0; i < mComPortStrings.size(); ++i) {
      if(mComPortStrings[i].contains(pressurePort, Qt::CaseInsensitive)) {
        mComPort->converterComboBox->setCurrentIndex(i);
      } else if(mComPortStrings[i].contains(linMotPort, Qt::CaseInsensitive)) {
        mComPort->linMotComPortComboBox->setCurrentIndex(i);
      }
    }

    connect(mComPort->comPortCancelButton,  SIGNAL(clicked()),
            this,                           SLOT(on_comPortCancelButton_clicked()));
    connect(mComPort->comPortOKButton,  SIGNAL(clicked()),
            this,                       SLOT(on_comPortOKButton_clicked()));
    connect(this,             SIGNAL(closePressureSensorComPort()),
            mPressureSensor,  SLOT(closePort()));
    connect(this,         SIGNAL(closeLinearMotorComPort()),
            mLinearMotor, SLOT(closePort()));
    connect(this,             SIGNAL(openPressureSensorComPort(std::string)),
            mPressureSensor,  SLOT(openPort(std::string)));
    connect(this,         SIGNAL(openLinearMotorComPort(std::string)),
            mLinearMotor, SLOT(openPort(std::string)));
    connect(this,             SIGNAL(setPressureSensorComPort(std::string)),
            mPressureSensor,  SLOT(setPort(std::string)));
    connect(this,         SIGNAL(setLinearMotorComPort(std::string)),
            mLinearMotor, SLOT(setPort(std::string)));
  }

  /**
   * @brief Creates and brings up the "scale factor" menu and makes the required connections.
   */
  void MainWindow::scaleFactorWindow() {
    mScaleFactor = new Ui::ScaleFactor;
    mScaleFactorDialog = new QDialog;
    mScaleFactor->setupUi(mScaleFactorDialog);
    mScaleFactor->scaleFactorButtonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mScaleFactorDialog->show();

    connect(mScaleFactor->scaleFactorButtonBox, SIGNAL(accepted()),
            this,                               SLOT(on_buttonBox_accepted()));
    connect(this,            SIGNAL(setScaleFactor(double,double,double,double)),
            mPressureSensor, SLOT(setScaleFactor(double,double,double,double)));
  }

  /**
   * @brief Creates and brings up the "Stretcher" menu and makes the required connections.
   */
  void MainWindow::stretcherWindow() {
    mStretcher = new Ui::Stretcher;
    mStretcherDialog = new QDialog(this);
    mStretcher->setupUi(mStretcherDialog);
    mStretcherDialog->show();

    connect(mStretcher->linMotHomeButton, SIGNAL(clicked()),
            this,                         SLOT(setUpHomeWindow()));
    connect(mStretcher->linMotGotoClampingPosButton, SIGNAL(clicked()),
            this,                                    SLOT(on_linMotGoToClampingPosButton_clicked()));
    connect(this,         SIGNAL(gotoClampingDistance(double)),
            mLinearMotor, SLOT(gotoMMDistance(double)));
    connect(mStretcher->linMotSetClampingPosButton, SIGNAL(clicked()),
            mAutoStretch,                           SLOT(setClampingDistance()));
    connect(mStretcher->preCondStartButton, SIGNAL(clicked()),
            this,                           SLOT(on_preCondStartButton_clicked()));
    connect(mStretcher->rampToFailureStartButton, SIGNAL(clicked()),
            this,                                 SLOT(on_RampToFailureStartButton_clicked()));
    connect(mStretcher->rampToFailureStopButton,  SIGNAL(clicked()),
            mAutoStretch,                         SLOT(stopRampToFailure()));

  }

  /**
   * @brief Creates and brings up the "setUpHome" menu and makes the required connections.
   */
  void MainWindow::setUpHomeWindow() {
    mSetUpHome = new Ui::SetUpHome;
    mSetUpHomeDialog = new QDialog(mStretcherDialog);
    mSetUpHome->setupUi(mSetUpHomeDialog);
    mSetUpHome->setUpHomeButtonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mSetUpHomeDialog->show();

    connect(mSetUpHome->setUpHomeButtonBox,SIGNAL(rejected()),
            this,                          SLOT(on_setUpHomeButtonBox_rejected()));
    connect(mSetUpHome->setUpHomeButtonBox,SIGNAL(accepted()),
            this,                          SLOT(on_setUpHomeButtonBox_accepted()));
  }

  /**
   * @brief Sets the Graph dockwidget visible/invisible
   */
  void MainWindow::acdeacGraph() {
    if(graphDockWidget->isVisible() == true) {
      graphDockWidget->setVisible(false);
    } else {
      graphDockWidget->setVisible(true);
    }
  }

  /**
   * @brief Sets the LinearMotor dockwidget visible/invisible
   */
  void MainWindow::acdeacLinMot() {
    if(linmotdockWidget->isVisible() == true) {
      linmotdockWidget->setVisible(false);
    } else {
      linmotdockWidget->setVisible(true);
    }
  }

  /**
   * @brief Sets the Logger dockwidget visible/invisible
   */
  void MainWindow::acdeacLogger() {
    if(logDockWidget->isVisible() == true) {
      logDockWidget->setVisible(false);
    } else {
      logDockWidget->setVisible(true);
    }
  }

  /**
   * @brief Sets the Camera dockwidget visible/invisible
   */
  void MainWindow::acdeacCamera() {
    if(cameraDockWidget->isVisible() == true) {
      cameraDockWidget->setVisible(false);
    } else {
      cameraDockWidget->setVisible(true);
    }
  }

  /**
   * @brief Sets the Tracker dockwidget visible/invisible
   */
  void MainWindow::acdeacTracker() {
    if(trackerDockWidget->isVisible() == true) {
      trackerDockWidget->setVisible(false);
    } else {
      trackerDockWidget->setVisible(true);
    }
  }

  /**
   * @brief Sets the TrackerOptions dockwidget visible/invisible
   */
  void MainWindow::acdeacTrackerOptions() {
    if(trackerOptionsDockWidget->isVisible() == true) {
      trackerOptionsDockWidget->setVisible(false);
    } else {
      trackerOptionsDockWidget->setVisible(true);
    }
  }

  /**
   * @brief Sets the Stage dockwidget visible/invisible
   */
  void MainWindow::acdeacStage() {
    if(stageDockWidget->isVisible() == true) {
      stageDockWidget->setVisible(false);
    } else {
      stageDockWidget->setVisible(true);
    }
  }

  /**
   * @brief Sets the Pressure dockwidget visible/invisible
   */
  void MainWindow::acdeacPressure() {
    if(pressureDockWidget->isVisible() == true) {
      pressureDockWidget->setVisible(false);
    } else {
      pressureDockWidget->setVisible(true);
    }
  }

  /**
   * @brief Sets the PositionList dockwidget visible/invisible
   */
  void MainWindow::acdeacPositionList() {
    if(ListDockWidget->isVisible() == true) {
      ListDockWidget->setVisible(false);
    } else {
      ListDockWidget->setVisible(true);
    }
  }

  /**
   * @brief Sets the FocusInformation dockwidget visible/invisible
   */
  void MainWindow::acdeacFocusInformation() {
    if(focusdockWidget->isVisible() == true) {
      focusdockWidget->setVisible(false);
    } else {
      focusdockWidget->setVisible(true);
    }
  }

  /**
   * @brief Sets the Protocols dockwidget visible/invisible
   */
  void MainWindow::acdeacProtocols() {
    if(protocoldockWidget->isVisible() == true) {
      protocoldockWidget->setVisible(false);
    } else {
      protocoldockWidget->setVisible(true);
    }
  }

  /**
   * @brief Remove the tick in the view menu, when widget isn't visible
   * @param visible State of the visibility of the widget
   */
  void MainWindow::on_cameraDockWidget_visibilityChanged(bool visible) {
    if((false == visible)) {
      acdeacCameraAction->setChecked(false);
    } else {
      acdeacCameraAction->setChecked(true);
    }
  }

  /**
   * @brief Remove the tick in the view menu, when widget isn't visible
   * @param visible State of the visibility of the widget
   */
  void MainWindow::on_trackerDockWidget_visibilityChanged(bool visible) {
    if(false == visible) {
      acdeacTrackerAction->setChecked(false);
    } else {
      acdeacTrackerAction->setChecked(true);
    }
  }

  /**
   * @brief Remove the tick in the view menu, when widget isn't visible
   * @param visible State of the visibility of the widget
   */
  void MainWindow::on_trackerOptionsDockWidget_visibilityChanged(bool visible) {
    if(false == visible) {
      acdeacTrackerOptionsAction->setChecked(false);
    } else {
      acdeacTrackerOptionsAction->setChecked(true);
    }
  }

  /**
   * @brief Remove the tick in the view menu, when widget isn't visible
   * @param visible State of the visibility of the widget
   */
  void MainWindow::on_ListDockWidget_visibilityChanged(bool visible) {
    if(false == visible) {
      acdeacPositionListAction->setChecked(false);
    } else {
      acdeacPositionListAction->setChecked(true);
    }
  }

  /**
   * @brief Remove the tick in the view menu, when widget isn't visible
   * @param visible State of the visibility of the widget
   */
  void MainWindow::on_linmotdockWidget_visibilityChanged(bool visible) {
    if(false == visible) {
      acdeacLinMotAction->setChecked(false);
    } else {
      acdeacLinMotAction->setChecked(true);
    }
  }

  /**
   * @brief Remove the tick in the view menu, when widget isn't visible
   * @param visible State of the visibility of the widget
   */
  void MainWindow::on_graphDockWidget_visibilityChanged(bool visible) {
    if(false == visible) {
      acdeacGraphAction->setChecked(false);
    } else {
      acdeacGraphAction->setChecked(true);
    }
  }

  /**
   * @brief Remove the tick in the view menu, when widget isn't visible
   * @param visible State of the visibility of the widget
   */
  void MainWindow::on_storageDockWidget_visibilityChanged(bool visible) {
    if(false == visible) {
      acdeacStorageAction->setChecked(false);
    } else {
      acdeacStorageAction->setChecked(true);
    }
  }

  /**
   * @brief Remove the tick in the view menu, when widget isn't visible
   * @param visible State of the visibility of the widget
   */
  void MainWindow::on_stageDockWidget_visibilityChanged(bool visible) {
    if(false == visible) {
      acdeacStageAction->setChecked(false);
    } else {
      acdeacStageAction->setChecked(true);
    }
  }

  /**
   * @brief Remove the tick in the view menu, when widget isn't visible
   * @param visible State of the visibility of the widget
   */
  void MainWindow::on_pressureDockWidget_visibilityChanged(bool visible) {
    if(false == visible) {
      acdeacPressureAction->setChecked(false);
    } else {
      acdeacPressureAction->setChecked(true);
    }
  }

  /**
   * @brief Remove the tick in the view menu, when widget isn't visible
   * @param visible State of the visibility of the widget
   */
  void MainWindow::on_focusdockWidget_visibilityChanged(bool visible) {
    if(false == visible) {
      acdeacFocusInformationAction->setChecked(false);
    } else {
      acdeacFocusInformationAction->setChecked(true);
    }
  }

  /**
   * @brief Remove the tick in the view menu, when widget isn't visible
   * @param visible State of the visibility of the widget
   */
  void MainWindow::on_protocoldockWidget_visibilityChanged(bool visible) {
    if(false == visible) {
      acdeacProtocolsAction->setChecked(false);
    } else {
      acdeacProtocolsAction->setChecked(true);
    }
  }

  /**
   * @brief Saves the changed filename/path as new filename/path.
   * @param arg1 New filename/path
   */
  void MainWindow::on_settingsStorageFilenameEdit_textChanged(const QString &arg1) {
    newStorageFilename = arg1;
  }

  /**
   * @brief Opens a FileDialog where the user can choose the file name and the path,
   *        where the picture will be saved. Also updates the FilenameEdit field with the changed value.
   */
  void MainWindow::on_settingsStorageLocationButton_clicked() {
    QString filename = QFileDialog::getSaveFileName(this, "Choose storage location");
    if (filename.isEmpty()) {
      return;
    }
    mStoragePath->settingsStorageFilenameEdit->setText(filename);
  }

  /**
   * @brief Saves the changed value as new value.
   * @param value New value
   */
  void MainWindow::on_settingsStorageRunSpinBox_valueChanged(int value) {
    newStorageRunSpinBoxValue = value;
  }

  /**
   * @brief Close Path configuration window.
   */
  void MainWindow::on_storagePathCancelButton_clicked() {
    mStoragePathDialog->close();
  }

  /**
   * @brief Stores the new values (filename and value) and updates the widgets in the MainWindow. When all this is done, the window will close.
   */
  void MainWindow::on_storagePathOKButton_clicked() {
    storageFilename = newStorageFilename;
    storageRunSpinBoxValue = newStorageRunSpinBoxValue;

    QString temp = storageFilename;
    storageFilenameEdit->setText(temp);
    storageRunSpinBox->setValue(storageRunSpinBoxValue);
    mStoragePathDialog->close();
  }

  /**
   * @brief Close Com Port configuration window.
   */
  void MainWindow::on_comPortCancelButton_clicked() {
    mComPortDialog->close();
  }

  /**
   * @brief Modificate the string from the mComPortStrings vector to the right format and informs
   *        PressureSensor and LinearMotor about the com port. When all this is done, the window will close.
   */
  void MainWindow::on_comPortOKButton_clicked() {
    // Change mCOmPortStrings[] "COMX ....." to "comX"
    std::string converterTemp = mComPortStrings[mComPort->converterComboBox->currentIndex()].toStdString();
    int blankPos = converterTemp.find(" ");
    converterTemp = converterTemp.substr(0, blankPos);
    std::transform(converterTemp.begin(), converterTemp.end(), converterTemp.begin(), ::tolower);

    // Change mCOmPortStrings[] "COMX ....." to "comX"
    std::string motorTemp = mComPortStrings[mComPort->linMotComPortComboBox->currentIndex()].toStdString();
    blankPos = motorTemp.find(" ");
    motorTemp = motorTemp.substr(0, blankPos);
    std::transform(motorTemp.begin(), motorTemp.end(), motorTemp.begin(), ::tolower);

    emit closePressureSensorComPort();
    emit closeLinearMotorComPort();
    emit openPressureSensorComPort(converterTemp);
    emit openLinearMotorComPort(motorTemp);
    /*emit setPressureSensorComPort(temp);
    emit setLinearMotorComPort(temp);
    */

    mComPortDialog->close();
  }

  /**
   * @brief Inform PressureSensor about the new scale factor parameters and close window afterwards.
   */
  void MainWindow::on_buttonBox_accepted() {
    emit setScaleFactor(mScaleFactor->nominalValueSpinBox->value(),
                        mScaleFactor->endValueSpinBox->value(),
                        mScaleFactor->nominalForceSpinBox->value(),
                        mScaleFactor->InputSensitivitySpinBox->value());
    mScaleFactorDialog->close();
  }

  /**
   * @brief Close SetUpHome window when "Cancel" button was pressed.
   */
  void MainWindow::on_setUpHomeButtonBox_rejected() {
    mSetUpHomeDialog->close();
  }

  /**
   * @brief Send the motors to the home position and after close the SetUpHome window when "OK" button was pressed.
   */
  void MainWindow::on_setUpHomeButtonBox_accepted() {
    mLinearMotor->sendHome();
    mSetUpHomeDialog->close();
  }

  void MainWindow::on_linMotGoToClampingPosButton_clicked() {
    emit gotoClampingDistance(mStretcher->linMotClampingSpinBox->value());
  }

  void MainWindow::on_preCondStartButton_clicked() {
    QtConcurrent::run(mAutoStretch, &AutoStretch::makePreConditioning, mStretcher->preCondSpeedSpinBox->value(),
                                                                       mStretcher->preCondPreloadSpinBox->value(),
                                                                       mStretcher->preCondCyclesSpinBox->value(),
                                                                       mStretcher->preCondWithSpinBox->value());
  }

  void MainWindow::on_RampToFailureStartButton_clicked() {
    QtConcurrent::run(mAutoStretch, &AutoStretch::measurementRampToFailure, mStretcher->rampToFailureSpeedSpinBox->value(),
                                                                            mStretcher->rampToFailurePreloadSpinBox->value(),
                                                                            mStretcher->rampToFailureForcAtFailureSpinBox->value());
  }

  void MainWindow::on_RampToFailureStopButton_clicked() {
    mAutoStretch->stopRampToFailure();
  }

  //Lamp State control from Controller  
  void MainWindow::Turn_ON(){
      std::cout<<mMainChannel<<" - TimeLapsewithout the stretcher"<<std::endl;
      switch(mMainChannel)
      {
      case 1:{
                on_Channel_1_clicked();
                break;
             }
      case 2:{
                on_Channel_2_clicked();
                break;
             }
      case 3:{
                on_Channel_3_clicked();
                break;
             }
      case 4:{
                on_Channel_4_clicked();
                std::cout<<"TimeLapsewithout the stretcher"<<std::endl;
                break;
             }
      }
  }

  //Update Lamp state
    void MainWindow:: on_Channel_0_clicked() {
       channel = 0;
       intensity = I_Channel_0->value();
       mLamp->setFreqBandAndIntensity(channel, intensity);

      /// LampSetFreqBandAndIntensityWorkerThread *lampSetFreqBandAndIntensityWorkerThread = new LampSetFreqBandAndIntensityWorkerThread();
       //connect(lampWorkerThread, SIGNAL(finishedSetFreqBandAndIntensity(bool)), this, SLOT((bool)));
       //connect(lampWorkerThread, SIGNAL(finished()), this, SLOT());
      /// lampSetFreqBandAndIntensityWorkerThread->setData(mLamp, channel, intensity);
      /// lampSetFreqBandAndIntensityWorkerThread->start();
    }
    void MainWindow:: on_Channel_1_clicked() {
       channel = 1;
       intensity = I_Channel_1->value();
       mLamp->setFreqBandAndIntensity(channel, intensity);
       // Temporal change to connect illuminuation to imaging mode @20180924
            //displayModeBox_currentIndexChanged(0);
    }
    void MainWindow:: on_Channel_2_clicked() {
       channel = 2;
       intensity = I_Channel_2->value();
       mLamp->setFreqBandAndIntensity(channel, intensity);
       // Temporal change to connect illuminuation to imaging mode @20180924
            //displayModeBox_currentIndexChanged(2);
            multiIm->setChecked(true);
            multichannel->setChecked(true);
            //on_firstCubeButton_clicked();
    }
    void MainWindow:: on_Channel_3_clicked() {
       channel = 3;
       intensity = I_Channel_3->value();
       mLamp->setFreqBandAndIntensity(channel, intensity);
       // Temporal change to connect illuminuation to imaging mode @20180924
            //displayModeBox_currentIndexChanged(0);
            multiIm->setChecked(false);
            multichannel->setChecked(false);
           //on_fifthCubeButton_clicked();
    }
    void MainWindow:: on_Channel_4_clicked() {
       channel = 4;
       intensity = I_Channel_4->value();
       mLamp->setFreqBandAndIntensity(channel, intensity);
       // Temporal change to connect illuminuation to imaging mode @20180924
            //displayModeBox_currentIndexChanged(0);
            multiIm->setChecked(false);
            multichannel->setChecked(false);
            //on_fifthCubeButton_clicked();
    }
    void MainWindow:: SetLampFromCameraSignal(int channel, double intensity) {
       mLamp->setFreqBandAndIntensity(channel, intensity);
    }
    void MainWindow::MultiImageToggle(){
        bool i = multiIm->isChecked();
        if (i)
            multiIm->setChecked(false);
        else
            multiIm->setChecked(true);
    }
    void MainWindow:: MultiChannelToggle(){
        bool c = multichannel->isChecked();
        if (c)
            multichannel->setChecked(false);
        else
            multichannel->setChecked(true);
    }

    void MainWindow::SetTimeLapse(bool state){
         mController->setTimeLapse(state);
    }

    //TimeLapse Control via Shortcut
    /*void MainWindow::SetTimeLapse(){
        bool state = TimeLapse->isChecked();
        if (state)
            TimeLapse->setChecked(false);
        else
            TimeLapse->setChecked(true);

        std::cout<<"State: "<<state<<std::endl;
        //mController->setTimeLapse(TimeLapse->isChecked());
    }*/

    /*void MainWindow:: testgrouping() {
        QButtonGroup group;
        QList<QRadioButton *> allButtons = ui->groupBox->findChildren<QRadioButton *>();
        qDebug() <<allButtons.size();
        for(int i = 0; i < allButtons.size(); ++i)
        {
            group.addButton(allButtons[i],i);
        }
        qDebug() << group.checkedId();
        qDebug() << group.checkedButton();
    }*/

   void MainWindow::on_StartMP_clicked() {
       int channel = 2;
       double intensity = I_Channel_2->value();;

       int t_on = tOndoubleSpinBox->value();
       int t_off = tOffdoubleSpinBox->value();
       int iterations = iterationsSpinBox->value();

     //  LampMP1WorkerThread *lampWorkerThread = new LampMP1WorkerThread(); // decomment 20190131
       //connect(lampWorkerThread, SIGNAL(finishedMP1(bool)), this, SLOT((bool)));
       //connect(lampWorkerThread, SIGNAL(finished()), this, SLOT());
      // lampWorkerThread->setData(mLamp, channel, intensity, t_on, t_off, iterations);// decomment 20190131
      // lampWorkerThread->start();// decomment 20190131
       mLamp->startMP1(channel, intensity, t_on, t_off, iterations);
       //std::cout<<"Starting the MP1:- "<< mClock.getTime()<<std::endl;
       //SetAction(false);
   }




  void MainWindow::updateWidgetActivities()
  {
    if (cameraStartToolButton->isChecked()) {
      // Camera running

      for (int i = 0; i < displayModeBox->count() - 1; ++i) {
        storageHL_1->itemAt(i)->widget()->setEnabled(true);
        if(mStoragePath != NULL) {
          mStoragePath->storageHL->itemAt(i)->widget()->setEnabled(true);
        }
      }

      removeDisplayModeButton->setEnabled(false);
      newDisplayModeButton->setEnabled(false);
    } else {
      // Camera not running

      for (int i = 0; i < displayModeBox->count() - 1; ++i) {
        storageHL_1->itemAt(i)->widget()->setEnabled(false);
        if(mStoragePath != NULL) {
          mStoragePath->storageHL->itemAt(i)->widget()->setEnabled(false);
        }
      }

      removeDisplayModeButton->setEnabled(displayModeBox->currentText() != "Tracking");
      newDisplayModeButton->setEnabled(true);
    }

    bool bControllerRunning = false;

    if (displayModeBox->currentIndex() != 0) {
      // One of the storage images selected

      trackerDockWidget->setEnabled(false);
      trackerOptionsDockWidget->setEnabled(false);
    } else {
      // Tracker image selected

      trackerDockWidget->setEnabled(true);
      trackerOptionsDockWidget->setEnabled(true);

      if (cameraStartToolButton->isChecked()) {
        // Camera running

        trackerStartToolButton->setEnabled(mController->valid());
        timingStartToolButton->setEnabled(mController->valid());
        measuringStartToolButton->setEnabled(mController->valid());
        zstackStartToolButton->setEnabled(mController->valid());

        if (trackerStartToolButton->isChecked()) {
          bControllerRunning = true;
          timingStartToolButton->setEnabled(false);
          measuringStartToolButton->setEnabled(false);
          zstackStartToolButton->setEnabled(false);
        }
        if (timingStartToolButton->isChecked()) {
          bControllerRunning = true;
          trackerStartToolButton->setEnabled(false);
          measuringStartToolButton->setEnabled(false);
          zstackStartToolButton->setEnabled(false);
        }
        if (measuringStartToolButton->isChecked()) {
          bControllerRunning = true;
          trackerStartToolButton->setEnabled(false);
          timingStartToolButton->setEnabled(false);
          zstackStartToolButton->setEnabled(false);
        }
        if (zstackStartToolButton->isChecked()) {
            bControllerRunning = true;
            trackerStartToolButton->setEnabled(false);
            measuringStartToolButton->setEnabled(false);
            timingStartToolButton->setEnabled(false);
        }

        fftImageSizeXBox->setEnabled(!bControllerRunning);
        fftImageSizeYBox->setEnabled(!bControllerRunning);
        correlatorDepthBox->setEnabled(!bControllerRunning);
      } else {
        // Camera not running

        trackerStartToolButton->setEnabled(false);
        timingStartToolButton->setEnabled(false);
        measuringStartToolButton->setEnabled(false);
        zstackStartToolButton->setEnabled(false);

        fftImageSizeXBox->setEnabled(true);
        fftImageSizeYBox->setEnabled(true);
        correlatorDepthBox->setEnabled(true);
      }
    }

    if (bControllerRunning) {
      cameraModeBox->setEnabled(false);
      displayModeBox->setEnabled(false);
    } else {
      cameraModeBox->setEnabled(true);
      displayModeBox->setEnabled(true);
    }
  }


  /**************************************************************************/
  /*************************** Central Image ********************************/
  /**************************************************************************/

  void MainWindow::displayImage(const QImage image, quint64, quint64)
  {
    mImageLabel->displayImage(image);
    displayPressure(10 * (10 - mDac->readvoltage()));
    displayXYpos(mStage->getXpos(), mStage->getYpos());
    displayZpos(mStage->getZpos());
  }

  void MainWindow::on_mImageLabel_zoomChanged(double value)
  {
    mImageZoomLabel->setText(QString::number(value, 'f', 2) + "x");
  }

  /**************************************************************************/
  /******************************* Pressure Plot ****************************/
  /**************************************************************************/

  /**
   * @brief Opens a FileDialog where the user can choose the path and the filename, Extends the file suffix, if needed
   *        and writes the measured pressure in the file.
   */
  void MainWindow::exportPressure()
  {
    QString filename = QFileDialog::getSaveFileName(this, "Storage location");

    // Extends file name with the suffix ".csv" if there was no suffix
    QFileInfo fileInfo(filename);
    if(fileInfo.suffix().isEmpty()) {
      filename += ".csv";
    }

    QFile file(filename);
    if (file.open(QFile::WriteOnly|QFile::Truncate)) {
      QTextStream stream(&file);
      stream << "time point, measured pressure;" << endl;
      for (QVector<double>::iterator i = mTimeValues.begin(), j = mPressureValues.begin(), y = mTimeValues.end(), z = mPressureValues.end(); ((i != y) && (j != z)); ++i, ++j) {
        stream << *i << ", " << *j << ";" << endl;
      }
      file.close();
    }
  }


  /*************************************************************************/
  /************************ Linear Motor / Force Sensor *********************/
  /**************************************************************************/
  void MainWindow::forceUpdated(double force) {
    // Update force in GUI every 250ms
    if(timer100ms >= 5) {
      forcelcdNumber->display(force);
      distanceLcdNumber->display(static_cast<double>(mLinearMotor->getDistance() * 0.000047625/*mm/microstep*/));
      timer100ms = 0;
    } else {
      timer100ms++;
    }

    // If current unit is force, update mAutoStretch's force
    //if(chooseEF->currentIndex() == AutoStretch::Unit::Force) {
      mAutoStretch->updateAutoStretchForce(force);
    //}
  }

  void MainWindow::setStretchUntilValueSuffix(const QString & suffix) {
    stretchUntilValueBox->setSuffix(suffix);
    stretchUntilValueBox->setValue(0);

    if(" N" == suffix) {
      stretchUntilValueBox->setSingleStep(0.1);
    } else if(" %" == suffix) {
      //stretchUntilValueBox->setMinimum(0);
      stretchUntilValueBox->setSingleStep(1.0);
    }
  }

  void MainWindow::on_sendcommandButton_pressed()
  {
    //mPressureSensor->sendHome();
    mLinearMotor->moveSteps(movestepsspinBox->value());//sendHome();
  }
  void MainWindow::on_homeButton_pressed()
  {

    mLinearMotor->sendHome();

  }
  void MainWindow::on_workButton_pressed()
  {

    mLinearMotor->sendWork();

  }
  void MainWindow::on_speedButton_pressed()
  {

    mLinearMotor->setSpeed(speedspinBox->value());
  }
  void MainWindow::on_resetButton_pressed()
  {

    mLinearMotor->reset();
  }
  void MainWindow::on_incsmallButton_pressed()
  {
    mLinearMotor->moveMillimeters(-smallLMSpinBox->value());
  }
  void MainWindow::on_decsmallButton_pressed()
  {
    mLinearMotor->moveMillimeters(smallLMSpinBox->value());
  }
  void MainWindow::on_decbigButton_pressed()
  {
    mLinearMotor->moveMillimeters(bigLMSpinBox->value());
  }
  void MainWindow::on_incbigButton_pressed()
  {
    mLinearMotor->moveMillimeters(-bigLMSpinBox->value());
  }

  void MainWindow::on_startoscLMButton_pressed()
  {
    if(mLinearMotor->checkoscillation(amplitudeLMBox->value(), periodLMBox->value())) { // check if motor can handle the oscillation
      mLinearMotor->startoscillation(amplitudeLMBox->value(), periodLMBox->value());
      warningLabel->setText(" ");
    } else
      warningLabel->setText("<font color='red'>decrease A/P ratio! </font>");
  }
  void MainWindow::on_stoposcLMButton_pressed()
  {
    mLinearMotor->stoposcillation();
  }

  void MainWindow::on_zeroDistanceButton_clicked() {
    emit setZeroDistance(((zeroDistanceSpinBox->value()) / 0.000047625));
  }

  void MainWindow::on_startStretchButton_clicked()
  {
    mAutoStretch->start(chooseEF->currentIndex());
  }

  void MainWindow::on_chooseMode_currentIndexChanged(int index)
  {
   // Inform mAutoStretch about changed mode
   emit updateAutoStretchMode(index);

   // Remove "Expansion" in Hold mode, add it if needed in Until mode
   if(AutoStretch::Mode::Hold == index) {
     chooseEF->removeItem(AutoStretch::Unit::Expansion);
   } else if((AutoStretch::Mode::Until == index) && (chooseEF->itemText(AutoStretch::Unit::Expansion) != "Expansion")) {
     chooseEF->insertItem(AutoStretch::Unit::Expansion, "Expansion");
   }
  }

  void MainWindow::updateMeasuredPoints(double mForce, double mLength) {
    if(graphOn == true) {
      // Add length and force point
      measuredLength.append(static_cast<double>(mLength * 0.000047625));
      measuredForce.append(mForce);

      // Set up graph
      graphWidget->graph(0)->setData(measuredLength, measuredForce);

      graphWidget->replot();
      /*std::cout << "MainWindows should have plotted graph!" << std::endl;
      std::cout << "    mLength: " << measuredLength.last() << ", mForce: " << measuredForce.last() << std::endl;
      */
    }
  }

  void MainWindow::exportPng()
  {
    QString filename = QFileDialog::getSaveFileName(this, "Storage location");

    // Extends file name with the suffix ".png" if there was no or an other suffix
    QFileInfo fileInfo(filename);
    if(fileInfo.suffix() != "png") {
      filename += ".png";
    }
    graphWidget->savePng(filename, pictureWidth, pictureHeight);
  }

  void MainWindow::on_pictureHeight_valueChanged(double height) {
    pictureHeight = height;
  }

  void MainWindow::on_pictureWidth_valueChanged(double width) {
    pictureWidth = width;
  }

  void MainWindow::on_clearGraphButton_clicked() {
    measuredLength.resize(0);
    measuredForce.resize(0);
    graphWidget->graph(0)->setData(measuredLength, measuredForce);
    graphWidget->replot();
    if(graphOn == false) {
      graphStartStophButton->setText("Start");
    }
  }

  void MainWindow::on_graphStartStophButton_clicked() {
    if(graphOn == false) {
      graphOn = true;
      graphStartStophButton->setText("Pause");
    } else if(graphOn == true) {
      graphOn = false;
      graphStartStophButton->setText("Resume");
    }
  }

  /**
   * @brief Opens a FileDialog where the user can choose the path and the filename, Extends the file suffix, if needed
   *        and writes the measured data in the file.
   */
  void MainWindow::on_exportButton_clicked() {
    QString filename = QFileDialog::getSaveFileName(this, "Storage location");

    // Extends file name with the suffix ".csv" if there was no suffix
    QFileInfo fileInfo(filename);
    if(fileInfo.suffix().isEmpty()) {
      filename += ".csv";
    }

    QFile file(filename);
    if (file.open(QFile::WriteOnly|QFile::Truncate)) {
      QTextStream stream(&file);
      stream << "measured length, measured force;" << endl;
      for (QVector<double>::iterator i = measuredLength.begin(), j = measuredForce.begin(), y = measuredLength.end(), z = measuredForce.end(); ((i != y) && (j != z)); ++i, ++j) {
        stream << *i << ", " << *j << ";" << endl;
      }
      file.close();
    }
  }

  /**************************************************************************/
  /****************************** zTrack ************************************/
  /**************************************************************************/

  void MainWindow::displayFocus(const FocusValue &focus)
  {
    //lcdnumberRampFocus->display(100*focus.rampFocus);
    //lcdnumberLogFocus->display(100*focus.logFocus);
    if (checkBoxEnableBrenner->isChecked())
        lcdNumberFocus->display((double) focus.brennerFocus);
    else
        lcdNumberFocus->display((double) 100*focus.gaussFocus);
    //lcdnumberIntegralFocus->display(100*focuses.at(7));
    lcdNumberMaxvalue->display((double) focus.maxValue);
    lcdNumberNoiselevel->display((double) focus.noiseLevel);
    //lcdNumberTanhFocus->display(100*focuses.at(6));
  }

  void MainWindow::displayZpos(double zpos)
  {
    lcdNumberZposition->display(zpos);
  }

  void MainWindow::displayXYpos(double xpos, double ypos)
  {
    lcdNumberXposition->display(xpos);
    lcdNumberYposition->display(ypos);
  }

  void MainWindow::setPressure(float pressure)
  {
    mDac->setvoltage((pressure+100)/20); // set the corresponding voltage at the dac output
    if (correctzposcheckBox->isChecked()) { // autocorrect z position depending on stepsize
      // move stage to predicted zposition
      float actualpressure = -10 * (10 - mDac->readvoltage()); // acquire actual pressure
      float step = pressure - actualpressure;
      adjustStageZ(step);
    }
  }

  void MainWindow::adjustStageZ(float step)
  {
    mController->moveStageZ(step * corrfactorSpinbox->value(), 0);
  }

  void MainWindow::on_haltallButton_pressed()
  {
    //stop stagemovements from Oasis
    mStage->stopAll();
    //stop oscillations

    killTimer(dactimerID);
    dactimerID =0;
    tickcounter =1;
    rising = true;
  }

  /**
   * @brief Select first filter when choosen in the GUI.
   */
  void MainWindow::on_firstCubeButton_clicked(){
    mAhmmicroscope->setturretposition(1);
  }

  /**
   * @brief Select second filter when choosen in the GUI.
   */
  void MainWindow::on_secondCubeButton_clicked(){
    mAhmmicroscope->setturretposition(2);
  }

  /**
   * @brief Select third filter when choosen in the GUI.
   */
  void MainWindow::on_thirdCubeButton_clicked(){
    mAhmmicroscope->setturretposition(3);
  }

  /**
   * @brief Select fourth filter when choosen in the GUI.
   */
  void MainWindow::on_fourthCubeButton_clicked(){
    mAhmmicroscope->setturretposition(4);
  }

  /**
   * @brief Select fifth filter when choosen in the GUI.
   */
  void MainWindow::on_fifthCubeButton_clicked(){
    mAhmmicroscope->setturretposition(5);
  }

  /**
   * @brief Change to the previous filter in order.
   */
  void MainWindow::on_decreaseCubeButton_clicked(){
    int pos = mAhmmicroscope->getturretposition();
    if(1 == pos){
      mAhmmicroscope->setturretposition(5);
    }else{
      mAhmmicroscope->setturretposition(pos - 1);
    }
  }

  /**
   * @brief Change to the next filter in order.
   */
  void MainWindow::on_increaseCubeButton_clicked(){
    int pos = mAhmmicroscope->getturretposition();
    if(5 == pos){
      mAhmmicroscope->setturretposition(1);
    }else{
      mAhmmicroscope->setturretposition(pos + 1);
    }
  }

  /*void MainWindow::on_openshutterButton_pressed(){
      mAhmmicroscope->openshutter();
  }
  void MainWindow::on_closeshutterButton_pressed(){
      mAhmmicroscope->closeshutter();
  }*/

  void MainWindow::on_toggleshutterButton_pressed()
  {
    mAhmmicroscope->toggleShutter();
    switch(mAhmmicroscope->getFlShutterState()){
      case 0:
        shutterLabel->setText("close");
         if (trackerStartToolButton->isChecked()) {
            //Kill the controller if the shutter is closed
            mControllerThread->quit();
            // Thread might not quit immediately
            trackerStartToolButton->setChecked(true);
        }
        //Set the pressure to zero if the shutter is closed
        emit pressureSetRequested(-0.0);
        //writing the acquired images to the hard disk
        saveBuffertoHarddisk();

        break;
      case 1:
        shutterLabel->setText("open");
        break;
    }
  }

  void MainWindow::on_decreaseintensityButton_pressed()
  {
    mAhmmicroscope->changeintensity(-1);
  }

  void MainWindow::on_increaseintensityButton_pressed()
  {
    mAhmmicroscope->changeintensity(1);
  }

  /**
   * @brief Change illumination method to incident light
   */
  void MainWindow::on_incidentButton_clicked(){
    mAhmmicroscope->incidentIllumination();
    illuminationLabel->setText("incident");
  }

  /**
   * @brief Change illumination method to transmitted light
   */
  void MainWindow::on_transmittedButton_clicked(){
    mAhmmicroscope->transmittedIllumination();
    illuminationLabel->setText("transmitted");
  }

  void MainWindow::on_startoscbutton_pressed()
  {
    if (dactimerID) {
      killTimer(dactimerID);
      dactimerID =0;
    }

    timeinterval = timestepspinbox->value();//800;
    steps =pressurestepsspinbox->value();//21;
    stepsize=amplitudespinbox->value()/steps;
    dactimerID = this->startTimer(timeinterval);
    tickcounter=1;
    //cameratimer->singleShot(timeinterval-150,this,SLOT(shootautopicture())); //shoot picture at start
    //shootautopicture(); // Shot 1 autopicture and save it immediatelly
    rising = true;

    mController->setAutofocusModeAfterLastPressureChange(false);

    // Reset pressure plot
    /*mPressureValues.resize(0);
    mTimeValues.resize(0);
    mTimeValues.append(0);
    */
    // mPressurePlot->graph(0)->setData(mTimeValues,mPressureValues);
    //mPressurePlot->replot();

  }

  void MainWindow::on_stoposcButton_pressed()
  {
    killTimer(dactimerID);
    dactimerID =0;
    tickcounter =1;
    rising = true;
  }
  /**************************************************************************/
  /****************************** Protocols**********************************/
  /**************************************************************************/
  void MainWindow::on_addstackButton_pressed()
  {
    mProtocolwidget->addItem("Stack(steps: "+protzstepsBox->text() +"; stepsize: "+protstepsizeBox->text()+"; size top: " + "; size: " + protStackSizeBox->text()+")");
  }
  void MainWindow::on_addpauseButton_pressed()
  {
    mProtocolwidget->addItem("Pause("+protpausedurBox->text() +" ms)");
  }
  void MainWindow::on_addposButton_pressed()
  {
    mProtocolwidget->addItem("Move to Position: "+ protposnrBox->text());
  }

  /**
   * @brief Calculates the Stack size when the Step size changes.
   */
  void MainWindow::on_protstepsizeBox_editingFinished(){
    protStackSizeBox->setValue(protstepsizeBox->value() * protzstepsBox->value());
  }

  /**
   * @brief Calculates the Step size when the Stack size changes.
   */
  void MainWindow::on_protStackSizeBox_editingFinished(){
    protstepsizeBox->setValue(protStackSizeBox->value() / protzstepsBox->value());
  }

  void  MainWindow::on_addphotoButton_pressed()
  {
    mProtocolwidget->addItem("Photo");
  }
  void MainWindow::on_protupButton_pressed()
  {
    int currentIndex = mProtocolwidget->currentRow();
    QListWidgetItem *currentItem = mProtocolwidget->takeItem(currentIndex);
    mProtocolwidget->insertItem(currentIndex-1, currentItem);
    mProtocolwidget->setCurrentRow(currentIndex-1);
  }

  void MainWindow::on_protdownButton_pressed()
  {

    int currentIndex = mProtocolwidget->currentRow();
    QListWidgetItem *currentItem = mProtocolwidget->takeItem(currentIndex);
    mProtocolwidget->insertItem(currentIndex+1, currentItem);
    mProtocolwidget->setCurrentRow(currentIndex+1);
  }

  void MainWindow::on_protdelButton_pressed()
  {
    if (mProtocolwidget->selectedItems().count() >0)

      delete mProtocolwidget->selectedItems().at(0);

  }

  void MainWindow::on_execButton_pressed()
  {
    //generate Protocol from Protocollistwidget
    //parse text for information
    execButton->setDisabled(true);
    mProtocolstoprequest=false;
    mProtocol= new QList<Protocolentry>;
    int itemcount=0;

    itemcount=mProtocolwidget->count();
    if (itemcount==0) {
      TRACKER_WARNING("Empty Protocol list. Add events first");
      return;
    }
    for (int i=0; i<itemcount; i++) {
      QString text;
      text=mProtocolwidget->item(i)->text();

      if(text.contains("Pause")) {
        Protocolentry entry;
        int start,end,time;
        QString timestring;

        //get pauseduration
        start=text.indexOf("(")+1;;
        end=text.indexOf(" ms");
        timestring=text.mid(start,end-start);
        time=timestring.toInt();

        entry.belongsto=i;
        entry.listposition=0;
        entry.duration=time;
        entry.type=0;
        mProtocol->append(entry);
      }
      if(text.contains("Move to Position")) {

        Protocolentry entry;
        int start,end,position;
        QString positionstring;

        //get pauseduration
        start=text.indexOf(": ")+1;;
        end=text.length();
        positionstring=text.mid(start,end-start);
        position=positionstring.toInt()-1;

        if(position+1>mPoslist->count()) {
          TRACKER_WARNING("Error. At least one Position does not exist in list");
          execButton->setDisabled(false);
          return;
        }

        entry.belongsto=i;
        entry.listposition=position;
        entry.duration=1000;
        entry.type=1;
        mProtocol->append(entry);
      }
      if(text.contains("Photo")) {

        Protocolentry entry;
        entry.belongsto=i;
        entry.listposition=0;
        entry.duration=1000;
        entry.type=2;
        mProtocol->append(entry);

      }

      if(text.contains("Stack")) {

        int steps,topsize,botsize,zstepsize,rest;
        steps=text.mid(text.indexOf("steps: ")+7,text.indexOf("; stepsize")-(text.indexOf("steps: ")+7)).toInt();
        zstepsize=text.mid(text.indexOf("stepsize: ")+10,text.indexOf("; size top")-(text.indexOf("stepsize: ")+10)).toInt();
        topsize=text.mid(text.indexOf("size top: ")+10,text.indexOf("; size bot")-(text.indexOf("size top: ")+10)).toInt();
        botsize=text.mid(text.indexOf("size bot: ")+10,text.indexOf(")")-(text.indexOf("size bot: ")+10)).toInt();

        if (zstepsize>0) {
          steps=(topsize+botsize)/zstepsize;
          rest=(topsize+botsize)%zstepsize;
        } else {
          if (steps>0) {
            zstepsize=(topsize+botsize)/steps;
            rest=steps*((topsize+botsize)%steps);
          } else
            TRACKER_WARNING("Error in Stack generation");
          execButton->setDisabled(false);
          return;
        }
        for(int j=0; j<=steps+1; j++) {
          Protocolentry moveentry, photoentry;
          moveentry.state=new ExperimentState;

          //moveentry.state->xpos=0;
          //moveentry.state->ypos=0;
          //moveentry.state->zpos=0;
          //moveentry.state->pressure=0;

          if (j==0) {
            moveentry.state->zpos= topsize;
          }		// move to top
          else {
            if(j==steps+1)
              moveentry.state->zpos=botsize-rest;// move back to center
            else
              moveentry.state->zpos= -zstepsize;
          } // step

          moveentry.type=3;
          moveentry.duration=1000;
          moveentry.belongsto=i;
          mProtocol->append(moveentry);

          if(!(j==steps+1))							// do not a take photo in return step
            photoentry.type=2;
          photoentry.duration=1000;
          photoentry.belongsto=i;
          mProtocol->append(photoentry);

        }
        //create relative states with info
        //generate entries
        //append em
      }
    }
    mCurrentprotocolentry=0;
    executeprotocolentry(mCurrentprotocolentry);
    /*
    mProtocol= new QList<Protocolentry>;


    Protocolentry entry1;
    Protocolentry entry2;
    Protocolentry entry3;

    entry1.duration=1000;
    entry2.duration=2000;
    entry3.duration=3000;
    entry1.type=1;
    entry2.type=2;
    entry3.type=1;
    entry1.listposition=0;
    entry2.listposition=1;
    entry3.listposition=2;

    mProtocol->append(entry1);
    mProtocol->append(entry2);
    mProtocol->append(entry3);
    mCurrentprotocolentry=0;
    executeprotocolentry(0);*/
  }

  void MainWindow::executeprotocolentry(int entryindex)
  {

    Protocolentry entry;
    mProtocolwidget->item(mProtocol->at(entryindex).belongsto)->setSelected(true);
    entry=mProtocol->at(entryindex);

    entry.timer= new QTimer();

    if(entry.type==1) {
      setstate(getstatefromlistpos(entry.listposition));
    }
    if(entry.type==2) {
      //shootautopicture(); //

      //QString name = "auto"; //"auto"
      //mStoringImage = true;
      //assert(displayModeBox->findText(name) != -1);
      //DisplayMode& mode = mDisplayModes[displayModeBox->findText(name)];
      //mStoringDisplayMode = name;

      //int expt= mPoslist->at(mCurrentlistposition-1)->exposuretime;
      //emit singleShotRequested(mode.cameraMode, *entry.state->exposuretime, mode.gain);
      //
    }
    if(entry.type==3) {
      //move relative
      setrelativestate(*entry.state);

    }

    testlabel->setText(QString::number(entry.duration));

    entry.timer->singleShot(entry.duration, this, SLOT(executenextprotocolentry()) ); //wait duration

  }

  void MainWindow::executenextprotocolentry()
  {
    if(!mProtocolstoprequest) {
      mCurrentprotocolentry++;
      if (mCurrentprotocolentry < mProtocol->count())
        executeprotocolentry(mCurrentprotocolentry);
      else {
        if(loopcheckBox->isChecked()) { //restart protocol;
          mCurrentprotocolentry=0;
          executeprotocolentry(mCurrentprotocolentry);
        } else
          execButton->setDisabled(false);
      }
    } else
      execButton->setDisabled(false);

  }

  void MainWindow::on_stopexecButton_pressed()
  {
    mProtocolstoprequest=true;
  }
  /**************************************************************************/
  /****************************** Storage ***********************************/
  /**************************************************************************/

  void MainWindow::on_storageRunSpinBox_valueChanged(int value) {
    storageRunSpinBoxValue = value;
    if(NULL != mStoragePath){
      mStoragePath->settingsStorageRunSpinBox->setValue(storageRunSpinBoxValue);
    }
  }

  void MainWindow::on_storageFilenameEdit_textChanged(const QString &arg1) {
    storageFilename = arg1;
    if(NULL != mStoragePath) {
      QString temp = arg1;
      mStoragePath->settingsStorageFilenameEdit->setText(temp);
    }
  }
  /**
      Tracks the elapsed time from request to capture a full resulution image
      @param accumulated time from the request of the image
  */
  void MainWindow::TrackFullImageAcq(quint64 time) {
      mTimerForOneCompleteImage += time;
  }
  /**
      Returns the starting time point of the requested image
      @return the starting time point of 1 requested image
  */
  quint64 MainWindow::getTrackFullImageAcq() {
      return mTimerForOneCompleteImage;
  }

  void MainWindow::on_mSingleShotButtonMapper_mapped(QString name)
  {
    // Protect against user hitting store button too many times and causing
    // some buffers to page memory onto the hard drive...
    if (!mStoringImage) {
      mStoringImage = true;
      assert(displayModeBox->findText(name) != -1);
      DisplayMode& mode = mDisplayModes[displayModeBox->findText(name)];
      mStoringDisplayMode = name;
      std::cout<<"F1 single shot acquisition: "<<mClock.getTime()<<std::endl;
     // qDebug() << name;
     // std::cout<<"Exposure for SingleShot"<<mode.exposureTime<<std::endl;
     // std::cout<<"Gain for SingleShot"<<mode.gain<<std::endl;
   /**   if (multiIm->isChecked()){
        mCamera->setNumOfPlanes(SizeOfZstack->value());
        mCamera->setZstepsize(Zstackstepsize->value());
        emit multipleShotRequested(mode.cameraMode, mode.exposureTime, mode.gain);
      }
      else{
        emit singleShotRequested(mode.cameraMode, mode.exposureTime, mode.gain);
      }
      */
      NumOfImages = 1;
      if (multichannel->isChecked())
      {
          mCamera->setMultichannel(true);
          double ChannelOffsetvalue = ChannelOffset->value();
          mCamera->setOffsetValue(ChannelOffsetvalue);
      }
      else
         mCamera->setMultichannel(false);

      if (multiIm->isChecked()){
          int NumOfPlanes = SizeOfZstack->value();
          double ZstackStepSize = Zstackstepsize->value();
          ZPosAtImaging =  mStage->getZpos();
          std::cout<<"ZposAtImaging: "<< ZPosAtImaging<< std::endl;
          for (unsigned int i = 0; i < NumOfPlanes / 2; ++i){
             // mStage->moveZ(-ZstackStepSize, 1);
              mController->moveStageZ(-ZstackStepSize, 1);
          }
          mCamera->setNumOfPlanes(NumOfPlanes);
          mCamera->setZstepsize(ZstackStepSize);
      }
     else
      mCamera->setNumOfPlanes(1);

     emit RecordMW(mode.cameraMode,channel,intensity, 1,I_Channel_1->value(),mode.exposureTime, mode.gain);
      //Start the image aquisition time
      mTimerForOneCompleteImage = 0;
      TrackFullImageAcq(mClock.getTime());
      std::cout<<"F1 single shot acquisition end of the function call: "<<mClock.getTime()<<std::endl;
    }
  }

  void MainWindow::on_storageLocationButton_clicked() {
    QString filename = QFileDialog::getSaveFileName(this, "Choose storage location");
    if (filename.isEmpty()) {
      return;
    }
    storageFilenameEdit->setText(filename);

    if(mStoragePath != NULL) { 
      mStoragePath->settingsStorageFilenameEdit->setText(filename);
      // To set the log file folder where we are going to save the images as well
      // mController->mCorrelatorFileName = filename;
    }
  }

  void MainWindow::saveBuffertoHarddisk()
  {
      std::cout<< "Are we storing ????"<< std::endl;
    // if everything is okay for storing the data.
      int i = Images.size()-1;
      while (!Images.empty()){
          // Saving the images from the buffer
         // Images[i].Image.save(Images[i].Save);
          QString format("png");
          Images[i].Image.save(Images[i].Save,format.toAscii(), 100); // Improved speed with no compression instead of -1 to 100
          TRACKER_INFO("Stored Image '" + Images[i].Save + "'", Logger::Low);
          std::cout<< "Elapsed time for 1 full resolution image after saving the image:" << mClock.getTime()-getTrackFullImageAcq() << std::endl;
          // Saving the corresponding metadata files from the buffer
          const char *filename = Images[i].ba.data();
          std::ofstream metadata(filename);
          if(metadata.is_open()){
              /* fprintf(metadata
                      "x: %f\n"
                      "y: %f\n"
                      "z: %f\n"
                      "pressure: %f\n"
                      "gain: %f\n"
                      "exposure_time: %f\n"
                      "captureTime: %f\n"
                      "processTime: %f\n",
                      Images[i].X, Images[i].Y, Images[i].Z, Images[i].pressure,
                      Images[i].gain,  Images[i].exposure, Images[i].captureTime, Images[i].processTime);
                  fflush(metadata);*/

                metadata << "x: " << Images[i].X << std::endl
                         << "y: " << Images[i].Y << std::endl
                         << "z: " << Images[i].Z << std::endl
                         << "pressure: " << Images[i].pressure << std::endl
                         << "gain: " << Images[i].gain << std::endl
                         << "exposure_time: " <<  Images[i].exposure   << "ms" << std::endl
                         //<< "illumination_mode: " << illMode << std::endl
                        // << "cube: " << cube << std::endl
                       //  << "shutter: " << shutter << std::endl
                         << "captureTime: " << Images[i].captureTime << std::endl
                         << "processTime: " << Images[i].processTime << std::endl;
                metadata.close();

            std::cout<< "Elapsed time for 1 full resolution image after saving the image and the metadata:" << mClock.getTime()-getTrackFullImageAcq() << std::endl;
            TRACKER_INFO("Stored meta data '" + Images[i].path + Images[i].distinction +".txt" + "'", Logger::Low);
          }
          i = i-1;
          Images.pop_back();
      }
  }

  void MainWindow::storeSingleShot(QImage image, quint64 captureTime, quint64 processTime)
  {
    mStoringImage = false;
    if (image.isNull()){ // Acquisition failed
      return;
    }

 ///!!!   std::cout<< "Elapsed time for 1 full resolution image before storing it:" << mClock.getTime()-getTrackFullImageAcq() << std::endl;

    //if(mKeepinmemory){// check if we want to keep image in memory
    //	mKeepinmemory=false;
    //	mPoslist->at(mCurrentlistposition)->image=image;
    //}

    //QString path = storageFilenameEdit->text();
    ///std::cout<< "Printing out the Counter: " << Counter << ":  NumOfImages: "<<NumOfImages<< std::endl;

  // Images[Counter].Image = image;
    QString path = storageFilename;
    // Images[Counter].path = storageFilename;
/*!    if (path.isEmpty()) {
      TRACKER_WARNING("Could not store image: No filename given");
      return;
    }
*/
    if (path.isEmpty()) {
       TRACKER_WARNING("Could not store image: No filename given");
       return;
     }

    if (!QFileInfo(path).isAbsolute())
      path = PathConfig::getDataPath().path() + "/" + path;
//    if (!QFileInfo(Images[Counter].path).isAbsolute())
  //    Images[Counter].path = PathConfig::getDataPath().path() + "/" + Images[Counter].path;

    // New filename: XXXX_YYYYMMDD_HHMMSS.png
    QDate cd = QDate::currentDate();
    QTime ct = QTime::currentTime();
    path += "_" +QString::number(channel)+"_"+QString::number(intensity)+ "_"+ cd.toString("yyyyMMdd") + "_" + ct.toString("hhmmss") + "_" + ct.toString("zzz");
    QString format = ".png";

    // Find unique filename
    int i = 0;
    QString distinction;
    ///while (QFile::exists(Images[Counter].path + Images[Counter].distinction + Images[Counter].format)) {
    while (QFile::exists(path + distinction + format)) {
      ++i;
      distinction = "-" + QString::number(i);
    }
    QString Save = path + distinction + format;

    /*
    // Getting the MetaData Information
    int tIlluminationMethod = mAhmmicroscope->getContrastingMethod();
    std::string illMode;
    switch(tIlluminationMethod){
      case ahmMicroscope::ilWhiteLight:
        illMode = "white light";
        break;
      case ahmMicroscope::ilFluorescence:
        illMode = "fluorescence";
        break;
      default:
        illMode = "unknown";
        break;
    }
    std::string cube;
    int cubePos = mAhmmicroscope->getturretposition();
    if(tIlluminationMethod == ahmMicroscope::ilFluorescence){
      switch(cubePos){
        case 1:
          cube = "Cy3";
          break;
        case 2:
          cube = "GFP";
          break;
        case 3:
          cube = "DAPI";
          break;
        default:
          cube = "unknown";
          break;
      }
    }else{
      cube = "none";
    }
    /*
    std::string shutter;
    if(1 == mAhmmicroscope->getFlShutterState()){
      shutter = "open";
    } else{
      shutter = "close";
    }*/

    //Images[Counter].ba = (Images[Counter].path + Images[Counter].distinction + ".txt").toLocal8Bit();
    QByteArray ba = (path + distinction + ".txt").toLocal8Bit();
    //  Images[Counter].Z = mStage->getZpos();
    double   Z = mStage->getZpos();
    double x        = mStage->getXpos();
    double y        = mStage->getYpos();
    float pressure  = -10*(10-mDac->readvoltage());
    double gain     = gainBox->value();
    double expt     = ((double)mCamera->getExposureTime()/1000);

    Images.push_back(ImageBuffer(image,captureTime,processTime,path,format,Save,distinction,ba,Z,x,y,pressure,expt,gain));

    /// Testing the new struct and structure for saving the files
    //int isSAVE = 4; // For testing the struct
 /*
    if (NumOfImages == Counter){
        // Requesting for uniform state information for metadata
        std::cout<< "here we manage:" << mClock.getTime()-getTrackFullImageAcq() << std::endl;

        int i = Images.size()-1;
        while (!Images.empty()){
            // Saving the images from the buffer
           // Images[i].Image.save(Images[i].Save);
            Images[i].Image.save(Images[i].Save);
            TRACKER_INFO("Stored Image '" + Images[i].Save + "'", Logger::Low);
            std::cout<< "Elapsed time for 1 full resolution image after saving the image:" << mClock.getTime()-getTrackFullImageAcq() << std::endl;
            // Saving the corresponding metadata files from the buffer
            const char *filename = Images[i].ba.data();
            std::ofstream metadata(filename);
            if(metadata.is_open()){
              metadata << "x: " << x << std::endl
                       << "y: " << y << std::endl
                       //<< "z: " << Images[i].Z << std::endl
                       << "z: " << Images[i].Z << std::endl
                       << "pressure: " << pressure << std::endl
                       << "gain: " << gain << std::endl
                       << "exposure_time: " <<  expt   << "ms" << std::endl
                       << "illumination_mode: " << illMode << std::endl
                       << "cube: " << cube << std::endl
                     //  << "shutter: " << shutter << std::endl
                       << "captureTime: " << Images[i].captureTime << std::endl
                       << "processTime: " << Images[i].processTime << std::endl;
              metadata.close();

              std::cout<< "Elapsed time for 1 full resolution image after saving the image and the metadata:" << mClock.getTime()-getTrackFullImageAcq() << std::endl;
              TRACKER_INFO("Stored meta data '" + Images[i].path + Images[i].distinction +".txt" + "'", Logger::Low);
          }         */
        //    if (i == 0){ // Actually if there are more picture in the buffer it will show the last one.
                // Removes and deletes all item from the scene to prevent memory overflow.
                previewGraphicsView->mGraphicsScene->clear();
                // Display image in the preview screen
                mPreviewPixmapItem = new QGraphicsPixmapItem(QPixmap::fromImage(image,0));
                //mPreviewScene = new QGraphicsScene;
                previewGraphicsView->mGraphicsScene->addItem(mPreviewPixmapItem);
                //mPreviewScene->addItem(mPreviewPixmapItem);

                /*
                previewGraphicsView->setScene(mPreviewScene);
                previewGraphicsView->fitInView(mPreviewScene->sceneRect());
                */
                previewGraphicsView->setScene(previewGraphicsView->mGraphicsScene);
                previewGraphicsView->fitInView(previewGraphicsView->mGraphicsScene->sceneRect(),Qt::KeepAspectRatio);
               //QGraphicsView::fitInView(scene.sceneRect());
                previewGraphicsView->show();
      //      }
      //      i = i-1;
      //      Images.pop_back();

      //  }
      //  std::cout<< "Elapsed time for "<< Counter << " full resolution image:" << mClock.getTime()-getTrackFullImageAcq() << std::endl;
       // Counter = 0;
     // }
  //  else{
      //  Counter +=1;
     //   return;
   // }

    /*! image.save(path + distinction + format);
    TRACKER_INFO("Stored Image '" + path + distinction + format + "'", Logger::Low);

    std::cout<< "Elapsed time for 1 full resolution image after saving the image:" << mClock.getTime()-getTrackFullImageAcq() << std::endl;

    // Write metadata to file.

    int tIlluminationMethod = mAhmmicroscope->getContrastingMethod();
    std::string illMode;
    switch(tIlluminationMethod){
      case ahmMicroscope::ilWhiteLight:
        illMode = "white light";
        break;
      case ahmMicroscope::ilFluorescence:
        illMode = "fluorescence";
        break;
      default:
        illMode = "unknown";
        break;
    }
    std::string cube;
    int cubePos = mAhmmicroscope->getturretposition();
    if(tIlluminationMethod == ahmMicroscope::ilFluorescence){
      switch(cubePos){
        case 1:
          cube = "Cy3";
          break;
        case 2:
          cube = "GFP";
          break;
        case 3:
          cube = "DAPI";
          break;
        default:
          cube = "unknown";
          break;
      }
    }else{
      cube = "none";
    }
    std::string shutter;
    if(1 == mAhmmicroscope->getFlShutterState()){
      shutter = "open";
    } else{
      shutter = "close";
    }

    QByteArray ba = (path + distinction + ".txt").toLocal8Bit();
    const char *filename = ba.data();
    std::ofstream metadata(filename);
    if(metadata.is_open()){
      metadata << "x: " << mStage->getXpos() << std::endl
               << "y: " << mStage->getYpos() << std::endl
               << "z: " << mStage->getZpos() << std::endl
               << "pressure: " << -10*(10-mDac->readvoltage()) << std::endl
               << "gain: " << gainBox->value() << std::endl
               << "exposure_time: " << ((double)mCamera->getExposureTime()/1000) << "ms" << std::endl
               << "illumination_mode: " << illMode << std::endl
               << "cube: " << cube << std::endl
               << "shutter: " << shutter << std::endl
               << "captureTime: " << captureTime << std::endl
               << "processTime: " << processTime << std::endl;

      metadata.close();
    }


    std::cout<< "Elapsed time for 1 full resolution image after saving the image and the metadata:" << mClock.getTime()-getTrackFullImageAcq() << std::endl;

    TRACKER_INFO("Stored meta data '" + path + distinction + ".txt" + "'", Logger::Low);
*/
/// Completely not used stuff but leave it here for a second
//
    //storageRunSpinBox->setValue(storageRunSpinBox->value()+1);
    storageRunSpinBox->setValue(storageRunSpinBoxValue + 1);
    //mStoragePath->setSpinBoxValue(storageRunSpinBoxValue + 1);
    if(mStoragePath != NULL) {
      mStoragePath->settingsStorageRunSpinBox->setValue(storageRunSpinBoxValue + 1);
    }
    //emit setStoragePathSpinBoxValue(storageRunSpinBoxValue + 1);
/*!
    // Removes and deletes all item from the scene to prevent memory overflow.
    previewGraphicsView->mGraphicsScene->clear();
    // Display image in the preview screen
    mPreviewPixmapItem = new QGraphicsPixmapItem(QPixmap::fromImage(image,0));
    //mPreviewScene = new QGraphicsScene;
    previewGraphicsView->mGraphicsScene->addItem(mPreviewPixmapItem);
    //mPreviewScene->addItem(mPreviewPixmapItem);
*/
    /*
    previewGraphicsView->setScene(mPreviewScene);
    previewGraphicsView->fitInView(mPreviewScene->sceneRect());
    */
/*!    previewGraphicsView->setScene(previewGraphicsView->mGraphicsScene);
    previewGraphicsView->fitInView(previewGraphicsView->mGraphicsScene->sceneRect(),Qt::KeepAspectRatio);
   //QGraphicsView::fitInView(scene.sceneRect());
    previewGraphicsView->show();
*/
///!!!     std::cout<< "Elapsed time for 1 full resolution image:" << mClock.getTime()-getTrackFullImageAcq() << std::endl;
  }

  QString MainWindow::getFilenameSuffix()
  {
    //return QDateTime::currentDateTime().toString("_yyyy-MM-dd_hh-mm-ss-zzz_") +
    //  QString::number(pressureLCDNumber->value(), 'f', 2) + // QString::number(mPressureSensor->getPressure()
    //   "kPa";
    return QDateTime::currentDateTime().toString("_yyyyMMdd_") +
           QString::number(pressureLCDNumber->value(), 'f', 2) + // QString::number(mPressureSensor->getPressure()
           "kPa" + "_pos_" + QString::number(mCurrentlistposition);
  }

  /**************************************************************************/
  /****************************** Camera ************************************/
  /**************************************************************************/

  void MainWindow::on_cameraStartToolButton_clicked(bool checked)
  {
    if (checked) {
      this->updateWidgetActivities();

#ifdef TRACKER_DUMMY
      DummyCamera* dummyCam = dynamic_cast<DummyCamera*>(mCamera.get());
      assert(dummyCam);
      connect(mMicroscope.get(), SIGNAL(offsetChanged(QPointF, double, quint64)),
              dummyCam,          SLOT(makeStackImage(QPointF, double, quint64)));
#endif

      mStoringImage = false;

      connect(mCamera.get(), SIGNAL(imageProcessed(QImage, quint64, quint64)),
              this,          SLOT(displayImage(const QImage, quint64, quint64)));

      mCamera->moveToThread(mCameraThread);
      mCameraThread->start();
    } else {
      // No controller without camera (UI counts on that!)
      mControllerThread->quit();
      mCameraThread->quit();
      // Thread might not quit immediately
      cameraStartToolButton->setChecked(true);
    }
  }

  void MainWindow::cameraFinished()
  {
#ifdef TRACKER_DUMMY
    DummyCamera* dummyCam = dynamic_cast<DummyCamera*>(mCamera.get());
    disconnect(mMicroscope.get(), SIGNAL(offsetChanged(QPointF, double, quint64)),
               dummyCam,          SLOT(makeStackImage(QPointF, double, quint64)));
#endif

    cameraStartToolButton->setChecked(false);
    this->updateWidgetActivities();
    this->displayCameraFrameRate(0);
  }

  void MainWindow::cameraModeBox_currentIndexChanged(int index)
  {
    assert(mCamera->hasMode(cameraModeBox->currentText()));

    // Adjust exposure time linearly with image area
    QSize size = mCamera->getImageSize(cameraModeBox->currentText());
    if (mCurrentImageArea > 0) {
      double areaRatio = (double)(size.width() * size.height()) / mCurrentImageArea;
      int newExposureTime = qRound(mExposureTimeBox->value() * areaRatio);
      mExposureTimeBox->setValue(newExposureTime);
    }
    mCurrentImageArea = size.width() * size.height();

    emit cameraModeChanged(cameraModeBox->currentText(), -1, -1.0);
    ///Trigger
    mAction = true;
    this->updateWidgetActivities();

    if (displayModeBox->currentIndex() != 0)
      return; // Controller only affected by tracking image (0)

    mController->setOptionsKey(mCamera->getName() + "__sep__" + cameraModeBox->currentText());

    const int goodValues[] = {
      // Contains only numbers of the form s = 2^n * 3^m | 64 <= s <= 2048
      64,   72,   81,   96,  108,  128,  144,  162, 192, 216, 243, 256,
      288,  324,  384,  432,  486,  512,  576,  648, 729, 768, 864, 972,
      1024, 1152, 1296, 1458, 1536, 1728, 1944, 2048
    };
    const int nGoodValues = sizeof(goodValues) / sizeof(goodValues[0]);

    // Protect against unwanted changes to the Controller
    mIgnoreFFTImageSizeChanges = true;

    fftImageSizeXBox->clear();
    for (int i = 0; i < nGoodValues; ++i)
      if (goodValues[i] <= size.width())
        fftImageSizeXBox->addItem(QString::number(goodValues[i]));

    fftImageSizeYBox->clear();
    for (int i = 0; i < nGoodValues; ++i)
      if (goodValues[i] <= size.height())
        fftImageSizeYBox->addItem(QString::number(goodValues[i]));

    mFFTSizeXValidator->setTop(size.width());
    mFFTSizeYValidator->setTop(size.height());

    mIgnoreFFTImageSizeChanges = false;

    maxStageMoveBox->setValue(mController->getMaxTotalStageMove());
    tuningStepBox->setValue(mController->getTuningStep());
    pixelSizeXBox->setValue(mController->getPixelSize().x());
    pixelSizeYBox->setValue(mController->getPixelSize().y());
    fftImageSizeXBox->setEditText(QString::number(mController->getFFTImageSize().width()));
    fftImageSizeYBox->setEditText(QString::number(mController->getFFTImageSize().height()));
    controllerGainBox->setValue(mController->getControllerGain());
    stageCommandDelayBox->setValue(mController->getStageCommandDelay());
    correlatorDepthBox->setValue(mController->getCorrelatorDepth());
    predictorSizeBox->setValue(mController->getPredictorSize());
  }

  void MainWindow::displayModeBox_currentIndexChanged(int index)
  {
    if (mPreviousDisplayModeIndex >= 0) {
      // Save settings first
      DisplayMode& mode = mDisplayModes[mPreviousDisplayModeIndex];
      mode.cameraMode = cameraModeBox->currentText();
      mode.exposureTime = mExposureTimeBox->value();
      mode.gain = gainBox->value();
      mode.zoom = mImageLabel->getZoom();
    }
    mPreviousDisplayModeIndex = index;

    DisplayMode& mode = mDisplayModes[index];
    mCurrentImageArea = -1; // --> Don't adjust the exposure time
    if (cameraModeBox->findText(mode.cameraMode) != cameraModeBox->currentIndex())
      cameraModeBox->setCurrentIndex(cameraModeBox->findText(mode.cameraMode));
    else
      this->cameraModeBox_currentIndexChanged(cameraModeBox->currentIndex());
    mExposureTimeBox->setValue(mode.exposureTime);
    gainBox->setValue(mode.gain);
    mImageLabel->setZoom(mode.zoom);

    // Clear image because of possible zoom issues
    mImageLabel->clearImage();

    this->updateWidgetActivities();
    ///
    mAction = true;
    std::cout<<"test: "<<index<<std::endl;
  }

  void MainWindow::on_removeDisplayModeButton_clicked()
  {
    if (displayModeBox->currentText() == "Tracking")
      return;

    QMessageBox::StandardButton result = QMessageBox::question(this, "Display Mode Deletion",
                                         "Are you sure you want to delete that display mode?",
                                         QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (result == QMessageBox::Yes) {
      int index = displayModeBox->currentIndex();
      mDisplayModes.remove(index);
      mPreviousDisplayModeIndex = -1; // Nothing so save
      displayModeBox->removeItem(index);

      QWidget* button = storageHL_1->itemAt(index - 1)->widget();
      storageHL_1->removeWidget(button);
      if(mStoragePath != NULL) {
        mStoragePath->storageHL->removeWidget(button);
      }
      delete button;

      delete mSingleShotButtonShortcuts.last();
      mSingleShotButtonShortcuts.pop_back();

      // Reassign shortcuts
      for (int i = 0; i < mSingleShotButtonShortcuts.size(); ++i) {
        mSingleShotButtonShortcuts[i]->disconnect();
        connect(mSingleShotButtonShortcuts[i],    SIGNAL(activated()),
                storageHL_1->itemAt(i)->widget(), SLOT(click()));
        if(mStoragePath != NULL) {
          connect(mSingleShotButtonShortcuts[i],                    SIGNAL(activated()),
                  mStoragePath->storageHL->itemAt(i)->widget(), SLOT(click()));
        }
        if (i < 4) {
          storageHL_1->itemAt(i)->widget()->setToolTip("Store single image in this mode [F" + QString::number(i + 1) + "]");
          if(mStoragePath != NULL) {
            mStoragePath->storageHL->itemAt(i)->widget()->setToolTip("Store single image in this mode [F" + QString::number(i + 1) + "]");
          }
        } else {
          storageHL_1->itemAt(i)->widget()->setToolTip("Store single image in this mode");
          if(mStoragePath != NULL) {
            mStoragePath->storageHL->itemAt(i)->widget()->setToolTip("Store single image in this mode");
          }
        }
      }
    }
  }

  void MainWindow::on_newDisplayModeButton_clicked()
  {
    bool ok;
    QString name = QInputDialog::getText(this, "New Display Mode", "Name:", QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty() && displayModeBox->findText(name) == -1) {
      if (name.contains(','))
        TRACKER_WARNING("The name may not contain ','");
      else
        this->addDisplayMode(name);
    }
  }

  void MainWindow::addDisplayMode(QString name)
  {
    DisplayMode mode = {name, cameraModeBox->itemText(0), 10000, 1.0, 1.0};
    mDisplayModes.append(mode);
    int index = mDisplayModes.count() - 1;
    mDisplayModes[index].name = name;

    if (name != "Tracking") {
      QPushButton* button = new QPushButton(QIcon(":/tracker/icons/misc/camera_32.png"), name, this);
      button->setMinimumSize(100, 30);
      button->setIconSize(QSize(24, 24));
      storageHL_1->insertWidget(index - 1, button);
      if(mStoragePath != NULL) {
        mStoragePath->storageHL->insertWidget(index - 1, button);
      }
      //mStoragePath->insertWidget(index - 1, button);
      mSingleShotButtonMapper->setMapping(button, name);
      connect(button, SIGNAL(clicked()), mSingleShotButtonMapper, SLOT(map()));

      QShortcut* shortcut = NULL;
      if (index <= 4) { // Only have F1 - F4
        shortcut = new QShortcut("F" + QString::number(index), this);
        connect(shortcut, SIGNAL(activated()), button, SLOT(click()));
        button->setToolTip("Store single image in this mode [F" + QString::number(index) + "]");
      } else
        button->setToolTip("Store single image in this mode");
      mSingleShotButtonShortcuts.append(shortcut);
    }

    // Do this only later because it sets off some events
    displayModeBox->addItem(name);

    this->updateWidgetActivities();
  }

  void MainWindow::on_mExposureTimeBox_valueChanged(int value)
  {
    if (!mInitialised)
      return;

    double temp = std::log10((double)value / mExposureTimeBox->minimum());
    temp /= std::log10((double)mMaxSliderExposureTime / mExposureTimeBox->minimum());
    temp *= exposureTimeSlider->maximum();
    int sliderValue = (int)(temp + 0.5);
    if (sliderValue != exposureTimeSlider->value()) {
      mDiscardExposureTimeValue = true;
      exposureTimeSlider->setValue(sliderValue);
    }
    // update exposuretime without changing the mode

    DisplayMode& mode = mDisplayModes[displayModeBox->currentIndex()];
    mode.exposureTime = mExposureTimeBox->value();
    //
  }

  void MainWindow::on_exposureTimeSlider_valueChanged(int value)
  {
    if (!mInitialised)
      return;

    // What's with mDiscardExposureTimeValue? --> see on_stageStepBox_valueChanged
    if (!mDiscardExposureTimeValue) {
      double temp = (double)value / exposureTimeSlider->maximum();
      temp *= std::log10((double)mMaxSliderExposureTime / mExposureTimeBox->minimum());
      temp += std::log10((double)mExposureTimeBox->minimum());
      temp = std::pow(10.0, temp);
      int boxValue = (int)(temp + 0.5);
      mExposureTimeBox->setValue(boxValue);
    } else
      mDiscardExposureTimeValue = false;
  }

  void MainWindow::on_gainBox_valueChanged(double value)
  {
    if (!mInitialised)
      return;

    double temp = (value - gainBox->minimum()) / (gainBox->maximum() - gainBox->minimum());
    int sliderValue = (int)(temp * gainSlider->maximum() + 0.5);
    if (sliderValue != gainSlider->value()) {
      mDiscardGainValue = true;
      gainSlider->setValue(sliderValue);
    }

    // update gain without changing the mode

    DisplayMode& mode = mDisplayModes[displayModeBox->currentIndex()];
    mode.gain = gainBox->value();
    //

  }

  void MainWindow::on_gainSlider_valueChanged(int value)
  {
    if (!mInitialised)
      return;

    // What's with mDiscardGainValue? --> see on_stageStepBox_valueChanged
    if (!mDiscardGainValue) {
      double temp = (double)value / gainSlider->maximum();
      double boxValue = temp * (gainBox->maximum() - gainBox->minimum()) + gainBox->minimum();
      gainBox->setValue(boxValue);
    } else
      mDiscardGainValue = false;
  }

  void MainWindow::displayCameraFrameRate(double frameRate)
  {
    if (mCameraThread->isRunning())
      mCameraFrameRateLabel->setText(QString::number(frameRate, 'f', 1));
    else
      mCameraFrameRateLabel->setText("0");
  }


  /**************************************************************************/
  /************************ Tracker & Options *******************************/
  /**************************************************************************/

  void MainWindow::on_trackerStartToolButton_clicked(bool checked)
  {
    if (checked) {
      if (this->startController(Controller::Tracking) != 0)
          trackerStartToolButton->setChecked(false);
          // Set mode for imaging
      // allowing feedback from camera to controller
       mCamera->Feedbacktotracker(true);
       mMainChannel = channel;//MainChannel_spinBox->value();
    } else {
      //TODO: temporal solution of the multiple image acquistion 20170608
      mController->setAutofocusModeAfterLastPressureChange(false);      
      // Thread might not quit immediately
      mControllerThread->quit();
      // decoupling the multiimage acquistion from controller
      mCamera->Feedbacktotracker(false);
      trackerStartToolButton->setChecked(true);
    }
  }

  void MainWindow::on_timingStartToolButton_clicked(bool checked)
  {
    if (checked) {
      if (this->startController(Controller::Timing) != 0)
        timingStartToolButton->setChecked(false);
    } else {
      mControllerThread->quit();
      // Thread might not quit immediately
      timingStartToolButton->setChecked(true);
    }
  }

  void MainWindow::on_measuringStartToolButton_clicked(bool checked)
  {
    if (checked) {
      if (this->startController(Controller::Measuring) != 0)
        measuringStartToolButton->setChecked(false);
    } else {
      mControllerThread->quit();
      // Thread might not quit immediately
      measuringStartToolButton->setChecked(true);
    }
  }

  void MainWindow::on_zstackStartToolButton_clicked(bool checked)
  {
    if (checked) {
      if (this->startController(Controller::ZStack) != 0)
        zstackStartToolButton->setChecked(false);
    } else {
      mControllerThread->quit();
      // Thread might not quit immediately
      zstackStartToolButton->setChecked(true);
    }
  }

  int MainWindow::startController(Controller::Mode mode)
  {
    if (mAhmmicroscope->getFlShutterState() == 0) {
        TRACKER_WARNING("Please open shutter before starting tracker.");
        return -1;
    }

    // set controller mode
    mController->setMode(mode);
    mController->setExposureTime(mExposureTimeBox->value());

    // Enable/Disable controls
    this->updateWidgetActivities();

    mTrackerAnimation->start();

    // Set controller options
    mController->setZStackEnabled(checkBoxZStack->isChecked()); // enable Z stack autocorrection
    mController->setZStackAcquisitionEnabled(multiIm->isChecked()); // enable Z stack auto acquisition

    mController->setXYStageEnabledBlocking(checkBoxEnableXYStageBlocking->isChecked());
    mController->setZStageEnabledBlocking(checkBoxEnableZStageBlocking->isChecked());
    mController->setUseEstimatedWindowSize(checkBoxUseEstimatedWindowSize->isChecked());
    mController->setUseNoiseLevelAtBrenner(checkBoxUseNoiseLevelAtBrenner->isChecked());

    mController->setAdaptiveAutoFocus(checkBoxAdaptiveAutoFocus->isChecked());
    mController->setXYTrackingEnabled(checkBoxXYTracking->isChecked());
    mController->setBrennerEnabled(checkBoxEnableBrenner->isChecked());
    mController->setUpperBrennerThreshold(spinBoxUpperBrennerThreshold->value() / 100.0);
    mController->setLowerBrennerThreshold(spinBoxLowerBrennerThreshold->value() / 100.0);
    mController->setLargeCorrectionStep(doubleSpinBoxLargeCorrectionStep->value());
    mController->setXYTrackerDurationOscillation(doubleSpinBoxOscillationDuration->value());
    mController->setZStackSize(spinBoxZStackSize->value());
    mController->setZStackStepSize(spinBoxZStackStepSize->value());
    mController->setBrennerRoiPercentage(spinBoxBrennerRoi->value());
    // Setting the number of planes for the Zstack acquisition
    //mCamera->setNumOfPlanes(SizeOfZstack->value());

    connect(mCamera.get(),     SIGNAL(imageProcessed(QImage, quint64, quint64)),
            mController.get(), SLOT(trackImage(const QImage, quint64, quint64)));

    mController->moveToThread(mControllerThread);
    mController->setPropGainZ(focuscontrollergainSpinBox->value());

    // setting camera options for the logfile
    mController->setExposureTime( mCamera->getExposureTime() );
    // mController->setCameraImageSize( mCamera->getImageSize(mCamera->getMode()) );
    mController->setStorageFilename( storageFilename );

    // protocol information (optimally, should be a specific object)
    mController->setCorrectionFactor( corrfactorSpinbox->value() );
    mController->setTimestepDuration( timestepspinbox->value() );
    mController->setMaxAmplitude( amplitudespinbox->value() );
    mController->setNumberOfPressureSteps( pressurestepsspinbox->value() );

    mControllerThread->start();

    return 0;
  }

  void MainWindow::controllerFinished()
  {
    disconnect(mCamera.get(),     SIGNAL(imageProcessed(QImage, quint64, quint64)),
               mController.get(), SLOT(trackImage(const QImage, quint64, quint64)));

    // Stop all stage movements (security measure)
    mStage->stopAll();

    mController->setAutofocusModeAfterLastPressureChange(false);

    trackerStartToolButton->setChecked(false);
    timingStartToolButton->setChecked(false);
    measuringStartToolButton->setChecked(false);
    zstackStartToolButton->setChecked(false);

    this->updateWidgetActivities();
    this->displayControllerFrameRate(0);
    mTrackerAnimation->stop();
  }

  void MainWindow::on_fftImageSizeXBox_editTextChanged(QString text)
  {
    if (mInitialised && !mIgnoreFFTImageSizeChanges)
      mController->setFFTImageSizeX(qMax(1, text.toInt()));
  }

  void MainWindow::on_fftImageSizeYBox_editTextChanged(QString text)
  {
    if (mInitialised && !mIgnoreFFTImageSizeChanges)
      mController->setFFTImageSizeY(qMax(1, text.toInt()));
  }

  void MainWindow::displayControllerFrameRate(double frameRate)
  {
    if (mControllerThread->isRunning())
      mControllerFrameRateLabel->setText(QString::number(frameRate, 'f', 1));
    else
      mControllerFrameRateLabel->setText("0");
  }

  /**************************************************************************/
  /****************************** Stage *************************************/
  /**************************************************************************/

  void MainWindow::on_stageRightToolButton_pressed()
  {
    emit stageMoveRequested(QPointF(stageStepBox->value(), 0.0));
  }

  void MainWindow::on_stageUpRightToolButton_pressed()
  {
    emit stageMoveRequested(QPointF(stageStepBox->value(), -stageStepBox->value()));
  }

  void MainWindow::on_stageUpToolButton_pressed()
  {
    emit stageMoveRequested(QPointF(0.0, -stageStepBox->value()));
  }

  void MainWindow::on_stageUpLeftToolButton_pressed()
  {
    emit stageMoveRequested(QPointF(-stageStepBox->value(), -stageStepBox->value()));
  }

  void MainWindow::on_stageLeftToolButton_pressed()
  {
    emit stageMoveRequested(QPointF(-stageStepBox->value(), 0.0));
  }

  void MainWindow::on_stageDownLeftToolButton_pressed()
  {
    emit stageMoveRequested(QPointF(-stageStepBox->value(), stageStepBox->value()));
  }

  void MainWindow::on_stageDownToolButton_pressed()
  {
    emit stageMoveRequested(QPointF(0.0, stageStepBox->value()));
  }

  void MainWindow::on_stageDownRightToolButton_pressed()
  {
    emit stageMoveRequested(QPointF(stageStepBox->value(), stageStepBox->value()));
  }

  void MainWindow::on_stageStepBox_valueChanged(int value)
  {
    if (!mInitialised)
      return;

    // Note:
    // mDiscardStageStepValue is part of a little workaround because changing
    // the value of either the dial or the spin box triggers the valueChanged
    // event (which we need for the user interactions). It would work without
    // the workaround but like this for example:
    // You change the spin box value from 78 to 79, you get into this function
    // that then calculates a value for the dial which may not even change it
    // because of rounding. However if the value of the dial does in fact change,
    // it's valueChanged function will be called that calculates a new value
    // for the box that is not exactly the same as we originally went for.

    double temp = std::log10((double)value) / 4.0 * stageStepDial->maximum();
    int dialValue = (int)(temp + 0.5);
    if (dialValue != stageStepDial->value()) {
      mDiscardStageStepValue = true;
      stageStepDial->setValue(dialValue);
    }
  }

  void MainWindow::on_stageStepDial_valueChanged(int value)
  {
    if (!mInitialised)
      return;

    // What's with mDiscardStageStepValue? --> see on_stageStepBox_valueChanged
    if (!mDiscardStageStepValue) {
      double temp = std::pow(10.0, value * 4.0 / stageStepDial->maximum());
      int boxValue = (int)(temp + 0.5);
      stageStepBox->setValue(boxValue);
    } else
      mDiscardStageStepValue = false;
  }

  void MainWindow::on_mImageLabel_mouseDragged(QPoint value)
  {
    emit stageMoveRequested(mController->stageCoordinates(QPointF(value) / mImageLabel->getZoom()));
  }

  void MainWindow::on_ZoomIncButton_pressed()
  {
    mImageLabel->setZoom(mImageLabel->getZoom() * (0.1 + 1));
  }
  void MainWindow::on_ZoomDecButton_pressed()
  {
    mImageLabel->setZoom(mImageLabel->getZoom() / (0.1 + 1));
  }
  void MainWindow::wheelzmove(double scrollamount)
  {
    mController->moveStageZ(scrollamount * WZfactorSpinBox->value(), 0);
  }

  /**************************************************************************/
  /**************************** Pressure ************************************/
  /**************************************************************************/

  void MainWindow::displayPressure(double value)
  {
    pressureLCDNumber->display(value);

    // pass on the pressure value to a log file managed in the controller
    // and use a generated timestamp there 
	mController->writePressureInfoToLogFile(value);

    /*
    mPressureGauge->setValue((int)(value * 10 + 0.5));
    */
    /*
    if(mTimeValues.last() >= 30) {
      mPressureValues.resize(0);
      mTimeValues.resize(0);
      mTimeValues.append(0);
    }
    mPressureValues.append(value * 10 + 0.5);
    mTimeValues.append(mTimeValues.last() + 1);
    mPressurePlot->graph(0)->setData(mTimeValues, mPressureValues);
    mPressurePlot->replot();
    */
  }
  void MainWindow::on_setpressureButton_pressed()
  {
    emit pressureSetRequested(-(setpressurespinBox->value()));
  }
  void MainWindow::on_setpressure2Button_pressed()
  {
    emit pressureSetRequested(-(setpressure2spinBox->value()));
  }
  void MainWindow::on_setpressure3Button_pressed()
  {
    emit pressureSetRequested(-(setpressure3spinBox->value()));
  }

  /**************************************************************************/
  /***************************** zTrack** ***********************************/
  /**************************************************************************/


  void MainWindow::on_ButtonMoveUp_pressed()
  {
    emit ZMoveRequested(ZmoveSpinBox->value(), 0);
  }
  void MainWindow::on_ButtonMoveDown_pressed()
  {
    emit ZMoveRequested(-(ZmoveSpinBox->value()), 0);
  }

  void MainWindow::on_ButtonMoveUp_2_pressed()
  {
    emit ZMoveRequested(ZmoveSpinBox_2->value(), 0);
  }
  void MainWindow::on_ButtonMoveDown_2_pressed()
  {
    emit ZMoveRequested(-(ZmoveSpinBox_2->value()), 0);
  }
  void MainWindow::on_ButtonMoveUp_3_pressed()
  {
    emit ZMoveRequested(ZmoveSpinBox_3->value(), 0);
  }
  void MainWindow::on_ButtonMoveDown_3_pressed()
  {
    emit ZMoveRequested(-(ZmoveSpinBox_3->value()), 0);
  }
  void MainWindow::on_clearsafetyButton_pressed()
  {
    if (mSafety==true) {
      mSafety=false;
      mStage->clearZlimits();
      safetyLabel->setText("<font color='red'>safety OFF!</font>");
      clearsafetyButton->setText("Set safety");
    } else {
      mSafety=true;
      mStage->setZlimits(mStage->getZpos());
      safetyLabel->setText("Safety ON");
      clearsafetyButton->setText("Clear safety");
    }
  }

  /**
   * @brief Go back home for example when the sample is dismounted.
   */
  void MainWindow::on_stageGoHomeButton_clicked(){
    /**
     * @todo Try distance
     */
    emit ZMoveRequested(20000, 0);
  }

  void MainWindow::on_savestateButton_pressed()
  {
    //get state and add it to list

    ExperimentState* newitem = new ExperimentState;

    QString positionstring;

    mPoslist->append(newitem);

    QWidget* xyPosWdg = new QWidget();
    QWidget* zPosWdg = new QWidget();
    QWidget* pressureWdg = new QWidget();
    QWidget* gainWdg = new QWidget();
    QWidget* exposuretimeWdg = new QWidget();
    QWidget* illuminationWdg = new QWidget();
    QWidget* cubeWdg = new QWidget();
    QWidget* intensityWdg = new QWidget();
    QWidget* shutterWdg = new QWidget();

    QHBoxLayout* xyPosaLayout = new QHBoxLayout(xyPosWdg);
    QHBoxLayout* zPosLayout = new QHBoxLayout(zPosWdg);
    QHBoxLayout* gainLayout = new QHBoxLayout(gainWdg);
    QHBoxLayout* exposuretimeLayout = new QHBoxLayout(exposuretimeWdg);
    QHBoxLayout* pressureLayout = new QHBoxLayout(pressureWdg);
    QHBoxLayout* illuminationLayout = new QHBoxLayout(illuminationWdg);
    QHBoxLayout* cubeLayout = new QHBoxLayout(cubeWdg);
    QHBoxLayout* intensityLayout = new QHBoxLayout(intensityWdg);
    QHBoxLayout* shutterLayout = new QHBoxLayout(shutterWdg);

    mPoslist->last()->xpos = mStage->getXpos();
    mPoslist->last()->xpositem= new QTableWidgetItem(positionstring.sprintf("%.2f",mStage->getXpos()));
    mPoslist->last()->ypos = mStage->getYpos();
    mPoslist->last()->ypositem= new QTableWidgetItem(positionstring.sprintf("%.2f",mStage->getYpos()));

    mPoslist->last()->xyPosItem = new QTableWidgetItem(positionstring.sprintf("%.2f;%.2f",mStage->getXpos(),mStage->getYpos()));
    mPoslist->last()->xyPosCheckBox = new QCheckBox();
    mPoslist->last()->xyPosCheckBox->setChecked(true);

    mPoslist->last()->zpos = mStage->getZpos();
    mPoslist->last()->zPosItem = new QTableWidgetItem(positionstring.sprintf("%.2f",mStage->getZpos()));
    mPoslist->last()->zposCheckBox = new QCheckBox();
    mPoslist->last()->zposCheckBox->setChecked(true);

    mPoslist->last()->pressure = -10*(10-mDac->readvoltage());
    mPoslist->last()->pressureItem = new QTableWidgetItem(positionstring.sprintf("%.2f",-10*(10-mDac->readvoltage())));
    mPoslist->last()->pressureCheckBox = new QCheckBox();
    mPoslist->last()->pressureCheckBox->setChecked(false);

    mPoslist->last()->gain = gainBox->value();
    mPoslist->last()->gainItem = new QTableWidgetItem(positionstring.sprintf("%.2f",gainBox->value()));
    mPoslist->last()->gainCheckBox = new QCheckBox();
    mPoslist->last()->gainCheckBox->setChecked(true);

    mPoslist->last()->exposuretime = mCamera->getExposureTime();
    mPoslist->last()->exposuretimeItem = new QTableWidgetItem(positionstring.sprintf("%.2f",((double)mCamera->getExposureTime()/1000)));
    mPoslist->last()->exposuretimeCheckBox = new QCheckBox();
    mPoslist->last()->exposuretimeCheckBox->setChecked(true);

    // Get current illumination method.
    int tIlluminationMethod = mAhmmicroscope->getContrastingMethod();
    QString illMode;
    switch(tIlluminationMethod){
      case ahmMicroscope::ilWhiteLight:
        illMode = "TL";
        break;
      case ahmMicroscope::ilFluorescence:
        illMode = "IL";
        break;
      default:
        illMode = "unknown";
        break;
    }

    mPoslist->last()->illumination = tIlluminationMethod;
    mPoslist->last()->illuminationItem = new QTableWidgetItem(illMode);
    mPoslist->last()->illuminationCheckBox = new QCheckBox();
    mPoslist->last()->illuminationCheckBox->setChecked(true);

    // Get current filter from the microscope and set it according to it or set "none" if illumination mode is white light.
    mPoslist->last()->cubeItem = new QTableWidgetItem();
    QString cube;
    int cubePos = mAhmmicroscope->getturretposition();
    if(tIlluminationMethod == ahmMicroscope::ilFluorescence){
      switch(cubePos){
        case 1:
          cube = "Green";
          break;
        case 2:
          cube = "Red";
          break;
        case 3:
          cube = "Blue";
          break;
        case 4:
          cube = "FarRed";
          break;
        default:
          cube = "Multiband";
          break;
      }
    }else{
      cube = "none";
    }

    mPoslist->last()->cube = cubePos;
    mPoslist->last()->cubeItem->setText(cube);
    mPoslist->last()->cubeCheckBox = new QCheckBox();
    mPoslist->last()->cubeCheckBox->setChecked(true);
    mPoslist->last()->intensityCheckBox = new QCheckBox();
    mPoslist->last()->intensityCheckBox->setChecked(true);

    QString shutter;
    if(1 == mAhmmicroscope->getFlShutterState()){
      shutter = "open";
    } else{
      shutter = "close";
    }
    mPoslist->last()->shutterItem = new QTableWidgetItem(shutter);
    mPoslist->last()->shutterCheckBox = new QCheckBox();
    mPoslist->last()->shutterCheckBox->setChecked(true);

    mPoslist->last()->ButtonItem = new QPushButton("Go To",mPoslistwidget);
    mPoslist->last()->DeleteButtonitem = new QPushButton("Delete",mPoslistwidget);

    mPoslist->last()->cube = cubePos;
    mPoslist->last()->intensity = mAhmmicroscope->getintensity();
    mPoslist->last()->intensityItem = new QTableWidgetItem(positionstring.sprintf("%d",mAhmmicroscope->getintensity()));
    mPoslist->last()->shutter = mAhmmicroscope->getFlShutterState();

    xyPosaLayout->addWidget(mPoslist->last()->xyPosCheckBox);
    zPosLayout->addWidget(mPoslist->last()->zposCheckBox);
    pressureLayout->addWidget(mPoslist->last()->pressureCheckBox);
    gainLayout->addWidget(mPoslist->last()->gainCheckBox);
    exposuretimeLayout->addWidget(mPoslist->last()->exposuretimeCheckBox);
    illuminationLayout->addWidget(mPoslist->last()->illuminationCheckBox);
    cubeLayout->addWidget(mPoslist->last()->cubeCheckBox);
    intensityLayout->addWidget(mPoslist->last()->intensityCheckBox);
    shutterLayout->addWidget(mPoslist->last()->shutterCheckBox);

    xyPosaLayout->setAlignment(Qt::AlignHCenter);
    zPosLayout->setAlignment(Qt::AlignHCenter);
    pressureLayout->setAlignment(Qt::AlignHCenter);
    gainLayout->setAlignment(Qt::AlignHCenter);
    exposuretimeLayout->setAlignment(Qt::AlignHCenter);
    illuminationLayout->setAlignment(Qt::AlignHCenter);
    cubeLayout->setAlignment(Qt::AlignHCenter);
    intensityLayout->setAlignment(Qt::AlignHCenter);
    shutterLayout->setAlignment(Qt::AlignHCenter);

    mPoslistwidget->setItem(mPoslist->length()-1,1,mPoslist->last()->xyPosItem);
    mPoslistwidget->setCellWidget(mPoslist->length()-1,2,xyPosWdg);
    mPoslistwidget->setItem(mPoslist->length()-1,3,mPoslist->last()->zPosItem);
    mPoslistwidget->setCellWidget(mPoslist->length()-1,4,zPosWdg);
    mPoslistwidget->setItem(mPoslist->length()-1,5,mPoslist->last()->pressureItem);
    mPoslistwidget->setCellWidget(mPoslist->length()-1,6,pressureWdg);
    mPoslistwidget->setItem(mPoslist->length()-1,7,mPoslist->last()->gainItem);
    mPoslistwidget->setCellWidget(mPoslist->length()-1,8,gainWdg);
    mPoslistwidget->setItem(mPoslist->length()-1,9,mPoslist->last()->exposuretimeItem);
    mPoslistwidget->setCellWidget(mPoslist->length()-1,10,exposuretimeWdg);
    mPoslistwidget->setItem(mPoslist->length()-1,11,mPoslist->last()->illuminationItem);
    mPoslistwidget->setCellWidget(mPoslist->length()-1,12,illuminationWdg);
    mPoslistwidget->setItem(mPoslist->length()-1,13,mPoslist->last()->cubeItem);
    // Change the background color of the filter/cube item according to the used filter.
    if(tIlluminationMethod == ahmMicroscope::ilFluorescence){
      switch(cubePos){
        case 1:
          mPoslist->last()->cubeItem->setBackground(QBrush(Qt::green));
          break;
        case 2:
          mPoslist->last()->cubeItem->setBackground(QBrush(Qt::red));
          break;
        case 3:
          mPoslist->last()->cubeItem->setBackground(QBrush(Qt::blue));
          break;
        case 4:
          mPoslist->last()->cubeItem->setBackground(QBrush(Qt::black));
          break;
        default:
          mPoslist->last()->cubeItem->setBackground(QBrush(Qt::white));
          break;
      }
    }
    mPoslistwidget->setCellWidget(mPoslist->length()-1,14,cubeWdg);
    mPoslistwidget->setItem(mPoslist->length()-1,15,mPoslist->last()->intensityItem);
    mPoslistwidget->setCellWidget(mPoslist->length()-1,16,intensityWdg);
    mPoslistwidget->setItem(mPoslist->length()-1,17,mPoslist->last()->shutterItem);
    mPoslistwidget->setCellWidget(mPoslist->length()-1,18,shutterWdg);
    mPoslistwidget->setCellWidget(mPoslist->length()-1,19,mPoslist->last()->ButtonItem);
    mPoslistwidget->setCellWidget(mPoslist->length()-1,20,mPoslist->last()->DeleteButtonitem);
    mPoslistwidget->resizeColumnsToContents();
    // Set size of the first column "Name" a bit bigger.
    mPoslistwidget->setColumnWidth(0, 80);

    //mKeepinmemory =true;
    //shootautopicture();
    mCurrentlistposition=mPoslist->length();// keep track of position for filename

    // Save the current displayMode
    mPoslist->last()->name = displayModeBox->currentText();
    // QTextStream(stdout) << displayModeBox->currentText();
    //mStoringDisplayMode = name;
    //emit singleShotRequested(mode.cameraMode, mode.exposureTime, mode.gain);

    connect(mPoslist->last()->ButtonItem,	SIGNAL(clicked()),
            this,	SLOT(tablegotobuttonclicked()));
    connect(mPoslist->last()->DeleteButtonitem,	SIGNAL(clicked()),
            this,	SLOT(tabledeletebuttonclicked()));
  }

  void MainWindow::tablegotobuttonclicked()
  {
    //identify list postion of sender
    QObject* senderp = sender();
    //QObject* parent;
    //QString name;
    //QPushButton* buttonpointer;
    //buttonpointer=mPoslist->at(0)->Buttonitem;
    //name=senderp->objectName();
    //parent=senderp->parent();
    //name=parent->objectName();

    int k = 0;
    // look for sender in the list of saved states
    for(int j = 0; j < mPoslist->length(); j++) {
      if (senderp == mPoslist->at(j)->ButtonItem) {
        k = j;
        break;
      }
    }

    ExperimentState state;
    state = getstatefromlistpos(k);
    if (setstate(state)) {
      //if(mPoslist->at(k)->cubecheckbox->isChecked())
      //	mAhmmicroscope->setturretposition(mPoslist->at(k)->cube);
      //if(mPoslist->at(k)->intensitycheckbox->isChecked())
      //	mAhmmicroscope->setintensity(mPoslist->at(k)->intensity);
    }

    /*
    QString xtext;
    QString ytext;
    QString ztext;
    QString pressuretext;
    // read text
    xtext=mPoslist->at(k)->xpositem->text();
    ytext=mPoslist->at(k)->ypositem->text();
    ztext=mPoslist->at(k)->zpositem->text();
    pressuretext=mPoslist->at(k)->pressureitem->text();

    double x, currentx;
    double y, currenty;
    double z, currentz;
    double pressure, currentpressure;
    double debugpressure;
    //convert text to numbers
    x = xtext.toDouble();
    y=ytext.toDouble();
    z=ztext.toDouble();
    pressure=pressuretext.toDouble();

    //movetostate(state);
    currentx = mStage->getXpos();
    currenty = mStage->getYpos();
    currentz = mStage->getZpos();
    currentpressure=10*(10-mDac->readvoltage());
    QPointF position(x,y), currentposition(currentx,currenty);

    if (abs(x-currentx)<5000 && abs(y-currenty)<5000 && abs(z-currentz)< 400) // limit movement for safety
    {
        mController->moveStageXY(position-currentposition);

        mController->moveStageZ(z-currentz);
        //displayZpos(mStage->getZpos());

    	debugpressure=((pressure)+100)/20;

    	mDac->setvoltage(debugpressure);
    	if(mPoslist->at(k)->cubecheckbox->isChecked())
    		mAhmmicroscope->setturretposition(mPoslist->at(k)->cube);
    	if(mPoslist->at(k)->intensitycheckbox->isChecked())
    		mAhmmicroscope->setintensity(mPoslist->at(k)->intensity);


    }
    else{
    TRACKER_WARNING("Safety limit exceeded, go closer manually first");
    }

    //QPointF position(mposlist->at(j)->xpositem->text(),10);
    //actualposition=mStage->getstate();
    //mController->moveStageXY(position-actualposition);
    */
  }

  MainWindow::ExperimentState MainWindow::getstatefromlistpos(int row)
  {
    ExperimentState state;
    /*
    double x, y, z, pressure, gain, exposuretime;
    QString xtext;
    QString ytext;
    QString ztext;
    QString pressuretext;
    QString gaintext;
    QString exposuretimetext;
    // read text
    xtext=mPoslist->at(row)->xpositem->text();
    ytext=mPoslist->at(row)->ypositem->text();
    ztext=mPoslist->at(row)->zPosItem->text();
    pressuretext=mPoslist->at(row)->pressureItem->text();
    gaintext = mPoslist->at(row)->gainItem->text();
    exposuretimetext = mPoslist->at(row)->exposuretimeItem->text();
    mCurrentlistposition=row+1; // keep track of position for filename
    //convert text to numbers
    x = xtext.toDouble();
    y = ytext.toDouble();
    z = ztext.toDouble();
    pressure = pressuretext.toDouble();
    gain = gaintext.toDouble();
    exposuretime = exposuretimetext.toDouble();
    */
    state.name = mPoslist->at(row)->name;
    state.xpos=mPoslist->at(row)->xpos;
    state.ypos=mPoslist->at(row)->ypos;
    state.zpos=mPoslist->at(row)->zpos;
    state.pressure = mPoslist->at(row)->pressure;
    state.gain = mPoslist->at(row)->gain;
    state.exposuretime = mPoslist->at(row)->exposuretime;
    state.illumination = mPoslist->at(row)->illumination;
    state.intensity = mPoslist->at(row)->intensity;
    state.shutter = mPoslist->at(row)->shutter;
    state.shutterChecked = mPoslist->at(row)->shutterCheckBox->isChecked();
    state.cube = mPoslist->at(row)->cube;
    state.xyPosChecked = mPoslist->at(row)->xyPosCheckBox->isChecked();
    state.zPosChecked = mPoslist->at(row)->zposCheckBox->isChecked();
    state.pressureChecked = mPoslist->at(row)->pressureCheckBox->isChecked();
    state.gainChecked = mPoslist->at(row)->gainCheckBox->isChecked();
    state.exposuretimeChecked = mPoslist->at(row)->exposuretimeCheckBox->isChecked();
    state.illuminationChecked = mPoslist->at(row)->illuminationCheckBox->isChecked();
    state.intensityChecked = mPoslist->at(row)->intensityCheckBox->isChecked();
    state.filterChecked = mPoslist->at(row)->cubeCheckBox->isChecked();

    return state;

  }
  bool MainWindow::setstate(ExperimentState state)
  {
    //load current state
    double currentx = mStage->getXpos();
    double currenty = mStage->getYpos();
    double currentz = mStage->getZpos();

    QPointF position(state.xpos,state.ypos), currentposition(currentx,currenty);

    // set saved exposuretime
    QString name = state.name;
    assert(displayModeBox->findText(name) != -1);
    // Find the corresponding display mode TODO - it is not clear completely. there is a function in displayModebox->setCurrentText()
    DisplayMode& mode = mDisplayModes[displayModeBox->findText(name)];
    displayModeBox->setCurrentIndex(displayModeBox->findText(name));
    mode.exposureTime = state.exposuretime;
    mode.gain = state.gain;

    //mode.zoom = ;


   // if(displayModeBox->currentText()==name)
    //  mExposureTimeBox->setValue(state.exposuretime);


    //move stage to desired state
    if(state.xyPosChecked){
        if ((abs(state.xpos-currentx)<5000 && abs(state.ypos-currenty)<5000) || !mSafety) { // limit movement for safety (if safety is enabled)
             mController->moveStageXY(position - currentposition);
        }
        else{
           TRACKER_WARNING("Safety limit in XY exceeded, go closer manually first");
           return false;
        }
    }

    if(state.zPosChecked){ // limit movement for safety (if safety is enabled)
        if (abs(state.zpos-currentz)< 400 || !mSafety) {
                mController->moveStageZ(state.zpos-currentz, 0);
        }
        else{
           TRACKER_WARNING("Safety limit in Z exceeded, go closer manually first");
           return false;
        }
    }
    /*
    if ((abs(state.xpos-currentx)<5000 && abs(state.ypos-currenty)<5000 && abs(state.zpos-currentz)< 400) || !mSafety) { // limit movement for safety (if safety is enabled)
      // Check if x and y position should change
      if(state.xyPosChecked){
        mController->moveStageXY(position - currentposition);
      }

      // Check if z position should change
      if(state.zPosChecked){
        mController->moveStageZ(state.zpos-currentz, 0);
      }
    } else {
      TRACKER_WARNING("Safety limit exceeded, go closer manually first");
      return false;
    }*/

    if(state.pressureChecked){
        mDac->setvoltage((state.pressure+100)/20);
    }
    // Check if illumination mode should change.
    if(state.illuminationChecked){
      // Check if saved illumination mode differs from the current illumination mode.
      if(state.illumination != mAhmmicroscope->getContrastingMethod()){
        mAhmmicroscope->changemethod();
        switch(state.illumination){
          case ahmMicroscope::ilFluorescence:
            illuminationLabel->setText("IL");
            break;
          case ahmMicroscope::ilWhiteLight:
            illuminationLabel->setText("TL");
            break;
        }
      }
    }
    // Check if shutter should change.
    if(state.shutterChecked){
      mAhmmicroscope->setShutter(state.shutter);
      switch(state.shutter){
        case 0:
          shutterLabel->setText("close");
          break;
        case 1:
          shutterLabel->setText("open");
          break;
      }
    }
    // Check if filter should change.
    if(state.filterChecked){
      mAhmmicroscope->setturretposition(state.cube);
    }
    // Check if intensity should change.
    if(state.intensityChecked){
      mAhmmicroscope->setintensity(state.intensity);
    }
    // Check if pressure should change.
    if(state.pressureChecked){
      setPressure(state.pressure);
    }
    // Check if gain should change.
    if(state.gainChecked){
      mCamera->setGain(state.gain);
      gainBox->setValue(state.gain);
    }
    // Check if exposure time should change.
    if(state.exposuretimeChecked){
      mCamera->setExposureTime(state.exposuretime);
      mExposureTimeBox->setValue(state.exposuretime);
      // Here we have some isseues, please check it out.
    }

    return true;
  }

  bool MainWindow::setrelativestate(ExperimentState relativestate)
  {

    QPointF position(relativestate.xpos,relativestate.ypos);
    double currentpressure;
    currentpressure=20*(mDac->readvoltage()-5);

    if ((abs(relativestate.xpos)<5000 && abs(relativestate.ypos)<5000 && abs(relativestate.zpos)< 400) || !mSafety) { // limit movement for safety (if safety is enabled)
      mController->moveStageXY(position);
      mController->moveStageZ(relativestate.zpos, 0);

      mDac->setvoltage((currentpressure+relativestate.pressure+100)/20);
      return true;
    } else {
      TRACKER_WARNING("Safety limit exceeded, go closer manually first");
      return false;
    }
  }

  void MainWindow::tabledeletebuttonclicked()
  {
    QObject* senderp = sender();

    //look for sender in the list of saved states
    for (int j = 0; j < mPoslist->length(); ++j) {
      if (senderp == mPoslist->at(j)->DeleteButtonitem) {
        delete mPoslist->at(j);
        mPoslist->removeAt(j);
        mPoslistwidget->removeRow(j);
        break;
      }
    }
  }

  /**************************************************************************/
  /***************************** Settings ***********************************/
  /**************************************************************************/

  void MainWindow::readSettings()
  {
    QSettings settings(PathConfig::getConfigFilename(), QSettings::IniFormat);

    // Restore window positions, etc.
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    stageStepBox->setValue(settings.value("Stage_Step", 10).toInt());
    mMaxSliderExposureTime = qMax(1, settings.value("Max_Slider_Exposure_Time", 1000000).toInt());
    mImageLabel->setDisplayFrequency(qMax(0.0, settings.value("Image_Display_Frequency", 24.0).toDouble()));
    storageFilename = settings.value("Base_Filename", "").toString();
    storageFilenameEdit->setText(storageFilename);
    if(mStoragePath != NULL) {
      mStoragePath->settingsStorageFilenameEdit->setText(storageFilename);
    }
    //  storageRunSpinBox->setValue(settings.value("Experiment_Run", 0).toInt());

    // Restore order (QSettings doesn't)
    QString displayModeStrings = settings.value("Display_Mode_Order").toString();

    // Read all display modes
    settings.beginGroup("Display_Modes");
    this->readDisplayMode(settings, "Tracking");
    foreach(QString displayModeString, displayModeStrings.split(",", QString::SkipEmptyParts)) {
        this->readDisplayMode(settings, displayModeString);
    }
    settings.endGroup();
  }

  void MainWindow::readDisplayMode(QSettings& settings, QString displayModeString)
  {
    DisplayMode displayMode;
    settings.beginGroup(displayModeString);

    displayMode.name = displayModeString;
    QString cameraMode = this->readCameraMode(settings.value("Camera_Mode").toString());
    if (cameraMode.isEmpty())
      displayMode.cameraMode = cameraModeBox->itemText(0);
    else
      displayMode.cameraMode = cameraMode;
    displayMode.exposureTime = qMax(0,   settings.value("Exposure_Time", 10000).toInt());
    displayMode.gain         = qMax(0.0, settings.value("Camera_Gain",     1.0).toDouble());
    displayMode.zoom         = qMax(0.0, settings.value("Image_Zoom",      1.0).toDouble());
    displayMode.microscopeIntensityState = qMax(0,   settings.value("Intensity", 1).toInt());

    this->addDisplayMode(displayModeString);
    mDisplayModes.last() = displayMode;
    settings.endGroup();
  }

  QString MainWindow::readCameraMode(QString text)
  {
    // Unscramble \ and /
    text = text.replace("__fwd_sl__", "\\").replace("__bwd_sl__", "/");
    if (text.contains("__sep__")) {
      if (text.section("__sep__", 0, 0) == mCamera->getName()) {
        QString name = text.section("__sep__", 1, 1);
        if (mCamera->hasMode(name))
          return name;
      }
    }
    return "";
  }

  void MainWindow::writeSettings()
  {
    QSettings settings(PathConfig::getConfigFilename(), QSettings::IniFormat);

    // Save window positions, etc.
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());

    settings.setValue("Stage_Step", stageStepBox->value());
    settings.setValue("Max_Slider_Exposure_Time", mMaxSliderExposureTime);
    settings.setValue("Image_Display_Frequency", mImageLabel->getDisplayFrequency());
    //settings.setValue("Base_Filename", storageFilenameEdit->text());
    settings.setValue("Base_Filename", storageFilename);
    settings.setValue("Experiment_Run", storageRunSpinBox->value());

    // Save values to array first
    displayModeBox_currentIndexChanged(displayModeBox->currentIndex());

    // Save display mode order
    QStringList order;
    for (int i = 1; i < mDisplayModes.size(); ++i)
      order.append(mDisplayModes[i].name);
    settings.setValue("Display_Mode_Order", order.join(","));

    settings.beginGroup("Display_Modes");
    settings.remove(""); // Remove all subgroups
    foreach (const DisplayMode& displayMode, mDisplayModes) {
      settings.beginGroup(displayMode.name);
      QString cameraMode = mCamera->getName() + "__sep__" + displayMode.cameraMode;
      // A single key may not contain / or \ because they separate keys
      cameraMode = cameraMode.replace('\\', "__fwd_sl__").replace('/', "__bwd_sl__");
      settings.setValue("Camera_Mode", cameraMode);
      settings.setValue("Exposure_Time", displayMode.exposureTime);
      settings.setValue("Camera_Gain",   displayMode.gain);
      settings.setValue("Image_Zoom",    displayMode.zoom);
      settings.setValue("Intensity",    displayMode.microscopeIntensityState);
      settings.endGroup();
    }
    settings.endGroup();
  }

  void MainWindow::timerEvent(QTimerEvent*)
  {
      if(rising) {
          // augment pressure
          mDac->setvoltage((100-((tickcounter%steps)*stepsize))/20);
          //Pressure change triggered directionality change during tracking
          mController->setPressureisIncreased();
      } else {
          mDac->setvoltage((100-(steps*stepsize-(tickcounter%steps)*stepsize))/20); // reduce pressure
          //Pressure change triggered directionality change during tracking
          mController->setPressureisDecreased();
      }
      // Estimated stage Z adjustion based on magnitude of the pressure change
      emit pressureStepped(-stepsize*2*(0.5-((tickcounter-1)/steps)%2));  // apply the change to the stage      

/*
    if (pictureCheckbox->isChecked()) {
      // Setting to pressure is changed and make pictures after it is in focus
      mController->setPressureisChanged( true );
    }
    */

      if ((tickcounter%steps)==0) {
          if(stopatmaxCheckbox->isChecked()) {
              killTimer(dactimerID);
              tickcounter=1;
              rising = true;
              // shoot a (hopefully) focused picture after last step (waited 2*timeinterval)!
              // instead of the original timeinterval between pressure changes
              // cameratimer->singleShot(2*timeinterval,this,SLOT(shootautopicture()));
              // TODO: move this for now into the controller trackImage

              // experimental: only correct z with autofocus after last pressure change
              // the xy tracker works sufficiently well with forward autofocus, and thus
              // the z tracking does not interfere with smooth xy tracking
              // mController->setZStackEnabled( true );
              mController->setLastPressureChangePassed( true , multiIm->isChecked());
              mController->setZStackMaximumNumber(MaxNumZstack->value());
              mController->setTimeLapse(TimeLapse->isChecked());
          }
      }

      if ((tickcounter%steps)==steps-1) {
          rising = !rising; // toggle state
      }
      tickcounter++;
  }

/**  void MainWindow::shootautopicture()
  {
      std::cout<<"TEst01"<<std::endl;
    if (!mStoringImage) {
      QString name = "auto"; //"auto"
      mStoringImage = true;
      assert(displayModeBox->findText(name) != -1);
      DisplayMode& mode = mDisplayModes[displayModeBox->findText(name)];
      mStoringDisplayMode = name;
      //TODO: Seting the microscope illumination intensity - took out
      std::cout<<"Here is the intensity value before setting it: "<<mAhmmicroscope->getintensity()<<std::endl;
      //mAhmmicroscope->getintensity()
      //mAhmmicroscope->setintensity(mode.microscopeIntensityState);
      std::cout<<"Here is the intensity value: "<<mode.microscopeIntensityState<<std::endl;     
      NumOfImages = 1;
      if (multiIm->isChecked()){
        mCamera->setNumOfPlanes(SizeOfZstack->value());
        mCamera->setZstepsize(Zstackstepsize->value());
      //  mController->setZStackMaximumNumber(MaxNumZstack->value());
        emit multipleShotRequested(mode.cameraMode, mode.exposureTime, mode.gain);
      }
      else{
        emit singleShotRequested(mode.cameraMode, mode.exposureTime, mode.gain);
      }
    }
  }
  */

  void MainWindow::shoothighreshighintensitypicture() //using this function for the multpiple images.
  {
    if (!mStoringImage) {
      //QString name = "hrhi";
      QString name = "auto";
      // TODO: fix this so it should not be realised until the last image has been processed, otherwise with "F1" image could be initiated.
      mStoringImage = true; // problem of the system,
      assert(displayModeBox->findText(name) != -1);
      DisplayMode& mode = mDisplayModes[displayModeBox->findText(name)];
      mStoringDisplayMode = name;
      ///Temporal solution for passing thtrough the Zstack information
     // double stepSize = 0.1; // the distance between the images in the Z stack
      // change the illumination mode of the microscope
      // mAhmmicroscope->setintensity(mode.microscopeIntensityState);
      // problem here: there is no way to defie a call back to set the
      // illumination state back to 1 as soon as the shot is taken.
      //TODO,.,
      //emit multipleShotRequested(mode.cameraMode, mode.exposureTime, mode.gain);
    }
  }

  void MainWindow::shootautopicture()
  {
      //std::cout<<"TEst01"<<std::endl;
      if (!mStoringImage) {
          QString name = "auto"; //"auto"
          mStoringImage = true;
          assert(displayModeBox->findText(name) != -1);
          DisplayMode& mode = mDisplayModes[displayModeBox->findText(name)];
          mStoringDisplayMode = name;

          NumOfImages = 1;
          if (multichannel->isChecked())
          {
              mCamera->setMultichannel(true);
              double ChannelOffsetvalue = ChannelOffset->value();
              mCamera->setOffsetValue(ChannelOffsetvalue);
          }
          else
             mCamera->setMultichannel(false);

          if (multiIm->isChecked()){
            int NumOfPlanes = SizeOfZstack->value();
            double ZstackStepSize = Zstackstepsize->value();
            ZPosAtImaging =  mStage->getZpos();
            for (unsigned int i = 0; i < NumOfPlanes / 2; ++i){
             //   mStage->moveZ(-ZstackStepSize, 1);
                mController->moveStageZ(-ZstackStepSize, 1);
            }
            mCamera->setNumOfPlanes(NumOfPlanes);
            mCamera->setZstepsize(ZstackStepSize);
          //  mController->setZStackMaximumNumber(MaxNumZstack->value());
          }
         else
          mCamera->setNumOfPlanes(1);

         emit RecordMW(mode.cameraMode,channel,intensity, 1,I_Channel_1->value(),mode.exposureTime, mode.gain);


    }
  }

  void MainWindow:: SetStageBack()
  {
    double Zpos =  mStage->getZpos();
    double Diff = ZPosAtImaging-Zpos;
    if (std::abs(Diff)>0.1){
          //mStage->moveZ(Diff,1);//
          mController->moveStageZ(Diff,1);}
     std::cout<<"ZposAtImaging: "<< ZPosAtImaging<< std::endl;
     std::cout<<"Zpos: "<<Zpos<<std::endl;
     std::cout<<"Diff: "<<Diff<<std::endl;
  }

  void MainWindow:: SetChannelandIntensityValuesforSavedimages(int ch, double i)
  {
      channel = ch;
      intensity = i;
  }

  void MainWindow::TimeLapseCounterUpdated(QString counter){
      TimeLapseCounterLabel->setText(counter);
  }

 void MainWindow::TriggerFromCameraForFlash(){
        on_StartMP_clicked();
   }
/*    if (mAction){
        //Actions:
            // gain - done
            // exposuretime- done
            // mode change - done
            // Z,XY motion 3190-3025 function are involved

        //on_StartMP_clicked();
        mAction = false;
    }
     elseif (mTracking_and_Imaging){ // 3008 set on when the controller starts
       // check imaging mode:
       // set intensity accordingly
       // on_StartMP_clicked();
     else

    }
  }
  void MainWindow::SetAction(){
      mAction = true;
  }*/

}




