/*
 Copyright (c) 2009-2011, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

/**
@file
@brief
    Declaration of custom assertion facilities
*/

#ifndef _TrackerAssert_H__
#define _TrackerAssert_H__

#include "TrackerPrereqs.h"

#include <cassert>
#include "Logger.h"

#ifndef NDEBUG
/** Run time assertion like assert(), but with an embedded message.
@details
    The message will be printed as assertion. \n
    @code
        TRACKER_ASSERT(condition, "Message");
    @endcode
*/
#   define TRACKER_ASSERT(condition, message) \
        condition ? ((void)0) : (void)(Logger::getInstance().logMessage( \
            message, Logger::Assertion, Logger::High, __LINE__, __FILE__, __FUNCTION__)); \
        assert(condition)
#else
#   define TRACKER_ASSERT(condition, errorMessage) ((void)0)
#endif

#endif /* _TrackerAssert_H__ */
