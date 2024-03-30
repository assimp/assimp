#ifndef BVH_MORTON_CODE_BASED_BUILDER_HPP
#define BVH_MORTON_CODE_BASED_BUILDER_HPP

#include <algorithm>
#include <memory>
#include <cassert>
#include <type_traits>

#include "bvh/bounding_box.hpp"
#include "bvh/vector.hpp"
#include "bvh/morton.hpp"
#include "bvh/radix_sort.hpp"

namespace bvh {

template <typename Bvh, typename Morton>
class MortonCodeBasedBuilder {
    using Scalar = typename Bvh::ScalarType;

    /// Number of bits processed by every iteration of the radix sort.
    static constexpr size_t bits_per_iteration = 10;

public:
    static_assert(std::is_unsigned_v<Morton>);
    using MortonType = Morton;

    /// Maximum number of bits available per dimension.
    static constexpr size_t max_bit_count = (sizeof(Morton) * CHAR_BIT) / 3;

    /// Number of bits to use per dimension.
    size_t bit_count = max_bit_count;

    /// Threshold (number of nodes) under which the loops execute serially.
    size_t loop_parallel_threshold = 256;

protected:
    using SortedPairs = std::pair<std::unique_ptr<size_t[]>, std::unique_ptr<Morton[]>>;

    RadixSort<bits_per_iteration> radix_sort;

    ~MortonCodeBasedBuilder() {}

    SortedPairs sort_primitives_by_morton_code(
        const BoundingBox<Scalar>& global_bbox,
        const Vector3<Scalar>* centers,
        size_t primitive_count)
    {
        assert(bit_count <= max_bit_count);
        auto morton_codes           = std::make_unique<Morton[]>(primitive_count);
        auto morton_codes_copy      = std::make_unique<Morton[]>(primitive_count);
        auto primitive_indices      = std::make_unique<size_t[]>(primitive_count);
        auto primitive_indices_copy = std::make_unique<size_t[]>(primitive_count);

        Morton* sorted_morton_codes        = morton_codes.get();
        size_t* sorted_primitive_indices   = primitive_indices.get();
        Morton* unsorted_morton_codes      = morton_codes_copy.get();
        size_t* unsorted_primitive_indices = primitive_indices_copy.get();

        MortonEncoder<Morton, Scalar> encoder(global_bbox, size_t(1) << bit_count);

        #pragma omp parallel if (primitive_count > loop_parallel_threshold)
        {
            #pragma omp for
            for (size_t i = 0; i < primitive_count; ++i) {
                morton_codes[i] = encoder.encode(centers[i]);
                primitive_indices[i] = i;
            }

            // Sort primitives by morton code
            radix_sort.sort_in_parallel(
                sorted_morton_codes,
                unsorted_morton_codes,
                sorted_primitive_indices,
                unsorted_primitive_indices,
                primitive_count, bit_count * 3);
        }

        if (sorted_morton_codes != morton_codes.get()) {
            std::swap(morton_codes, morton_codes_copy);
            std::swap(primitive_indices, primitive_indices_copy);
        }

        assert(std::is_sorted(morton_codes.get(), morton_codes.get() + primitive_count));
        return std::make_pair(std::move(primitive_indices), std::move(morton_codes));
    }
};

} // namespace bvh

#endif
