/*
 Copyright (c) 2009-2013, Andreas Ziegler

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef MESSAGESHANDLER_H
#define MESSAGESHANDLER_H

// included dependencies
#include <QObject>
#include <QTimer>
#include "TrackerPrereqs.h"

namespace tracker
{

  // forward declared dependencies
  class LinearMotor;

  class MessagesHandler : public QObject
  {
    Q_OBJECT
  public:
    /** The constructor
     * @brief Initialization of all the variables, creates the timer and the required connection and initialize mLinearMotor.
     * @param *iLinearMotor Pointer to the LinearMotor object
     * @param *parent Pointer to the corresponding parent
     */
    explicit MessagesHandler(LinearMotor *iLinearMotor, QObject *parent = 0);

     /** The deconstructor
      * @brief Stop timer.
      */
    ~MessagesHandler();

  signals:

    // TODO TODO TODO
    void stopTimer();

  public slots:

    /**
     * @brief Start timer with the defined duration time. It is executed when the thread in which the object will run is started via signal/slot.
     */
    void start();

    /**
     * @brief Is executed, when the timer expired. Reads the incoming messages from the com interface, calculates the position if received
     *        and and inform LinearMotor via wakeAll().
     */
    void checkMessages();

    /**
     * @brief Calculates the position from the receiving position data.
     * @param i Position of the position data in the answerBuffer.
     */
    void calculatePositions(int i);

  private:

    enum { ANSWER_RESET = 0x00};
    enum { ANSWER_SET_DEVICE_MODE = 0x28};
    enum { MESSAGE_CURRENT_POSITION = 0x08};          /**< Reply only message from the motors during moving */
    enum { ANSWER_CURRENT_POSITION = 0x3c};           /**< Answer from the command "Set Current Position" */
    enum { ANSWER_GO_HOME = 0x01};                    /**< Answer from the command "Home" */
    enum { ANSWER_MOVE_ABSOLUT = 0x14};               /**< Answer from the command "Move Absolute" */
    enum { ANSWER_MOVE_RELATIVE = 0x15};              /**< Answer from the command "Move Relative" */
    enum { ANSWER_SET_SPEED = 0x2a};                  /**< Answer from the command "Set Target Speed" */
    enum { ANSWER_MOVE_AT_CONSTANT_SPEED = 0x16};     /**< Answer from the command "Move At Constant Speed" */
    enum { ANSWER_MOTOR_STOP = 0x17};                 /**< Answer from the command "Stop" */

    LinearMotor *mLinearMotor;                        /**< Pointer to the LinearMotor object */
    wxSerialPort* mSerialPort;                        /**< Pointer to the SerialPort object */
    const int mMessagesTimerDuration;                 /**< Timer time duration */
    int mMessagesTimerID;                             /**< ID of the used timer */
    char answerBuffer[12];                            /**< buffer, where the received messages are saved temporarly */

    QTimer *mTimer;                                   /**< Pointer to the Timer object */

  };
}

#endif // MESSAGESHANDLER_H
