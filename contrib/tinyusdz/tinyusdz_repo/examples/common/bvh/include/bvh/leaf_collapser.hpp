#ifndef BVH_LEAF_COLLAPSER_HPP
#define BVH_LEAF_COLLAPSER_HPP

#include <memory>

#include "bvh/bvh.hpp"
#include "bvh/sah_based_algorithm.hpp"
#include "bvh/bottom_up_algorithm.hpp"
#include "bvh/prefix_sum.hpp"
#include "bvh/platform.hpp"

namespace bvh {

/// Collapses leaves of the BVH according to the SAH. This optimization
/// is only helpful for bottom-up builders, as top-down builders already
/// have a termination criterion that prevents leaf creation when the SAH
/// cost does not improve.
template <typename Bvh>
class LeafCollapser : public SahBasedAlgorithm<Bvh>, public BottomUpAlgorithm<Bvh> {
    using Scalar = typename Bvh::ScalarType;
    using IndexType = typename Bvh::IndexType;

    PrefixSum<size_t> prefix_sum;

    using BottomUpAlgorithm<Bvh>::traverse_in_parallel;
    using BottomUpAlgorithm<Bvh>::parents;
    using BottomUpAlgorithm<Bvh>::bvh;

public:
    using SahBasedAlgorithm<Bvh>::traversal_cost;

    LeafCollapser(Bvh& bvh)
        : BottomUpAlgorithm<Bvh>(bvh)
    {}

    void collapse() {
        if (bvh_unlikely(bvh.nodes[0].is_leaf()))
            return;

        std::unique_ptr<size_t[]> primitive_indices_copy;
        std::unique_ptr<typename Bvh::Node[]> nodes_copy;

        auto node_counts      = std::make_unique<size_t[]>(bvh.node_count);
        auto primitive_counts = std::make_unique<size_t[]>(bvh.node_count);
        size_t node_count = 0;

        #pragma omp parallel
        {
            #pragma omp for
            for (size_t i = 0; i < bvh.node_count; ++i)
                node_counts[i] = 1;

            // Bottom-up traversal to collapse leaves
            traverse_in_parallel(
                [&] (size_t i) { primitive_counts[i] = bvh.nodes[i].primitive_count; },
                [&] (size_t i) {
                    const auto& node = bvh.nodes[i];
                    assert(!node.is_leaf());
                    auto first_child  = node.first_child_or_primitive;

                    auto left_primitive_count  = primitive_counts[first_child + 0];
                    auto right_primitive_count = primitive_counts[first_child + 1];
                    auto total_primitive_count = left_primitive_count + right_primitive_count;

                    // Compute the cost of collapsing this node when both children are leaves
                    if (left_primitive_count > 0 && right_primitive_count > 0) {
                        const auto& left_child  = bvh.nodes[first_child + 0];
                        const auto& right_child = bvh.nodes[first_child + 1];
                        auto collapse_cost =
                            node.bounding_box_proxy().to_bounding_box().half_area() * (Scalar(total_primitive_count) - traversal_cost);
                        auto base_cost =
                            left_child .bounding_box_proxy().to_bounding_box().half_area() * static_cast<Scalar>(left_primitive_count) +
                            right_child.bounding_box_proxy().to_bounding_box().half_area() * static_cast<Scalar>(right_primitive_count);
                        if (collapse_cost <= base_cost) {
                            primitive_counts[i] = total_primitive_count;
                            primitive_counts[first_child + 0] = 0;
                            primitive_counts[first_child + 1] = 0;
                            node_counts[first_child + 0] = 0;
                            node_counts[first_child + 1] = 0;
                        }
                    }
                });

            prefix_sum.sum_in_parallel(node_counts.get(), node_counts.get(), bvh.node_count);
            prefix_sum.sum_in_parallel(primitive_counts.get(), primitive_counts.get(), bvh.node_count);

            #pragma omp single
            {
                node_count = node_counts[bvh.node_count - 1];
                if (primitive_counts[0] > 0) {
                    // This means the root node has become a leaf.
                    // We avoid copying the data and just swap the old primitive array with the new one.
                    bvh.nodes[0].first_child_or_primitive = 0;
                    bvh.nodes[0].primitive_count = static_cast<IndexType>(primitive_counts[0]);
                    std::swap(bvh.primitive_indices, primitive_indices_copy);
                    std::swap(bvh.nodes, nodes_copy);
                    bvh.node_count = 0;
                } else {
                    nodes_copy = std::make_unique<typename Bvh::Node[]>(node_count);
                    primitive_indices_copy = std::make_unique<size_t[]>(primitive_counts[bvh.node_count - 1]);
                    nodes_copy[0] = bvh.nodes[0];
                    nodes_copy[0].first_child_or_primitive = static_cast<IndexType>(
                        node_counts[nodes_copy[0].first_child_or_primitive - 1]);
                }
            }

            #pragma omp for
            for (size_t i = 1; i < bvh.node_count; i++) {
                size_t node_index = node_counts[i - 1];
                if (node_index == node_counts[i])
                    continue;

                nodes_copy[node_index] = bvh.nodes[i];
                size_t first_primitive = primitive_counts[i - 1];
                if (first_primitive != primitive_counts[i]) {
                    nodes_copy[node_index].primitive_count = static_cast<IndexType>(primitive_counts[i] - first_primitive);
                    nodes_copy[node_index].first_child_or_primitive = static_cast<IndexType>(first_primitive);

                    // Top-down traversal to store the primitives contained in this subtree.
                    size_t j = i;
                    while (true) {
                        const auto& node = bvh.nodes[j];
                        if (node.primitive_count != 0) {
                            std::copy(
                                bvh.primitive_indices.get() + node.first_child_or_primitive,
                                bvh.primitive_indices.get() + node.first_child_or_primitive + node.primitive_count,
                                primitive_indices_copy.get() + first_primitive);
                            first_primitive += node.primitive_count;
                            while (!bvh.is_left_sibling(j) && j != i)
                                j = parents[j];
                            if (j == i)
                                break;
                            j = bvh.sibling(j);
                        } else
                            j = node.first_child_or_primitive;
                    }
                    assert(first_primitive == primitive_counts[i]);
                } else {
                    auto& first_child = nodes_copy[node_index].first_child_or_primitive;
                    first_child = static_cast<IndexType>(node_counts[first_child - 1]);
                }
            }
        }

        std::swap(bvh.nodes, nodes_copy);
        std::swap(bvh.primitive_indices, primitive_indices_copy);
        bvh.node_count = node_count;
    }
};

} // namespace bvh

#endif
