/*
 Copyright (c) 2009-2011, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef _PathConfig_H__
#define _PathConfig_H__

#include "TrackerPrereqs.h"

#include <QDir>
#include <QString>
#include "Singleton.h"

namespace tracker
{
    /**
    @brief
        Singleton class that sets up the necessary paths.
    @details
        The class provides information about the data, config, log, executable
        and root path. \n
        It determines those by the use of platform specific functions.
    @remarks
        Not all paths are always available:
        - root only available when installed copyable
    */
    class PathConfig : protected Singleton<PathConfig>
    {
        friend class Singleton<PathConfig>;

        public:
            /**
            @brief
                Retrieves the executable path and sets all hard coded fixed paths (currently only the module path)
                Also checks for "tracker_dev_build.keep_me" in the executable directory.
                If found it means that this is not an installed run, hence we
                don't write the logs and config files to ~/.tracker
            @throw
                GeneralException
            */
            PathConfig();
            ~PathConfig();

            //! Returns the path to the root folder
            static const QDir& getRootPath()
                { return getInstance().rootPath_; }
            //! Returns the path to the executable folder
            static const QDir& getExecutablePath()
                { return getInstance().executablePath_; }
            //! Returns the path to the data files
            static const QDir& getDataPath()
                { return getInstance().dataPath_; }
            //! Returns the path to the config files
            static const QDir& getConfigPath()
                { return getInstance().configPath_; }
            //! Returns the path to the log files
            static const QDir& getLogPath()
                { return getInstance().logPath_; }

            //! Return true for runs in the build directory (not installed)
            static bool isDevelopmentRun() { return getInstance().bDevRun_; }

            static QString getConfigFilename()
                { return getConfigPath().path() + "/tracker.ini"; }

        private:
            PathConfig(const PathConfig&); //!< Don't use (undefined symbol)

            /**
            @brief
                Sets config, log and media path and creates the folders if necessary.
            @throws
                GeneralException
            */
            void setConfigurablePaths();

            //! Path to the parent directory of the ones above if program was installed with relative paths
            QDir               rootPath_;              //!< Path to the parent path to bin, doc, etc.
            QDir               executablePath_;        //!< Path to the executable
            QDir               dataPath_;              //!< Path to the data files folder
            QDir               configPath_;            //!< Path to the config files folder
            QDir               logPath_;               //!< Path to the log files folder

            bool               bDevRun_;               //!< True for runs in the build directory (not installed)
            static PathConfig* singletonPtr_s;         //!< Instance pointer
    }; //tolua_export
} //tolua_export

#endif /* _PathConfig_H__ */
