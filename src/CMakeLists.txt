#
# Copyright (c) 2009-2012, Reto Grieder
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# This software is provided 'as-is', without any express or implied warranty.
#
#  Description:
#

# For referencing library headers
INCLUDE_DIRECTORIES(.)

# Required when using Qt (also adds the include directories)
INCLUDE(${QT_USE_FILE})

# Process libraries
ADD_SUBDIRECTORY(loki)
ADD_SUBDIRECTORY(wxctb)
ADD_SUBDIRECTORY(gauge)
ADD_SUBDIRECTORY(tracker)


#################### Doxygen ####################

# Prepare include paths for Doxygen. This is necessary to display
# the correct path to use when including a file, e.g.
# core/XMLPort.h instead of src/core/XMLPort.h

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/tracker)
GET_DIRECTORY_PROPERTY(_temp INCLUDE_DIRECTORIES)
# Replace ';' by spaces
STRING(REPLACE ";" " " _temp "${_temp}")
SET(DOXYGEN_INCLUDE_DIRECTORIES "${_temp}" PARENT_SCOPE)
