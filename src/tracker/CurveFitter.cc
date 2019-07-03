/*
 Copyright (c) 2014, Tobias Klauser

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#include <cmath>
#include <iostream>

// TEMP
#include <cstdio>

#include "CurveFitter.h"
#include "TMath.h"

namespace tracker {

    CurveFitter::CurveFitter(CurveType type)
        : mType(type)
        , mControl(lm_control_double)
        , mModelFunction(NULL)
    {
        int nParams = 0;

        switch (mType) {
        case CurveGaussian1:
            mModelFunction = &modelFnGaussian1;
            mModelInverseFunction = &modelFnInverseGaussian1;
            nParams = 3;
            break;
        case CurveGaussian1Offset:
            mModelFunction = &modelFnGaussian1Offset;
            mModelInverseFunction = &modelFnInverseGaussian1Offset;
            nParams = 4;
            break;
        }

        // make sure we have the right number of parameters
        mParams.resize(nParams);
    }

    CurveFitter::~CurveFitter()
    {
    }

    bool CurveFitter::fit(QVector<double> x, QVector<double> y)
    {
        if (x.size() != y.size()) {
            std::cout << "CurveFitter: X and Y dimension of data not matching" << std::endl;
            return false;
        }

        // fit the curve using the lmfit libary
        lmcurve(mParams.size(), mParams.data(), x.size(), x.data(), y.data(), mModelFunction, &mControl, &mStatus);

        switch (mStatus.outcome) {
        // curve fitting converged, see lm_infmsg in lmfit.c
        case 1: case 2: case 3:
            return true;
        // in all other cases an error occured
        default:
            return false;
        }
    }

    const QVector<double> CurveFitter::getParams()
    {
        return mParams;
    }

    void CurveFitter::setParams(const QVector<double> params)
    {
        int oldSize = mParams.size();

        mParams = params;

        if (mParams.size() != oldSize) {
            std::cout << "CurveFitter: Invalid number of params (" << params.size()
                      << " instead of " << oldSize << "), resizing" << std::endl;
            mParams.resize(oldSize);
        }
    }

    double CurveFitter::interpolateY(double x)
    {
        return mModelFunction(x, mParams.data());
    }

    double CurveFitter::interpolateX(double y)
    {
        return mModelInverseFunction(y, mParams.data());
    }

    double modelFnGaussian1(double x, const double *p)
    {
        return p[0] * std::exp(-sqr((x - p[1]) / p[2]));
    }

    double modelFnInverseGaussian1(double y, const double *p)
    {
        y = std::abs(y);

        // cut off at peak of Gaussian, otherwise we get imaginary results
        double ymax = p[0];
        y = y > ymax ? ymax : y;
        // prevent zero values, otherwise we get +/- INF
        double ymin = 0.1;
        y = y < ymin ? ymin : y;

        return p[1] + p[2] * std::sqrt(-std::log(y / p[0]));
    }

    double modelFnGaussian1Offset(double x, const double *p)
    {
        return p[0] * std::exp(-sqr((x - p[1]) / p[2])) + p[3];
    }

    double modelFnInverseGaussian1Offset(double y, const double *p)
    {
        y = std::abs(y);

        // cut of at peak of Gaussian, otherwise we get imaginary results
        double ymax = p[0] + p[3];
        y = y > ymax ? ymax : y;
        // prevent values below p[3], otherwise we get +/- INF
        double ymin = p[3] + 0.1;
        y = y < ymin ? ymin : y;

        return p[1] + p[2] * std::sqrt(-std::log((y - p[3]) / p[0]));
    }

} // namespace tracker
