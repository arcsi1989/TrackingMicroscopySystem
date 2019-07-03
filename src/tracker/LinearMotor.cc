/*
 Copyright (c) 2009-2013, Reto Grieder, Benjamin Beyeler, Andreas Ziegler

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#include "LinearMotor.h"
#include <iostream>

#include <cstring>
#include <QSettings>
#include <QWaitCondition>
#include <QMutexLocker>
#include <QTimerEvent>
#include <wxctb/serport.h>

#include "Exception.h"
#include "PathConfig.h"
#include "AutoStretch.h"

namespace tracker
{
  LinearMotor::LinearMotor(QObject* parent, AutoStretch* iAutoStretch)
    : QObject(parent),
    SerialInterface("com4", 9600/*baudrate*/),
    mCurrentPosMutex(QMutex::NonRecursive),
    mGetPosMutex(QMutex::NonRecursive),
    mExpectedMessagesFlag(0),
    mOldPositionFlag(true),
    mMoveFinishedFlag(true),
    mStepsize(0.000047625),                    //Stepsize of Zaber T-LSM025A motor in millimeters
    mOscstate(0),
    mStoprequest(false),
    mLMtimerID(0),
    mAmplitude(0),
    mAutoStretch(iAutoStretch),
    MOTORS_DEVICE_MODE("\x00\x028"),
    MOTORS_RESET("\x00\x00"),
    MOTORS_RETURN_CURRENT_POSITION("\x00\x03c"),
    MOTORS_GO_HOME("\x00\x01"),
    MOTORS_MOVE_ABSOLUTE("\x00\x014"),
    MOTORS_MOVE_RELATIVE("\x00\x015"),
    MOTORS_SET_SPEED("\x00\x02a"),
    MOTORS_MOVE_AT_CONSTANT_SPEED("\x00\x016"), //: QObject(parent)
    MOTORS_STOP("\x00\x017"),
    MM_PER_MS(0.000047625),
    DecIncSpeed(0.3),
    mZeroDistance(45144/*microsteps=6.39mm offset */),
    mPositionPendingFlag(false),
    mStartUpFlag(true)
  {
    this->readSettings();
    this->openConnection();
    this->setDeviceMode();
    mAutoStretch->connect(this);                  // Let make AutoStretch an aggregation via pointer
    mMessagesHandler = new MessagesHandler(this);
  }

  /**
   * @brief Creates the MessagesHandler in a new thread.
   */
  void LinearMotor::createMessagesHandler() {
    mMessagesHandlerThread = new QThread(this);
    connect(mMessagesHandlerThread, SIGNAL(started()),
            mMessagesHandler, SLOT(start()));
    mMessagesHandler->moveToThread(mMessagesHandlerThread);
    mMessagesHandlerThread->start();
  }

  LinearMotor::~LinearMotor()
  {
    if(mLMtimerID > 0) {
      killTimer(mLMtimerID);
    }
    mMessagesHandlerThread->quit();

    this->writeSettings();
    delete mMessagesHandlerThread;
    delete mMessagesHandler;
  }

  /**
   * @brief Set the com port for the linear motorsl
   * @param iPort Com port on which the linear motors are attached
   */
  void LinearMotor::setPort(std::string iPort)
  {
    this->closeConnection();
    mPort = iPort;
    this->openConnection();
  }

  /**
   * @brief Close the com port
   */
  void LinearMotor::closePort() {
    this->closeConnection();
  }

  /**
   * @brief Opens the specified com port
   * @param iPort Com port on which the linear motors are attached
   */
  void LinearMotor::openPort(std::string iPort) {
    mPort = iPort;
    this->openConnection();
  }

  /**
   * @brief Set the distance when the motors are in position 53334 (max. position).
   * @param distance Distance in microsteps
   */
  void LinearMotor::setZeroDistance(double distance)
  {
    mZeroDistance = distance;
  }

  /**
   * @brief Activate "Move Tracking" and deactivate "Potentiometer" on the linear motors.
   */
  void LinearMotor::setDeviceMode() {
    char buffer[6];
    char command[6]="";
    memcpy(command, MOTORS_DEVICE_MODE, 2);

    char *value = transformdectotext(16 + 8);             // bit_4*2^4 = Enable Move Tracking + bit_3*2^3 = Deactivate potentiometer
    memcpy(command+2, value, 4);
    memcpy(buffer, command, 6);
    {
      QMutexLocker locker(&mSendMotorMutex);
      size_t writtenLength = mSerialPort->Writev(buffer, 6, 5/*ms*/);
    }
    {
      QMutexLocker locker2(&mExpectedMessagesMutex);
      mExpectedMessagesFlag++;
    }
  }

  void LinearMotor::reset(void)
  {
    char buffer[6];
    memcpy(buffer, MOTORS_RESET, 6);
    {
      QMutexLocker locker(&mSendMotorMutex);
      size_t writtenLength = mSerialPort->Writev(buffer, 6, 5/*ms*/);
    }
    {
      QMutexLocker locker2(&mExpectedMessagesMutex);
      mExpectedMessagesFlag = true;
    }
  }

  /**
   * @brief Calculated the distance between the motor positions and returns it in microsteps.
   * @return Length of material.
   */
  unsigned int LinearMotor::getCurrentDistance() {
    mPositonPendingMutex.lock();
    QMutexLocker locker(&mGetPosMutex);
    mOldPositionMutex.lock();

    // Only wait for new position if current position is not longer up to date
    if(false == mOldPositionFlag) {
      mOldPositionFlag = true;
      mOldPositionMutex.unlock();
      mPositionPendingFlag = true;
      mPositonPendingMutex.unlock();

      std::cout << "LineaMotor current Mutex wait" << std::endl;
      mCurrentPosMutex.lock();
      mCurrentPosCond.wait(&mCurrentPosMutex);
      mCurrentPosMutex.unlock();

    } else {
        mOldPositionMutex.unlock();
        mPositonPendingMutex.unlock();
    }
    unsigned int distance = 0;
    {
      QMutexLocker locker(&mCurrentPosMutex);
      distance = (std::abs(533334 /*max. position*/ - mCurrentPosMotor[0]) + std::abs(533334 - mCurrentPosMotor[1])) + mZeroDistance ; //134173 /*microsteps=6.39mm offset */;
    }

    //std::cout << "LinearMotor pos 1: " << currentPosMotor[0] << ", pos 2: " << currentPosMotor[1] << std::endl;
    //std::cout << "    Distance: " << static_cast<double>(distance * MM_PER_MS)  << std::endl;

    return(distance);
  }

  /**
   * @brief Calculated the distance between the motor positions (if available otherwise the last positions) and returns it in microsteps.
   * @return Length of material.
   */
  int LinearMotor::getDistance() {
    // Get current positio when software is starting up
    if(true == mStartUpFlag) {
      mStartUpFlag = false;
      QMutexLocker locker(&mGetPosMutex);
      getPosition();
    }
    int distance = 0;
    {
      QMutexLocker locker(&mCurrentPosMutex);
      distance = (std::abs(533334 /*max. position*/ - mCurrentPosMotor[0]) + std::abs(533334 - mCurrentPosMotor[1])) + mZeroDistance ; //134173 /*microsteps=6.39mm offset */;
    }

    //std::cout << "LinearMotor pos 1: " << currentPosMotor[0] << ", pos 2: " << currentPosMotor[1] << std::endl;
    //std::cout << "    Distance: " << static_cast<double>(distance * MM_PER_MS)  << std::endl;

    return(distance);
  }

  /**
   * @brief Returns the current position of the two motors.
   * @return Pointer to the int arry, which holds the position values.
   */
  void LinearMotor::getPosition(void) {
    char buffer[6];

    // Send "return current position" command to the motors.
    memcpy(buffer, MOTORS_RETURN_CURRENT_POSITION, 6);
    size_t writtenLength = 0;
    {
      {
        QMutexLocker locker(&mSendMotorMutex);
        writtenLength = mSerialPort->Writev(buffer, 6, 5/*ms*/);
      }
      {
        QMutexLocker locker2(&mExpectedMessagesMutex);
        mExpectedMessagesFlag = true;
      }
    }

    // Wait until position message is received
    std::cout << "LineaMotor Mutex wait" << std::endl;
    mCurrentPosMutex.lock();
    mCurrentPosCond.wait(&mCurrentPosMutex);
    mCurrentPosMutex.unlock();

  }

  void LinearMotor::sendHome(void)
  {
      char buffer[6];
      memcpy(buffer, MOTORS_GO_HOME, 6);
      {
        QMutexLocker locker1(&mSendMotorMutex);
        size_t writtenLength = mSerialPort->Writev(buffer, 6, 5/*ms*/);
      }
      {
        QMutexLocker locker2(&mExpectedMessagesMutex);
        mExpectedMessagesFlag = true;
      }
  }
  void LinearMotor::sendWork(void)
  {
      char buffer[6];
      char command[6]="";
      char* workdistance;
      workdistance= transformdectotext(200000);
      memcpy(command,MOTORS_MOVE_ABSOLUTE,2);
      memcpy(command+2,workdistance,4);
      memcpy(buffer, command, 6);

      mMoveFinishedMutex.lock();
      if(true == mMoveFinishedFlag) {
        mMoveFinishedMutex.unlock();
        {
          QMutexLocker locker(&mSendMotorMutex);
          size_t writtenLength = mSerialPort->Writev(buffer, 6, 5/*ms*/);
        }
        {
          QMutexLocker locker2(&mExpectedMessagesMutex);
          mExpectedMessagesFlag = true;
        }
        {
          QMutexLocker locker3(&mMoveFinishedMutex);
          mMoveFinishedFlag = false;
        }
    } else {
      mMoveFinishedMutex.unlock();

      {
        QMutexLocker locker(&mSendMotorMutex);
        size_t writtenLength = mSerialPort->Writev(buffer, 6, 5/*ms*/);
      }
      {
        QMutexLocker locker2(&mMoveFinishedMutex);
        mMoveFinishedFlag = false;
      }
    }
  }

  /**
   * @brief Calculate the amount of steps, that the motors have to move to reach the desired distance
   *        and start the motors.
   * @param distance Desired clamping distance in mm from the GUI
   */
  void LinearMotor::gotoMMDistance(double mmDistance)
  {
    int currentDistance = getCurrentDistance();

    int amSteps = (currentDistance - mmDistance/MM_PER_MS) / 2;
    moveSteps(amSteps);
  }

  /**
   * @brief Calculate the amount of steps, that the motors have to move to reach the desired distance
   *        and start the motors.
   * @param distance Desired clamping distance in micro steps from the GUI
   */
  void LinearMotor::gotoMSDistance(double msDistance)
  {
    int currentDistance = getCurrentDistance();

    int amSteps = (currentDistance - msDistance) / 2;
    moveSteps(amSteps);
  }

  void LinearMotor::moveSteps(int steps)
  {
      char buffer[6];
      char command[6]="";
      char* number;
      memcpy(command,MOTORS_MOVE_RELATIVE,2);
      number = transformdectotext(steps);
      memcpy(command+2,number,4);
      memcpy(buffer,command , 6);

      mMoveFinishedMutex.lock();
      if(true == mMoveFinishedFlag) {
        mMoveFinishedMutex.unlock();
        {
          QMutexLocker locker(&mSendMotorMutex);
          size_t writtenLength = mSerialPort->Writev(buffer, 6, 5/*ms*/);
        }
        {
          QMutexLocker locker2(&mExpectedMessagesMutex);
          mExpectedMessagesFlag = true;
        }
        {
          QMutexLocker locker4(&mMoveFinishedMutex);
          mMoveFinishedFlag = false;
        }
    } else {
      mMoveFinishedMutex.unlock();
      {
        QMutexLocker locker(&mSendMotorMutex);
        size_t writtenLength = mSerialPort->Writev(buffer, 6, 5/*ms*/);
      }
      {
        QMutexLocker locker4(&mMoveFinishedMutex);
        mMoveFinishedFlag = false;
      }
    }
  }
  void LinearMotor::setSpeed(double speedinmm)
  {
    int speed=0;
    char buffer[6];
    char command[6]="";
    char* number;

    if(speedinmm != 0) {
      speed=speedinmm/(9.375*mStepsize); //transformation from mm/s to datavalue
      mCurrentSpeed = speed;
    } else {
      speed = mCurrentSpeed;
    }

    memcpy(command,MOTORS_SET_SPEED,2);
    number = transformdectotext(speed);
    memcpy(command+2,number,4);
    memcpy(buffer,command , 6);
    size_t writtenLength = 0;
    {
      QMutexLocker locker(&mSendMotorMutex);
      writtenLength = mSerialPort->Writev(buffer, 6, 5/*ms*/);
    }
    {
      QMutexLocker locker2(&mExpectedMessagesMutex);
      mExpectedMessagesFlag = true;
    }
    emit speedChanged(speed);
  }

  /**
   * Increase current speed.
   */
  void LinearMotor::increaseSpeed()
  {
    int speed=0;
    char buffer[6];
    char command[6]="";
    char* number;

    if(mCurrentSpeed <= 0) {
      speed = 1.0 /(9.375*mStepsize); //transformation from mm/s to datavalue
    } else {
      speed = mCurrentSpeed + DecIncSpeed /(9.375*mStepsize); //transformation from mm/s to datavalue
    }

    if(speed <= 0) {
      speed = 1;
    }
    mCurrentSpeed = speed;
    if(speed >= 5000) {
      // Inform AutoStretch, that number of collected samples can be reduced.
      mAutoStretch->setStdForceSamples();
    }
    if(speed >= 10000) {
      speed = 10000;
      mAutoStretch->setSpeedIsMax();
    }
    std::cout << "LinearMotor speed: " << mCurrentSpeed << std::endl;

    memcpy(command,MOTORS_SET_SPEED,2);
    number = transformdectotext(speed);
    memcpy(command+2,number,4);
    memcpy(buffer,command , 6);
    size_t writtenLength = 0;
    {
      QMutexLocker locker(&mSendMotorMutex);
      writtenLength = mSerialPort->Writev(buffer, 6, 5/*ms*/);
    }
    {
      QMutexLocker locker2(&mExpectedMessagesMutex);
      mExpectedMessagesFlag = true;
    }
    emit speedChanged(speed);
  }

  /**
   * Decrease current speed.
   */
  void LinearMotor::decreaseSpeed()
  {
    int speed=0;
    char buffer[6];
    char command[6]="";
    char* number;

    speed = mCurrentSpeed - DecIncSpeed /(9.375*mStepsize); //transformation from mm/s to datavalue
    if(speed <= 2000) {
      speed = 2000;
      // Inform AutoStretch, that number of collected samples can be increased.
      mAutoStretch->setZeroForceSamples();
    }
    mCurrentSpeed = speed;
    std::cout << "LinearMotor speed: " << mCurrentSpeed << std::endl;

    memcpy(command,MOTORS_SET_SPEED,2);
    number = transformdectotext(speed);
    memcpy(command+2,number,4);
    memcpy(buffer,command , 6);
    size_t writtenLength = 0;
    {
      QMutexLocker locker(&mSendMotorMutex);
      writtenLength = mSerialPort->Writev(buffer, 6, 5/*ms*/);
    }
    {
      QMutexLocker locker2(&mExpectedMessagesMutex);
      mExpectedMessagesFlag = true;
    }
    emit speedChanged(speed);
  }

  /**
   * Moves the motors with the defined speed.
   * \param[in] Speed with which the motor should move.
   */
  void LinearMotor::moveSpeed(double speedinmm)
  {
      int speed=0;
      char buffer[6];
      char command[6] = "";
      char *number;

      if(speedinmm != 0) {
        speed=speedinmm/(9.375*mStepsize); //transformation from mm/s to datavalue
        mCurrentSpeed = speed;
      } else {
        speed = mCurrentSpeed;
      }

      memcpy(command, MOTORS_MOVE_AT_CONSTANT_SPEED, 2);
      number = transformdectotext(-speed);
      memcpy(command+2, number, 4);
      memcpy(buffer, command, 6);

      mMoveFinishedMutex.lock();
      if(true == mMoveFinishedFlag) {
        mMoveFinishedMutex.unlock();
        {
          QMutexLocker locker(&mSendMotorMutex);
          size_t writtenLength = mSerialPort->Writev(buffer, 6, 5/*ms*/);
        }
        {
          QMutexLocker locker2(&mExpectedMessagesMutex);
          mExpectedMessagesFlag = true;
        }
        {
          QMutexLocker locker3(&mMoveFinishedMutex);
          mMoveFinishedFlag = false;
        }
        {
          QMutexLocker locker4(&mOldPositionMutex);
          mOldPositionFlag = false;
        }
      } else {
        mMoveFinishedMutex.unlock();
        {
          QMutexLocker locker(&mSendMotorMutex);
          size_t writtenLength = mSerialPort->Writev(buffer, 6, 5/*ms*/);
        }
        {
          QMutexLocker locker2(&mMoveFinishedMutex);
          mMoveFinishedFlag = false;
        }
        {
          QMutexLocker locker3(&mOldPositionMutex);
          mOldPositionFlag = false;
        }
      }
  }

  /**
   * Moves the motors with the defined speed in reverse direction.
   * \param[in] Speed with which the motor should move.
   */
  void LinearMotor::reverseMoveSpeed(double speedinmm)
  {
      int speed=0;
      char buffer[6];
      char command[6]="";
      char* number;

      if(speedinmm != 0) {
        speed=speedinmm/(9.375*mStepsize); //transformation from mm/s to datavalue
        mCurrentSpeed = speed;
      } else {
        speed = mCurrentSpeed;
      }

      memcpy(command,MOTORS_MOVE_AT_CONSTANT_SPEED,2);
      number = transformdectotext(speed);
      memcpy(command+2,number,4);
      memcpy(buffer,command , 6);

      mMoveFinishedMutex.lock();
      if(true == mMoveFinishedFlag) {
        mMoveFinishedMutex.unlock();
        {
          QMutexLocker locker(&mSendMotorMutex);
          size_t writtenLength = mSerialPort->Writev(buffer, 6, 5/*ms*/);
        }
        {
          QMutexLocker locker2(&mExpectedMessagesMutex);
          mExpectedMessagesFlag = false;
        }
        {
          QMutexLocker locker3(&mMoveFinishedMutex);
          mMoveFinishedFlag = false;
        }
        {
          QMutexLocker locker4(&mOldPositionMutex);
          mOldPositionFlag = false;
        }
      } else {
        mMoveFinishedMutex.unlock();
        {
          QMutexLocker locker(&mSendMotorMutex);
          size_t writtenLength = mSerialPort->Writev(buffer, 6, 5/*ms*/);
        }
        {
          QMutexLocker locker2(&mMoveFinishedMutex);
          mMoveFinishedFlag = false;
        }
        {
          QMutexLocker locker3(&mOldPositionMutex);
          mOldPositionFlag = false;
        }
    }
  }

  /**
   * Stops the motors.
   */
  void LinearMotor::stop()
  {
    char buffer[6];
    char command[6] = "";

    memcpy(command, MOTORS_STOP, 2);
    memcpy(buffer, command, 6);

    mMoveFinishedMutex.lock();
    if(true == mMoveFinishedFlag) {
      mMoveFinishedMutex.unlock();
      {
        QMutexLocker locker(&mSendMotorMutex);
        size_t writtenLength = mSerialPort->Writev(buffer, 6, 5/*ms*/);
      }
      {
        QMutexLocker locker2(&mExpectedMessagesMutex);
        mExpectedMessagesFlag = true;
      }
      {
        QMutexLocker locker3(&mOldPositionMutex);
        mOldPositionFlag = true;
      }
      {
        QMutexLocker locker4(&mMoveFinishedMutex);
        mMoveFinishedFlag = false;
      }
    } else {
        mMoveFinishedMutex.unlock();
        {
          QMutexLocker locker(&mSendMotorMutex);
          size_t writtenLength = mSerialPort->Writev(buffer, 6, 5/*ms*/);
        }
        {
          QMutexLocker locker3(&mOldPositionMutex);
          mOldPositionFlag = true;
        }
        {
          QMutexLocker locker4(&mMoveFinishedMutex);
          mMoveFinishedFlag = false;
        }
    }
  }

  void LinearMotor::moveMillimeters(double millimeters)
  {
    int steps=0;
    steps=millimeters/mStepsize;//transformation from millimeters to steps
    moveSteps(steps);
  }

  char* LinearMotor::transformdectotext(int dec)
  {
    char bytes[4];
    unsigned int decnumber;
    decnumber=dec;
    if (decnumber<0) {
      decnumber=decnumber+(256*256*256*256);
    }

    bytes[3]=decnumber/(256*256*256);
    decnumber=decnumber-256*256*256*bytes[3];

    bytes[2] =decnumber/(256*256);
    decnumber=decnumber-256*256*bytes[2];

    bytes[1] =decnumber/256;
    decnumber=decnumber-256*bytes[1];

    bytes[0] =decnumber;
    memcpy(mbytedata,bytes,4);
    return mbytedata;
  }

  void LinearMotor::startoscillation(double amplitude, double period)
  {
    int timerinterval=1000*period/4;

    if(!mLMtimerID) {
      mLMtimerID=this->startTimer(timerinterval);
      mAmplitude = amplitude;
      setSpeed(7.0);
      mOscstate=1;

    }

  }
  void LinearMotor::stoposcillation()
  {
    mStoprequest=true;
  }

  void  LinearMotor::timerEvent(QTimerEvent* event)
  {
    if(event->timerId() == mLMtimerID) {
      switch (mOscstate) {

        case 0:	//wait
          if (mStoprequest) {
            killTimer(mLMtimerID);
            mStoprequest = false;
            mLMtimerID=0;
          }
          mOscstate=1;
          break;

        case 1:	//move closer
          moveMillimeters(mAmplitude);
          mOscstate=2;

          break;

        case 2:	//wait
          mOscstate=3;

          break;

        case 3: //move back
          moveMillimeters(-mAmplitude);
          mOscstate=0;
          break;

        default:
          ;
      }
    }
  }

  bool LinearMotor::checkoscillation(double amplitude, double period)
  {
    if((amplitude/period)<1.4f)// 1.4 mm/s is the threshold due to the max speed of 7mm/s (divided by 4(they only have time one fourth of a period to move the distance) minus some safety for timing inaccuracies)
      return true;
    else
      return false;

  }

  void LinearMotor::readSettings()
  {
    QSettings settings(PathConfig::getConfigFilename(), QSettings::IniFormat);
    settings.beginGroup("LinearMotor");

    mPort        =            settings.value("Port", "com5").toString().toStdString();
    mBaudrate    = qMax(9600, settings.value("Baudrate", 9600).toInt());
	mZeroDistance =		      settings.value("ZeroDistance", 45144).toInt(); 

    settings.endGroup();
  }

  void LinearMotor::writeSettings() const
  {
    QSettings settings(PathConfig::getConfigFilename(), QSettings::IniFormat);
    settings.beginGroup("LinearMotor");

    settings.setValue("Port", QString::fromStdString(mPort));
    settings.setValue("Baudrate", mBaudrate);
	settings.setValue("ZeroDistance", mZeroDistance);

    settings.endGroup();
  }
}
