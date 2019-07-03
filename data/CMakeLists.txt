#
# Copyright (c) 2009-2012, Reto Grieder
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# This software is provided 'as-is', without any express or implied warranty.
#
#  Description:
#    Configures the data installation.
#

SET(DATA_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
SET(DATA_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

################ Installation #################

# Copy stuff from here to the installation data folder
INSTALL(
  DIRECTORY ${DATA_DIRECTORY}/
  DESTINATION ${DATA_INSTALL_DIRECTORY}
  REGEX "\\.svn|CMakeLists\\.txt$" EXCLUDE
)
