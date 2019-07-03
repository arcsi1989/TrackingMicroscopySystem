/*
 Copyright (c) 2013, Andreas Ziegler

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef AUTOSTRETCH_H
#define AUTOSTRETCH_H

// included dependencies
#include <QObject>
#include <QVector>
#include <QMutex>
#include <QWaitCondition>

namespace tracker {

  // forward declared dependencies
  class MainWindow;
  class LinearMotor;

  /**
   * \class AutoStretch AutoStretch.h
   * \brief Provides the stretching funcionality.
   */
  class AutoStretch : public QObject {

      Q_OBJECT
    public:
      /**
       * @enum Event
       * @brief Defines the events which can occur for the AutoStretch FSM.
       */
      enum Event{evStart,         /**< AutoStretch should start */
                 evUpdate,        /**< New measured value */
                 evStop};         /**< AutoStretch should stop */

      enum Mode{Until,
                Hold};

      /**
       * @enum Unit
       * @brief Defines the two possible units.
       */
      enum Unit{Force = 0,
                Expansion = 1};

      QWaitCondition mPreCondMotorWaitCond;    /**< Wait condition variable to inform AutoStretch, that the motors finished moving. */

      /** The constructor
       * @brief Initialization of all the variables, and setting force as the default unit.
       * @param[in] *iMainWindows Pointer to the MainWindow object.
       */
      AutoStretch(MainWindow* iMainWindow);

      /** Start the auto stretch process.
       * \param[in] unit 0 Force / 1 Expension
       */
      void start(int unit);

      /**
       * @brief Updates the current force meassured by the sensor.
       * @param force Force meassured by the sensor.
       */
      void updateAutoStretchForce(double force);

      /**
       * @brief Set current amount of collected samples to amZeroForceSamples.
       */
      void setZeroForceSamples();

      /**
       * @brief Set current amount of collected samples to amStdForceSamples.
       */
      void setStdForceSamples();

      /**
       * @brief Changes the state of the FSM based on the event e.
       * @param e The event
       */
      void process(const Event e);

      /**
       * @brief Let LinearMotor connect with AutoStretch, that *mLinearMotor can be defined.
       * @param *iLinearMotor Pointer to the LinearMotor object.
       */
      void connect(LinearMotor *iLinearMotor);

      /**
       * @brief Performs the Preloading
       * @param preload Preload defined in the stretcher GUI.
       */
      void gotoPreLoad(double preload);

    public slots:
      /**
       * @brief Turns the AutoStretch FSM to the state stopState (Stop motor).
       */
      void stop();

      /**
       * @brief Updates currentMode from GUI.
       * @param mode Choosen mode from GUI.
       */
      void updateAutoStretchMode(int mode);

      /**
       * @brief Updates currentUnit from GUI.
       * @param unit Choosen unit from GUI.
       */
      void updateAutoStretchUnit(int unit);

      /**
       * @brief Updates stretchUntilValue from GUI.
       * @param value Choosen value from GUI.
       */
      void updateAutoStretchValue(double value);

      /**
       * @brief Set amStdForceSamples from GUI.
       * @param iamForceSamples Choosen standard amount of samples from GUI.
       */
      void setAmStdForceSamples(double iamForceSamples);

      /**
       * @brief Set amZeroForceSamples from GUI.
       * @param iamZeroForceSamples Choosen amount of samples when speed is 0 from GUI.
       */
      void setAmZeroForceSamples(double iamZeroForceSamples);

      /**
       * @brief Set forceThreshold from GUI.
       * @param iforceThreshold Choosen force threshold from GUI.
       */
      void setForceThreshold(double iforceThreshold);

      /**
       * @brief Set changeForceLowThreshold from GUI.
       * @param ichangeForceLowThreshold Choosen force threshold low from GUI.
       */
      void setChangeForceLowThreshold(double ichangeForceLowThreshold);

      /**
       * @brief Set changeForceHighThreshold from GUI.
       * @param ichangeForceHighThreshold Choosen force threshold high from GUI.
       */
      void setChangeForceHighThreshold(double ichangeForceHighThreshold);

      /**
       * @brief Set the flag speedIsMax
       */
      void setSpeedIsMax();

      /**
       * @brief Set current distance as clamping distance.
       */
      void setClampingDistance();

      /**
       * @brief Performs the Preconditioning.
       * @param speed Speed defined in the stretcher GUI.
       * @param preload Preload defined in the stretcher GUI.
       * @param cycles Cycles defined in the stretcher GUI.
       * @param expansionPerCent Expansion in % defined in the stretcher GUI.
       */
      void makePreConditioning(double speed, double preload, int cycles, double expansionPerCent);

      /**
       * @brief Performs the "ramp to failure" measurement after reaching the preload.
       * @param speed Speed for the "ramp to failure" measurement in [% preload[mm]/s].
       * @param preload Preload force.
       */
      void measurementRampToFailure(double speed, double preload, double ipercentFailureForce);

      /**
       * @brief Stops the "ramp to failure" measurement and the preloading. Used when material is broken and can't reach
       *        preload or 60% of max. force.
       */
      void stopRampToFailure();

    signals:
      /**
       * @brief Signal to inform MainWindow about new measured points.
       * @param mForce Force value of the measured point
       * @param mLength Lenght value of the measured point (Not normalized)
       */
      void updateMeasuredPoints(double mForce, double mLength);

      /**
       * @brief Set the current suffix in the stretchUntilValue QDoubleSpinBox
       * @param suffix Current suffix " N" or " %"
       */
      void setStretchUntilValueSuffix(const QString & suffix);

    private:
      /**
       * @enum State
       * @brief Defines the states of the AutoStretch FSM.
       */
      enum State{stopState,       /**< Stop state */
                 runState};       /**< Run state */


      /**
       * @enum Direction
       * @brief Defines Forwards, Backwards and Stop
       */
      enum Direction{Forwards,
                     Backwards,
                     Stop};

      /**
       * @enum ChangeSpeedThreshold
       * @brief Defines the amount of direction changes which are needed to activate flag flagCareForce.
       */
      enum {ChangeSpeedThreshold = 3};

      const double MM_PER_MS;             /**< milimeter per microstep */

      int amForceUpdate;                  /**< Amount of already collected samples */
      int amStdForceSamples;              /**< Amount of samples to collect in standard case */
      int amZeroForceSamples;             /**< Amount of samples to collect when speed is 0 */
      int amForceSamples;                 /**< Current amount of samples to collect */

      double forceThreshold;              /**< Threshold between < and > */
      double changeForceLowThreshold;     /**< When force is under this amount, the speed will be decreased */
      double changeForceHighThreshold;    /**< When force is upper this amount, the speed will be increased */

      State currentState;                 /**< The current state */
      int currentMode;                    /**< The current mode */

      int currentUnit;                    /**< The current unit */
      double currentForce;                /**< The current force */

      double stretchUntilValue;           /**< The value until the material should be stretched */
      int lastDirection;                  /**< The last direction */
      double lastForce;                   /**< The average over the last amForceSamples force samples */
      double thisForce;
      double maxForce;                    /**< Maximal force during ramp to failure measurement */
      double percentFailureForce;         /**< Percent of maximal force at point of failure */
      int amChanges;                      /**< Amount of changes between move, reverse move and stop */
      bool flagCareForce;                 /**< true: Depending on the force change, the speed can be changed / false: Speed won't change */
      bool speedIsMin;                    /**< Indicates, that the motors run with minimal speed */
      bool speedIsMax;                    /**< Indicates, that the motors run with maximal speed */
      unsigned int mClampingDistance;     /**< Clamping distance */
      int mPreCondPreloadDistance;        /**< Zero distance for preconditioning */
      bool mRampToFailureFlag;            /**< Ramp to failure measurement is running */
      bool mRampToFailureStopFlag;        /**< Ramp to failure measurement will be stopped */

      QMutex mRampToFailureMutex;         /**< Required for the ramp to failure wait condition */
      QWaitCondition mRampToFailureWaitCond; /**< Wait condition for thr ramp to failure measurement */


      unsigned int currentPosMotor[2];    /**< Current position of motor 1 */
      QMutex mSyncMutex;                  /**< Mutex to synchroniye/wait during preconditioning and got to preload */

      MainWindow *mMainWindow;            /**< Pointer to the MainWindow object */
      LinearMotor *mLinearMotor;          /**< Pointer to the LinearMotor object */
  };
}

#endif // AUTOSTRETCH_H
