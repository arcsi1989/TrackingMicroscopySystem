/*
 Copyright (c) 2009-2011, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#include "PressureSensor.h"

#include <cstring>
#include <QSettings>
#include <wxctb/serport.h>
#include <iostream>

#include "Exception.h"
#include "PathConfig.h"

namespace tracker
{
  //! Command string that persuades the sensor to send the pressure
  //  See manual

  const char PRESSURE_SENSOR_GET_STRING[] = "\x2\x41\x1\xff\x0\x41\x06";
  /*STX, ReadRaw, RX address, TX address, Number of parameters, Checksum, Weighted checksum*/

  PressureSensor::PressureSensor(QObject* parent)
    : QObject(parent),//, SerialInterface("com3", 115200)
      mScalingFactor(161380.83),
      mNominalForce(20/*N*/),
      mNominalValue(0.4965/*mv/V*/),
      mInputSensitivity(1.0/*mv/V*/),
      mMeasureEndValue(0xffffff/105*100),
      mZeroValue(0x800000),
      mPressureSensorTimerID(0)
  {
    this->readSettings();
    this->calculateScaleFactor();

    mPressure = 0.0f;
    mReadAttempts = 0;
    mbRequestSent = false;

    this->openConnection();

    // Timer event for periodic pressure value reading
    //std::cout << "PressureSensor mUpdateCycle: " << mUpdateCycle << std::endl;
    mPressureSensorTimerID = this->startTimer(mUpdateCycle);
    //this->startTimer(1000);
  }

  PressureSensor::~PressureSensor()
  {
    this->killTimer(mPressureSensorTimerID);
    this->writeSettings();
  }

  void PressureSensor::setPort(std::string iPort)
  {
    this->closeConnection();
    mPort = iPort;
    this->openConnection();
  }

  /**
   * @brief Close the com port
   */
  void PressureSensor::closePort() {
    this->closeConnection();
  }

  /**
   * @brief Opens the specified com port
   * @param iPort Com port on which the pressure sensor is attached
   */
  void PressureSensor::openPort(std::string iPort) {
    mPort = iPort;
    this->openConnection();
  }

  /**
   * @brief Updated scale factor parameters with the values given from the GUI.
   * @param iNominalValue Nominal value
   * @param iMeasureEndValue Measure end value
   * @param iNominalForce Nominal force
   * @param iInputSensitivity Input sensitivity
   */
  void PressureSensor::setScaleFactor(double iNominalValue, double iMeasureEndValue, double iNominalForce, double iInputSensitivity) {
    mNominalValue = iNominalValue;
    mMeasureEndValue = iMeasureEndValue;
    mNominalForce = iNominalForce;
    mInputSensitivity = iInputSensitivity;

    calculateScaleFactor();
  }

  /**
   * @brief Calculate scaling factor depending on the nominal value, the measurement end value, the nominal force and the input sensitivity.
   */
  void PressureSensor::calculateScaleFactor() {
    mScalingFactor = mNominalValue * mMeasureEndValue / ((mNominalForce * 2) * mInputSensitivity);
  }

  void PressureSensor::timerEvent(QTimerEvent* event)
  {
    char answerBuffer[10];
    size_t answerLen;
    if(1 == mSerialPort->IsOpen()) {
      answerLen = mSerialPort->Readv(answerBuffer, 10, 5/*ms*/);
      //std::cout << "PressureSensor answerLen: " << answerLen << std::endl;
    }
    if (answerLen > 4) {

      int measforce = 0;
      double force = 0;

      //for(int i = 0; i < (sizeof(answerBuffer) / sizeof(answerBuffer[0])); ++i) {
      for(int i = 0; i < (static_cast<int>(answerLen) - 3); ++i) {
        //std::cout << "PressureSensor answerBuffer[i]: " << static_cast<unsigned int>(answerBuffer[i]) << std::endl;
        if((answerBuffer[i] == 44 /*sync symbol*/)) { // && (answerBuffer[i + 5] == 44)) {

          //measforce = static_cast<unsigned char>(answerBuffer[2])*256*256 + static_cast<unsigned char>(answerBuffer[3])*256 + static_cast<unsigned char>(answerBuffer[4]);
          measforce = (static_cast<unsigned char>(answerBuffer[i+2]) << 16) + (static_cast<unsigned char>(answerBuffer[i+3]) << 8) + (static_cast<unsigned char>(answerBuffer[i+4]));
          //calforce =unsigned char (answerBuffer[9])*256+unsigned char(answerBuffer[10]);

          //force=99.0099/10000.0*calforce;
          //force = 100/10000.0 *calforce;
          force = (measforce - mZeroValue) / mScalingFactor;
          //std::cout << "PressureSensor force: " << force << " at pos: " << i << std::endl;
          mbRequestSent = false;

          emit pressureUpdated(force);
          //break;
        }
      }
    }
  }

  void PressureSensor::readSettings()
  {
    QSettings settings(PathConfig::getConfigFilename(), QSettings::IniFormat);
    settings.beginGroup("PressureSensor");

    mUpdateCycle = qMax(1,    settings.value("Update_Cycle_[ms]", 10).toInt());
    mPort        =            settings.value("Port", "com3").toString().toStdString();
    mBaudrate    = qMax(9600, settings.value("Baudrate", 115200).toInt());

    settings.endGroup();
  }

  void PressureSensor::writeSettings() const
  {
    QSettings settings(PathConfig::getConfigFilename(), QSettings::IniFormat);
    settings.beginGroup("PressureSensor");

    settings.setValue("Update_Cycle_[ms]", mUpdateCycle);
    settings.setValue("Port", QString::fromStdString(mPort));
    settings.setValue("Baudrate", mBaudrate);

    settings.endGroup();
  }
}
