#ifndef BVH_NODE_LAYOUT_OPTIMIZER_HPP
#define BVH_NODE_LAYOUT_OPTIMIZER_HPP

#include <memory>

#include "bvh/bvh.hpp"
#include "bvh/utilities.hpp"
#include "bvh/radix_sort.hpp"

namespace bvh {

/// Optimizes the layout of BVH nodes so that the nodes with
/// the highest area are closer to the beginning of the array
/// of nodes. This does not change the topology of the BVH;
/// only the memory layout of the nodes is affected.
template <typename Bvh>
class NodeLayoutOptimizer {
    using Scalar = typename Bvh::ScalarType;
    using Key    = typename SizedIntegerType<sizeof(Scalar) * CHAR_BIT>::Unsigned;

    RadixSort<8> radix_sort;

    Bvh& bvh;

public:
    NodeLayoutOptimizer(Bvh& bvh)
        : bvh(bvh)
    {}

    void optimize() {
        size_t pair_count = (bvh.node_count - 1) / 2;
        auto keys         = std::make_unique<Key[]>(pair_count * 2);
        auto indices      = std::make_unique<size_t[]>(pair_count * 2);
        auto nodes_copy   = std::make_unique<typename Bvh::Node[]>(bvh.node_count);
        nodes_copy[0] = bvh.nodes[0];

        auto sorted_indices   = indices.get();
        auto unsorted_indices = indices.get() + pair_count;
        auto sorted_keys      = keys.get();
        auto unsorted_keys    = keys.get() + pair_count;

        #pragma omp parallel
        {
            // Compute the surface area of each pair of nodes
            #pragma omp for
            for (size_t i = 1; i < bvh.node_count; i += 2) {
                auto area = bvh.nodes[i + 0]
                    .bounding_box_proxy()
                    .to_bounding_box()
                    .extend(bvh.nodes[i + 1].bounding_box_proxy())
                    .half_area();
                size_t j = (i - 1) / 2;
                keys[j]    = as<Key>(area);
                indices[j] = j;
            }

            // Sort pairs of nodes by area. This can be done with a
            // standard radix sort that interprets the floating point
            // data as integers, because the area is positive, and
            // positive floating point numbers can be compared like
            // integers (mandated by IEEE-754).
            radix_sort.sort_in_parallel(
                sorted_keys,
                unsorted_keys,
                sorted_indices,
                unsorted_indices,
                pair_count,
                sizeof(Scalar) * CHAR_BIT);

            // Copy the nodes of the old layout into the new one
            #pragma omp for
            for (size_t i = 0; i < pair_count; ++i) {
                auto j = sorted_indices[pair_count - i - 1];
                auto k = 1 + j * 2;
                auto l = 1 + i * 2;
                nodes_copy[l + 0] = bvh.nodes[k + 0];
                nodes_copy[l + 1] = bvh.nodes[k + 1];
                unsorted_indices[j] = l;
            }

            // Remap children indices to the new layout
            #pragma omp for
            for (size_t i = 0; i < bvh.node_count; ++i) {
                if (nodes_copy[i].is_leaf())
                    continue;
                nodes_copy[i].first_child_or_primitive = static_cast<typename Bvh::IndexType>(
                    unsorted_indices[(nodes_copy[i].first_child_or_primitive - 1) / 2]);
            }
        }

        std::swap(nodes_copy, bvh.nodes);
    }
};

} // namespace bvh

#endif
