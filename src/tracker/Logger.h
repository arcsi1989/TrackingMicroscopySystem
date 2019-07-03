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
    Declaration of facilities to log messages.
*/

#ifndef _Logger_H__
#define _Logger_H__

#include "TrackerPrereqs.h"

#include <QFile>
#include <QObject>
#include <QString>
#include <QTextStream>
#include <QVector>

#include "Singleton.h"

namespace tracker
{
    /** The Logger is responsible for distributing messages received via TRACKER_INFO, etc.

        The possible log targets are a file, std::cout, the gui console and a message box.
        Whether a target is written to depends both on the LogLevel of the message (low, normal, high),
        the order of the list of source-target priorities and the target log level. \n
        That means each possible LogType-LogLevel combination has an assigned priority that
        can be found in the static section of the source file (sOverallLogLevelsReverse).
    @par Example:
        Say you log a message with TRACKER_WARNING("foobar", Logger::Low), then the LogLevel is 'Low'
        and the LogType is 'Warning'. In sOverallLogLevelsReverse we can see that this combination
        is at the 8th position, thus has priority 8 (not very high). And looking at the assigned
        target priorities in the constructor, we find that our message will be written to std::cout
        and the log file.
    @note
        Using any of the logging macros or Logger::logMessage() is thread safe.
    */
    class Logger : public QObject, public Singleton<Logger>
    {
        Q_OBJECT;
        friend class Singleton<Logger>;

    public:
        //! Input types of the log message
        enum LogType
        {
            Assertion = 0,
            Exception = 3,
            Warning   = 6,
            Info      = 9
        };
        //! Input log levels (specified within the macros)
        enum LogLevel
        {
            High   = 0,
            Normal = 1,
            Low    = 2,
            Auto   = 3
        };
        //! Number of all LogLevel-LogType combinations
        static const unsigned int sOverallLogLevelCount = 12;
        //! Output log targets
        enum LogTarget
        {
            File    = 0,
            Console = 1,
            GUI     = 2,
            MsgBox  = 3
        };
        //! Number of LogTargets
        static const unsigned int sDeviceCount = 4;

    public:
        /** Configure the logfile details and the target log levels here.

            Configures some hard coded settings, assigns the map with the log levels
            and starts the logfile into a buffer because the filename and path is
            not yet known.
        */
        Logger(QObject* parent = 0);
        //! Closes the logfile and deletes the stored log entries.
        ~Logger();

        /** Call this as soon as you know the full path of the logfile.

            Opens the log file and writes the buffer content to it. \n
            A second call will do nothing.
        */
        void openLogFile(const QString& filename);

        /** Writes a log message to the target devices.
        @note This function is thread safe.
        @param LogType
            Cause of the message (Info, Assert, Exception or Warning)
        @param LogLevel
            Priority of the message (Low, Normal, High)
        @param lineNumber
            The line of code the message was sent from
        @param filename
            The filename of the code the message was sent from
        @param functionName
            The function name the message was sent from.
        */
        void logMessage(const QString& message, LogType type, LogLevel level, unsigned int lineNumber,
                        const QString& filename, const QString& functionName);

        // Used with __VA_ARGS__ below in the macros
        static LogLevel _getLogLevelHelper(int)                 { return Logger::Auto; }
        static LogLevel _getLogLevelHelper(int, LogLevel level) { return level; }

    signals:
        //! Listen to this signal if you want to receive the messages logged.
        void messageLogged(QString text);

    /*private*/ signals:
        /** Simple workaround signal to make logMessage() thread safe because
            the signal/slot connection between logEntryCreated(LogEntry*) and
            writeLogEntry(LogEntry*) is asynchronous (if required).
        */
        void logEntryCreated(LogEntry*);

    private slots:
        //! Actually writes the message to the log
        void writeLogEntry(LogEntry* entry);

    private:
        Q_DISABLE_COPY(Logger);

        unsigned int           mTargetLogLevels[sDeviceCount];  //!< Output log levels for the targets
        QString                mLogFileBuffer;                  //!< File buffer to be used before the filename is fully known
        QFile                  mLogFile;                        //!< Log file for output
        QTextStream            mLogFileStream;                  //!< Output stream for the log file
        QTextStream            mConsoleStream;                  //!< Output stream for the console
        bool                   mbDetailedLogFile;               //!< Whether to output line number and filename as well
        QVector<LogEntry*>     mLogEntries;                     //!< Array of all messages ever logged

        /** Maps the input LogType-LogLevel combinations to output log levels.

            The array gets configured in the Logger constructor
        */
        unsigned int mOverallLogLevels[sOverallLogLevelCount];

        //! Static pointer to the singleton instance
        static Logger* singletonPtr_s;
    };
}

/** Generic macro for logging messages.
@note
    There is a mysterious call to std::ostringstream.flush(). The reason for that is because the << operator
    is only overloaded non member and just for references of course. But std::ostringstream() cannot be a reference.
    By calling flush() (which is a member function) we can get the reference we need.
*/
#define TRACKER_LOG_GENERIC(type, message, ...) \
    tracker::Logger::getInstance().logMessage(message, type, \
        tracker::Logger::_getLogLevelHelper(__VA_ARGS__), __LINE__, __FILE__, __FUNCTION__)

/** Logs an informational message. Use this for the non erroneous case.
@param message
    Message in a form that can be fed to a std::ostream, e.g. "message" or "number: " << num or just num.
@param ...
    Optionally you can specify the log level from Logger::High (default), Logger::Normal and Logger::Low.
*/
#define TRACKER_INFO(message, ...) \
    TRACKER_LOG_GENERIC(tracker::Logger::Info, message, 0, __VA_ARGS__)

/** Logs an error message. Use this for the erroneous case.
@param message
    Message in a form that can be fed to a std::ostream, e.g. "message" or "number: " << num or just num.
@param ...
    Optionally you can specify the log level from Logger::High, Logger::Normal (default) and Logger::Low.
*/
#define TRACKER_WARNING(message, ...) \
    TRACKER_LOG_GENERIC(tracker::Logger::Warning, message, 0, __VA_ARGS__)

#endif /* _Logger_H__ */
