#
# Copyright (c) 2009-2011, Reto Grieder
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# This software is provided 'as-is', without any express or implied warranty.
#

SET(LOKI_FILES
  NullType.h
  RefToValue.h
  ScopeGuard.h
  static_check.h
  TypeManip.h
  TypeTraits.h
)

ADD_CUSTOM_TARGET(loki SOURCES ${LOKI_FILES})
