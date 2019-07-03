/*
 Copyright (c) 2009-2013, Andreas Ziegler

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

// included dependencies
#include <iostream>
#include "MessagesHandler.h"
#include "LinearMotor.h"
#include "AutoStretch.h"
#include "SerialInterface.h"
#include <wxctb/serport.h>

namespace tracker
{
  /** The constructor
   * @brief Initialization of all the variables, creates the timer and the required connection and initialize mLinearMotor.
   * @param *iLinearMotor Pointer to the LinearMotor object
   * @param *parent Pointer to the corresponding parent
   */
  MessagesHandler::MessagesHandler(LinearMotor *iLinearMotor, QObject *parent)
    : mLinearMotor(iLinearMotor),
      QObject(parent),
      mMessagesTimerDuration(30/*ms*/)
  {
    // Get mSerialPort from mLinearMotor
    mSerialPort = mLinearMotor->mSerialPort;

    // Create timer, get ID and connect Timer's timeout to checkMessages()
    mTimer = new QTimer(this); //parent must be null
    mMessagesTimerID = mTimer->timerId();
    connect(mTimer, SIGNAL(timeout()),
            this,   SLOT(checkMessages()));
    connect(this,   SIGNAL(stopTimer()),
            mTimer, SLOT(stop()));
  }

  /** The deconstructor
   * @brief Stop timer.
   */
   MessagesHandler::~MessagesHandler() {
     emit stopTimer();
     delete mTimer;
   }

  /**
   * @brief Start timer with the defined duration time. It is executed when the thread in which the object will run is started via signal/slot.
   */
  void MessagesHandler::start()
  {
    mTimer->setInterval(mMessagesTimerDuration);
    mTimer->start();
  }

  /**
   * @brief Is executed, when the timer expired. Reads the incoming messages from the com interface, calculates the position if received
   *        and and inform LinearMotor via wakeAll().
   */
  void MessagesHandler::checkMessages()
  {

    mLinearMotor->mExpectedMessagesMutex.lock();
    // Only receive messages, when there are some messages expected.
    if(true == mLinearMotor->mExpectedMessagesFlag) {
      mLinearMotor->mExpectedMessagesMutex.unlock();
      //std::cout << "MessagesHandler Amount of Messages: " << mLinearMotor->mAmountOfMessages << std::endl;

      int size = 6;
      size_t answerLen = 0;
      // Only receive messages, when there is no other method writing to the motors.
      if(true == mLinearMotor->mSendMotorMutex.tryLock()) {
        if(1 == mSerialPort->IsOpen()) {
          answerLen = mSerialPort->Readv(&answerBuffer[answerLen], size, 5/*ms*/);
          // Check if there is any data
          if(answerLen > 0) {
            // Read data until 2 messages (one per motor) is received.
            while(answerLen < 12) {
              if(answerLen > 6) {
                size = 12 - answerLen;
              }
              answerLen += mSerialPort->Readv(&answerBuffer[answerLen], size, 5/*ms*/);
              //std::cout << "MessagesHandler debug answerLen: " << answerLen << std::endl;
            }
          }
        }
        mLinearMotor->mSendMotorMutex.unlock();
      }

      // If messages are received, process the messages.
      if(answerLen >  0) {
        //std::cout << "MessagesHandler Breakpoint answerLen: " << answerLen << std::endl;

        // Check in the whole answerBuffer in the case, that the "Command" is not the second byte.
        for(int i = 0; i < (static_cast<int>(answerLen) - 11); ++i) {
          if((answerBuffer[i] == 0x01) && (answerBuffer[i + 6] == 0x02) ||
             (answerBuffer[i] == 0x02) && (answerBuffer[i + 6] == 0x01)) {
            //std::cout << "MessagesHandler: message start detected." << std::endl;
            //std::cout << " answer 1: " << static_cast<unsigned int>(answerBuffer[i + 1]) << " answer 2: " << static_cast<unsigned int>(answerBuffer[i + 7]) << std::endl;
            // Check if both motors sent the same command answer.
            if(answerBuffer[i + 1] == answerBuffer[i + 7]) {
              //std::cout << "MessagesHandler: correct message received command: " << static_cast<unsigned int>(answerBuffer[i + 1]) << std::endl;
              switch(answerBuffer[i + 1]) {

                case ANSWER_RESET:
                  {
                    QMutexLocker locker2(&(mLinearMotor->mPositonPendingMutex));
                    mLinearMotor->mPositionPendingFlag = false;
                  }
                  break;

                case ANSWER_SET_DEVICE_MODE:
                  {
                    QMutexLocker locker2(&(mLinearMotor->mPositonPendingMutex));
                    mLinearMotor->mPositionPendingFlag = false;
                  }
                  break;

                case ANSWER_CURRENT_POSITION:
                  calculatePositions(i + 1);
                  {
                    // Indicate, that the motors finished moving.
                    QMutexLocker locker1(&(mLinearMotor->mMoveFinishedMutex));
                    mLinearMotor->mMoveFinishedFlag = true;
                  }
                  {
                    QMutexLocker locker2(&(mLinearMotor->mPositonPendingMutex));
                    mLinearMotor->mPositionPendingFlag = false;
                  }
                  // Inform about the new position.
                  mLinearMotor->mCurrentPosCond.wakeAll();
                  std::cout << "MessagesHandler wakeAll" << std::endl;
                  break;

                case MESSAGE_CURRENT_POSITION:
                  calculatePositions(i + 1);
                  {
                    QMutexLocker locker1(&(mLinearMotor->mPositonPendingMutex));
                    mLinearMotor->mPositionPendingFlag = false;
                  }
                  // Inform about the new position.
                  mLinearMotor->mCurrentPosCond.wakeAll();
                  std::cout << "MessagesHandler reply message wakeAll" << std::endl;
                  break;

                case ANSWER_GO_HOME:
                  calculatePositions(i + 1);
                  {
                    // Indicate, that the motors finished moving.
                    QMutexLocker locker1(&(mLinearMotor->mMoveFinishedMutex));
                    mLinearMotor->mMoveFinishedFlag = true;
                  }
                  {
                    // There are no messages expected anymore.
                    QMutexLocker locker(&(mLinearMotor->mExpectedMessagesMutex));
                    mLinearMotor->mExpectedMessagesFlag = false;
                  }
                  // Inform about the new position.
                  mLinearMotor->mCurrentPosCond.wakeAll();
                  std::cout << "MessagesHandler wakeAll" << std::endl;
                  break;

                case ANSWER_MOVE_ABSOLUT:
                  calculatePositions(i + 1);
                  {
                    // Indicate, that the motors finished moving.
                    QMutexLocker locker2(&(mLinearMotor->mMoveFinishedMutex));
                    mLinearMotor->mMoveFinishedFlag = true;
                  }
                  {
                    // There are no messages expected anymore.
                    QMutexLocker locker(&(mLinearMotor->mExpectedMessagesMutex));
                    mLinearMotor->mExpectedMessagesFlag = false;
                  }
                  // Inform about the new position.
                  mLinearMotor->mCurrentPosCond.wakeAll();
                  std::cout << "MessagesHandler wakeAll" << std::endl;
                  break;

                case ANSWER_MOVE_RELATIVE:
                  // Inform mAutoStretch, that the motors finished moving.
                  mLinearMotor->mAutoStretch->mPreCondMotorWaitCond.wakeOne();
                  calculatePositions(i + 1);
                  {
                    // Indicate, that the motors finished moving.
                    QMutexLocker locker2(&(mLinearMotor->mMoveFinishedMutex));
                    mLinearMotor->mMoveFinishedFlag = true;
                  }
                  {
                    // There are no messages expected anymore.
                    QMutexLocker locker(&(mLinearMotor->mExpectedMessagesMutex));
                    mLinearMotor->mExpectedMessagesFlag = false;
                  }
                  // Inform about the new position.
                  mLinearMotor->mCurrentPosCond.wakeAll();
                  std::cout << "MessagesHandler wakeAll" << std::endl;
                  break;

                case ANSWER_SET_SPEED:
                  //emit signalSetSpeed();
                  break;

                case ANSWER_MOVE_AT_CONSTANT_SPEED:
                  //emit signalMoveAtConstatntSpeed();
                  break;

                case ANSWER_MOTOR_STOP:
                  calculatePositions(i + 1);
                  {
                    // Indicate, that the motors finished moving.
                    QMutexLocker locker2(&(mLinearMotor->mMoveFinishedMutex));
                    mLinearMotor->mMoveFinishedFlag = true;
                  }
                  {
                    // There are no messages expected anymore.
                    QMutexLocker locker(&(mLinearMotor->mExpectedMessagesMutex));
                    mLinearMotor->mExpectedMessagesFlag = false;
                  }
                  // Inform about the new position.
                  mLinearMotor->mCurrentPosCond.wakeAll();
                  std::cout << "MessagesHandler wakeAll" << std::endl;
                  break;
              }
            }

          } else {

          }
        }
      }
    } else {
      mLinearMotor->mExpectedMessagesMutex.unlock();
    }
  }

  /**
   * @brief Calculates the position from the receiving position data. See manual page. 7
   * @param i Position of the position data in the answerBuffer.
   */
  void MessagesHandler::calculatePositions(int i)
  {

    QMutexLocker locker(&(mLinearMotor->mCurrentPosMutex));
    mLinearMotor->mCurrentPosMotor[0] = static_cast<unsigned char>(answerBuffer[i + 4]) * (256*256*256) +
                                        static_cast<unsigned char>(answerBuffer[i + 3]) * (256*256) +
                                        static_cast<unsigned char>(answerBuffer[i + 2]) * 256 +
                                        static_cast<unsigned char>(answerBuffer[i + 1]);
    mLinearMotor->mCurrentPosMotor[1] = static_cast<unsigned char>(answerBuffer[i + 10]) * 256*256*256 +
                                        static_cast<unsigned char>(answerBuffer[i + 9]) * 256*256 +
                                        static_cast<unsigned char>(answerBuffer[i + 8]) * 256 +
                                        static_cast<unsigned char>(answerBuffer[i + 7]);

    if(answerBuffer[i + 4] > 127) {
      mLinearMotor->mCurrentPosMotor[0] -= 256*256*256*256;
    }
    if(answerBuffer[i + 10] > 127) {
      mLinearMotor->mCurrentPosMotor[1] -= 256*256*256*256;
    }
  }
}
