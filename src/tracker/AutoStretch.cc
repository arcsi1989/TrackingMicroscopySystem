/*
 Copyright (c) 2013, Andreas Ziegler

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

// included dependencies
#include <iostream>
#include <cmath>
#include <QtConcurrentRun>
#include <QFuture>
#include <QWaitCondition>

#include "MainWindow.h"
#include "Thread.h"
#include "AutoStretch.h"
#include "LinearMotor.h"

namespace tracker
{

  /** The constructor
   * @brief Initialization of all the variables, and setting force as the default unit.
   * @param[in] *iMainWindows Pointer to the MainWindow object.
   */
  AutoStretch::AutoStretch(MainWindow *iMainWindow)
    :mMainWindow(iMainWindow),
     MM_PER_MS(0.000047625),
     currentState(State::stopState),
     currentMode(Mode::Until),
     stretchUntilValue(0),
     currentUnit(Unit::Force),
     currentForce(0),
     amChanges(0),
     lastDirection(Direction::Stop),
     lastForce(0),
     maxForce(0),
     //ForceThreshold(0.05),
     //ChangeForceLowThreshold(0.03),
     //ChangeForceHighThreshold(0.05),
     forceThreshold(0.1),
     changeForceLowThreshold(0.01),
     changeForceHighThreshold(0.10),
     amForceUpdate(0),
     flagCareForce(false),
     amForceSamples(10),
     amStdForceSamples(10),
     amZeroForceSamples(5),
     speedIsMin(false),
     speedIsMax(false),
     mPreCondPreloadDistance(0),
     mRampToFailureFlag(false),
     mRampToFailureStopFlag(false)
 /*,
 timeDivider(0)
 */
  {
    updateAutoStretchUnit(0);
  }

  /**
   * @brief Let LinearMotor connect with AutoStretch, that *mLinearMotor can be defined.
   * @param *iLinearMotor Pointer to the LinearMotor object.
   */
  void AutoStretch::connect(LinearMotor *iLinearMotor)
  {
    mLinearMotor = iLinearMotor;
  }

  /**
   * @brief Set amStdForceSamples from GUI.
   * @param iamForceSamples Choosen standard amount of samples from GUI.
   */
  void AutoStretch::setAmStdForceSamples(double iamForceSamples)
  {
    amStdForceSamples = iamForceSamples;
  }

  /**
   * @brief Set amZeroForceSamples from GUI.
   * @param iamZeroForceSamples Choosen amount of samples when speed is 0 from GUI.
   */
  void AutoStretch::setAmZeroForceSamples(double iamZeroForceSamples)
  {
    amZeroForceSamples = iamZeroForceSamples;
  }

  /**
   * @brief Set forceThreshold from GUI.
   * @param iforceThreshold Choosen force threshold from GUI.
   */
  void AutoStretch::setForceThreshold(double iforceThreshold)
  {
    forceThreshold = iforceThreshold;
  }

  /**
   * @brief Set changeForceLowThreshold from GUI.
   * @param ichangeForceLowThreshold Choosen force threshold low from GUI.
   */
  void AutoStretch::setChangeForceLowThreshold(double ichangeForceLowThreshold)
  {
    changeForceLowThreshold = ichangeForceLowThreshold;
  }

  /**
   * @brief Set changeForceHighThreshold from GUI.
   * @param ichangeForceHighThreshold Choosen force threshold high from GUI.
   */
  void AutoStretch::setChangeForceHighThreshold(double ichangeForceHighThreshold)
  {
    changeForceHighThreshold = ichangeForceHighThreshold;
  }

  /** Start the auto stretch process.
   * \param[in] unit 0 Force / 1 Expension
   */
  void AutoStretch::start(int unit)
  {
    process(Event::evStart);
  }

  /**
   * @brief Turns the AutoStretch FSM to the state stopState (Stop motor).
   */
  void AutoStretch::stop()
  {
    process(Event::evStop);
  }

  /**
  * @brief Updates stretchUntilValue from GUI.
  * @param value Choosen value from GUI.
  */
  void AutoStretch::updateAutoStretchValue(double value)
  {
    stretchUntilValue = value;
    process(Event::evUpdate);
  }

  /**
   * @brief Updates currentMode from GUI.
   * @param mode Choosen mode from GUI.
   */
  void AutoStretch::updateAutoStretchMode(int mode)
  {
    currentMode = mode;
  }

  /**
   * @brief Updates currentUnit from GUI.
   * @param unit Choosen unit from GUI.
   */
  void AutoStretch::updateAutoStretchUnit(int unit)
  {
    //if(currentState == State::stopState) {
    std::cout << "AutoStretch unit changed." << std::endl;
    currentUnit = unit;

    if(currentUnit == Unit::Force) {
      emit setStretchUntilValueSuffix(" N");
    } else if(currentUnit == Unit::Expansion) {
      emit setStretchUntilValueSuffix(" %");
    }
    //}
  }

  /**
   * @brief Set current amount of collected samples to amStdForceSamples.
   */
  void AutoStretch::setStdForceSamples()
  {
    amForceSamples = amStdForceSamples;
  }

  /**
   * @brief Set current amount of collected samples to amZeroForceSamples.
   */
  void AutoStretch::setZeroForceSamples()
  {
    amForceSamples = amZeroForceSamples;
    speedIsMin = true;
  }

  /**
   * @brief Set the flag speedIsMax
   */
  void AutoStretch::setSpeedIsMax()
  {
    speedIsMax = true;
  }

  /**
   * @brief Set current distance as clamping distance.
   */
  void AutoStretch::setClampingDistance()
  {
    mClampingDistance = mLinearMotor->getCurrentDistance();
    std::cout << "AutoStretch setClamping mClampingDistance: " << mClampingDistance * 0.000047625 << std::endl;
  }

  /**
   * @brief Performs the Preconditioning.
   * @param speed Speed defined in the stretcher GUI.
   * @param preload Preload defined in the stretcher GUI.
   * @param cycles Cycles defined in the stretcher GUI.
   * @param expansionPerCent Expansion in % defined in the stretcher GUI.
   */
  void AutoStretch::makePreConditioning(double speed, double preload, int cycles, double expansionPerCent)
  {
    gotoPreLoad(preload);

    for(int i = 0; i < cycles; ++i) {
      // Calculate expension value
      double expansionLength = mPreCondPreloadDistance * (static_cast<double>(expansionPerCent)/100);
      std::cout << " mPreCondZeroDistance: " << mPreCondPreloadDistance * MM_PER_MS << " Expansion: " <<  /*currentPosMotor[0]*/ (expansionLength/2) << ", " << /*currentPosMotor[1]*/ (expansionLength/2) << std::endl;
      // motor speed = speed [% preload[mm] / s] * preload[mm] * mm per microstep
      mLinearMotor->setSpeed(speed * static_cast<double>(mPreCondPreloadDistance * MM_PER_MS));
      mLinearMotor->moveSteps(-(expansionLength/2));

      // Wait until motors reached new positions.
      mSyncMutex.lock();
      mPreCondMotorWaitCond.wait(&mSyncMutex);
      mSyncMutex.unlock();
      // Go back to preload position
      mLinearMotor->moveSteps(expansionLength/2);

      // Wait until motors reached new positions.
      mSyncMutex.lock();
      mPreCondMotorWaitCond.wait(&mSyncMutex);
      mSyncMutex.unlock();
    }
    mLinearMotor->gotoMSDistance(mClampingDistance);
    std::cout << "AutoStretch mClampingDistance: " << mClampingDistance * MM_PER_MS << std::endl;
  }

  /**
   * @brief Performs the Preloading
   * @param preload Preload defined in the stretcher GUI.
   */
  void AutoStretch::gotoPreLoad(double preload)
  {
    // start condition for FSM
    currentState = State::stopState;
    updateAutoStretchMode(Mode::Until);
    updateAutoStretchUnit(Unit::Force);
    updateAutoStretchValue(preload);

    // start FSM
    mSyncMutex.lock();
	mLinearMotor->setSpeed(2.0);
    process(Event::evStart);

    //std::cout << "AutoStretch pos 1: " << currentPosMotor1 << ", pos 2: " << currentPosMotor2 << std::endl;
    //std::cout << "    Distance: " << static_cast<double>(distance * 0.000047625)  << std::endl;

    // Wait until preload is reached
    mPreCondMotorWaitCond.wait(&mSyncMutex);
    mSyncMutex.unlock();
    std::cout << "AutoStretch: Not longer in runState." << std::endl;

    if(false == mRampToFailureStopFlag) {
      mPreCondPreloadDistance = mLinearMotor->getCurrentDistance();
      std::cout << "AutoStretch: distance: " << mPreCondPreloadDistance * MM_PER_MS << std::endl;
    }
  }

  /**
   * @brief Performs the "ramp to failure" measurement after reaching the preload.
   * @param speed Speed for the "ramp to failure" measurement in [% preload[mm]/s].
   * @param preload Preload force.
   */
  void AutoStretch::measurementRampToFailure(double speed, double preload, double ipercentFailureForce)
  {
    mRampToFailureStopFlag = false;
    maxForce = 0;
    percentFailureForce = ipercentFailureForce;
    gotoPreLoad(preload);

    // Only go further if measurement was not stopped by stopRampToFailure()
    if(false == mRampToFailureStopFlag) {
      mRampToFailureFlag = true;
      // motor speed = speed [% preload[mm] / s] * preload[mm] * mm per microstep
      mLinearMotor->moveSpeed(speed * static_cast<double>(mPreCondPreloadDistance * MM_PER_MS));

      // Wait until failure reached indicated by updateAutoStretchForce()
      mRampToFailureMutex.lock();
      mRampToFailureWaitCond.wait(&mRampToFailureMutex);
      mRampToFailureMutex.unlock();
    }

    // Only go further if measurement was not stopped by stopRampToFailure()
    if(false == mRampToFailureStopFlag) {
      // Wait until motors stopped and returned their position.
      mLinearMotor->mCurrentPosMutex.lock();
      mLinearMotor->mCurrentPosCond.wait(&mLinearMotor->mCurrentPosMutex);
      mLinearMotor->mCurrentPosMutex.unlock();

      // Go to clamping position
      mLinearMotor->gotoMSDistance(mClampingDistance);
      std::cout << "AutoStretch mClampingDistance: " << mClampingDistance * MM_PER_MS << std::endl;
    }
  }

  /**
   * @brief Stops the "ramp to failure" measurement and the preloading. Used when material is broken and can't reach
   *        preload or 60% of max. force.
   */
  void AutoStretch::stopRampToFailure()
  {
    //std::cout << "AutoStretch stop ramp to failure." << std::endl;
    // Stop if measurement is preloading
    if(State::runState == currentState) {
      process(Event::evStop);
      mRampToFailureStopFlag = true;
      mPreCondMotorWaitCond.wakeOne();
    // Stop if it is in measurement
    } else {
      mRampToFailureStopFlag = true;
      mLinearMotor->stop();
      mRampToFailureWaitCond.wakeOne();
    }
  }

  /**
   * @brief Updates the current force meassured by the sensor. If the motor isn't running or if the motor direction changed
   *        >=3 timer (flagCareForce == true) the speed of the motors will be adapted, depending on the change of the force.
   * @param force Force meassured by the sensor.
   */
  void AutoStretch::updateAutoStretchForce(double force)
  {
    if(currentUnit == Unit::Force) {
      if(amForceUpdate >= amForceSamples) { // Enough samples collected.
        amForceUpdate = 0;
        thisForce += force;
        thisForce /= amForceSamples;        // Build average.

        if((currentState == State::runState) && (currentMode == Mode::Hold) && flagCareForce) {
          //std::cout << "AutoStretch Force update. flagCareForce: " << flagCareForce << std::endl;
          if((!speedIsMin) && (std::abs(thisForce - lastForce) <= changeForceLowThreshold)) {       // Decrease speed, if the force change is lower then changeForceLow Threshold
            std::cout << "AutoStretch Force diff <= with diff: " << std::abs(thisForce - lastForce) << " thisForce: " << thisForce << " lastForce: " << lastForce << std::endl;
            speedIsMax = false;
            mLinearMotor->decreaseSpeed();
          }
          if((!speedIsMax) && (std::abs(thisForce - lastForce) >= changeForceHighThreshold)) {      // Increase spped, if the force change is higher then changeForceHighThreshold
            std::cout << "AutoStretch Force diff >= with diff: " << std::abs(thisForce - lastForce) << " thisForce: " << thisForce << " lastForce: " << lastForce << std::endl;
            speedIsMin = false;
            mLinearMotor->increaseSpeed();
          }
        }
      }

      lastForce = thisForce;
      thisForce = 0;
      //lastForce = force;
    } else {                              // Not enough samples collected, keep collecting.
      amForceUpdate++;
      thisForce += force;
    }

    if(true == mRampToFailureFlag) {
      //std::cout << "AutoStretch RampToFailure debugg." << std::endl;
      //std::cout << " thisForce: " << force << ", maxForce: " << maxForce << std::endl;
      if(std::abs(force) > std::abs(maxForce)) {
        //std::cout << "AutoStretch new maxForce." << std::endl;
        maxForce = force;
      }

      if(std::abs(force) < percentFailureForce/100 * std::abs(maxForce)) {
        //std::cout << "AutoStretch force < " << percentFailureForce/100 << "*maxForce." << std::endl;
        mRampToFailureFlag = false;
        mLinearMotor->stop();
        mRampToFailureWaitCond.wakeOne();
      }
    }

    currentForce = force;

    //if(timeDivider >= 20) {
    // timeDivider = 0;
    emit updateMeasuredPoints(force, mLinearMotor->getDistance());
    //} else {
    // timeDivider++;
    //}

    if(currentUnit == Unit::Force) {
      process(Event::evUpdate);
    }
  }

  /**
   * @brief Changes the state of the FSM based on the event e.
   * @param e The event
   */
  void AutoStretch::process(const Event e)
  {
    switch(currentState) {

      case stopState:
        if(evStart == e) {
          std::cout << "AutoStretch switch to state: runState." << std::endl;
          currentState = runState;

          if(currentUnit == Unit::Force) {
            std::cout << "Motors will run until defined force is reached" << std::endl;

            if((currentForce - stretchUntilValue) > forceThreshold) {   // Current force is higher than stretchUntilValue
              lastDirection = Direction::Forwards;
              mLinearMotor->moveSpeed();
            } else if((stretchUntilValue - currentForce) > forceThreshold) { // Current force is lower than stretchUntilValue
              lastDirection = Direction::Backwards;
              mLinearMotor->reverseMoveSpeed();
            }

          } else if(currentUnit == Unit::Expansion) {
            std::cout << "Motors will run until defined expansion reached" << std::endl;

            int length = mLinearMotor->getDistance();
            // Calculate expension value
            double expension = (length * (1 + stretchUntilValue/100)) - length;
            std::cout << " New positions: " <<  currentPosMotor[0] - (expension/2) << ", " << currentPosMotor[1] - (expension/2) << std::endl;
            mLinearMotor->moveSteps(-(expension/2));
            currentState = stopState;
          }

          //mLinearMotor->getPosition();
        }
        break;

      case runState:
        if(evStop == e) {
          std::cout << "AutoStretch switch to state: stopState." << std::endl;
          currentState = stopState;

          if(currentUnit == Unit::Force) {
            std::cout << "AutoStretch motor stops." << std::endl;
            mLinearMotor->stop();
          }
          break;
        }

        if(evUpdate == e) {
          //std::cout << "AutoStretch stays in state: runState." << std::endl;

          if(currentUnit == Unit::Force) {
            if((currentForce - stretchUntilValue) > forceThreshold) {   // Current force is higher than stretchUntilValue
              std::cout << "AutoStretch currentForce - stretchUntilValue: " << currentForce - stretchUntilValue << std::endl;
              if((lastDirection == Direction::Backwards) || (lastDirection == Direction::Stop)) {     // Only start motor, if state changed
                amChanges++;
                flagCareForce = false;
                lastDirection = Direction::Forwards;
                mLinearMotor->moveSpeed();
                //std::cout << "AutoStretch: continue/startMotor();" << std::endl;
              }
              if(amChanges > ChangeSpeedThreshold) {    // Activate flagCareForce after ChangeSpeedThreshold direction changes
                amChanges = 0;
                flagCareForce = true;
                /*mLinearMotor->decreaseSpeed();
                std::cout << "AutoStretch currentForce > decrease speed" << std::endl;
                */
              }
            } else if((stretchUntilValue - currentForce) > forceThreshold) { // Current force is lower than stretchUntilValue
              std::cout << "AutoStretch stretchUntilValue - currentForce: " << stretchUntilValue - currentForce << std::endl;
              if((lastDirection == Direction::Forwards) || (lastDirection == Direction::Stop)) {      // Only reverse motor, if state changed
                amChanges++;
                flagCareForce = false;
                lastDirection = Direction::Backwards;
                mLinearMotor->reverseMoveSpeed();
                //std::cout << "AutoStretch: reverseMotor();" << std::endl;
              }
              if(amChanges > ChangeSpeedThreshold) {   // Activate flagCareForce after ChangeSpeedThreshold direction changes

                amChanges = 0;
                flagCareForce = true;
                /*mLinearMotor->decreaseSpeed();
                std::cout << "AutoStretch currentForce < decrease speed" << std::endl;
                */
              }
            } else {
              if((lastDirection == Direction::Forwards) || (lastDirection == Direction::Backwards)) { // Only reverse motor, if state changed
                amChanges++;
                flagCareForce = true;                  // Activate flagCareForce
                lastDirection = Direction::Stop;
                mLinearMotor->stop();
                //std::cout << "AutoStretch: stopMotor();" << std::endl;
              }
              if(amChanges > ChangeSpeedThreshold) {
                amChanges = 0;
                /*mLinearMotor->increaseSpeed();
                std::cout << "AutoStretch currentForce = increase speed" << std::endl;
                */
              }
              //std::cout << "AutoStretch before Mode check mode: " << currentMode << std::endl;
              if(currentMode == Mode::Until) {
                std::cout << "AutoStretch switch to state: stopState." << std::endl;
                currentState = State::stopState;
                mPreCondMotorWaitCond.wakeOne();
              }
              //getLength();
            }
          }
          break;
        }
        break;
    }
  }
}
