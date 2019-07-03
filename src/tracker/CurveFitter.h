/*
 Copyright (c) 2014, Tobias Klauser

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef __CURVEFITTER_H__
#define __CURVEFITTER_H__

#include <QVector>

#include "lmfit/lmfit.h"

namespace tracker {

    /** Curve fitting based on the lmfit library. */
    class CurveFitter
    {
    public:
        enum CurveType {
            /// Gaussian with 1 term, see modelFnGaussian1
            CurveGaussian1,
            /// Gaussian with 1 term and constant offset, see
            /// modelFnGaussian1Offset
            CurveGaussian1Offset,
        };

        /// type for function pointer to a model function
        typedef double (*ModelFunction)(double x, const double *p);

        CurveFitter(CurveType type);
        ~CurveFitter();

        /** Fit data points to a curve.

         @param x
            x values
         @param y
            y values
         @return
            true or false, depending on whether the curve could be fitted
         */
        bool fit(QVector<double> x, QVector<double> y);

        /** Return the curve parameters.
         *
         * It is up to the caller to properly interprete these based on the
         * type of curve requested.
         */
        const QVector<double> getParams();

        /** Manually set the curve parameters.
         *
         * This can e.g. be used to adjust the mean of a Gaussian.
         *
         * It is up to the caller to properly interprete these based on the
         * type of curve requested.
         */
        void setParams(const QVector<double> params);

        /** Use the model function to get the value y = f(x). */
        double interpolateY(double x);

        /** Use the inverse function to get the value x = f^(-1)(y). */
        double interpolateX(double y);

    private:
        CurveType           mType;          ///< Type of the function to fit
        ModelFunction       mModelFunction; ///< Model function (according to type specified in ctor)
        ModelFunction       mModelInverseFunction;  ///< Inverse model function
        QVector<double>     mParams;        ///< Parameters of the model function to fit
        lm_control_struct   mControl;       ///< lmfit control data structure
        lm_status_struct    mStatus;        ///< lmfit status data structure
    };

    /** Model function for Gaussian with 1 term (3 parameters).
     *
     * @param x
     *  x value
     * @param p
     *  Array of 3 parameters.
     */
    double modelFnGaussian1(double x, const double *p);

    /** Model function for Gaussian with 1 term and constant offset (4 parameters).
     *
     * @param x
     *  x value
     * @param p
     *  Array of 4 parameters.
     */
    double modelFnGaussian1Offset(double x, const double *p);

    /** Model functional inverse for Gaussian with 1 term (3 parameters). */
    double modelFnInverseGaussian1(double y, const double *p);

    /** Model functional inverse for Gaussian with 1 term and constant offset
     * (4 parameters).
     */
    double modelFnInverseGaussian1Offset(double y, const double *p);

} // namespace tracker

#endif // __CURVEFITTER_H__
