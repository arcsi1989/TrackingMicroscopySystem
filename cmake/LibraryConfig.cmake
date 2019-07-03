#
# Copyright (c) 2009-2012, Reto Grieder
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# This software is provided 'as-is', without any express or implied warranty.
#
#  Description:
#    Configures the external libraries. Whenever possible, the find scripts
#    from the CMake module path are used.
#


# Prevent CMake from finding libraries in the installation folder on Windows.
# There might already be an installation from another compiler
LIST(REMOVE_ITEM CMAKE_SYSTEM_PREFIX_PATH  "${CMAKE_INSTALL_PREFIX}")
LIST(REMOVE_ITEM CMAKE_SYSTEM_LIBRARY_PATH "${CMAKE_INSTALL_PREFIX}/bin")


############ Packaged Dependencies ##############

# 64 bit system? (64 bit is not yet supported though)
IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
  SET(_cpu_type x64)
ELSE()
  SET(_cpu_type x86)
ENDIF()

# MSVC version
#STRING(REGEX REPLACE "^Visual Studio ([0-9][0-9]?) .*$" "\\1" _msvc_version "${CMAKE_GENERATOR}")
SET(_msvc_version 9)

# Generate the paths
SET(_dep_dir ${CMAKE_SOURCE_DIR}/dependencies)
SET(DEP_INCLUDE_DIR ${_dep_dir}/include                               CACHE PATH "")
SET(DEP_LIBRARY_DIR ${_dep_dir}/lib/msvc${_msvc_version}-${_cpu_type} CACHE PATH "")
SET(DEP_BINARY_DIR  ${_dep_dir}/bin/msvc${_msvc_version}-${_cpu_type} CACHE PATH "")
IF(NOT EXISTS ${DEP_LIBRARY_DIR})
  message(FATAL_ERROR "No dependencies found for your MSVC Version (${CMAKE_GENERATOR})")
ENDIF()

MARK_AS_ADVANCED(DEP_INCLUDE_DIR DEP_LIBRARY_DIR DEP_BINARY_DIR)

# This path will be added to PATH when starting the program from the build directory
# It contains the dependency DLLs
SET(RUNTIME_LIBRARY_DIRECTORY ${DEP_BINARY_DIR})

# Copy DLLs to the binary directory when installing
INSTALL(
  DIRECTORY ${DEP_BINARY_DIR}/
  DESTINATION bin
  REGEX "FxLibD\\.dll$|\\.svn|SysInfoD\\.dll$" EXCLUDE
)


############# Find/Set Libraries ################

# Qt version 4
FIND_PACKAGE(Qt4 COMPONENTS QtCore QtGui REQUIRED)

# Check whether we can use Visual Leak Detector
FIND_PACKAGE(VLD QUIET)

# National Instruments DAQ SDK
FIND_PACKAGE(NIDAQ REQUIRED)

# OASIS stage control
FIND_PACKAGE(OASIS REQUIRED)

# FFTW
SET(FFTW3_INCLUDE_DIR       ${DEP_INCLUDE_DIR}/fftw3/include   CACHE PATH "")
SEt(FFTW3_LIBRARY           ${DEP_LIBRARY_DIR}/libfftw3f-3.lib CACHE FILEPATH "")

# Baumer FX Lib SDK for Leica cameras
SET(FXLIB_INCLUDE_DIR       ${DEP_INCLUDE_DIR}/fxlib/include   CACHE PATH "")
SET(FXLIB_LIBRARY optimized ${DEP_LIBRARY_DIR}/FxLib.lib
                  debug     ${DEP_LIBRARY_DIR}/FxLibD.lib      CACHE FILEPATH "")

# Leica SDK for DM microscopes
SET(AHM_INCLUDE_DIR         ${DEP_INCLUDE_DIR}/ahm/include     CACHE PATH "")
SET(AHM_LIBRARIES           ${DEP_LIBRARY_DIR}/ahmcore.lib
                            ${DEP_LIBRARY_DIR}/ahmcorelocator.lib CACHE STRING "")
# QtSerialPort		    
SET(QTSERIALPORT_INDLUDE_DIR	${DEP_INCLUDE_DIR}/qtserialport/include	CACHE PATH "")
SET(QTSERIALPORT_LIBRARY	${DEP_LIBRARY_DIR}/QtSerialPortd.lib	CACHE STRING "")

MARK_AS_ADVANCED(
  FFTW3_INCLUDE_DIR FFTW3_LIBRARY
  OASIS_INCLUDE_DIR OASIS_LIBRARY
  FXLIB_INCLUDE_DIR FXLIB_LIBRARY
  AHM_INCLUDE_DIR AHM_LIBRARIES
  QTSERIALPORT_INCLUDE_DIR QTSERIALPORT_LIBRARY
)
