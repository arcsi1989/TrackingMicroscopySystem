/*
 Copyright (c) 2009-2012, Reto Grieder & Benjamin Beyeler

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#include "DummyDac.h"

namespace tracker
{
    /*static*/ Dac* Dac::makeDac(QObject* parent)
    {
        return new DummyDac(parent);
    }

    DummyDac::DummyDac(QObject* parent)
        : Dac(parent)
    {
    }

    DummyDac::~DummyDac()
    {
    }
}