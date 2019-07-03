/*
 Copyright (c) 2009-2012, Reto Grieder & Benjamin Beyeler

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef _MainWindow_H__
#define _MainWindow_H__

#include "TrackerPrereqs.h"

#include <memory>
#include <QLabel>
#include <QMainWindow>
#include <QMovie>
#include <QIntValidator>
#include <QSettings>
#include <QShortcut>
#include <QSignalMapper>
#include <QThread>
#include <QLCDNumber>
#include <QMetaType>
#include <QTableWidget>

#include "ui_MainWindow.h"
#include "ui_StoragePath.h"
#include "ui_ComPorts.h"
#include "ui_ScaleFactor.h"
#include "ui_Stretcher.h"
#include "ui_SetUpHome.h"
#include "Timing.h"
#include "Dac.h"
#include "Microscope.h"
#include "AutoStretch.h"
#include "LinearMotor.h"
#include "Controller.h"
#include "Lamp.h"

#include <iostream>
#include <vector>

class QMenu;

/// Main namespace for everything self-written.
namespace tracker
{

  class StoragePath;
  /** Single instance class that manages the whole program.
      MainWindow is responsible for \ref MainWindow() "instancing" and
      destroying all components, including everything associated with the
      user interface. \n
      This class also manages the different threads and does most of the
      Signal/Slot \ref signalslotspage "connections". \n
      The main event loop handling all GUI related events is run in the thread
      which this class belongs to.
  @par Settings
      The following settings are read and stored in the ini file:
       - Window geometry and state
       - \c Stage_Step: step size used in the Stage UI in micrometers
       - \c Max_Slider_Exposure_Time: Maximum user selectable
         exposure time (default: \c 1000000)
       - \c Image_Display_Frequency: Display update refresh rate
         in Hertz (default: 24)
       - \c Base_Filename: User specified infix for the image filenames
       - \c Experiment_Run: Currently used experiment run Nr.
       - List of all \ref DisplayMode "Display Modes"
  */
  class MainWindow : public QMainWindow, private Ui::MainWindow
  {
    Q_OBJECT;

  public:
    /** Sets up the entire application (except for PathConfig and the
        Logger, which are done beforehand in main()).
        The following steps are performed:
         -# Set internal variables to useful values
         -# Create all the UI elements (specified by both code and designer)
         -# Create the application components:
            -# Camera
            -# Controller
            -# Stage
            -# PressureSensor
            -# \ref Thread "Threads"
         -# Connect and configure UI
         -# readSettings() and apply them
         -# When testing offline (#TRACKER_DUMMY), create the DummyMicroscope
    */

    /** A Struct to store acquired images with meta data in order to flush the data
          only once in an offline fashion to speed up the sequence image acquiring with
          elimination of the individual file storage (~300-400ms)
       */
       struct ImageBuffer {
           ImageBuffer(QImage i,quint64 c,quint64 p,QString pa,QString f, QString S,QString d,QByteArray b,
                       double z, double x, double y, float pres, double exp, double g) :
               Image(i), captureTime(c), processTime(p),
               path(pa), format(f), Save(S), distinction(d), ba(b), Z(z),
               X(x), Y(y), pressure(pres), exposure(exp), gain(g){}

           QImage  Image;             //!< Acquired and processed image
           quint64 captureTime;       //!< Image capture time
           quint64 processTime;       //!< Image process time (?estimated?)
           QString path;              //!< Image storage path
           QString format;            //!< Image storage format
           QString Save;              //!< Total path with set file name and format
           QString distinction;       //!< Distinction file format
           QByteArray ba;             //!< Metadata storage place
           double   Z;                //!< Storing the Z position of the image
           double   X;                //!< Storing the x position of the image
           double   Y;                //!< Storing the x position of the image
           float    pressure;         //!< Storing the pressure position of the image
           double   exposure;         //!< Storing the exposure position of the image
           double   gain;             //!< Storing the gain position of the image
       };      

    MainWindow();

    /** Stops all threads currently running and stores the settings.
    @note
        Destruction of QObjects is done automatically.
    */
    ~MainWindow();

  signals:
    /** Raised when the user wants to store a single shot image.
    @param mode
        String identifier for the Camera mode
    @param exposureTime
        Requested exposure time (-1 for no change)
    @param gain
        Requested gain (-1.0 for no change)
    */
    /// Raised when single shot has been requested
    void RecordMW(QString mode,int ch1,double i1,int ch2, double i2, int exposureTime, double gain);
    /// Raised when single shot has been requested
   // void singleShotRequested(QString mode, int exposureTime, double gain);
    /// Raised when multiple shots have been requested
   // void multipleShotRequested(QString mode, int exposureTime, double gain);
    //// Raised when the stage should be moved vertically.
    void ZMoveRequested(double distance, int block);
    /// Raised when the pressure should be set to another value-
    void pressureSetRequested(float);
    /// Raised when the pressure was set to another value.
    void pressureStepped(float);

    /** Raised when the user changes the camera mode.
    @copydetails singleShotRequested()
    */
    void cameraModeChanged(QString mode, int exposureTime, double gain);

    /** Raised when the stage should be moved.
        This is just a way to easily implement an asynchronous call to
        resolve threading issues.
    @param distance
        Moving distance in micrometers
    */
    void stageMoveRequested(QPointF distance);
    void entryfinished();

    /**
     * @brief Signal to inform AutoStretch about the current mode.
     * @param index Current mode
     */
    void updateAutoStretchMode(int index);

    /**
     * @brief Signal to inform PressureSensor about the com port on which the pressure sensor is attached.
     * @param mPort Com port on which the force sensor is attached
     */
    void setPressureSensorComPort(std::string mPort);

    /**
     * @brief Signal to inform PressureSensor to close the com port
     */
    void closePressureSensorComPort();

    /**
     * @brief Signal to inform PressureSensor to open the specified com port
     * @param mPort Com port on which the force sensor is attached
     */
    void openPressureSensorComPort(std::string mPort);

    /**
     * @brief Signal to inform LinearMotor about the com port on which the linear motors are attached.
     * @param mPort Com port on which the linear motors are attached
     */
    void setLinearMotorComPort(std::string mPort);

    /**
     * @brief Signal to inform LinearMotor to close the com port
     */
    void closeLinearMotorComPort();

    /**
     * @brief Signal to inform LinearMotor to open the specified com port
     * @param mPort Com port on which the linear motors are attached
     */
    void openLinearMotorComPort(std::string mPort);

    /**
     * @brief Inform AutoStretch about the distance in the max. motor position.
     * @param distance Distance in microsteps
     */
    void setZeroDistance(double distance);

    /**
     * @brief Inform LinearMotor about the desired clamping distance.
     * @param mm Desired clamping distance in mm
     */
    void setClampingDistance(double mm);

    /**
     * @brief Inform AutoStretch to go to the desired clamping distance.
     * @param distance Desired clamping distance
     */
    void gotoClampingDistance(double distance);

    /**
     * @brief Inform AutoStretch to perform the Preconditioning.
     * @param speed Speed in % off preload length per second
     * @param preload Preload force
     * @param cycles Amount of cycles performed during Preconditioning
     * @param expansionPerCent Amount of expansion
     */
    void startPreConditioning(double speed, double preload, int cycles, double expansionPerCent);

    /**
     * @brief Inform PressureSensor about the scale factor parameters from the GUI.
     * @param iNominalValue Nominal value
     * @param iMeasureEndValue Measure end value
     * @param iNominalForce Nominal force
     * @param iInputSensitivity Input sensitivity
     */
    void setScaleFactor(double iNominalValue, double iMeasureEndValue, double iNominalForce, double iInputSensitivity);

  private slots:
    /** Controller thread (in any \ref Controller::Mode "mode") finished
        (disconnects the signals, updates the UI and stops the animation)
    */
    void controllerFinished();

    /**
     * @brief Creates and brings up the "Storage path" menu and make the required connections.
     */
    void storagePathWindow();

    /**
     * @brief Creates an brings up the "Com port" menu and make the required connections.
     */
    void comPortWindow();

    /**
     * @brief Creates and brings up the "scale factor" menu and makes the required connections.
     */
    void scaleFactorWindow();

    /**
     * @brief Creates and brings up the "Stretcher" menu and makes the required connections.
     */
    void stretcherWindow();

    /**
     * @brief Creates and brings up the "setUpHome" menu and makes the required connections.
     */
    void setUpHomeWindow();

    void timerEvent(QTimerEvent* event);

    /// Updates the activity status of all widgets that may not always be enabled
    void updateWidgetActivities();


    /*** Central Widget ***/

    /// Updates image display and correspondsing numeric values in the UI
    ///
    /// @note Only relays to DraggableLabel::displayImage() (fewer arguments)
    void displayImage(const QImage image, quint64 captureTime, quint64 processTime);

    /// Changes the zoom value text in the status bar
    void on_mImageLabel_zoomChanged(double value);

    /**
     * @brief Sets the Logger dockwidget visible/invisible
     */
    void acdeacLogger();
    /**
     * @brief Sets the Camera dockwidget visible/invisible
     */
    void acdeacCamera();
    /**
     * @brief Sets the Tracker dockwidget visible/invisible
     */
    void acdeacTracker();
    /**
     * @brief Sets the TrackerOptions dockwidget visible/invisible
     */
    void acdeacTrackerOptions();
    /**
     * @brief Sets the Stage dockwidget visible/invisible
     */
    void acdeacStage();
    /**
     * @brief Sets the Pressure dockwidget visible/invisible
     */
    void acdeacPressure();
    /**
     * @brief Sets the PositionList dockwidget visible/invisible
     */
    void acdeacPositionList();
    /**
     * @brief Sets the FocusInformation dockwidget visible/invisible
     */
    void acdeacFocusInformation();
    /**
     * @brief Sets the LinearMotor dockwidget visible/invisible
     */
    void acdeacLinMot();
    /**
     * @brief Sets the Protocols dockwidget visible/invisible
     */
    void acdeacProtocols();
    /**
     * @brief Sets the Graph dockwidget visible/invisible
     */
    void acdeacGraph();

    //void SetAction();


    /*** Camera UI ***/

    /** Starts or stops the camera thread and
        with it the image transmission and displaying.
    @param checked
        Camera starts on \c true and stops on \c false
    */
    void on_cameraStartToolButton_clicked(bool checked);

    /// Camera thread finished (disconnect signals and update UI)
    void cameraFinished();

    /** User selected another camera mode.
        This automatically adjusts the Camera exposure time linearly with
        the resolution (area-based). \n
        Also, all Controller related UI widgets get updated with the
        associated values (dictated by a Controller::OptionSet). That update
        process includes choosing some good (and fast) values for the FFT
        image size and putting them into the combo boxes.
    */
    void cameraModeBox_currentIndexChanged(int index);

    /** User selected another display mode.
        The state of the old DisplayMode gets saved first because the
        current values are actually 'stored' in the user interface widgets.
        \n Also note that there is a little workaround because normally the
        exposure time would be adjusted to the new image area when changing
        the camera mode. But we don't want that. \n
        And lastly, the image pane gets cleared.
    */
    void displayModeBox_currentIndexChanged(int index);

    /** Removes a DisplayMode and it's associated UI button.
    @note
        You cannot remove the \c Tracking mode.
    */

    void on_removeDisplayModeButton_clicked();
    /** Adds a new display mode by displaying an edit box and then calling
        addDisplayMode().
    @note
        The name may not contain ',' and it must not yet exist.
    */

    void on_newDisplayModeButton_clicked();
    /// Camera frame rate was updated (only display if Camera is running)
    void displayCameraFrameRate(double frameRate);
    /// Value of the exposure time spin box changed (also updates the slider)
    void on_mExposureTimeBox_valueChanged(int value);
    /// Value in the exposure time slider changed (also updates the spin box)
    void on_exposureTimeSlider_valueChanged(int value);
    /// Value in the camera gain spin box changed (also updates the slider)
    void on_gainBox_valueChanged(double value);
    /// Value of the camera gain slider changed (also updates the spin box)
    void on_gainSlider_valueChanged(int value);

    // Update TimeLapse
    /// Counter for TimeLapseimages
    void TimeLapseCounterUpdated(QString);

    // Storage

    /// Callback for the different buttons for storing a single shot image
    void on_mSingleShotButtonMapper_mapped(QString name);

    /** To decouple the storing and saving, this new function is implemented in order to
     make the saving - writing the data on disk - offline. Does not interrapt the imaging
    */
    void saveBuffertoHarddisk();

    //Tracking of storage time from the request for the image
    /// Tracking the time for full image resolution image acqusition
    void TrackFullImageAcq(quint64 value);

    /// Tracking the time for full image resolution image acqusition
    quint64 getTrackFullImageAcq();

    /// User wants to specify the storage location of the images
    //void on_storageLocationButton_pressed();

    /** Single shot image was taken, actually store it on disk.
        The image will be stored in the location given in the
        \c storageFilenameEdit. If that location is given as a relative
        path, the image will be stored relative to the data path. \n
        The filename depends on data, time and pressure generated by
        getFilenameSuffix() as well as the 'run Nr.' and, if necessary, a
        unique distinction in the form of a number. \n
        The image format is PNG.
    */
    void storeSingleShot(QImage image, quint64 captureTime, quint64 processTime);    
    /** This shoots an automatic picture with the "auto" profile.
    This profile has to be created in the GUI if it is not present.
    */
    void shootautopicture();
    void shoothighreshighintensitypicture();

    // Tracker & Options

    /** Starts or stops the controller thread in
        \ref Controller::Tracking "tracking" mode
    @param checked
        Tracking starts on \c true and stops on \c false
    */
    void on_trackerStartToolButton_clicked(bool checked);

    /** Starts or stops the controller thread in
        \ref Controller::Timing "timing" mode
    @param checked
        Tracking starts on \c true and stops on \c false
    */
    void on_timingStartToolButton_clicked(bool checked);

    /** Starts or stops the controller thread in
        \ref Controller::Measuring mode
    @param checked
        Tracking starts on \c true and stops on \c false
    */
    void on_measuringStartToolButton_clicked(bool checked);

    /** Starts or stops the controller thread in
        \ref Controller::ZStack mode
    @param checked
        Z stack acquisition starts on \c true and stops on \c false
    */
    void on_zstackStartToolButton_clicked(bool checked);

    void on_fftImageSizeXBox_editTextChanged(QString text);
    void on_fftImageSizeYBox_editTextChanged(QString text);
    /// Controller frame rate was updated - display it (if Controller is running)
    void displayControllerFrameRate(double frameRate);

    // zTracker/ z movement
    void on_ButtonMoveUp_pressed();
    void on_ButtonMoveDown_pressed();
    void on_ButtonMoveUp_2_pressed();
    void on_ButtonMoveDown_2_pressed();
    void on_ButtonMoveUp_3_pressed();
    void on_ButtonMoveDown_3_pressed();
    void on_haltallButton_pressed();

    void displayXYpos(double xpos, double ypos);
    void displayZpos(double zpos);
    void displayFocus(const FocusValue&);

    // pressure control
    void setPressure(float);
    void on_setpressureButton_pressed();
    void on_setpressure2Button_pressed();
    void on_setpressure3Button_pressed();
    void on_startoscbutton_pressed();
    void on_stoposcButton_pressed();
    void exportPressure();

    void adjustStageZ(float step);
    void displayPressure(double value);

    // microscope control

    /**
     * @brief Select first filter when choosen in the GUI.
     */
    void on_firstCubeButton_clicked();

    /**
     * @brief Select second filter when choosen in the GUI.
     */
    void on_secondCubeButton_clicked();

    /**
     * @brief Select third filter when choosen in the GUI.
     */
    void on_thirdCubeButton_clicked();

    /**
     * @brief Select fourth filter when choosen in the GUI.
     */
    void on_fourthCubeButton_clicked();

    /**
     * @brief Select fifth filter when choosen in the GUI.
     */
    void on_fifthCubeButton_clicked();

    /**
     * @brief Change to the previous filter in order.
     */
    void on_decreaseCubeButton_clicked();

    /**
     * @brief Change to the next filter in order.
     */
    void on_increaseCubeButton_clicked();

    void on_toggleshutterButton_pressed();
    void on_increaseintensityButton_pressed();
    void on_decreaseintensityButton_pressed();

    // Stage UI
    void on_stageRightToolButton_pressed();
    void on_stageUpRightToolButton_pressed();
    void on_stageUpToolButton_pressed();
    void on_stageUpLeftToolButton_pressed();
    void on_stageLeftToolButton_pressed();
    void on_stageDownLeftToolButton_pressed();
    void on_stageDownToolButton_pressed();
    void on_stageDownRightToolButton_pressed();
    void on_clearsafetyButton_pressed();
    void tablegotobuttonclicked();
    void tabledeletebuttonclicked();

    // List UI
    void on_savestateButton_pressed();
    //void on_prepareButton_pressed();

    // Linear Motor UI
    void on_sendcommandButton_pressed();
    void on_homeButton_pressed();
    void on_workButton_pressed();
    void on_speedButton_pressed();
    void on_resetButton_pressed();
    void forceUpdated(double force);
    void setStretchUntilValueSuffix(const QString & suffix);
    void on_incsmallButton_pressed();
    void on_incbigButton_pressed();
    void on_decsmallButton_pressed();
    void on_decbigButton_pressed();
    void on_startoscLMButton_pressed();
    void on_stoposcLMButton_pressed();
    void on_startStretchButton_clicked();
    void on_chooseMode_currentIndexChanged(int index);
    void updateMeasuredPoints(double mForce, double mLength);
    void exportPng();

    // Protocol UI
    void on_addstackButton_pressed();
    void on_addpauseButton_pressed();
    void on_addposButton_pressed();
    void on_addphotoButton_pressed();
    void on_protupButton_pressed();
    void on_protdownButton_pressed();
    void on_protdelButton_pressed();
    void on_execButton_pressed();

    void executeprotocolentry(int entryindex);
    void executenextprotocolentry();
    void on_stopexecButton_pressed();
    void on_ZoomIncButton_pressed();
    void on_ZoomDecButton_pressed();

    /// User changed the stage step in the spin box (also updates the dial)
    void on_stageStepBox_valueChanged(int value);
    /// User changed the stage step with the dial (also updates the spin box)
    void on_stageStepDial_valueChanged(int value);
    /// User dragged the content of the image --> move the stage
    void on_mImageLabel_mouseDragged(QPoint value);
    void wheelzmove(double);

    void on_pictureHeight_valueChanged(double height);

    void on_pictureWidth_valueChanged(double arg1);

    void on_clearGraphButton_clicked();

    void on_graphStartStophButton_clicked();

    void on_storageRunSpinBox_valueChanged(int value);

    /**
     * @brief Saves the changed filename/path as new filename/path.
     * @param arg1 New filename/path
     */
    void on_storageFilenameEdit_textChanged(const QString &arg1);

    void on_settingsStorageFilenameEdit_textChanged(const QString &arg1);

    //void on_settingsStorageLocationButton_pressed();

    void on_storageLocationButton_clicked();

    /**
     * @brief Opens a FileDialog where the user can choose the file name and the path,
     *        where the picture will be saved. Also updates the FilenameEdit field with the changed value.
     */
    void on_settingsStorageLocationButton_clicked();

    /**
     * @brief Saves the changed value as new value.
     * @param value New value
     */
    void on_settingsStorageRunSpinBox_valueChanged(int value);

    /**
     * @brief Close Com Port configuration window.
     */
    void on_comPortCancelButton_clicked();

    void on_comPortOKButton_clicked();

    /**
     * @brief Close Path configuration window.
     */
    void on_storagePathCancelButton_clicked();

    /**
     * @brief Stores the new values (filename and value) and updates the widgets in the MainWindow. When all this is done, the window will close.
     */
    void on_storagePathOKButton_clicked();

    void on_zeroDistanceButton_clicked();

    /**
     * @brief Opens a FileDialog where the user can choose the path and the filename, Extends the file suffix, if needed
     *        and writes the measured data in the file.
     */
    void on_exportButton_clicked();

    /**
     * @brief Close SetUpHome window when "Cancel" button is pressed.
     */
    void on_setUpHomeButtonBox_rejected();

    /**
     * @brief Send the motors to the home position and after close the SetUpHome window when "OK" button was pressed.
     */
    void on_setUpHomeButtonBox_accepted();

    void on_linMotGoToClampingPosButton_clicked();

    void on_preCondStartButton_clicked();

    void on_RampToFailureStartButton_clicked();

    void on_RampToFailureStopButton_clicked();

    void SetTimeLapse(bool state);

    void Turn_ON();
    void on_Channel_0_clicked();
    void on_Channel_1_clicked();
    void on_Channel_2_clicked();
    void on_Channel_3_clicked();
    void on_Channel_4_clicked();
    void SetLampFromCameraSignal(int,double);
    void SetChannelandIntensityValuesforSavedimages(int, double);
    void TriggerFromCameraForFlash();

    void SetStageBack();
    //multi channel / imaging slots for shortcuts
    void MultiImageToggle();
    void MultiChannelToggle();

    //void testgrouping();

    void on_StartMP_clicked();

    /**
     * @brief Inform PressureSensor about the new scale factor parameters and close window afterwards.
     */
    void on_buttonBox_accepted();

    /**
     * @brief Remove the tick in the view menu, when widget isn't visible
     * @param visible State of the visibility of the widget
     */
    void on_cameraDockWidget_visibilityChanged(bool visible);

    /**
     * @brief Remove the tick in the view menu, when widget isn't visible
     * @param visible State of the visibility of the widget
     */
    void on_trackerDockWidget_visibilityChanged(bool visible);

    /**
     * @brief Remove the tick in the view menu, when widget isn't visible
     * @param visible State of the visibility of the widget
     */
    void on_trackerOptionsDockWidget_visibilityChanged(bool visible);

    /**
     * @brief Remove the tick in the view menu, when widget isn't visible
     * @param visible State of the visibility of the widget
     */
    void on_ListDockWidget_visibilityChanged(bool visible);

    /**
     * @brief Remove the tick in the view menu, when widget isn't visible
     * @param visible State of the visibility of the widget
     */
    void on_linmotdockWidget_visibilityChanged(bool visible);

    /**
     * @brief Remove the tick in the view menu, when widget isn't visible
     * @param visible State of the visibility of the widget
     */
    void on_graphDockWidget_visibilityChanged(bool visible);

    /**
     * @brief Remove the tick in the view menu, when widget isn't visible
     * @param visible State of the visibility of the widget
     */
    void on_storageDockWidget_visibilityChanged(bool visible);

    /**
     * @brief Remove the tick in the view menu, when widget isn't visible
     * @param visible State of the visibility of the widget
     */
    void on_stageDockWidget_visibilityChanged(bool visible);

    /**
     * @brief Remove the tick in the view menu, when widget isn't visible
     * @param visible State of the visibility of the widget
     */
    void on_pressureDockWidget_visibilityChanged(bool visible);

    /**
     * @brief Remove the tick in the view menu, when widget isn't visible
     * @param visible State of the visibility of the widget
     */
    void on_focusdockWidget_visibilityChanged(bool visible);

    /**
     * @brief Remove the tick in the view menu, when widget isn't visible
     * @param visible State of the visibility of the widget
     */
    void on_protocoldockWidget_visibilityChanged(bool visible);

    /**
     * @brief Calculates the Stack size when the Step size changes.
     */
    void on_protstepsizeBox_editingFinished();

    /**
     * @brief Calculates the Step size when the Stack size changes.
     */
    void on_protStackSizeBox_editingFinished();

    /**
     * @brief Go back home for example when the sample is dismounted.
     */
    void on_stageGoHomeButton_clicked();

    /**
     * @brief Change illumination method to incident light
     */
    void on_incidentButton_clicked();

    /**
     * @brief Change illumination method to transmitted light
     */
    void on_transmittedButton_clicked();

  private:
    /** Describes one single operation mode for the camera.
        It stores the camera settings (mode, exposure time, gain) and the
        current image zoom value. That makes it easy to switch between
        different modes like the one used for tracking or another one used
        for storing the images. \n
        The mode named 'Tracking' is a built in mode that cannot be removed,
        nor does it have a button for storing images (like all the others).
    */
    HPClock              mClock;       //!< High precision clock used for checking auto image aquisition
    quint64              mTimerForOneCompleteImage; //!< For observation of how long one full resolution image is taken.

    struct DisplayMode {
      QString name;           ///< User given name used to identify the mode
      QString cameraMode;     ///< Camera \ref Camera::Mode "mode"
      int     exposureTime;   ///< Camera exposure time in microseconds (default: \c 10000)
      double  gain;           ///< Camera gain (default: \c 1.0)
      double  zoom;           ///< Center image zoom (default: \c 1.0)
      int     microscopeIntensityState; /// Added the microscope intensity state
    };
    struct ExperimentState {
      QString name;
      double xpos;
      double ypos;
      double zpos;
      double pressure;
      double gain;
      double exposuretime;
      int illumination;
      int intensity;
      int cube;
      int shutter;
      bool xyPosChecked;
      bool zPosChecked;
      bool pressureChecked;
      bool gainChecked;
      bool exposuretimeChecked;
      bool illuminationChecked;
      bool filterChecked;
      bool intensityChecked;
      bool shutterChecked;

      QTableWidgetItem* xpositem;
      QTableWidgetItem* ypositem;
      QTableWidgetItem* xyPosItem;
      QCheckBox* xyPosCheckBox;
      QTableWidgetItem* zPosItem;
      QCheckBox* zposCheckBox;
      QTableWidgetItem* pressureItem;
      QCheckBox* pressureCheckBox;
      QTableWidgetItem* gainItem;
      QCheckBox* gainCheckBox;
      QTableWidgetItem* exposuretimeItem;
      QCheckBox* exposuretimeCheckBox;
      QTableWidgetItem* illuminationItem;
      QCheckBox* illuminationCheckBox;
      QPushButton* ButtonItem;
      QPushButton* DeleteButtonitem;
      QTableWidgetItem* checkboxitem;
      QTableWidgetItem* cubeItem;
      QCheckBox* cubeCheckBox;
      QTableWidgetItem* intensityItem;
      QCheckBox* intensityCheckBox;
      QTableWidgetItem* shutterItem;
      QCheckBox* shutterCheckBox;
      QImage image;

      ExperimentState() {
        xpos=0;
        ypos=0;
        zpos=0;
        pressure=0;

      }
    };
    struct Protocolentry {

      int belongsto; // listentry to which the protocolentry belongs to. (Stacks have more than one protocolentry)
      int type; //0: wait, 1: move to state & wait, 2:photo & wait 3: move relative & wait
      int duration; // wait time
      int listposition;
      QTimer* timer; // timer
      ExperimentState* state; // relative state to go to (used for type 3 entry)

      Protocolentry() {
        belongsto=0;
        type =0;
        duration=0;
        listposition=0;
      }
    };

    Q_DISABLE_COPY(MainWindow);

    /** Internal method called by the starter functions of the three modes
        It simply collects the common tasks.

    @param mode
        Mode in which to start the controller.
    @return
        0 if the controller thread was successfully started, -1 otherwise.
    */
    int startController(Controller::Mode mode);

    /** Returns the current (incl. data, time and pressure) filename suffix.
        Format is \c _yyyy-MM-dd_hh-mm-ss_##kPa where ## is the pressure and
        the first part represents date and time.
    */
    QString getFilenameSuffix();

    /// Adds a new DisplayMode (also sets up the UI part), identified by a string
    void addDisplayMode(QString name);

    /** Reads all settings from the ini file.
        For a list see class \ref MainWindow "overview".
    */
    void readSettings();

    /** Writes all settings to the ini file.
        For a list see class \ref MainWindow "overview". \n
        This function also writes the DisplayModes and it's settings.
    */
    void writeSettings();

    /** Deciphers a camera mode.
        A string in the QSettings may not contain '/' and '\', so they were
        replaced by \c __fwd_sl__ and \c __bwd_sl__ respectively. \n
        Also, \c text is comprised of the Camera name separated by \c __sep__
        from the actual camera \ref Camera::Mode "mode" string. \n
        If the camera doesn't exist, the mode is not loaded and "" is
        returned.
    */
    QString readCameraMode(QString text);

    /** Read a DisplayMode from \c settings by parsing the QSettings group
        \c displayModeString for \c Camera_Mode, \c Exposure_Time,
        \c Camera_Gain and \c Image_Zoom.
    */
    void readDisplayMode(QSettings& settings, QString displayModeString);

    ExperimentState getstatefromlistpos(int row);

    bool setstate(ExperimentState);
    bool setrelativestate(ExperimentState);

    DraggableLabel*      mImageLabel;               ///< Center image
    DraggableLabel*      mGraphLabel;
    QtSvgDialGauge*      mPressureGauge;            ///< Pressure Gauge
    QCustomPlot*         mPressurePlot;             ///< Pressure plot
    QVector<double>      mPressureValues;           ///< Pressure values
    QVector<double>      mTimeValues;               ///< Time values
    TimeSpinBox*         mExposureTimeBox;          ///< Camera Exposure Time Spin Box
    QLabel*              mCameraFrameRateLabel;     ///< Label for the camera fps
    QLabel*              mControllerFrameRateLabel; ///< Label for the controller fps
    QLabel*              mImageZoomLabel;           ///< Label for the center image zoom
    QIntValidator*       mFFTSizeXValidator;        ///< Qt validator for the FFT width
    QIntValidator*       mFFTSizeYValidator;        ///< Qt validator for the FFT height
    QSignalMapper*       mSingleShotButtonMapper;   ///< Connects multiple single shot buttons to one slot
    QShortcut*           mNextRunShortcut;          ///< Extra shortcut for 'next run'
    QShortcut*			 mSetShutterShortcut;		///< Extra shortcut for shutter application
    QShortcut*			 mSetChannel0Shortcut;		///< Extra shortcut for light application
    QShortcut*			 mSetChannel1Shortcut;		///< Extra shortcut for light application
    QShortcut*			 mSetChannel2Shortcut;		///< Extra shortcut for light application
    QShortcut*			 mSetChannel3Shortcut;		///< Extra shortcut for light application
    QShortcut*			 mSetChannel4Shortcut;		///< Extra shortcut for light application
    QShortcut*			 mSetMultiImagesShortcut;   ///< Extra shortcut for multiimage application
    QShortcut*			 mSetMultiChannelsShortcut;	///< Extra shortcut for multichannel application
    QVector<QShortcut*>  mSingleShotButtonShortcuts;///< Shortcuts for the single shot buttons
    QMovie*              mTrackerAnimation;         ///< Lousy tracker animation
    QTableWidget*        mPoslistwidget;            ///<
    QList<ExperimentState*>* mPoslist;
    QListWidget*	     mProtocolwidget;
    QList<Protocolentry>*mProtocol;
    int					 mCurrentprotocolentry;
    bool				 mProtocolstoprequest;
    PressureSensor*      mPressureSensor;           ///< Pressure Sensor object (managed here)
    auto_ptr<Camera>     mCamera;                   ///< Camera object (managed here)
    QGraphicsScene*      mPreviewScene;             ///< GraphicsScene object required for the image preview
    QGraphicsPixmapItem* mPreviewPixmapItem;        ///< GraphicsPixmapItem object required for the image preview

    auto_ptr<Controller> mController;               ///< Controller object (managed here)
    Stage*               mStage;                    ///< Stage object (managed here)
    Dac*                 mDac;
    ahmMicroscope*       mAhmmicroscope;
    Lamp*                mLamp;                     ///< LED lamp object
    ///TODO one day... started : 20180507
    //ExperimentState*     mCurrentImagingSetting;    ///< Always the actual imaging mode.
    AutoStretch*         mAutoStretch;
    LinearMotor*         mLinearMotor;

    //signalgeneration
    QTimer*              cameratimer;
    int                  dactimerID;
    int                  timeinterval;
    int                  tickcounter;
    int                  steps;
    float                stepsize;
    bool                 rising;
    //CurrentLEDsettting
    int                  channel;
    double               intensity;
    int                  mMainChannel;
    //Lamp trigger variables
    bool                 mNewFrame;
    bool                 mAction;
    //CurrentStagePositionForImaging;
    double               ZPosAtImaging;

    bool				 mSafety;					///< Safetyflag is true if OASIS user limits are set
    //bool				 mKeepinmemory;				///< flag for keeping a requested single shot image in memory
    int					 mCurrentlistposition;		///< Listposition (used to assign kept images to positions)


#ifdef TRACKER_DUMMY
    auto_ptr<DummyMicroscope> mMicroscope;          ///< Dummy Microscope object (managed here)

    Thread*              mMicroscopeThread;         ///< Thread for the dummy microscope
#endif
    Thread*              mCameraThread;             ///< Thread for the Camera
    Thread*              mControllerThread;         ///< Thread for the Controller

    bool                 mDiscardStageStepValue;    ///< See notes in source of on_stageStepBox_valueChanged()
    bool                 mDiscardExposureTimeValue; ///< See notes in source of on_stageStepBox_valueChanged()
    bool                 mDiscardGainValue;         ///< See notes in source of on_stageStepBox_valueChanged()
    bool                 mIgnoreFFTImageSizeChanges;///< Breaks recursion with UI elements
    bool                 mInitialised;              ///< UI elements should discard values if \c false
    /// Protects against queuing shingle shot requests by only allowing one at a time.
    bool                 mStoringImage;
    QString              mStoringDisplayMode;       ///< Temporary value

    QVector<DisplayMode> mDisplayModes;             ///< Lists all available \ref DisplayMode "DisplayModes"
    /// Helps saving changes made to a DisplayMode when it is about to change
    int                  mPreviousDisplayModeIndex;
    ///< Total pixel area of the image (used to adjust Camera parameters automatically)
    int                  mCurrentImageArea;

    int                  newStorageRunSpinBoxValue; /**< New value of the storageSpinBoxes */
    int                  storageRunSpinBoxValue;    /**< Current value of the storageSpinBoxes */
    QString              newStorageFilename;        /**< New name of the file in which the pictures will be stored */
    QString              storageFilename;           /**< Name of the file in which the pictures will be stored */
    int                  mMaxSliderExposureTime;    ///< See settings list in class \ref MainWindow "overview"
    int                  timer100ms;                ///< Make an 100ms timer out of an 10ms timer
    int                  Counter;                   ///< Counter for the Image buffer for "offline" saving
    int                  NumOfImages;               /**< Number Of Images have been requested initiated from Controller::emitAutoPicture();
                                                         thorugh Camera::takeAutoPicture and finally set in MainWindow::shootautopicture()*/
    std::vector<ImageBuffer>  Images;                    /**< Vector of the Images for online saving and easy way to push and pop values */

    QVector<double> measuredForce;                  /**< Vector for the measured force */
    QVector<double> measuredLength;                 /**< Vector for the measured length */
    int                  pictureHeight;             /**< Height of the exported pictures */
    int                  pictureWidth;              /**< Width of the exported pictures */
    bool                 graphOn;                   /**< true = Graph on / false = Graph off */

    QDialog              *mStoragePathDialog;       /**< Pointer to the StoragePath dialog */
    Ui::StoragePath      *mStoragePath;             /**< Pointer to the StoragePath window */
    QMenu                *mViewMenu;                 /**< Pointer to the StoragePath menu*/
    QAction              *acdeacLoggerAction;       /**< Pointer to the LoggerDockWidget activation/deactivation action */
    QAction              *acdeacCameraAction;       /**< Pointer to the CameraDockWidget activation/deactivation action */
    QAction              *acdeacTrackerAction;      /**< Pointer to the TrackerDockWidget activation/deactivation action */
    QAction              *acdeacTrackerOptionsAction;/**< Pointer to the TrackerOptioinsDockWidget activation/deactivation action */
    QAction              *acdeacStageAction;        /**< Pointer to the StageDockWidget activation/deactivation action */
    QAction              *acdeacPressureAction;     /**< Pointer to the PressureDockWidget activation/deactivation action */
    QAction              *acdeacStorageAction;      /**< Pointer to the StorageDockWidget activation/deactivation action */
    QAction              *acdeacPositionListAction; /**< Pointer to the PositionListDockWidget activation/deactivation action */
    QAction              *acdeacFocusInformationAction;/**< Pointer to the FocusInformationDockWidget activation/deactivation action */
    QAction              *acdeacLinMotAction;       /**< Pointer to the LinearMotorDockWidget activation/deactivation action */
    QAction              *acdeacProtocolsAction;    /**< Pointer to the ProtocolsDockWidget activation/deactivation action */
    QAction              *acdeacGraphAction;        /**< Pointer to the GraphDockWidget activation/deactivation action */

    QMenu                *mSettingsMenu;            /**< Pointer to the settings menu */
    QAction              *mStoragePathAction;       /**< Pointer to the StoragePath action */
    QDialog              *mComPortDialog;           /**< Pointer to the ComPort dialog */
    Ui::ComPorts         *mComPort;                 /**< Pointer to the ComPort window */
    QAction              *mComPortAction;           /**< Pointer to the ComPort action */
    std::vector<QString> mComPortStrings;           /**< Vector with the com port numbers and names */
    QDialog              *mScaleFactorDialog;       /**< Pointer to the ScaleFactor dialog */
    Ui::ScaleFactor      *mScaleFactor;             /**< Pointer to the ScaleFactor window */
    QAction              *mScaleFactorAction;       /**< Pointer to the ScaleFactor action */

    QAction              *mStretcherAction;         /**< Pointer to the Stretcher action */
    QDialog              *mStretcherDialog;         /**< Pointer to the Stretcher dialog */
    Ui::Stretcher        *mStretcher;               /**< Pointer to the Stretcher window */

    QAction              *mSetUpHomeAction;         /**< Pointer to the SetUpHome action */
    QDialog              *mSetUpHomeDialog;         /**< Pointer to the SetUpHome dialog */
    Ui::SetUpHome        *mSetUpHome;               /**< Pointer to the SetUpHome window */
  };
}

#endif /* _MainWindow_H__ */
