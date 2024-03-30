#ifndef BVH_BINNED_SAH_BUILDER_HPP
#define BVH_BINNED_SAH_BUILDER_HPP

#include <optional>

#include "bvh/bvh.hpp"
#include "bvh/bounding_box.hpp"
#include "bvh/top_down_builder.hpp"
#include "bvh/sah_based_algorithm.hpp"

namespace bvh {

template <typename, size_t> class BinnedSahBuildTask;

/// This is a top-down, classic binned SAH BVH builder. It works by approximating
/// the SAH with bins of fixed size at every step of the recursion.
/// See "On fast Construction of SAH-based Bounding Volume Hierarchies",
/// by I. Wald.
template <typename Bvh, size_t BinCount>
class BinnedSahBuilder : public TopDownBuilder, public SahBasedAlgorithm<Bvh> {
    using Scalar    = typename Bvh::ScalarType;
    using BuildTask = BinnedSahBuildTask<Bvh, BinCount>;

    using TopDownBuilder::run_task;

    friend BuildTask;

    Bvh& bvh;

public:
    using TopDownBuilder::max_depth;
    using TopDownBuilder::max_leaf_size;
    using SahBasedAlgorithm<Bvh>::traversal_cost;

    BinnedSahBuilder(Bvh& bvh)
        : bvh(bvh)
    {}

    void build(
        const BoundingBox<Scalar>& global_bbox,
        const BoundingBox<Scalar>* bboxes,
        const Vector3<Scalar>* centers,
        size_t primitive_count)
    {
        assert(primitive_count > 0);

        // Allocate buffers
        bvh.nodes = std::make_unique<typename Bvh::Node[]>(2 * primitive_count + 1);
        bvh.primitive_indices = std::make_unique<size_t[]>(primitive_count);

        bvh.node_count = 1;
        bvh.nodes[0].bounding_box_proxy() = global_bbox;

        #pragma omp parallel
        {
            #pragma omp for
            for (size_t i = 0; i < primitive_count; ++i)
                bvh.primitive_indices[i] = i;

            #pragma omp single
            {
                BuildTask first_task(*this, bboxes, centers);
                run_task(first_task, 0, 0, primitive_count, 0);
            }
        }
    }
};

template <typename Bvh, size_t BinCount>
class BinnedSahBuildTask : public TopDownBuildTask {
    using Scalar    = typename Bvh::ScalarType;
    using IndexType = typename Bvh::IndexType;
    using Builder   = BinnedSahBuilder<Bvh, BinCount>;

    using TopDownBuildTask::WorkItem;

    struct Bin {
        BoundingBox<Scalar> bbox;
        size_t primitive_count;
        Scalar right_cost;
    };

    static constexpr size_t bin_count = BinCount;
    std::array<Bin, bin_count> bins_per_axis[3];

    Builder& builder;
    const BoundingBox<Scalar>* bboxes;
    const Vector3<Scalar>* centers;

    std::pair<Scalar, size_t> find_split(int axis) {
        auto& bins = bins_per_axis[axis];

        // Right sweep to compute partial SAH
        auto   current_bbox  = BoundingBox<Scalar>::empty();
        size_t current_count = 0;
        for (size_t i = bin_count - 1; i > 0; --i) {
            current_bbox.extend(bins[i].bbox);
            current_count += bins[i].primitive_count;
            bins[i].right_cost = current_bbox.half_area() * static_cast<Scalar>(current_count);
        }

        // Left sweep to compute full cost and find minimum
        current_bbox  = BoundingBox<Scalar>::empty();
        current_count = 0;

        auto best_split = std::pair<Scalar, size_t>(std::numeric_limits<Scalar>::max(), bin_count);
        for (size_t i = 0; i < bin_count - 1; ++i) {
            current_bbox.extend(bins[i].bbox);
            current_count += bins[i].primitive_count;
            auto cost = current_bbox.half_area() * static_cast<Scalar>(current_count) + bins[i + 1].right_cost;
            if (cost < best_split.first)
                best_split = std::make_pair(cost, i + 1);
        }
        return best_split;
    }

public:
    using WorkItemType = WorkItem;

    BinnedSahBuildTask(Builder& builder, const BoundingBox<Scalar>* bboxes, const Vector3<Scalar>* centers)
        : builder(builder), bboxes(bboxes), centers(centers)
    {}

    std::optional<std::pair<WorkItem, WorkItem>> build(const WorkItem& item) {
        auto& bvh  = builder.bvh;
        auto& node = bvh.nodes[item.node_index];

        auto make_leaf = [] (typename Bvh::Node& node, size_t begin, size_t end) {
            node.first_child_or_primitive = static_cast<IndexType>(begin);
            node.primitive_count          = static_cast<IndexType>(end - begin);
        };

        if (item.work_size() <= 1 || item.depth >= builder.max_depth) {
            make_leaf(node, item.begin, item.end);
            return std::nullopt;
        }

        auto primitive_indices = bvh.primitive_indices.get();

        std::pair<Scalar, size_t> best_splits[3];

        auto bbox = node.bounding_box_proxy().to_bounding_box();
        auto center_to_bin = bbox.diagonal().inverse() * Scalar(bin_count);
        auto bin_offset    = -bbox.min * center_to_bin;
        auto compute_bin_index = [=] (const Vector3<Scalar>& center, int axis) {
            auto bin_index = fast_multiply_add(center[axis], center_to_bin[axis], bin_offset[axis]);
            return std::min(bin_count - 1, size_t(std::max(Scalar(0), bin_index)));
        };

        // Setup bins
        for (int axis = 0; axis < 3; ++axis) {
            for (auto& bin : bins_per_axis[axis]) {
                bin.bbox = BoundingBox<Scalar>::empty();
                bin.primitive_count = 0;
            }
        }

        // Fill bins with primitives
        for (size_t i = item.begin; i < item.end; ++i) {
            auto primitive_index = bvh.primitive_indices[i];
            for (int axis = 0; axis < 3; ++axis) {
                Bin& bin = bins_per_axis[axis][compute_bin_index(centers[primitive_index], axis)];
                bin.primitive_count++;
                bin.bbox.extend(bboxes[primitive_index]);
            }
        }

        for (int axis = 0; axis < 3; ++axis)
            best_splits[axis] = find_split(axis);

        unsigned best_axis = 0;
        if (best_splits[0].first > best_splits[1].first)
            best_axis = 1;
        if (best_splits[best_axis].first > best_splits[2].first)
            best_axis = 2;

        auto split_index = best_splits[best_axis].second;

        // Make sure the cost of splitting does not exceed the cost of not splitting
        auto max_split_cost = node.bounding_box_proxy().half_area() *
            (static_cast<Scalar>(item.work_size()) - builder.traversal_cost);
        if (best_splits[best_axis].second == bin_count || best_splits[best_axis].first >= max_split_cost) {
            if (item.work_size() > builder.max_leaf_size) {
                // Fallback strategy: approximate median split on largest axis
                best_axis = node.bounding_box_proxy().to_bounding_box().largest_axis();
                for (size_t i = 0, count = 0; i < bin_count - 1; ++i) {
                    count += bins_per_axis[best_axis][i].primitive_count;
                    // Split when we reach 0.4 times the number of primitives in the node
                    if (count >= (item.work_size() * 2 / 5 + 1)) {
                        split_index = i + 1;
                        break;
                    }
                }
            } else {
                make_leaf(node, item.begin, item.end);
                return std::nullopt;
            }
        }

        // Split primitives according to split position
        size_t begin_right = std::partition(primitive_indices + item.begin, primitive_indices + item.end, [&] (size_t i) {
            return compute_bin_index(centers[i], best_axis) < split_index;
        }) - primitive_indices;

        // Check that the split does not leave one side empty
        if (begin_right > item.begin && begin_right < item.end) {
            // Allocate two nodes
            size_t first_child;
            #pragma omp atomic capture
            { first_child = bvh.node_count; bvh.node_count += 2; }

            auto& left  = bvh.nodes[first_child + 0];
            auto& right = bvh.nodes[first_child + 1];
            node.first_child_or_primitive = static_cast<IndexType>(first_child);
            node.primitive_count          = 0;

            // Compute the bounding boxes of each node
            auto& bins = bins_per_axis[best_axis];
            auto left_bbox  = BoundingBox<Scalar>::empty();
            auto right_bbox = BoundingBox<Scalar>::empty();
            for (size_t i = 0; i < best_splits[best_axis].second; ++i)
                left_bbox.extend(bins[i].bbox);
            for (size_t i = split_index; i < bin_count; ++i)
                right_bbox.extend(bins[i].bbox);
            left.bounding_box_proxy()  = left_bbox;
            right.bounding_box_proxy() = right_bbox;

            // Return new work items
            WorkItem first_item (first_child + 0, item.begin, begin_right, item.depth + 1);
            WorkItem second_item(first_child + 1, begin_right, item.end,   item.depth + 1);
            return std::make_optional(std::make_pair(first_item, second_item));
        }

        make_leaf(node, item.begin, item.end);
        return std::nullopt;
    }
};

} // namespace bvh

#endif
