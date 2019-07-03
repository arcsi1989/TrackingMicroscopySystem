/*
 Copyright (c) 2009-2013, Reto Grieder & Benjamin Beyeler

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef _PressureSensor_H__
#define _PressureSensor_H__

#include "TrackerPrereqs.h"

#include <string>
#include <QObject>
#include "SerialInterface.h"

namespace tracker
{
  /** Singleton class that provides access to the hardware module with the force sensor.

      The pressure sensor works like a console and therefore expects commands.
      We only send commands that delivers the pressure. \n\n

      Also, the singleton updates itself as long as the event loop is running.
      Qt timers are used for that purpose.
  @par Settings
       - \c Update_Cycle_[ms]: Time between two pressure readouts (default: 100)
       - \c Port: COM port used (default: "com1")
       - \c Baudrate: COM Baudrate used for the connection (default: 115200)
  */
  class PressureSensor : public QObject, public SerialInterface
  {
    Q_OBJECT;

  public:
    //! Port defaults to com1 and baud rate to 115200
    PressureSensor(QObject* parent = 0);
    ~PressureSensor();

    //! Returns the current measured pressure
    float getPressure() const {
      return this->mPressure;
    }

    /**
     * @brief Calculate scaling factor depending on the nominal value, the measurement end value, the nominal force and the input sensitivity.
     */
    void calculateScaleFactor();

  public slots:
    /**
     * @brief Set the com port for the force sensor
     * @param iPort Com port on which the force sensor is attached
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
     * @brief Updated scale factor parameters with the values given from the GUI.
     * @param iNominalValue Nominal value
     * @param iMeasureEndValue Measure end value
     * @param iNominalForce Nominal force
     * @param iInputSensitivity Input sensitivity
     */
    void setScaleFactor(double iNominalValue, double iMeasureEndValue, double iNominalForce, double iInputSensitivity);

  signals:
    //! Called whenever the pressure has been read out
    void pressureUpdated(double);

  private:
    PressureSensor(const PressureSensor& rhs); // don't use

    //! Timer callback.
    void timerEvent(QTimerEvent* event);

    /// Internal function called in the constructor to read settings values.
    void readSettings();

    /// Internal function called in the destructor to write settings values.
    void writeSettings() const;

    float mPressure;                        //!< Buffered pressure
    bool mbRequestSent;                     //!< Whether request to send the pressure has been sent
    unsigned short mReadAttempts;           //!< Number of attempts to read the pressure after a request has been sent
    std::string mReadBufferRemainder;       //!< What remains from the last readout (sort of a buffer)
    std::string mWriteBuffer;               //!< Write buffer (to avoid breaking a command)
    int mUpdateCycle;                       //!< Readout update time in milliseconds
    double mNominalForce;                   /**< Nominal force to calculate the scaling factor */
    double mNominalValue;                   /**< Nominal value to calculate the scaling factor */
    double mInputSensitivity;               /**< Input sensitivity to calculate the scaling factor */
    double mMeasureEndValue;                /**< Measure end value to calculate the scaling factor */
    double mZeroValue;                      /**< Zero value */
    double mScalingFactor;                  /**< Sensor scaling factor */
    int mPressureSensorTimerID;             /**< ID of the used timer */
  };
}

#endif /* _PressureSensor_H__ */
