/*
 Copyright (c) 2009-2012, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

/**
@file
@brief
    Declaration of facilities to handle exceptions.
@details
    Throwing exception is also very simple:
    @code
        TRACKER_EXCEPTION("Something went wrong");
    @endcode
    The exception will automatically contain information about file, line number
    and function name it occurred in. \n\n
*/

#ifndef _Exception_H__
#define _Exception_H__

#include "TrackerPrereqs.h"

#include <exception>
#include <sstream>
#include <QString>
#include "Logger.h"

namespace tracker
{
    /** Base class for all exceptions (derived from std::exception).
    @details
        This class provides support for information about the file, the line
        and the function the error occurred.
    @see Exception.h
    */
    class Exception : public std::exception
    {
    public:
        /**
        @brief
            Creates the exception but doesn't yet compose the full descrption (because of the virtual functions)
        @param description
            Exception description as string. This message is supposed to help developers!
        @param lineNumber
            The number of the code-line in which the exception occurred
        @param filename
            The file in which the exception occurred
        @param functionName
            The function in which the exception occurred
        */
        Exception(const QString& description, unsigned int lineNumber,
                  const char* filename, const char* functionName);
        //! Simplified constructor with just a description. If you need more, use the other one.
        Exception(const QString& description);

        //! Needed for compatibility with std::exception
        virtual ~Exception() throw() { }
        //! Returns the error description
        const char* what() const throw();

        //! Returns the short developer written exception
        virtual QString      getDescription()     const { return this->description_; }
        //! Returns the line number on which the exception occurred.
        virtual unsigned int getLineNumber()      const { return this->lineNumber_; }
        //! Returns the function in which the exception occurred.
        virtual QString      getFunctionName()    const { return this->functionName_; }
        //! Returns the filename in which the exception occurred.
        virtual QString      getFilename()        const { return this->filename_; }
        //! Returns a full description with type, line, file and function
        virtual QString      getFullDescription() const;

        /**
        @brief
            Retrieves information from an exception caught with "..."
            Works for std::exception and CEGUI::Exception
        @remarks
            Never ever call this function without an exception in the stack!
        */
        static QString handleMessage();

    protected:
        void compileFullDescription() const;

        const QString description_;          //!< User typed text about why the exception occurred
        const unsigned int lineNumber_;      //!< Line on which the exception occurred
        const QString functionName_;         //!< Function (including namespace and class) where the exception occurred
        const QString filename_;             //!< File where the exception occurred
        mutable QString fullDescription_;    //!< Full description with line, file and function
        mutable QByteArray whatReturnValue_; //!< Return value of what() so that const char* is valid
    };

    /** Helper function that forwards an exception and displays the message.
    @details
        This is necessary because only when using 'throw' the objects storage is managed.
    */
    //inline const T& exceptionThrowerHelper(const T& exception, Logger::LogLevel level)
    inline Exception makeException(const QString& description, unsigned int line,
        const char* filename, const char* functionName, Logger::LogLevel level)
    {
        // The normal log level is Normal which means you can only see it in the log.
        // Thereforce the catcher can decide what to do with the exception.
        Logger::getInstance().logMessage(description, Logger::Exception, level, line, filename, functionName);
        return Exception(description, line, filename, functionName);
    }

/** Throws an exception and logs a message beforehand.
@param description
    Exception description as string
@see Exception.h
@param ...
    Logger::LogLevel (High, Normal or Low), but the argument is optional
*/
#define TRACKER_EXCEPTION(description, ...) \
    throw tracker::makeException(description, __LINE__, __FILE__, __FUNCTION__, Logger::_getLogLevelHelper(0, __VA_ARGS__))

} /* namespace tracker */

#endif /* _Exception_H__ */
