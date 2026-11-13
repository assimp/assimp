/*
 * Shared overflow safety helpers for Jassimp.
 * This header is intentionally JNI-free so it can be used by unit tests.
 */
#ifndef ASSIMP_PORT_JASSIMP_JASSIMP_OVERFLOW_H_INCLUDED
#define ASSIMP_PORT_JASSIMP_JASSIMP_OVERFLOW_H_INCLUDED

#include <assimp/scene.h>
#include <cstddef>
#include <limits>

namespace jassimp {

inline bool safeMultiplySize(size_t a, size_t b, size_t &result)
{
    if (b != 0 && a > std::numeric_limits<size_t>::max() / b)
    {
        return false;
    }

    result = a * b;
    return true;
}

inline bool safeAddSize(size_t a, size_t b, size_t &result)
{
    if (a > std::numeric_limits<size_t>::max() - b)
    {
        return false;
    }

    result = a + b;
    return true;
}

inline bool fitsInJavaInt(size_t value)
{
    return value <= static_cast<size_t>(std::numeric_limits<int>::max());
}

inline bool computeFaceBufferSize(const aiMesh* cMesh, size_t &faceBufferSize)
{
    const size_t integerSize = sizeof(unsigned int);

    const bool isPureTriangle = cMesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE;
    if (isPureTriangle)
    {
        size_t faceCountTimes3;
        if (!safeMultiplySize(cMesh->mNumFaces, 3u, faceCountTimes3) ||
            !safeMultiplySize(faceCountTimes3, integerSize, faceBufferSize))
        {
            return false;
        }
    }
    else
    {
        size_t numVertexReferences = 0;
        for (unsigned int face = 0; face < cMesh->mNumFaces; face++)
        {
            size_t indices = cMesh->mFaces[face].mNumIndices;
            if (!safeAddSize(numVertexReferences, indices, numVertexReferences))
            {
                return false;
            }
        }

        if (!safeMultiplySize(numVertexReferences, integerSize, faceBufferSize))
        {
            return false;
        }
    }

    /* ensure the computed buffer size fits into a Java int (jint) */
    if (!fitsInJavaInt(faceBufferSize)) {
        return false;
    }

    return true;
}

} // namespace jassimp

#endif // ASSIMP_PORT_JASSIMP_JASSIMP_OVERFLOW_H_INCLUDED
