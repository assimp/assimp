//
//   Copyright 2022 Pixar
//
//   Licensed under the Apache License, Version 2.0 (the "Apache License")
//   with the following modification; you may not use this file except in
//   compliance with the Apache License and the following modification to it:
//   Section 6. Trademarks. is deleted and replaced with:
//
//   6. Trademarks. This License does not grant permission to use the trade
//      names, trademarks, service marks, or product names of the Licensor
//      and its affiliates, except as required to comply with Section 4(c) of
//      the License and to reproduce the content of the NOTICE file.
//
//   You may obtain a copy of the Apache License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the Apache License with the above modification is
//   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//   KIND, either express or implied. See the Apache License for the specific
//   language governing permissions and limitations under the Apache License.
//

#ifndef OPENSUBDIV3_BFR_POINT_OPERATIONS_H
#define OPENSUBDIV3_BFR_POINT_OPERATIONS_H

#include "../version.h"

#include <cstring>
#include <cassert>
#include <cstdio>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Bfr {

//
//  Internal utilities for efficiently dealing with single and multiple
//  floating point tuples, i.e. "points":
//
namespace points {

//
//  Simple classes for primitive point operations -- to be specialized for
//  small, fixed sizes via template <int SIZE>.
//
//  These class templates with static methods are partially specialized for
//  SIZE. The copy operation is made a separate class due to its additional
//  template parameters (different precision for source and destination).
//  Combining the two can lead to undesired ambiguities with the desired
//  partial specializations.
//
template <typename REAL, int SIZE>
struct PointBuilder {
    static void Set(REAL pDst[], REAL w, REAL const pSrc[], int size) {
        for (int i = 0; i < size; ++i) {
            pDst[i] = w * pSrc[i];
        }
    }
    static void Add(REAL pDst[], REAL w, REAL const pSrc[], int size) {
        for (int i = 0; i < size; ++i) {
            pDst[i] += w * pSrc[i];
        }
    }
};
template <typename REAL_DST, typename REAL_SRC, int SIZE = 0>
struct PointCopier {
    static void Copy(REAL_DST * pDst, REAL_SRC const * pSrc, int size) {
        for (int i = 0; i < size; ++i) {
            pDst[i] = (REAL_DST) pSrc[i];
        }
    }
};

//  Specialization for SIZE = 1:
template <typename REAL>
struct PointBuilder<REAL, 1> {
    static void Set(REAL * pDst, REAL w, REAL const * pSrc, int) {
        pDst[0] = w * pSrc[0];
    }
    static void Add(REAL * pDst, REAL w, REAL const * pSrc, int) {
        pDst[0] += w * pSrc[0];
    }
};
template <typename REAL>
struct PointCopier<REAL, REAL, 1> {
    static void Copy(REAL * pDst, REAL const * pSrc, int) {
        pDst[0] = pSrc[0];
    }
};

//  Specialization for SIZE = 2:
template <typename REAL>
struct PointBuilder<REAL, 2> {
    static void Set(REAL * pDst, REAL w, REAL const * pSrc, int) {
        pDst[0] = w * pSrc[0];
        pDst[1] = w * pSrc[1];
    }
    static void Add(REAL * pDst, REAL w, REAL const * pSrc, int) {
        pDst[0] += w * pSrc[0];
        pDst[1] += w * pSrc[1];
    }
};
template <typename REAL>
struct PointCopier<REAL, REAL, 2> {
    static void Copy(REAL * pDst, REAL const * pSrc, int) {
        pDst[0] = pSrc[0];
        pDst[1] = pSrc[1];
    }
};

//  Specialization for SIZE = 3:
template <typename REAL>
struct PointBuilder<REAL, 3> {
    static void Set(REAL * pDst, REAL w, REAL const * pSrc, int) {
        pDst[0] = w * pSrc[0];
        pDst[1] = w * pSrc[1];
        pDst[2] = w * pSrc[2];
    }
    static void Add(REAL * pDst, REAL w, REAL const * pSrc, int) {
        pDst[0] += w * pSrc[0];
        pDst[1] += w * pSrc[1];
        pDst[2] += w * pSrc[2];
    }
};
template <typename REAL>
struct PointCopier<REAL, REAL, 3> {
    static void Copy(REAL * pDst, REAL const * pSrc, int) {
        pDst[0] = pSrc[0];
        pDst[1] = pSrc[1];
        pDst[2] = pSrc[2];
    }
};

//  Specialization for SIZE = 4:
template <typename REAL>
struct PointBuilder<REAL, 4> {
    static void Set(REAL * pDst, REAL w, REAL const * pSrc, int) {
        pDst[0] = w * pSrc[0];
        pDst[1] = w * pSrc[1];
        pDst[2] = w * pSrc[2];
        pDst[3] = w * pSrc[3];
    }
    static void Add(REAL * pDst, REAL w, REAL const * pSrc, int) {
        pDst[0] += w * pSrc[0];
        pDst[1] += w * pSrc[1];
        pDst[2] += w * pSrc[2];
        pDst[3] += w * pSrc[3];
    }
};
template <typename REAL>
struct PointCopier<REAL, REAL, 4> {
    static void Copy(REAL * pDst, REAL const * pSrc, int) {
        pDst[0] = pSrc[0];
        pDst[1] = pSrc[1];
        pDst[2] = pSrc[2];
        pDst[3] = pSrc[3];
    }
};

//  Additional specialization for copy when precision matches:
template <typename REAL, int SIZE>
struct PointCopier<REAL, REAL, SIZE> {
    static void Copy(REAL * pDst, REAL const * pSrc, int size) {
        std::memcpy(pDst, pSrc, size * sizeof(REAL));
    }
};


//
//  Each major operation is encapsulated in a separate class consisting of
//  the following:
//
//      - a struct containing all parameters of the operation
//      - a single private method with a generic implementing for all SIZEs
//      - a public method invoking specializations for small SIZEs
//
//  Further specializations of the private implementation of each operation
//  are possible.
//

//
//  The Parameters for the operations vary slightly (based on the operation
//  and potentially varying data destinations) but generally consist of the
//  following:
//
//      - a description of a full set of input points involved, including:
//          - the data, size and stride of the array of points
//
//      - a description of the subset of the input points used, including
//          - the number and optional indices for each "source" point
//
//      - a description of the resulting points, including:
//          - the number of resulting points
//          - an array of locations or single location for resulting points
//          - an array of or consecutive weights for all resulting points
//
//  Several operations combining control points for patch evaluation make
//  use of the same parameters, so these are encapsulated in a common set.
//  All classes are expected to declare Parameters for their operation --
//  even if it is simply a typedef for the set of common parameters.
//
//  Common set of parameters for operations combining points:
template <typename REAL>
struct CommonCombinationParameters {
    REAL const * pointData;
    int          pointSize;
    int          pointStride;

    int const * srcIndices;
    int         srcCount;

    int                  resultCount;
    REAL              ** resultArray;
    REAL const * const * weightArray;
};

//
//  Combination of source points into a single result (for use computing
//  position only, applying single stencils, and other purposes):
//
template <typename REAL>
class Combine1 {
public:
    typedef CommonCombinationParameters<REAL> Parameters;

private:
    template <int SIZE = 0>
    static void apply(Parameters const & args) {
        typedef struct PointBuilder<REAL,SIZE> Point;

        int pSize   = args.pointSize;
        int pStride = args.pointStride;

        REAL const * w = args.weightArray[0];
        REAL       * p = args.resultArray[0];

        if (args.srcIndices == 0) {
            REAL const * pSrc = args.pointData;
            Point::Set(p, w[0], pSrc, pSize);

            for (int i = 1; i < args.srcCount; ++i) {
                pSrc += pStride;
                Point::Add(p, w[i], pSrc, pSize);
            }
        } else {
            REAL const * pSrc = args.pointData + pStride * args.srcIndices[0];
            Point::Set(p, w[0], pSrc, pSize);

            for (int i = 1; i < args.srcCount; ++i) {
                pSrc = args.pointData + pStride * args.srcIndices[i];
                Point::Add(p, w[i], pSrc, pSize);
            }
        }
    }

public:
    static void Apply(Parameters const & parameters) {
        switch (parameters.pointSize) {
        case 1:  apply<1>(parameters); break;
        case 2:  apply<2>(parameters); break;
        case 3:  apply<3>(parameters); break;
        case 4:  apply<4>(parameters); break;
        default: apply<>(parameters); break;
        }
    }
};

//
//  Combination of source points into three results (for use computing
//  position and 1st derivatives):
//
template <typename REAL>
class Combine3 {
public:
    typedef CommonCombinationParameters<REAL> Parameters;

 private:
    template <int SIZE = 0>
    static void apply(Parameters const & args) {
        typedef struct PointBuilder<REAL,SIZE> Point;

        int pSize   = args.pointSize;
        int pStride = args.pointStride;

        REAL const * const * wArray = args.weightArray;
        REAL              ** pArray = args.resultArray;

        //
        //  Apply each successive control point to all derivatives at once,
        //  rather than computing each derivate independently:
        //
        REAL const * pSrc = (args.srcIndices == 0) ? args.pointData :
                            (args.pointData + pStride * args.srcIndices[0]);
        Point::Set(pArray[0], wArray[0][0], pSrc, pSize);
        Point::Set(pArray[1], wArray[1][0], pSrc, pSize);
        Point::Set(pArray[2], wArray[2][0], pSrc, pSize);

        for (int i = 1; i < args.srcCount; ++i) {
            pSrc = (args.srcIndices == 0) ? (pSrc + pStride) :
                   (args.pointData + pStride * args.srcIndices[i]);
            Point::Add(pArray[0], wArray[0][i], pSrc, pSize);
            Point::Add(pArray[1], wArray[1][i], pSrc, pSize);
            Point::Add(pArray[2], wArray[2][i], pSrc, pSize);
        }
    }

public:
    static void Apply(Parameters const & parameters) {
        switch (parameters.pointSize) {
        case 1:  apply<1>(parameters); break;
        case 2:  apply<2>(parameters); break;
        case 3:  apply<3>(parameters); break;
        case 4:  apply<4>(parameters); break;
        default: apply<>(parameters); break;
        }
    }
};

//
//  Combination of source points into an arbitrary array of results (for
//  use computing position with all derivatives, i.e. 6 results):
//
template <typename REAL>
class CombineMultiple {
public:
    typedef CommonCombinationParameters<REAL> Parameters;

private:
    template <int SIZE = 0>
    static void
    apply(Parameters const & args) {
        typedef struct PointBuilder<REAL,SIZE> Point;

        int pSize   = args.pointSize;
        int pStride = args.pointStride;

        REAL const * const * wArray = args.weightArray;
        REAL              ** pArray = args.resultArray;

        //
        //  Apply each successive control point to all derivatives at once,
        //  rather than computing each derivate independently:
        //
        REAL const * pSrc = (args.srcIndices == 0) ? args.pointData :
                            (args.pointData + pStride * args.srcIndices[0]);

        for (int j = 0; j < args.resultCount; ++j) {
            Point::Set(pArray[j], wArray[j][0], pSrc, pSize);
        }

        for (int i = 1; i < args.srcCount; ++i) {
            pSrc = (args.srcIndices == 0) ? (pSrc + pStride) :
                   (args.pointData + pStride * args.srcIndices[i]);

            for (int j = 0; j < args.resultCount; ++j) {
                Point::Add(pArray[j], wArray[j][i], pSrc, pSize);
            }
        }
    }

public:
    static void
    Apply(Parameters const & parameters) {
        switch (parameters.pointSize) {
        case 1:  apply<1>(parameters); break;
        case 2:  apply<2>(parameters); break;
        case 3:  apply<3>(parameters); break;
        case 4:  apply<4>(parameters); break;
        default: apply<>(parameters); break;
        }
    }
};

//
//  Combination of a subset of N input points into M resulting points
//  in consecutive memory locations. The weights for the resulting M
//  points (N for the input points contributing to each result) are
//  also stored consecutively:
//
template <typename REAL>
class CombineConsecutive {
public:
    struct Parameters {
        REAL const * pointData;
        int          pointSize;
        int          pointStride;

        int srcCount;

        int          resultCount;
        REAL       * resultData;
        REAL const * weightData;
    };

private:
    template <int SIZE = 0>
    static void
    apply(Parameters const & args) {
        typedef struct PointBuilder<REAL,SIZE> Point;

        REAL const * w = args.weightData;
        REAL       * p = args.resultData;

        for (int i = 0; i < args.resultCount; ++i) {
            REAL const * pSrc = args.pointData;
            Point::Set(p, w[0], pSrc, args.pointSize);

            for (int j = 1; j < args.srcCount; ++j) {
                pSrc += args.pointStride;
                Point::Add(p, w[j], pSrc, args.pointSize);
            }

            p += args.pointStride;
            w += args.srcCount;
        }
    }

public:
    static void
    Apply(Parameters const & parameters) {
        switch (parameters.pointSize) {
        case 1:  apply<1>(parameters); break;
        case 2:  apply<2>(parameters); break;
        case 3:  apply<3>(parameters); break;
        case 4:  apply<4>(parameters); break;
        default: apply<>(parameters); break;
        }
    }
};

//
//  Split the N-sided face formed by the N input control points, i.e.
//  compute the midpoint of the face and the midpoint of each edge --
//  to be stored consecutively in the given location for results:
//
template <typename REAL>
class SplitFace {
public:
    struct Parameters {
        REAL const * pointData;
        int          pointSize;
        int          pointStride;

        int srcCount;

        REAL * resultData;
    };

private:
    template <int SIZE = 0>
    static void apply(Parameters const & args) {
        typedef struct PointBuilder<REAL,SIZE> Point;

        int N = args.srcCount;
        REAL invN = 1.0f / (REAL) N;

        REAL * facePoint = args.resultData;
        std::memset(facePoint, 0, args.pointSize * sizeof(REAL));

        for (int i = 0; i < N; ++i) {
            int j = (i < (N - 1)) ? (i + 1) : 0;

            REAL const * pi = args.pointData + args.pointStride * i;
            REAL const * pj = args.pointData + args.pointStride * j;

            Point::Add(facePoint, invN, pi, args.pointSize);

            REAL * edgePoint = args.resultData + args.pointStride * (1 + i);
            Point::Set(edgePoint, 0.5f, pi, args.pointSize);
            Point::Add(edgePoint, 0.5f, pj, args.pointSize);
        }
    }

public:
    static void Apply(Parameters const & parameters) {

        switch (parameters.pointSize) {
        case 1:  apply<1>(parameters); break;
        case 2:  apply<2>(parameters); break;
        case 3:  apply<3>(parameters); break;
        case 4:  apply<4>(parameters); break;
        default: apply<>(parameters); break;
        }
    }
};

//
//  Copy a subset of N input points -- identified by the indices given for
//  each -- to the resulting location specified:
//
template <typename REAL_DST, typename REAL_SRC>
class CopyConsecutive {
public:
    struct Parameters {
        REAL_SRC const * pointData;
        int              pointSize;
        int              pointStride;

        int const * srcIndices;
        int         srcCount;

        REAL_DST * resultData;
        int        resultStride;
    };

private:
    template <int SIZE = 0>
    static void apply(Parameters const & args) {
        typedef struct PointCopier<REAL_DST,REAL_SRC,SIZE> Point;

        for (int i = 0; i < args.srcCount; ++i) {
            REAL_DST       * pDst = args.resultData + args.resultStride * i;
            REAL_SRC const * pSrc = args.pointData +
                                    args.pointStride * args.srcIndices[i];

            Point::Copy(pDst, pSrc, args.pointSize);
        }
    }

public:
    static void Apply(Parameters const & parameters) {

        switch (parameters.pointSize) {
        case 1:  apply<1>(parameters); break;
        case 2:  apply<2>(parameters); break;
        case 3:  apply<3>(parameters); break;
        case 4:  apply<4>(parameters); break;
        default: apply<>(parameters); break;
        }
    }
};

} // end namespace points

} // end namespace Bfr

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv

#endif /* OPENSUBDIV3_BFR_POINT_OPERATIONS_H */
