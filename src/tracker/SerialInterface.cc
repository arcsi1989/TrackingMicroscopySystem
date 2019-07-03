/*
 Copyright (c) 2009-2011, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#include "SerialInterface.h"

#include <wxctb/serport.h>
#include "Logger.h"

namespace tracker
{
    SerialInterface::SerialInterface(std::string iPort, unsigned int iBaudrate)
      : mPort(iPort), mBaudrate(iBaudrate)
    {
      mSerialPort = new wxSerialPort();
    }

    SerialInterface::~SerialInterface()
    {
        if (mSerialPort->IsOpen())
            mSerialPort->Close();
        delete mSerialPort;
    }

    void SerialInterface::openConnection()
    {
        wxSerialPort_DCS parameters;
        parameters.baud = (wxBaud)mBaudrate;
        mSerialPort->Open(mPort.c_str(), &parameters);
    }

    /**
     * @brief Close the existing connection.
     */
    void SerialInterface::closeConnection() {
      mSerialPort->Close();
    }
}
