/*
 Copyright (c) 2009-2011, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef _SerialInterface_H__
#define _SerialInterface_H__

#include "TrackerPrereqs.h"

#include <string>

namespace tracker
{
    /** Abstract base class for serial communication.
    @details
        We use wxCTB as serial port library to ease up the handling.
    */
    class SerialInterface
    {
    public:
        //! Initialises the values and creates the wxSerialPort
        SerialInterface(std::string iPort = "com0", unsigned int iBaudrate = 9600);
        //! Closes the port if still open
        virtual ~SerialInterface();

        //! Opens the connection with the specified baud rate and port
        virtual void openConnection();

        /**
         * @brief Close the existing connection.
         */
        virtual void closeConnection();

    protected:
        wxSerialPort* mSerialPort;  //!< We use a pointer to avoid the include file in this header
        std::string   mPort;        //!< Port as string in the form "com3" for instance
        unsigned int  mBaudrate;    //!< Serial port baud rate

    private:
        SerialInterface(const SerialInterface&); //!< Don't use (undefined symbol)
    };
}

#endif /* _SerialInterface_H__ */
