/*
 Copyright (c) 2009-2011, Reto Grieder

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef __Util_Singleton_H__
#define __Util_Singleton_H__

#include "TrackerPrereqs.h"

#include <cassert>
#include <cstring>

namespace tracker
{
    /** Definition of the Singleton template that is used as base class for
        classes that allow only one instance.
    */
    template <class T>
    class Singleton
    {
    public:
        //! Returns a reference to the singleton instance
        static T& getInstance()
        {
            assert(T::singletonPtr_s != NULL);
            return *T::singletonPtr_s;
        }

    protected:
        //! Constructor sets the singleton instance pointer
        Singleton()
        {
            assert(T::singletonPtr_s == NULL);
            T::singletonPtr_s = static_cast<T*>(this);
        }

        //! Destructor resets the singleton instance pointer
        ~Singleton()
        {
            assert(T::singletonPtr_s != NULL);
            T::singletonPtr_s = NULL;
        }

    private:
        Singleton(const Singleton&); //!< Don't use (undefined)
    };
}

#endif /* __Util_Singleton_H__ */
