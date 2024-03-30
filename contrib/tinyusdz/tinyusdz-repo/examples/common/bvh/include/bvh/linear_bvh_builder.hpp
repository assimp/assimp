#ifndef BVH_LINEAR_BVH_BUILDER_HPP
#define BVH_LINEAR_BVH_BUILDER_HPP

#include <cstdint>
#include <cassert>
#include <numeric>

#include "bvh/morton_code_based_builder.hpp"
#include "bvh/prefix_sum.hpp"

namespace bvh {

/// Bottom-up BVH builder that uses Morton codes to create the hierarchy.
/// This implementation is vaguely inspired from the original LBVH publication:
/// "Fast BVH Construction on GPUs", by C. Lauterbach et al.
template <typename Bvh, typename Morton>
class LinearBvhBuilder : public MortonCodeBasedBuilder<Bvh, Morton> {
    using Scalar = typename Bvh::ScalarType;
    using IndexType = typename Bvh::IndexType;

    using ParentBuilder = MortonCodeBasedBuilder<Bvh, Morton>;
    using ParentBuilder::sort_primitives_by_morton_code;

    using Level = typename SizedIntegerType<round_up_log2(sizeof(Morton) * CHAR_BIT + 1)>::Unsigned;
    using Node  = typename Bvh::Node;

    Bvh& bvh;

    PrefixSum<size_t> prefix_sum;

    std::pair<size_t, size_t> merge(
        const Node* bvh_restrict input_nodes,
        Node* bvh_restrict output_nodes,
        const Level* bvh_restrict input_levels,
        Level* bvh_restrict output_levels,
        size_t* bvh_restrict merged_index,
        size_t* bvh_restrict needs_merge,
        size_t begin, size_t end,
        size_t previous_end)
    {
        size_t next_begin = 0;
        size_t next_end   = 0;

        merged_index[end - 1] = 0;
        needs_merge [end - 1] = 0;

        #pragma omp parallel if (end - begin > loop_parallel_threshold)
        {
            // Determine, for each node, if it should be merged with the one on the right.
            #pragma omp for
            for (size_t i = begin; i < end - 1; ++i)
                needs_merge[i] = input_levels[i] >= input_levels[i + 1] && (i == begin || input_levels[i] >= input_levels[i - 1]);

            // Resolve conflicts between nodes that want to be merged with different neighbors.
            #pragma omp for
            for (size_t i = begin; i < end - 1; i += 2) {
                if (needs_merge[i] && needs_merge[i + 1])
                    needs_merge[i] = 0;
            }
            #pragma omp for
            for (size_t i = begin + 1; i < end - 1; i += 2) {
                if (needs_merge[i] && needs_merge[i + 1])
                    needs_merge[i] = 0;
            }

            // Perform a prefix sum to compute the insertion indices
            prefix_sum.sum_in_parallel(needs_merge + begin, merged_index + begin, end - begin);
            size_t merged_count   = merged_index[end - 1];
            size_t unmerged_count = end - begin - merged_count;
            size_t children_count = merged_count * 2;
            size_t children_begin = end - children_count;
            size_t unmerged_begin = end - (children_count + unmerged_count);

            #pragma omp single nowait
            {
                next_begin = unmerged_begin;
                next_end   = children_begin;
            }

            // Perform one step of node merging
            #pragma omp for nowait
            for (size_t i = begin; i < end; ++i) {
                if (needs_merge[i]) {
                    size_t unmerged_index = unmerged_begin + i + 1 - begin - merged_index[i];
                    auto& unmerged_node = output_nodes[unmerged_index];
                    auto first_child = children_begin + (merged_index[i] - 1) * 2;
                    unmerged_node.bounding_box_proxy() = input_nodes[i]
                        .bounding_box_proxy()
                        .to_bounding_box()
                        .extend(input_nodes[i + 1].bounding_box_proxy());
                    unmerged_node.primitive_count = 0;
                    unmerged_node.first_child_or_primitive = static_cast<IndexType>(first_child);
                    output_nodes[first_child + 0] = input_nodes[i + 0];
                    output_nodes[first_child + 1] = input_nodes[i + 1];
                    output_levels[unmerged_index] = input_levels[i + 1];
                } else if (i == begin || !needs_merge[i - 1]) {
                    size_t unmerged_index = unmerged_begin + i - begin - merged_index[i];
                    output_nodes [unmerged_index] = input_nodes[i];
                    output_levels[unmerged_index] = input_levels[i];
                }
            }

            // Copy the nodes of the previous level into the current array of nodes.
            #pragma omp for nowait
            for (size_t i = end; i < previous_end; ++i)
                output_nodes[i] = input_nodes[i];
        }

        return std::make_pair(next_begin, next_end);
    }

public:
    using ParentBuilder::loop_parallel_threshold;

    LinearBvhBuilder(Bvh& bvh)
        : bvh(bvh)
    {}

    void build(
        const BoundingBox<Scalar>& global_bbox,
        const BoundingBox<Scalar>* bboxes,
        const Vector3<Scalar>* centers,
        size_t primitive_count)
    {
        assert(primitive_count > 0);

        std::unique_ptr<size_t[]> primitive_indices;
        std::unique_ptr<Morton[]> morton_codes;

        std::tie(primitive_indices, morton_codes) =
            sort_primitives_by_morton_code(global_bbox, centers, primitive_count);

        auto node_count = 2 * primitive_count - 1;

        auto nodes          = std::make_unique<Node[]>(node_count);
        auto nodes_copy     = std::make_unique<Node[]>(node_count);
        auto auxiliary_data = std::make_unique<size_t[]>(node_count * 2);
        auto level_data     = std::make_unique<Level[]>(node_count * 2);

        size_t begin        = node_count - primitive_count;
        size_t end          = node_count;
        size_t previous_end = end;

        auto input_levels  = level_data.get();
        auto output_levels = level_data.get() + node_count;

        #pragma omp parallel if (primitive_count > loop_parallel_threshold)
        {
            // Create the leaves
            #pragma omp for nowait
            for (size_t i = 0; i < primitive_count; ++i) {
                auto& node = nodes[begin + i];
                node.bounding_box_proxy()     = bboxes[primitive_indices[i]];
                node.primitive_count          = 1;
                node.first_child_or_primitive = static_cast<IndexType>(i);
            }

            // Compute the level of the tree where the current node is joined with the next.
            #pragma omp for nowait
            for (size_t i = 0; i < primitive_count - 1; ++i)
                input_levels[begin + i] = static_cast<Level>(count_leading_zeros(morton_codes[i] ^ morton_codes[i + 1]));
        }

        while (end - begin > 1) {
            auto [next_begin, next_end] = merge(
                nodes.get(),
                nodes_copy.get(),
                input_levels,
                output_levels,
                auxiliary_data.get(),
                auxiliary_data.get() + node_count,
                begin, end,
                previous_end);

            std::swap(nodes, nodes_copy);
            std::swap(input_levels, output_levels);

            previous_end = end;
            begin        = next_begin;
            end          = next_end;
        }

        std::swap(bvh.nodes, nodes);
        std::swap(bvh.primitive_indices, primitive_indices);
        bvh.node_count = node_count;
    }
};

} // namespace bvh

#endif
