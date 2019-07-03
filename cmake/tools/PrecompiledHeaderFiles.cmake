#
# Copyright (c) 2009-2012, Reto Grieder
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# This software is provided 'as-is', without any express or implied warranty.
#

MACRO(PRECOMPILED_HEADER_FILES_PRE_TARGET _target_name _header_file_arg _sourcefile_var)

  GET_FILENAME_COMPONENT(_pch_header_file ${_header_file_arg} ABSOLUTE)
  GET_FILENAME_COMPONENT(_pch_header_filename ${_pch_header_file} NAME)
  GET_FILENAME_COMPONENT(_pch_header_filename_we ${_pch_header_file} NAME_WE)

  IF(NOT EXISTS ${_pch_header_file})
    MESSAGE(FATAL_ERROR "Specified precompiled headerfile '${_header_file_arg}' does not exist.")
  ENDIF()

  # Extract arguments from ARGN
  FOREACH(_arg ${ARGN})
    IF(NOT "${_arg}" STREQUAL "EXCLUDE")
      IF(NOT _arg_second)
        # Source files with PCH support
        SET(_included_files ${_included_files} ${_arg})
      ELSE()
        # Source files to be excluded from PCH support (easier syntax this way)
        SET(_excluded files ${_excluded_files} ${_arg})
      ENDIF()
    ELSE()
      SET(_arg_second TRUE)
    ENDIF()
  ENDFOREACH(_arg)

  # Use ${_sourcefile_var} if no files were specified explicitely
  IF(NOT _included_files)
    SET(_source_files ${${_sourcefile_var}})
  ELSE()
    SET(_source_files ${_included_files})
  ENDIF()

  # Exclude files (if specified)
  FOREACH(_file ${_excluded_files})
    LIST(FIND _source_files ${_file} _list_index)
    IF(_list_index GREATER -1)
      LIST(REMOVE_AT _source_files _list_index)
    ELSE()
      MESSAGE(FATAL_ERROR "Could not exclude file ${_file} in target ${_target_name}")
    ENDIF()
  ENDFOREACH(_file)

  LIST(FIND ${_sourcefile_var} ${_pch_header_file} _list_index)
  IF(_list_index EQUAL -1) # Header file could already be included with GET_ALL_HEADER_FILES
    LIST(APPEND ${_sourcefile_var} ${_pch_header_file})
  ENDIF()

  IF(MSVC)

    # Write and add one source file, which generates the precompiled header file
    SET(_pch_source_file "${CMAKE_CURRENT_BINARY_DIR}/${_pch_header_filename_we}.cc")
    IF(NOT EXISTS ${_pch_source_file})
      FILE(WRITE ${_pch_source_file} "#include \"${_pch_header_file}\"\n")
    ENDIF()
    SET_SOURCE_FILES_PROPERTIES(_pch_source_file PROPERTIES GENERATED TRUE)
    LIST(APPEND ${_sourcefile_var} ${_pch_source_file})

    SET(_pch_file "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${_pch_header_filename}.pch")
    # Set compile flags for generated source file
    SET_SOURCE_FILES_PROPERTIES(${_pch_source_file} PROPERTIES COMPILE_FLAGS "/c /Yc\"${_pch_header_file}\" /Fp\"${_pch_file}\"")
    # Set Compile flags for the other source files
    FOREACH(_file ${_source_files})
      GET_SOURCE_FILE_PROPERTY(_is_header ${_file} HEADER_FILE_ONLY)
      IF(NOT _is_header)
        GET_SOURCE_FILE_PROPERTY(_old_flags ${_file} COMPILE_FLAGS)
        IF(NOT _old_flags)
          SET(_old_flags "")
        ENDIF()
        SET_SOURCE_FILES_PROPERTIES(${_file} PROPERTIES COMPILE_FLAGS "${_old_flags} /FI\"${_pch_header_file}\" /Yu\"${_pch_header_file}\" /Fp\"${_pch_file}\"")
      ENDIF(NOT _is_header)
    ENDFOREACH(_file)

  ENDIF()

ENDMACRO(PRECOMPILED_HEADER_FILES_PRE_TARGET)
