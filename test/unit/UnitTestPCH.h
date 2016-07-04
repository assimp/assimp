

// #ifndef ASSIMP_BUILD_SINGLETHREADED
// #    include <boost/thread.hpp>
// #endif

// We need to be sure to have the same STL settings as Assimp

#include <assimp/cimport.h>

#include <gtest/gtest.h>

#include <math.h>

template<class T>
struct TDataArray {
    size_t m_numItems;
    T *m_items;

    TDataArray( size_t numItems )
    : m_numItems( numItems )
    , m_items( nullptr ) {
        m_items = new T[ numItems ];
    }

    ~TDataArray() {
        delete [] m_items;
    }

    T operator [] ( size_t index ) const {
        EXPECT_TRUE( index < m_numItems );
        return m_items[ index ];
    }
};

#undef min
#undef max
