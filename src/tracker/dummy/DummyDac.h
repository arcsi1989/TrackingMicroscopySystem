/*
 Copyright (c) 2009-2012, Reto Grieder & Benjamin Beyeler

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef _DummyDac_H__
#define _DummyDac_H__

#include "Dac.h"

namespace tracker
{
    class DummyDac : public Dac
    {
    public:
        DummyDac(QObject* parent);
        ~DummyDac();

        void setvoltage(float) {}
        float readvoltage() const
            { return 0.0f; }
        void startdactimer() {}
        void stopdactimer() {}
    };
}

#endif /* _DummyDac_H__ */
