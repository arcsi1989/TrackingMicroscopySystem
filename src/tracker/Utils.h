/*
 Copyright (c) 2014, Tobias Klauser

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef __UTILS_H__
#define __UTILS_H__

#include <QVector>

namespace tracker {

    /** Binary search for the closest value in a vector sorted in ascending order.
     *
     * This function assumes the vector to be sorted in ascending order (lowest
     * value first, highest value last). If the value is outside the range of
     * the vector, it is clamped accordingly. If several elements in the vector
     * are equally close, the first one is returned.
     *
     * @param vec
     *   The vector to search, sorted in ascending order.
     * @param val
     *   Scalar value for which to search the closest element in the vector.
     * @return
     *   Index of the element in the vector that is closest to val or -1 if the
     *   vector contains no elements.
     */
    template<typename T>
    int binarySearchClosest(const QVector<T> vec, T val)
    {
        // empty vector
        if (vec.size() == 0)
            return -1;

        // clamp value to range of vector
        if (val < vec.first())
            return 0;
        if (val > vec.last())
            return vec.size() - 1;

        int imin = 0;
        int imax = vec.size() - 1;
        while (imax - imin > 1) {
            // determine midpoint, prevent integer overflow
            int imid = imin + ((imax - imin) / 2);
            if (vec[imid] >= val)
                imax = imid;
            else
                imin = imid;
        }

        if (imax - imin == 1 && std::abs(vec[imax] - val) < std::abs(vec[imin] - val))
            imin = imax;

        return imin;
    }

} // namespace tracker

#endif // __UTILS_H__
