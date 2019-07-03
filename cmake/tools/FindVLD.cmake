#
# Copyright (c) 2009-2012, Reto Grieder
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# This software is provided 'as-is', without any express or implied warranty.
#
#  Description:
#    Finds the Visual Leak Detector
#

INCLUDE(FindPackageHandleStandardArgs)

FIND_PATH(VLD_INCLUDE_DIR vld.h
  PATH_SUFFIXES "Visual Leak Detector/include"
)

IF(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 Bit
  SET(_suffix "Visual Leak Detector/lib/Win64")
ELSE() # 32 Bit
  SET(_suffix "Visual Leak Detector/lib/Win32")
ENDIF()
FIND_LIBRARY(VLD_LIBRARY
  NAMES vld
  PATH_SUFFIXES ${_suffix}
)

# Handle the REQUIRED argument and set POCO_FOUND
FIND_PACKAGE_HANDLE_STANDARD_ARGS(VLD DEFAULT_MSG
  VLD_LIBRARY
  VLD_INCLUDE_DIR
)

MARK_AS_ADVANCED(
  VLD_INCLUDE_DIR
  VLD_LIBRARY
)
