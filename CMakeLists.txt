#
# Copyright (c) 2009-2012, Reto Grieder
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# This software is provided 'as-is', without any express or implied warranty.
#
#  Description:
#    Configures some basics and controls the configuration scripts
#

PROJECT(Tracker C CXX)


################# Restrictions ##################

# CMake
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.3 FATAL_ERROR)

# Compiler
IF (NOT MSVC)
  MESSAGE(FATAL_ERROR "This project only supports the Microsoft compiler")
ENDIF()

# No version below 2008 has ever been tried or even tested
IF(MSVC_VERSION LESS 1500)
  MESSAGE(FATAL_ERROR "Microsoft Visual Studio versions below 9 (2008) are not supported.")
ENDIF()

# Keep devs from using the root directory as binary directory (messes up the source tree)
IF(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
  MESSAGE(FATAL_ERROR "Please do not use the root directory as CMake output directory!
  mkdir build; cd build; cmake ..
  And you will have to clean the source directory by deleting CMakeCache.txt and the folder CMakeFiles")
ENDIF()


################ General Config #################

# Version info
SET(TRACKER_VERSION_MAJOR 3)
SET(TRACKER_VERSION_MINOR 0)
SET(TRACKER_VERSION_PATCH 0)
SET(TRACKER_VERSION 3.0.0)
SET(TRACKER_VERSION_NAME "Third")

# Standard path suffixes
SET(DEFAULT_RUNTIME_PATH bin)
SET(DEFAULT_LIBRARY_PATH lib)
SET(DEFAULT_ARCHIVE_PATH lib/static)
SET(DEFAULT_DOC_PATH     doc)
SET(DEFAULT_DATA_PATH    data)
SET(DEFAULT_CONFIG_PATH  config)
SET(DEFAULT_LOG_PATH     log)

# Set output directories
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${DEFAULT_RUNTIME_PATH})
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${DEFAULT_LIBRARY_PATH})
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${DEFAULT_ARCHIVE_PATH})
SET(CMAKE_DOC_OUTPUT_DIRECTORY     ${CMAKE_BINARY_DIR}/${DEFAULT_DOC_PATH})
# Data directories are only inputs, no delclaration here
SET(CMAKE_CONFIG_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/${DEFAULT_CONFIG_PATH})
SET(CMAKE_LOG_OUTPUT_DIRECTORY     ${CMAKE_BINARY_DIR}/${DEFAULT_LOG_PATH})

# Sets where to find the external libraries like FFTW at runtime
# This only applies to development runs in the build tree
SET(RUNTIME_LIBRARY_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# This sets where to look for CMake modules (e.g. "Find*.cmake" files)
SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_SOURCE_DIR}/cmake/tools)

# Set Debug build to default when not having multi-config generator like msvc
IF(NOT CMAKE_CONFIGURATION_TYPES)
  IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Debug CACHE STRING
        "Build types are: Debug, Release, MinSizeRel, RelWithDebInfo" FORCE)
  ENDIF()
  MARK_AS_ADVANCED(CLEAR CMAKE_BUILD_TYPE)

  MESSAGE(STATUS "*** Build type is ${CMAKE_BUILD_TYPE} ***")
ELSE()
  IF(CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE CACHE STRING FORCE)
  ENDIF()
  MARK_AS_ADVANCED(CMAKE_BUILD_TYPE)
ENDIF()

########### Subfolders and Subscripts ###########

# General build and compiler options and configurations
INCLUDE(CompilerConfig)

# Configure installation paths and options
INCLUDE(InstallConfig)

# Library finding
INCLUDE(LibraryConfig)

# Configure data directory location and installation
ADD_SUBDIRECTORY(data)

# Create the actual project
ADD_SUBDIRECTORY(src)

# Configure the binary output directory. Do this after src!
ADD_SUBDIRECTORY(bin)

# Copy default config files
ADD_SUBDIRECTORY(config)

# Last but not least: Try to make a doc target with Doxygen
ADD_SUBDIRECTORY(doc)
