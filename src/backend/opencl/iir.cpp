/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#include <af/dim4.hpp>
#include <af/defines.h>
#include <ArrayInfo.hpp>
#include <Array.hpp>
#include <iir.hpp>
#include <err_opencl.hpp>
#include <math.hpp>
#include <arith.hpp>
#include <convolve.hpp>
#include <kernel/iir.hpp>

using af::dim4;

namespace opencl
{
    template<typename T>
    Array<T> iir(const Array<T> &b, const Array<T> &a, const Array<T> &x)
    {
        try {

            T h_a0 = scalar<T>(0);
            getQueue().enqueueReadBuffer(*(a.get()),
                                         CL_TRUE,
                                         sizeof(T) * a.getOffset(),
                                         sizeof(T),
                                         &h_a0);

            Array<T> a0 = createValueArray<T>(b.dims(), h_a0);
            Array<T> bNorm = arithOp<T, af_div_t>(b, a0, b.dims());

            ConvolveBatchKind type = x.ndims() == 1 ? ONE2ONE : MANY2MANY;
            if (x.elements() != b.elements()) {
                type = (x.elements() < b.elements()) ? ONE2MANY : MANY2ONE;
            }

            // Extract the first N elements
            Array<T> c = convolve<T, T, 1, true>(x, bNorm, type);
            dim4 cdims = c.dims();
            cdims[0] = x.dims()[0];
            c.resetDims(cdims);

            int num_a = a.dims()[0];

            if (num_a == 1) return c;

            dim4 ydims = c.dims();
            Array<T> y = createEmptyArray<T>(ydims);

            kernel::iir(y, c, a, h_a0);
            return y;
        } catch (cl::Error &err) {
            CL_TO_AF_ERROR(err);
        }
    }

#define INSTANTIATE(T)                          \
    template Array<T> iir(const Array<T> &b,    \
                          const Array<T> &a,    \
                          const Array<T> &x);   \

    INSTANTIATE(float)
    INSTANTIATE(double)
    INSTANTIATE(cfloat)
    INSTANTIATE(cdouble)
}
