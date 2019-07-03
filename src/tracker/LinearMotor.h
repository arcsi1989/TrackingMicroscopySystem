/*
 Copyright (c) 2009-2013, Reto Grieder, Benjamin Beyeler, Andreas Ziegler

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef _LINEARMOTOR_H__
#define _LINEARMOTOR_H__

// included dependencies
#include "TrackerPrereqs.h"

#include <string>
#include <QObject>
#include <QMutex>
#include <QTimer>
#include <QThread>
#include <QWaitCondition>
#include "SerialInterface.h"
#include "MessagesHandler.h"

namespace tracker
{
  class AutoStretch;
  /** Class that provides access to the hardware module with the zaber linear stepper motor.
  *
  */

  class LinearMotor : public QObject, public SerialInterface
  {
    Q_OBJECT

  public:
    //! Port defaults to com1 and baud rate to 115200
    LinearMotor(QObject* parent = 0, AutoStretch* iAutoStretch = NULL);
    ~LinearMotor();

    /**
     * @brief Creates the MessagesHandler in a new thread.
     */
    void createMessagesHandler();

    /**
     * @brief Activate "Move Tracking" and deactivate "Potentiometer" on the linear motors.
     */
    void setDeviceMode();

    //! Resets the motors
    void reset(void);

    /**
     * @brief Returns the current position of the two motors.
     * @return Pointer to the int arry, which holds the position values.
     */
    void getPosition(void);

    //! Sends the motors to the home position
    void sendHome(void);
    //! Sends the motors to a working position
    void sendWork(void);
    //! Sets the moving speed of the motors in millimeters per second
    void setSpeed(double speedinmm);
    /**
     * Increase current speed.
     */
    void increaseSpeed();
    /**
     * Decrease current speed.
     */
    void decreaseSpeed();
    /**
     * @brief Moves the motors with the defined speed.
     * @param[in] speedinmm Speed with which the motor should move.
     */
    void moveSpeed(double speedinmm = 0);
    /**
     * Moves the motors with the defined speed in reverse direction.
     * \param[in] speedinmm Speed with which the motor should move.
     */
    void reverseMoveSpeed(double speedinmm = 0);
    /**
     * Stops the motors.
     */
    void stop();
    //! Moves the motors an amount of microsteps
    void moveSteps(int steps);
    //! transforms data to text for communication
    char*transformdectotext(int dec);
    //! moves the motors an amount of millimeters
    void moveMillimeters(double millimeters);
    //! starts an oscillation with four steps: move closer, wait, move further away and wait
    void startoscillation(double amplitude, double period);
    //! finishes the ongoing oscillations
    void stoposcillation(void);
    //! checks if the oscillation is possible with the chosen amplitude and period
    bool checkoscillation(double amplitude, double period);
    /// Internal function called in the constructor to read settings values.
    void readSettings();

    /// Internal function called in the destructor to write settings values.
    void writeSettings() const;

  public slots:
    /**
     * @brief Set the com port for the linear motorsl
     * @param iPort Com port on which the linear motors are attached
     */
    void setPort(std::string iPort);

    /**
     * @brief Close the com port
     */
    void closePort();

    /**
     * @brief Opens the specified com port
     * @param iPort Com port on which the linear motors are attached
     */
    void openPort(std::string iPort);

    /**
     * @brief Set the distance when the motors are in position 53334 (max. position).
     * @param distance Distance in microsteps
     */
    void setZeroDistance(double distance);

    /**
     * @brief Calculated the distance between the motor positions and returns it in microsteps.
     * @return Length of material.
     */
    unsigned int getCurrentDistance();

    /**
     * @brief Calculated the distance between the motor positions (if available otherwise the last positions) and returns it in microsteps.
     * @return Length of material.
     */
    int getDistance();

    /**
     * @brief Calculate the amount of steps, that the motors have to move to reach the desired distance
     *        and start the motors.
     * @param distance Desired clamping distance in mm from the GUI
     */
    void gotoMMDistance(double mmDistance);

    /**
     * @brief Calculate the amount of steps, that the motors have to move to reach the desired distance
     *        and start the motors.
     * @param distance Desired clamping distance in microsteps from the GUI
     */
    void gotoMSDistance(double msDistance);

  signals:
    //! Infor when the speed changed
    void speedChanged(int mCurrentSpeed);

    friend class MessagesHandler;         /**< Defines MessagesHandler as friend class, that it has easy access to member variables */
    friend class AutoStretch;             /**< Defines AutoStretch as friend class, that it has easy access to member variables */
  private:
    //! Command strings that persuade the motors to move
    //Some commands need a suffix with data
    // additional commands can be found in the Zaber manual

    // Defined commands for the Zaber linear motors.
    const char *MOTORS_DEVICE_MODE;
    const char *MOTORS_RESET;
    const char *MOTORS_RETURN_CURRENT_POSITION;
    const char *MOTORS_GO_HOME;			//set to home distance (0)
    const char *MOTORS_MOVE_ABSOLUTE;
    const char *MOTORS_MOVE_RELATIVE;
    const char *MOTORS_SET_SPEED;
    const char *MOTORS_MOVE_AT_CONSTANT_SPEED;
    const char *MOTORS_STOP;

    const double MM_PER_MS;               /**< milimeter per microstep */

    const float DecIncSpeed;              /**< Amount of speed to increase/decrease */

    double mStepsize;			           			//!< Stepsize of the stepper motor in millimeters
    char mbytedata[4];
    int mOscstate;							          //!< Current state of the oscillation
    double mAmplitude;						        //!< Amplitude of the oscillation
    bool mStoprequest;						        //!< stopflag for the oscillation
    int mLMtimerID;							          //!< Timer ID of the oscillation timer
    int mCurrentSpeed;                    /**< The current speed */
    int mCurrentPosMotor[2];              /**< Position of the two motors */
    double mZeroDistance;                 /**< Distance when the motors are on max position (resulting in smallest distance) */
    QMutex mSendMotorMutex;               /**< Mutex to protect motor for simultaneous sending and receiving of messages */
    QMutex mCurrentPosMutex;              /**< Mutex to protect mCurrentPosMotor[] */
    QMutex mGetPosMutex;                  /**< Mutex to ensure, that getPosition is not performed multiple times the same time */
    QMutex mOldPositionMutex;             /**< Mutex to protect mOldPositionFlag */
    QWaitCondition mCurrentPosCond;       /**< Condition variable to inform getPosition() that new positions are available */
    QThread *mMessagesHandlerThread;      /**< Thread in which the MessagesHandler will run */
    bool mOldPositionFlag;                /**< Flag to indicate that the current "old" position is still up to date */
    bool mMoveFinishedFlag;               /**< Flag to indicate, that the motors finished moving */
    QMutex mMoveFinishedMutex;            /**< Mutex to protect mMoveFinishedFlag */
    bool mPositionPendingFlag;            /**< Indicate that a RETURN_CURRENT_POSITION command is executed but the answer not yet arrived */
    QMutex mPositonPendingMutex;          /**< Mutex to protect mPositionPendingFlag */
    bool mStartUpFlag;                    /**< Indicates that software is starting up or started up */

    bool mExpectedMessagesFlag;           /**< Indicates that messages are expected */
    QMutex mExpectedMessagesMutex;        /**< Mutex to protect mAmountOfMessages */

    //! Timer callback, the timer is used for the oscillation
    void timerEvent(QTimerEvent* event);

    AutoStretch *mAutoStretch;            /**< Pointer to the AutoStretch object */
    MessagesHandler *mMessagesHandler;    /**< Pointer to the MessagesHandler object */
  };
}

#endif /* _LINEARMOTOR_H__ */
