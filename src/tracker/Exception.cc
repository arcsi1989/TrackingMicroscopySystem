/*
 Copyright (c) 2009-2011, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

/**
@file
@brief
    Implementation of the Exception class.
*/

#include "Exception.h"

#include <QTextStream>

namespace tracker
{
    Exception::Exception(const QString& description, unsigned int lineNumber,
                         const char* filename, const char* functionName)
        : description_(description)
        , lineNumber_(lineNumber)
        , functionName_(functionName)
        , filename_(filename)
    { }

    Exception::Exception(const QString& description)
        : description_(description)
        , lineNumber_(0)
    { }

    void Exception::compileFullDescription() const
    {
        QTextStream fullDesc(&fullDescription_);

        fullDesc << "Exception";

        if (!this->filename_.isEmpty())
        {
            fullDesc << " in " << this->filename_;
            if (this->lineNumber_)
                fullDesc << '(' << this->lineNumber_ << ')';
        }

        if (!this->functionName_.isEmpty())
            fullDesc << " in function '" << this->functionName_ << '\'';

        fullDesc << ": ";
        if (!this->description_.isEmpty())
            fullDesc << this->description_;
        else
            fullDesc << "No description available.";

        whatReturnValue_ = fullDescription_.toAscii();
    }

    QString Exception::getFullDescription() const
    {
        if (fullDescription_.isEmpty())
            this->compileFullDescription();
        return fullDescription_;
    }

    const char* Exception::what() const throw()
    {
        if (fullDescription_.isEmpty())
            this->compileFullDescription();
        return whatReturnValue_;
    }

    /*static*/ QString Exception::handleMessage()
    {
        try
        {
            // rethrow
            throw;
        }
        catch (const std::exception& ex)
        {
            return ex.what();
        }
        catch (...)
        {
            TRACKER_WARNING("BIG: Unknown exception type encountered. Rethrowing");
            throw;
        }
    }
}
