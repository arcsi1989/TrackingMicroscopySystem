/*
 Copyright (c) 2009-2012, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#include "PathConfig.h"

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <QPair>
#include <QVector>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#undef min
#undef max

#include "Logger.h"
#include "Exception.h"

namespace tracker
{
    inline QDir operator/(const QDir& lhs, const QDir& rhs)
    {
        return QDir(lhs.path() + QDir::separator() + rhs.path());
    }

    inline QDir operator/(const QDir& lhs, const QString& rhs)
    {
        return operator/(lhs, QDir(rhs));
    }

    //! Static pointer to the singleton
    PathConfig* PathConfig::singletonPtr_s  = 0;

    PathConfig::PathConfig()
    {
        //////////////////////////
        // FIND EXECUTABLE PATH //
        //////////////////////////

        // get executable module
        TCHAR buffer[1024];
        if (GetModuleFileName(NULL, buffer, 1024) == 0)
            TRACKER_EXCEPTION("Could not retrieve executable path.");

        executablePath_ = QDir(buffer);
        executablePath_.cdUp(); // remove executable name

        if (executablePath_.exists("tracker_dev_build.keep_me"))
        {
            TRACKER_INFO("Running from the build tree.", Logger::Low);
            PathConfig::bDevRun_ = true;
        }
        else
        {
            // Also set the root path
            QDir relativeExecutablePath(pathConfig::defaultRuntimePath);
            rootPath_ = executablePath_;
            while (rootPath_ / relativeExecutablePath != executablePath_)
            {
                if (!rootPath_.cdUp())
                    TRACKER_EXCEPTION("Could not derive a root directory.\nThe executable probably doesn't sit directly in the 'bin' folder.");
            }
        }

        // Configure the rest too
        this->setConfigurablePaths();
    }

    PathConfig::~PathConfig()
    {
    }

    void PathConfig::setConfigurablePaths()
    {
        if (bDevRun_)
        {
            dataPath_   = pathConfig::dataDevDirectory;
            configPath_ = pathConfig::configDevDirectory;
            logPath_    = pathConfig::logDevDirectory;
        }
        else
        {
            // Using paths relative to the install prefix, complete them
            dataPath_   = rootPath_ / pathConfig::defaultDataPath;
            configPath_ = rootPath_ / pathConfig::defaultConfigPath;
            logPath_    = rootPath_ / pathConfig::defaultLogPath;
        }

        // Create directories to avoid problems when opening files in non existent folders.
        typedef QPair<QDir, QString> valuePair;
        QVector<valuePair> directories;
        directories.push_back(qMakePair(QDir(configPath_), QString("config")));
        directories.push_back(qMakePair(QDir(logPath_), QString("log")));

        foreach (valuePair entry, directories)
        {
            if (!entry.first.exists())
            {
                if (!entry.first.mkpath("."))
                    TRACKER_EXCEPTION("The " + entry.second + " directory could not be created.");
                TRACKER_INFO("Created " + entry.second + " directory", Logger::Low);
            }
        }
    }
}
