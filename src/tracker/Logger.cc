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
    Implementation of the Logging facilities.
*/

#include "Logger.h"

#include <cstdio>
#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextStream>
#include <QTime>

#include "PathConfig.h"

namespace tracker
{
    Logger* Logger::singletonPtr_s = NULL;

    //! Denotes the output log levels, the first one has the highest priority. Also see Logger::mOverallLogLevels
    const unsigned int gOverallLogLevelsReverse[Logger::sOverallLogLevelCount] =
    {
        // Highest priority
        Logger::Assertion + Logger::High,
        Logger::Assertion + Logger::Normal,
        Logger::Assertion + Logger::Low,
        Logger::Exception + Logger::High,
        Logger::Warning   + Logger::High,
        Logger::Warning   + Logger::Normal,
        Logger::Info      + Logger::High,
        Logger::Info      + Logger::Normal,

        Logger::Warning   + Logger::Low,
        Logger::Info      + Logger::Low,

        Logger::Exception + Logger::Normal,
        Logger::Exception + Logger::Low
        // Lowest priority
    };

    //! Represents one logged message with all its properties
    struct LogEntry
    {
        LogEntry(const QString& message, Logger::LogType type, Logger::LogLevel level, unsigned int lineNumber,
                 const QString& filename, const QString& functionName, const QTime& time)
            : mMessage(message)
            , mType(type)
            , mLevel(level)
            , mLineNumber(lineNumber)
            , mFilename(filename)
            , mFunctionName(functionName)
            , mLogTime(time)
        { }
        QString          mMessage;
        Logger::LogType  mType;
        Logger::LogLevel mLevel;
        unsigned int     mLineNumber;
        QString          mFilename;
        QString          mFunctionName;
        QTime            mLogTime;
    };


    Logger::Logger(QObject* parent)
        : QObject(parent)
        , mLogFileStream(&mLogFileBuffer)
        , mConsoleStream(stdout)
    {
        // Reverse the overall log level map
        for (unsigned int i = 0; i < sOverallLogLevelCount; ++i)
            mOverallLogLevels[gOverallLogLevelsReverse[i]] = i;

        mbDetailedLogFile = true;

        // Set target log levels
        mTargetLogLevels[Logger::File]    = mOverallLogLevels[Logger::Exception + Logger::Low];
        mTargetLogLevels[Logger::Console] = mOverallLogLevels[Logger::Exception + Logger::Low];
        mTargetLogLevels[Logger::GUI]     = mOverallLogLevels[Logger::Info      + Logger::Low];
        mTargetLogLevels[Logger::MsgBox]  = mOverallLogLevels[Logger::Info      + Logger::Normal];

        // Start the file log (to the buffer at first until file is opened)
        QDateTime now(QDateTime::currentDateTime());
        mLogFileStream     << "Starting tracker program log " << now.toString() << endl << endl;
        mLogFileStream     << "Time     ";
        if (mbDetailedLogFile)
            mLogFileStream << "Filename               Line ";
        mLogFileStream     << "Type   Message" << endl;
        if (mbDetailedLogFile)
            mLogFileStream << "----------------------------";
        mLogFileStream     << "-----------------------" << endl;

        // Connection can be both synchronous or asynchronous depending on the caller thread
        this->connect(this, SIGNAL(logEntryCreated(LogEntry*)), SLOT(writeLogEntry(LogEntry*)));
    }

    Logger::~Logger()
    {
        mLogFile.close();

        foreach (LogEntry* entry, mLogEntries)
            delete entry;
    }

    void Logger::openLogFile(const QString& filename)
    {
        if (!mLogFile.isOpen())
        {
            // Open log file
            mLogFile.setFileName(filename);
            mLogFile.open(QIODevice::WriteOnly | QIODevice::Text);
            if (!mLogFile.isOpen())
                TRACKER_WARNING("Could not open log file");
            else
            {
                mLogFileStream.setDevice(&mLogFile);
                // Flush buffer to the log file
                mLogFileStream << mLogFileBuffer;
                mLogFileStream.flush();
                mLogFileBuffer.clear();
            }
        }
    }

    void Logger::logMessage(const QString& message, LogType type, LogLevel level, unsigned int lineNumber,
                            const QString& filename, const QString& functionName)
    {
        LogEntry* entry = new LogEntry(message, type, level, lineNumber, filename,
                                       functionName, QTime::currentTime());

        // A signal/slot connection within the logger solves concurrency problems.
        // If the caller lives in the main thread, then the connection will be
        // direct, otherwise delayed.
        emit logEntryCreated(entry);
    }

    //! Does the actual log text writing
    void Logger::writeLogEntry(LogEntry* entry)
    {
        mLogEntries.push_back(entry);

        // Handle 'Auto' type argument
        if (entry->mLevel == Logger::Auto)
        {
            if (entry->mType == Logger::Info)
                entry->mLevel = Logger::High;
            else
                entry->mLevel = Logger::Normal;
        }

        // Get overall log level to compare it with the target levels afterwards
        unsigned int overallLogLevel = mOverallLogLevels[entry->mType + entry->mLevel];

        // File
        if (overallLogLevel <= mTargetLogLevels[Logger::File])
        {
            mLogFileStream << entry->mLogTime.toString() << ' ';
            if (mbDetailedLogFile)
            {
                QFileInfo file(entry->mFilename);
                mLogFileStream << file.fileName() << QString(23 - file.fileName().size(), ' ');
                mLogFileStream << qSetFieldWidth(4);
                mLogFileStream << right << entry->mLineNumber << left;
                mLogFileStream << qSetFieldWidth(0) << ' ';
            }
            switch (entry->mType)
            {
            case Logger::Assertion: mLogFileStream << "Assert "; break;
            case Logger::Exception: mLogFileStream << "Except "; break;
            case Logger::Warning:   mLogFileStream << "Warn   "; break;
            case Logger::Info:      mLogFileStream << "Info   "; break;
            }
            mLogFileStream << entry->mMessage << endl;
            mLogFileStream.flush();
        }

        // Prepare for Console && GUI
        QString text;
        QTextStream output(&text);
        switch (entry->mType)
        {
        case Logger::Assertion: output << "ASSERTION FAILED: "; break;
        case Logger::Exception: output << "EXCEPTION: ";        break;
        case Logger::Warning:   output << "WARNING: ";          break;
        default: break;
        }
        output << entry->mMessage;
        if (entry->mType == Logger::Assertion)
            output << endl << "For more information about the location open the log file.";
        output.flush();

        // Console
        if (overallLogLevel <= mTargetLogLevels[Logger::Console])
        {
            mConsoleStream << text << endl;
            mConsoleStream.flush();
        }

        // GUI, use a signal
        if (overallLogLevel <= mTargetLogLevels[Logger::GUI])
        {
            emit messageLogged(text);
        }

        // Message box
        if (overallLogLevel <= mTargetLogLevels[Logger::MsgBox] && !QCoreApplication::startingUp())
        {
            switch (entry->mType)
            {
            case Logger::Assertion:
                QMessageBox::critical(0, "Assertion Failed", entry->mMessage);
                break;
            case Logger::Exception:
                QMessageBox::critical(0, "Exception", entry->mMessage);
                break;
            case Logger::Warning:
                QMessageBox::warning(0, "Warning", entry->mMessage);
                break;
            case Logger::Info:
                QMessageBox::information(0, "Information", entry->mMessage);
                break;
            }
        }
    }
}
