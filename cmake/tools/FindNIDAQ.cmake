#
# Copyright (c) 2009-2012, Reto Grieder
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# This software is provided 'as-is', without any express or implied warranty.
#
#  Description:
#    Finds the National Instruments NI-DAQ library
#

INCLUDE(FindPackageHandleStandardArgs)

FIND_PATH(NIDAQ_INCLUDE_DIR NIDAQmx.h
  PATH_SUFFIXES "National Instruments/NI-DAQ/DAQmx ANSI C Dev/include"
)

IF(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 Bit
  SET(_suffix "National Instruments/Shared/ExternalCompilerSupport/C/lib64/msvc")
ELSE() # 32 Bit
  SET(_suffix "National Instruments/NI-DAQ/DAQmx ANSI C Dev/lib/msvc")
ENDIF()
FIND_LIBRARY(NIDAQ_LIBRARY
  NAMES NIDAQmx
  PATH_SUFFIXES "${_suffix}"
)

# Handle the REQUIRED argument and set NIDAQ_FOUND
FIND_PACKAGE_HANDLE_STANDARD_ARGS(NIDAQ DEFAULT_MSG
  NIDAQ_LIBRARY
  NIDAQ_INCLUDE_DIR
)

MARK_AS_ADVANCED(
  NIDAQ_INCLUDE_DIR
  NIDAQ_LIBRARY
)
