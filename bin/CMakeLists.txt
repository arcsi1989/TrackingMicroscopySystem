#
# Copyright (c) 2009-2012, Reto Grieder
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# This software is provided 'as-is', without any express or implied warranty.
#
#  Description:
#    Creates run scripts in the root directory of the build tree and
#    the runtime output directory.
#

# Files that get added to the runtime folder(s) where the executable sits
SET(_files run.bat tracker_dev_build.keep_me ahmconfig.xml)

IF(NOT CMAKE_CONFIGURATION_TYPES)
  SET(_subdirs .)
ELSE()
  SET(_subdirs ${CMAKE_CONFIGURATION_TYPES})
ENDIF()

# Variable used in the files to be configured/copied
# (describes where the DLLs from the dependencies sit)
STRING(REPLACE "/" "\\" RUNTIME_LIBRARY_DIRECTORY_WINDOWS ${RUNTIME_LIBRARY_DIRECTORY})

FOREACH(_subdir ${_subdirs})
  SET(CURRENT_RUNTIME_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${_subdir})

  FOREACH(_file ${_files})
    CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/${_file}.in" "${CURRENT_RUNTIME_DIR}/${_file}" @ONLY)
  ENDFOREACH(_file)
ENDFOREACH(_subdir)
