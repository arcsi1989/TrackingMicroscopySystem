/*
 Copyright (c) 2009-2011, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

/**
    @file
    @brief Declaration and implementation of several math-functions.
*/

#ifndef _Util_Math_H__
#define _Util_Math_H__

#include "TrackerPrereqs.h"

#include <QtGlobal>

// Certain headers might define unwanted macros...
#undef max
#undef min
#undef sgn
#undef clamp
#undef sqrt
#undef square
#undef mod
#undef rnd

namespace tracker
{
    // C++ doesn't define any constants for pi, e, etc.
    namespace math
    {
        const float pi      = 3.14159265f;      ///< PI
        const float pi_2    = 1.57079633f;      ///< PI / 2
        const float pi_4    = 7.85398163e-1f;   ///< PI / 4
        const float e       = 2.71828183f;      ///< e
        const float sqrt2   = 1.41421356f;      ///< sqrt(2)
        const float sqrt2_2 = 7.07106781e-1f;   ///< sqrt(2) / 2
        const float log10   = 2.30258509f;      ///< log(10)

        const double pi_d      = 3.14159265358979324;       ///< PI (double)
        const double pi_2_d    = 1.57079632679489662;       ///< PI / 2 (double)
        const double pi_4_d    = 7.85398163397448310e-1;    ///< PI / 4 (double)
        const double e_d       = 2.71828182845904524;       ///< e (double)
        const double sqrt2_d   = 1.41421356237309505;       ///< sqrt(2) (double)
        const double sqrt2_2_d = 7.07106781186547524e-1;    ///< sqrt(2) / 2 (double)
        const double log10_d   = 2.30258509299404590;       ///< log(10)
    }

    //! Returns a random number 0 <= x < 1
    inline float rnd()
    {
        return qrand() / (RAND_MAX + 1.0f);
    }

    //! Clamp value between min and max.
    template <typename T>
    inline T clamp(T value, T min, T max)
    {
        return (value < min) ? min : ((max < value) ? max : value);
    }

    //! Return the squared value.
    template <typename T>
    inline T sqr(T value)
    {
        return value * value;
    }
}

#endif /* _Util_Math_H__ */
