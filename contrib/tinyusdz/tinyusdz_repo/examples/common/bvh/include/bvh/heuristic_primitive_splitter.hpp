#ifndef BVH_HEURISTIC_PRIMITIVE_SPLITTER_HPP
#define BVH_HEURISTIC_PRIMITIVE_SPLITTER_HPP

#include <algorithm>
#include <optional>
#include <stack>

#include "bvh/bvh.hpp"
#include "bvh/bounding_box.hpp"
#include "bvh/prefix_sum.hpp"

namespace bvh {

/// Heuristic-based primitive splitter, inspired by the algorithm described in:
/// "Fast Parallel Construction of High-Quality Bounding Volume Hierarchies",
/// by T. Karras and T. Aila.
template <typename Primitive>
class HeuristicPrimitiveSplitter {
    using Scalar = typename Primitive::ScalarType;
    using IndexType = typename Bvh<Scalar>::IndexType;

    std::unique_ptr<size_t[]> original_indices;
    PrefixSum<size_t> prefix_sum;

    /// Returns the splitting priority of a primitive.
    static Scalar compute_priority(const Primitive& primitive, const BoundingBox<Scalar>& bbox) {
        // This is inspired from the priority function in the original paper,
        // except that the expression 2^i has been replaced by the largest
        // extent of the bounding box, which is similar in nature.
        return std::cbrt(bbox.largest_extent() * (Scalar(2) * bbox.half_area() - primitive.area()));
    }

public:
    /// Performs triangle splitting on the given array of triangles.
    /// It returns the number of triangles after splitting.
    std::tuple<size_t, std::unique_ptr<BoundingBox<Scalar>[]>, std::unique_ptr<Vector3<Scalar>[]>>
    split(
        const BoundingBox<Scalar>& global_bbox,
        const Primitive* primitives,
        size_t primitive_count,
        Scalar split_factor = Scalar(0.5))
    {
        auto split_indices = std::make_unique<size_t[]>(primitive_count);

        std::unique_ptr<BoundingBox<Scalar>[]> bboxes;
        std::unique_ptr<Vector3<Scalar>[]> centers;

        Scalar total_priority = 0;
        size_t reference_count = 0;

        #pragma omp parallel
        {
            #pragma omp for reduction(+: total_priority)
            for (size_t i = 0; i < primitive_count; ++i)
                total_priority += compute_priority(primitives[i], primitives[i].bounding_box());

            #pragma omp for
            for (size_t i = 0; i < primitive_count; ++i) {
                auto priority = compute_priority(primitives[i], primitives[i].bounding_box());
                split_indices[i] = 1 + static_cast<size_t>(priority *
                    (static_cast<Scalar>(primitive_count) * split_factor / total_priority));
            }

            prefix_sum.sum_in_parallel(split_indices.get(), split_indices.get(), primitive_count);

            #pragma omp single
            {
                reference_count = split_indices[primitive_count - 1];
                bboxes = std::make_unique<BoundingBox<Scalar>[]>(reference_count);
                centers = std::make_unique<Vector3<Scalar>[]>(reference_count);
                original_indices = std::make_unique<size_t[]>(reference_count);
            }

            std::stack<std::pair<BoundingBox<Scalar>, size_t>> stack;

            #pragma omp for
            for (size_t i = 0; i < primitive_count; ++i) {
                size_t split_begin = i > 0 ? split_indices[i - 1] : 0;
                size_t split_count = split_indices[i] - split_begin;

                // Use the primitive's center instead of the bounding box
                // center if the primitive is not split.
                if (split_count == 1) {
                    bboxes[split_begin]  = primitives[i].bounding_box();
                    centers[split_begin] = primitives[i].center();
                    original_indices[split_begin] = i;
                    continue;
                }

                // Split this primitive
                size_t j = split_begin;
                stack.emplace(primitives[i].bounding_box(), split_count);
                while (!stack.empty()) {
                    auto [bbox, count] = stack.top();
                    stack.pop();

                    if (count == 1) {
                        bboxes[j]  = bbox;
                        centers[j] = bbox.center();
                        original_indices[j] = i;
                        j++;
                        continue;
                    }

                    auto axis = bbox.largest_axis();

                    // Find the split depth (i.e. a power of 2 grid size)
                    auto depth = std::min(Scalar(-1), std::floor(std::log2(bbox.largest_extent() / global_bbox.diagonal()[axis])));
                    auto cell_size = std::exp2(depth) * global_bbox.diagonal()[axis];
                    if (cell_size >= bbox.largest_extent())
                        cell_size *= Scalar(0.5);

                    // Compute the split position
                    auto mid_pos   = (bbox.min[axis] + bbox.max[axis]) * Scalar(0.5);
                    auto split_pos = global_bbox.min[axis] + std::round((mid_pos - global_bbox.min[axis]) / cell_size) * cell_size;
                    if (split_pos < bbox.min[axis] || split_pos > bbox.max[axis]) {
                        // Should only happen very rarely because of floating-point errors
                        split_pos = mid_pos;
                    }

                    // Split the primitive and process fragments
                    auto [left_bbox, right_bbox] = primitives[i].split(axis, split_pos);
                    left_bbox.shrink(bbox);
                    right_bbox.shrink(bbox);

                    auto left_extent  = left_bbox.largest_extent();
                    auto right_extent = right_bbox.largest_extent();
                    size_t left_count = static_cast<size_t>(
                        static_cast<Scalar>(count) * left_extent / (right_extent + left_extent));
                    left_count = std::max(size_t(1), std::min(count - 1, left_count));

                    stack.emplace(left_bbox, left_count);
                    stack.emplace(right_bbox, count - left_count);
                }
            }
        }

        return std::make_tuple(reference_count, std::move(bboxes), std::move(centers));
    }

    /// Remaps BVH primitive indices and removes duplicate triangle references in the BVH leaves.
    void repair_bvh_leaves(Bvh<Scalar>& bvh) {
        #pragma omp parallel for
        for (size_t i = 0; i < bvh.node_count; ++i) {
            auto& node = bvh.nodes[i];
            if (node.is_leaf()) {
                auto begin = bvh.primitive_indices.get() + node.first_child_or_primitive;
                auto end   = begin + node.primitive_count;
                std::transform(begin, end, begin, [&] (size_t i) { return original_indices[i]; });
                std::sort(begin, end);
                node.primitive_count = static_cast<IndexType>(std::unique(begin, end) - begin);
            }
        }
    }
};

} // namespace bvh

#endif
