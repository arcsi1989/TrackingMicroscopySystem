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
    The main function of the Tracker (but not the entry point of the program!)
*/

#include "TrackerConfig.h"

#include <memory>
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QMessageBox>

#include "Logger.h"
#include "Exception.h"
#include "MainWindow.h"
#include "PathConfig.h"

#ifdef TRACKER_PLATFORM_WINDOWS
#include <windows.h>
#endif

using namespace tracker;

/** Program entry point.
    If you compile the program as console application, this function will get
    called directly. However, compile as windows application will call WinMain()
    instead. But that symbol is found in Qt Main, which we link against. That in
    turn then calls our main function after translating the arguments.
*/
int main(int argc, char** argv)
{
    // Using auto_ptr here to make sure everything gets destroyed correctly in
    // case of failure (exception thrown).
    auto_ptr<Logger> logger(new Logger());
    auto_ptr<PathConfig> pathConfig;
    try
    {
        // Setup required paths and the logger
        pathConfig.reset(new PathConfig());
        logger->openLogFile(PathConfig::getLogPath().path() + "/tracker.log");
    }
    catch (const tracker::Exception& ex)
    {
        TRACKER_LOG_GENERIC(tracker::Logger::Exception, ex.getDescription(), 0, Logger::High);
#ifdef TRACKER_PLATFORM_WINDOWS
        MessageBox(NULL, ex.getDescription().toStdString().c_str(), "Exception", MB_ICONERROR);
#endif
        return 1;
    }

    // Set some basic information
    QCoreApplication::setApplicationName("Tracker");
    QString versionString;
    versionString += QString::number(TRACKER_VERSION_MAJOR);
    versionString += QString::number(TRACKER_VERSION_MINOR);
    versionString += QString::number(TRACKER_VERSION_PATCH);
    QCoreApplication::setApplicationVersion(versionString);

    // Define Qt library path (used for the plugins)
    QStringList libraryPaths = QCoreApplication::libraryPaths();
    libraryPaths.prepend(PathConfig::getExecutablePath().path() + "/plugins");
    QCoreApplication::setLibraryPaths(libraryPaths);

    // Load Qt
    QApplication app(argc, argv);

    try
    {
        // Resources in a static library have to initialized manually
        Q_INIT_RESOURCE(tachometer_svgdialgauge);

        // Load main supervising class of the program
        MainWindow window;
        window.show();

        // Program happens in this event loop (returns on close)
        return app.exec();
    }
    catch (const tracker::Exception& ex)
    {
        TRACKER_LOG_GENERIC(tracker::Logger::Exception, ex.getDescription(), 0, Logger::High);
        return 1;
    }
}
