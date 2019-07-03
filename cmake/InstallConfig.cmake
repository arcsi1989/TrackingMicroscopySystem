#
# Copyright (c) 2009-2012, Reto Grieder
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# This software is provided 'as-is', without any express or implied warranty.
#
#  Description:
#    Configures the installation paths
#


# Put the default install directory inside the build directory
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH
      "Install path prefix, prepended onto install directories." FORCE)
ENDIF()

# Default installation paths
SET(RUNTIME_INSTALL_DIRECTORY ${CMAKE_INSTALL_PREFIX}/${DEFAULT_RUNTIME_PATH})
SET(LIBRARY_INSTALL_DIRECTORY ${CMAKE_INSTALL_PREFIX}/${DEFAULT_LIBRARY_PATH})
SET(ARCHIVE_INSTALL_DIRECTORY ${CMAKE_INSTALL_PREFIX}/${DEFAULT_ARCHIVE_PATH})
SET(DOC_INSTALL_DIRECTORY     ${CMAKE_INSTALL_PREFIX}/${DEFAULT_DOC_PATH})
SET(DATA_INSTALL_DIRECTORY    ${CMAKE_INSTALL_PREFIX}/${DEFAULT_DATA_PATH})
SET(CONFIG_INSTALL_DIRECTORY  ${CMAKE_INSTALL_PREFIX}/${DEFAULT_CONFIG_PATH})
SET(LOG_INSTALL_DIRECTORY     ${CMAKE_INSTALL_PREFIX}/${DEFAULT_LOG_PATH})

# Never install debug builds
INSTALL(CODE "
  IF(\"\${CMAKE_INSTALL_CONFIG_NAME}\" STREQUAL \"Debug\")
      MESSAGE(FATAL_ERROR \"Installing debug builds is a really bad idea\")
  ENDIF()
")
