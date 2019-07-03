#
# Copyright (c) 2009-2012, Reto Grieder
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# This software is provided 'as-is', without any express or implied warranty.
#
#  Description:
#    Several functions that help organising the source tree.
#

# Adds source files to a list and processes special QT files
MACRO(ADD_SOURCE_FILES _varname)
  SET(_compile_qt_next FALSE)
  FOREACH(_file ${ARGN})
    IF(_file STREQUAL "QT")
      SET(_compile_qt_next TRUE)
    ELSE()
      LIST(APPEND ${_varname} ${_file})

      # Handle Qt MOC and UI files
      IF(_compile_qt_next)
        GET_FILENAME_COMPONENT(_ext ${_file} EXT)
        IF(_ext STREQUAL ".ui")
          QT4_WRAP_UI(${_varname} ${_file})
        ELSEIF(_ext STREQUAL ".qrc")
          QT4_ADD_RESOURCES(${_varname} ${_file})
        ELSEIF(_ext MATCHES "^\\.(h|hpp|hxx)$")
          QT4_WRAP_CPP(${_varname} ${_file})
        ENDIF()
        SET(_compile_qt_next FALSE)
      ENDIF()
    ENDIF()
  ENDFOREACH(_file)

  # Don't compile header files
  FOREACH(_file ${${_varname}})
    IF(NOT _file MATCHES "\\.(c|cc|cpp|cxx)$")
      SET_SOURCE_FILES_PROPERTIES(${_file} PROPERTIES HEADER_FILE_ONLY TRUE)
    ENDIF()
  ENDFOREACH(_file)

ENDMACRO(ADD_SOURCE_FILES)


# Sets source files and processes special QT files
MACRO(SET_SOURCE_FILES _varname)
  SET(${_varname} "")
  ADD_SOURCE_FILES(${_varname} ${ARGN})
ENDMACRO(SET_SOURCE_FILES)


# Generate source groups according to the directory structure
FUNCTION(GENERATE_SOURCE_GROUPS)

  FOREACH(_file ${ARGN})
    GET_SOURCE_FILE_PROPERTY(_full_filepath ${_file} LOCATION)
    FILE(RELATIVE_PATH _relative_path ${CMAKE_CURRENT_SOURCE_DIR} ${_full_filepath})
    IF(NOT _relative_path MATCHES "^\\.\\./") # Has "../" at the beginning
      GET_FILENAME_COMPONENT(_relative_path ${_relative_path} PATH)
      STRING(REPLACE "/" "\\\\" _group_path "${_relative_path}")
      SOURCE_GROUP("Source\\${_group_path}" FILES ${_file})
    ELSE()
      # File is being generated in the binary directory
      SOURCE_GROUP("Generated" FILES ${_file})
    ENDIF()
  ENDFOREACH(_file)

ENDFUNCTION(GENERATE_SOURCE_GROUPS)
