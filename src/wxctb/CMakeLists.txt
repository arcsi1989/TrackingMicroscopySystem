#
# Copyright (c) 2009-2012, Reto Grieder
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# This software is provided 'as-is', without any express or implied warranty.
#

SET(WXCTB_FILES
  ctb.h
  fifo.cpp
  fifo.h
  iobase.cpp
  iobase.h
  serport.h
  serportx.h
  timer.h
  win32/serport.cpp
  win32/serport.h
  win32/timer.cpp
  win32/timer.h
)

# Link statically because the library doesn't have a DLL interface
ADD_LIBRARY(wxctb STATIC ${WXCTB_FILES})

# Link required dependencies
TARGET_LINK_LIBRARIES(wxctb winmm)
