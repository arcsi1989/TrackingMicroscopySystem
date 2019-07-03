#
# Copyright (c) 2009-2012, Reto Grieder
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# This software is provided 'as-is', without any express or implied warranty.
#
#  Description:
#    Sets the right compiler and linker flags for the Microsoft Compiler.
#

INCLUDE(FlagUtilities)


#################### Compiler Flags #####################

# CMake default flags : -DWIN32 -D_WINDOWS -W3 -Zm1000
# additionally for CXX: -EHsc -GR
# We keep these flags but reset the build specific flags
SET_COMPILER_FLAGS("" Debug RelWithDebInfo Release MinSizeRel)

# Make sure we define all the possible macros for identifying Windows
ADD_COMPILER_FLAGS("-D__WIN32__ -D_WIN32")
# Suppress some annoying warnings
ADD_COMPILER_FLAGS("-D_CRT_SECURE_NO_WARNINGS")
ADD_COMPILER_FLAGS("-D_SCL_SECURE_NO_WARNINGS")

# Use multiprocessed compiling (like "make -j3" on Unix)
ADD_COMPILER_FLAGS("-MP")

# Never omit frame pointers to avoid useless stack traces (the performance
# loss is almost immeasurable)
ADD_COMPILER_FLAGS("-Oy-")
# Enable non standard floating point optimisations
ADD_COMPILER_FLAGS("-fp:fast")

# Set build specific flags.
# -MD[d]    Multithreaded [debug] shared MSVC runtime library
# -Zi       Generate debug symbols
# -O[d|2|1] No optimisations, optimise for speed, optimise for size
# -Oi[-]    Use or disable use of intrinisic functions
# -GL       Link time code generation (see -LTCG in linker flags)
# -RTC1     Both basic runtime checks
ADD_COMPILER_FLAGS("-MDd -Zi -Od -Oi  -D_DEBUG -RTC1" Debug)
ADD_COMPILER_FLAGS("-MD  -Zi -O2 -Oi  -DNDEBUG -GL"   RelWithDebInfo)
ADD_COMPILER_FLAGS("-MD      -O2 -Oi  -DNDEBUG -GL"   Release)
ADD_COMPILER_FLAGS("-MD      -O1 -Oi- -DNDEBUG -GL"   MinSizeRel)


####################### Warnings ########################

# -WX    General warning Level X
# -wdX   Disable specific warning X
# -wnX   Set warning level of specific warning X to level n

OPTION(EXTRA_COMPILER_WARNINGS "Enable some extra warnings (heavily pollutes the output)" FALSE)

# Increase warning level if requested
IF(EXTRA_COMPILER_WARNINGS)
  REMOVE_COMPILER_FLAGS("-W1 -W2 -W3")
  ADD_COMPILER_FLAGS   ("-W4")
ELSE()
  REMOVE_COMPILER_FLAGS("-W1 -W2 -W4")
  ADD_COMPILER_FLAGS   ("-W3")
ENDIF()

# "<type> needs to have dll-interface to be used by clients'
# Happens on STL member variables which are not public
ADD_COMPILER_FLAGS("-w44251")
ADD_COMPILER_FLAGS("-w44275") # For inheritance


##################### Linker Flags ######################

# CMake default flags: -MANIFEST -STACK:10000000 -machine:I386
# We keep these flags but reset the build specific flags
SET_LINKER_FLAGS("" Debug RelWithDebInfo Release MinSizeRel)

# Generate debug symbols
ADD_LINKER_FLAGS("-DEBUG" Debug RelWithDebInfo)

# Incremental linking speeds up development builds
ADD_LINKER_FLAGS("-INCREMENTAL:YES" Debug)
ADD_LINKER_FLAGS("-INCREMENTAL:NO"  Release RelWithDebInfo MinSizeRel)

# Eliminate unreferenced data
ADD_LINKER_FLAGS("-OPT:REF" Release RelWithDebInfo MinSizeRel)

# Link time code generation can improve run time performance at the cost of
# increased link time (the total build time is about the same though)
ADD_LINKER_FLAGS("-LTCG" Release RelWithDebInfo MinSizeRel)
