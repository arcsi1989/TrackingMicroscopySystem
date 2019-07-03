#
# Copyright (c) 2009-2011, Reto Grieder
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# This software is provided 'as-is', without any express or implied warranty.
#
#  Description:
#    Write to the cache by force, but only if the user hasn't changed the value
#    Additional argument is the value (may also be a list)

FUNCTION(SET_CACHE_ADVANCED _varname _type _docstring)
  SET(_value ${ARGN})
  IF(NOT "${_type}" MATCHES "^(STRING|BOOL|PATH|FILEPATH)$")
    MESSAGE(FATAL_ERROR "${_type} is not a valid CACHE entry type")
  ENDIF()

  IF(NOT DEFINED _INTERNAL_${_varname} OR "${_INTERNAL_${_varname}}" STREQUAL "${${_varname}}")
    SET(${_varname} "${_value}" CACHE ${_type} "${_docstring}" FORCE)
    SET(_INTERNAL_${_varname} "${_value}" CACHE INTERNAL "Do not edit in any case!")
  ENDIF()
ENDFUNCTION(SET_CACHE_ADVANCED)
