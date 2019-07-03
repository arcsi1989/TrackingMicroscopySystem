#
# Copyright (c) 2009-2012, Reto Grieder
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# This software is provided 'as-is', without any express or implied warranty.
#
#  Description:
#    Copies configuration files to the output and installation folders.
#


# Files that get added to the configuration folder(s)
SET(_files tracker.ini)


IF(NOT CMAKE_CONFIGURATION_TYPES)
  SET(_subdirs .)
ELSE()
  SET(_subdirs ${CMAKE_CONFIGURATION_TYPES})
ENDIF()

# Copy the files to every config folder in the build directory
FOREACH(_subdir ${_subdirs})
  GET_FILENAME_COMPONENT(CURRENT_CONFIG_DIR ${CMAKE_CONFIG_OUTPUT_DIRECTORY}/${_subdir} ABSOLUTE)

  FOREACH(_file ${_files})
    CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/${_file}" "${CURRENT_CONFIG_DIR}/${_file}" @ONLY)
  ENDFOREACH(_file)
ENDFOREACH(_subdir)


################ Installation #################

FOREACH(_file ${_files})
  INSTALL(
    FILES ${_file}
    DESTINATION ${CONFIG_INSTALL_DIRECTORY}
  )
ENDFOREACH(_file)
