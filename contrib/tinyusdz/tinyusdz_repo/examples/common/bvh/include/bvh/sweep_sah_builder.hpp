#ifndef BVH_SWEEP_SAH_BUILDER_HPP
#define BVH_SWEEP_SAH_BUILDER_HPP

#include <array>
#include <optional>

#include "bvh/bvh.hpp"
#include "bvh/bounding_box.hpp"
#include "bvh/top_down_builder.hpp"
#include "bvh/sah_based_algorithm.hpp"
#include "bvh/radix_sort.hpp"

namespace bvh {

template <typename> class SweepSahBuildTask;

/// This is a top-down, full-sweep SAH-based BVH builder. Primitives are only
/// sorted once, and a stable partitioning algorithm is used when splitting,
/// so as to keep the relative order of primitives within each partition intact.
template <typename Bvh>
class SweepSahBuilder : public TopDownBuilder, public SahBasedAlgorithm<Bvh> {
    using Scalar    = typename Bvh::ScalarType;
    using BuildTask = SweepSahBuildTask<Bvh>;
    using Key       = typename SizedIntegerType<sizeof(Scalar) * CHAR_BIT>::Unsigned;
    using Mark      = typename BuildTask::MarkType;

    using TopDownBuilder::run_task;

    friend BuildTask;

    RadixSort<10> radix_sort;
    Bvh& bvh;

public:
    using TopDownBuilder::max_depth;
    using TopDownBuilder::max_leaf_size;
    using SahBasedAlgorithm<Bvh>::traversal_cost;

    SweepSahBuilder(Bvh& bvh)
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

        auto reference_data = std::make_unique<size_t[]>(primitive_count * 3);
        auto cost_data      = std::make_unique<Scalar[]>(primitive_count * 3);
        auto key_data       = std::make_unique<Key[]>(primitive_count * 2);
        auto mark_data      = std::make_unique<Mark[]>(primitive_count);

        std::array<Scalar*, 3> costs = {
            cost_data.get(),
            cost_data.get() + primitive_count,
            cost_data.get() + 2 * primitive_count
        };

        std::array<size_t*, 3> sorted_references;
        size_t* unsorted_references = bvh.primitive_indices.get();
        Key* sorted_keys = key_data.get();
        Key* unsorted_keys = key_data.get() + primitive_count;

        bvh.node_count = 1;
        bvh.nodes[0].bounding_box_proxy() = global_bbox;

        #pragma omp parallel
        {
            // Sort the primitives on each axis once
            for (int axis = 0; axis < 3; ++axis) {
                #pragma omp single
                {
                    sorted_references[axis] = unsorted_references;
                    unsorted_references = reference_data.get() + axis * primitive_count;
                    // Make sure that one array is the final array of references used by the BVH
                    if (axis != 0 && sorted_references[axis] == bvh.primitive_indices.get())
                        std::swap(sorted_references[axis], unsorted_references);
                    assert(axis < 2 ||
                           sorted_references[0] == bvh.primitive_indices.get() ||
                           sorted_references[1] == bvh.primitive_indices.get());
                }

                #pragma omp for
                for (size_t i = 0; i < primitive_count; ++i) {
                    sorted_keys[i] = radix_sort.make_key(centers[i][axis]);
                    sorted_references[axis][i] = i;
                }

                radix_sort.sort_in_parallel(
                    sorted_keys,
                    unsorted_keys,
                    sorted_references[axis],
                    unsorted_references,
                    primitive_count,
                    sizeof(Scalar) * CHAR_BIT);
            }

            #pragma omp single
            {
                BuildTask first_task(*this, bboxes, centers, sorted_references, costs, mark_data.get());
                run_task(first_task, 0, 0, primitive_count, 0);
            }
        }
    }
};

template <typename Bvh>
class SweepSahBuildTask : public TopDownBuildTask {
    using Scalar  = typename Bvh::ScalarType;
    using IndexType = typename Bvh::IndexType;
    using Builder = SweepSahBuilder<Bvh>;
    using Mark    = uint_fast8_t;

    using TopDownBuildTask::WorkItem;

    Builder& builder;
    const BoundingBox<Scalar>* bboxes;
    const Vector3<Scalar>* centers;

    std::array<size_t* bvh_restrict, 3> references;
    std::array<Scalar* bvh_restrict, 3> costs;
    Mark* marks;

    std::pair<Scalar, size_t> find_split(int axis, size_t begin, size_t end) {
        auto bbox = BoundingBox<Scalar>::empty();
        for (size_t i = end - 1; i > begin; --i) {
            bbox.extend(bboxes[references[axis][i]]);
            costs[axis][i] = bbox.half_area() * static_cast<Scalar>(end - i);
        }
        bbox = BoundingBox<Scalar>::empty();
        auto best_split = std::pair<Scalar, size_t>(std::numeric_limits<Scalar>::max(), end);
        for (size_t i = begin; i < end - 1; ++i) {
            bbox.extend(bboxes[references[axis][i]]);
            auto cost = bbox.half_area() * static_cast<Scalar>(i + 1 - begin) + costs[axis][i + 1];
            if (cost < best_split.first)
                best_split = std::make_pair(cost, i + 1);
        }
        return best_split;
    }

public:
    using MarkType     = Mark;
    using WorkItemType = WorkItem;

    SweepSahBuildTask(
        Builder& builder,
        const BoundingBox<Scalar>* bboxes,
        const Vector3<Scalar>* centers,
        const std::array<size_t*, 3>& references,
        const std::array<Scalar*, 3>& costs,
        Mark* marks)
        : builder(builder)
        , bboxes(bboxes)
        , centers(centers)
        , references { references[0], references[1], references[2] }
        , costs { costs[0], costs[1], costs[2] }
        , marks(marks)
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

        std::pair<Scalar, size_t> best_splits[3];
        [[maybe_unused]] bool should_spawn_tasks = item.work_size() > builder.task_spawn_threshold;

        // Sweep primitives to find the best cost
        #pragma omp taskloop if (should_spawn_tasks) grainsize(1) default(shared)
        for (int axis = 0; axis < 3; ++axis)
            best_splits[axis] = find_split(axis, item.begin, item.end);

        unsigned best_axis = 0;
        if (best_splits[0].first > best_splits[1].first)
            best_axis = 1;
        if (best_splits[best_axis].first > best_splits[2].first)
            best_axis = 2;

        auto split_index = best_splits[best_axis].second;

        // Make sure the cost of splitting does not exceed the cost of not splitting
        auto max_split_cost = node.bounding_box_proxy().half_area() *
            (static_cast<Scalar>(item.work_size()) - builder.traversal_cost);
        if (best_splits[best_axis].first >= max_split_cost) {
            if (item.work_size() > builder.max_leaf_size) {
                // Fallback strategy: median split on largest axis
                best_axis = node.bounding_box_proxy().to_bounding_box().largest_axis();
                split_index = (item.begin + item.end) / 2;
            } else {
                make_leaf(node, item.begin, item.end);
                return std::nullopt;
            }
        }

        unsigned other_axis[2] = { (best_axis + 1) % 3, (best_axis + 2) % 3 };

        for (size_t i = item.begin;  i < split_index; ++i) marks[references[best_axis][i]] = 1;
        for (size_t i = split_index; i < item.end;    ++i) marks[references[best_axis][i]] = 0;
        auto partition_predicate = [&] (size_t i) { return marks[i] != 0; };

        auto left_bbox  = BoundingBox<Scalar>::empty();
        auto right_bbox = BoundingBox<Scalar>::empty();

        // Partition reference arrays and compute bounding boxes
        #pragma omp taskgroup
        {
            #pragma omp task if (should_spawn_tasks) default(shared)
            { std::stable_partition(references[other_axis[0]] + item.begin, references[other_axis[0]] + item.end, partition_predicate); }
            #pragma omp task if (should_spawn_tasks) default(shared)
            { std::stable_partition(references[other_axis[1]] + item.begin, references[other_axis[1]] + item.end, partition_predicate); }
            #pragma omp task if (should_spawn_tasks) default(shared)
            {
                for (size_t i = item.begin; i < split_index; ++i)
                    left_bbox.extend(bboxes[references[best_axis][i]]);
            }
            #pragma omp task if (should_spawn_tasks) default(shared)
            {
                for (size_t i = split_index; i < item.end; ++i)
                    right_bbox.extend(bboxes[references[best_axis][i]]);
            }
        }

        // Allocate space for children
        size_t first_child;
        #pragma omp atomic capture
        { first_child = bvh.node_count; bvh.node_count += 2; }

        auto& left  = bvh.nodes[first_child + 0];
        auto& right = bvh.nodes[first_child + 1];
        node.first_child_or_primitive = static_cast<IndexType>(first_child);
        node.primitive_count          = 0;

        left.bounding_box_proxy()  = left_bbox;
        right.bounding_box_proxy() = right_bbox;
        WorkItem first_item (first_child + 0, item.begin, split_index, item.depth + 1);
        WorkItem second_item(first_child + 1, split_index, item.end,   item.depth + 1);
        return std::make_optional(std::make_pair(first_item, second_item));
    }
};

} // namespace bvh

#endif
