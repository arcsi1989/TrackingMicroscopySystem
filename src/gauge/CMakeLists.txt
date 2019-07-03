#
# Copyright (c) 2009-2012, Reto Grieder
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# This software is provided 'as-is', without any express or implied warranty.
#

INCLUDE(SourceFileUtilities)


SET_SOURCE_FILES(GAUGE_FILES
  QT qtsvgdialgauge.h              qtsvgdialgauge.cpp
     qtsvgpixmapcache.h            qtsvgpixmapcache.cpp
  QT tachometer_svgdialgauge.qrc
)

ADD_LIBRARY(gauge STATIC ${GAUGE_FILES})

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  # Make Qt UIC generated header files visible
  ${CMAKE_CURRENT_BINARY_DIR}
)

# Link dependencies
TARGET_LINK_LIBRARIES(gauge
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${QT_QTSVG_LIBRARY}
)

# Link Visual Leak Detector detector to the debug configuration if requested
IF(VISUAL_LEAK_DETECTOR_ENABLE)
  TARGET_LINK_LIBRARIES(gauge debug ${VLD_LIBRARY})
ENDIF()
