#
# Copyright (c) 2009-2012, Reto Grieder
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# This software is provided 'as-is', without any express or implied warranty.
#
#  Description:
#    Finds OASIS stage control library
#

# Note: When installing OASIS, all required files will be in
# "Objective Imaging/OASIS", without subfolders, 32 bit only.
# Therefore, 64 bit libraries have to be installed manually, done best by
# copying the Lib and include folder from the SDK install directory.

INCLUDE(FindPackageHandleStandardArgs)

FIND_PATH(OASIS_INCLUDE_DIR Oasis4i.h
  PATH_SUFFIXES "Objective Imaging/OASIS" "Objective Imaging/OASIS/include"
)

IF(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 Bit
  SET(_suffix "Objective Imaging/OASIS/Lib/64-bit")
ELSE() # 32 Bit
  SET(_suffix "Objective Imaging/OASIS" "Objective Imaging/OASIS/Lib")
ENDIF()
FIND_LIBRARY(OASIS_LIBRARY
  NAMES Oasis4i
  PATH_SUFFIXES ${_suffix}
)

# Handle the REQUIRED argument and set OASIS_FOUND
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OASIS DEFAULT_MSG
  OASIS_LIBRARY
  OASIS_INCLUDE_DIR
)

MARK_AS_ADVANCED(
  OASIS_INCLUDE_DIR
  OASIS_LIBRARY
)
