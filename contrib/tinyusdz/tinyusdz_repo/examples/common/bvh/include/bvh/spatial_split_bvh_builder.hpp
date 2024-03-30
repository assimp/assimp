#ifndef BVH_SPATIAL_SPLIT_BVH_BUILDER_HPP
#define BVH_SPATIAL_SPLIT_BVH_BUILDER_HPP

#include <optional>
#include <algorithm>
#include <vector>

#include "bvh/bvh.hpp"
#include "bvh/bounding_box.hpp"
#include "bvh/top_down_builder.hpp"
#include "bvh/sah_based_algorithm.hpp"

namespace bvh {

template <typename, typename, size_t> class SpatialSplitBvhBuildTask;

/// This is a top-down, spatial split BVH builder based on:
/// "Spatial Splits in Bounding Volume Hierarchies", by M. Stich et al.
/// Even though the object splitting strategy is a full-sweep SAH evaluation,
/// this builder is not as efficient as bvh::SweepSahBuilder when spatial splits
/// are disabled, because it needs to sort primitive references at every step.
template <typename Bvh, typename Primitive, size_t BinCount>
class SpatialSplitBvhBuilder : public TopDownBuilder, public SahBasedAlgorithm<Bvh> {
    using Scalar    = typename Bvh::ScalarType;
    using BuildTask = SpatialSplitBvhBuildTask<Bvh, Primitive, BinCount>;
    using Reference = typename BuildTask::ReferenceType;

    using TopDownBuilder::run_task;

    friend BuildTask;

    Bvh& bvh;

public:
    using TopDownBuilder::max_depth;
    using TopDownBuilder::max_leaf_size;
    using SahBasedAlgorithm<Bvh>::traversal_cost;

    /// Number of spatial binning passes that are run in order to
    /// find a spatial split. This brings additional accuracy without
    /// increasing the number of bins.
    size_t binning_pass_count = 2;

    SpatialSplitBvhBuilder(Bvh& bvh)
        : bvh(bvh)
    {}

    size_t build(
        const BoundingBox<Scalar>& global_bbox,
        const Primitive* primitives,
        const BoundingBox<Scalar>* bboxes,
        const Vector3<Scalar>* centers,
        size_t primitive_count,
        Scalar alpha = Scalar(1e-5),
        Scalar split_factor = Scalar(0.3))
    {
        assert(primitive_count > 0);

        size_t max_reference_count = primitive_count +
            static_cast<size_t>(static_cast<Scalar>(primitive_count) * split_factor);
        size_t reference_count = 0;

        bvh.nodes = std::make_unique<typename Bvh::Node[]>(2 * max_reference_count + 1);
        bvh.primitive_indices = std::make_unique<size_t[]>(max_reference_count);

        auto accumulated_bboxes = std::make_unique<BoundingBox<Scalar>[]>(max_reference_count);
        auto reference_data     = std::make_unique<Reference[]>(max_reference_count * 3);

        std::array<Reference*, 3> references = {
            reference_data.get(),
            reference_data.get() + max_reference_count,
            reference_data.get() + 2 * max_reference_count
        };

        // Compute the spatial split threshold, as specified in the original publication
        auto spatial_threshold = alpha * Scalar(2) * global_bbox.half_area();

        bvh.node_count = 1;
        bvh.nodes[0].bounding_box_proxy() = global_bbox;

        #pragma omp parallel
        {
            #pragma omp for
            for (size_t i = 0; i < primitive_count; ++i) {
                for (int j = 0; j < 3; ++j) {
                    references[j][i].bbox   = bboxes[i];
                    references[j][i].center = centers[i];
                    references[j][i].primitive_index = i;
                }
            }

            #pragma omp single
            {
                BuildTask first_task(
                    *this,
                    primitives,
                    accumulated_bboxes.get(),
                    references,
                    reference_count,
                    primitive_count,
                    spatial_threshold);
                run_task(first_task, 0, 0, primitive_count, max_reference_count, 0, false);
            }
        }

        return reference_count;
    }
};

template <typename Bvh, typename Primitive, size_t BinCount>
class SpatialSplitBvhBuildTask : public TopDownBuildTask {
    using Scalar    = typename Bvh::ScalarType;
    using IndexType = typename Bvh::IndexType;
    using Builder   = SpatialSplitBvhBuilder<Bvh, Primitive, BinCount>;

    struct WorkItem : public TopDownBuildTask::WorkItem {
        size_t split_end;
        bool   is_sorted;

        WorkItem() = default;
        WorkItem(
            size_t node_index,
            size_t begin,
            size_t end,
            size_t split_end,
            size_t depth,
            bool is_sorted = false)
            : TopDownBuildTask::WorkItem(node_index, begin, end, depth)
            , split_end(split_end)
            , is_sorted(is_sorted)
        {}
    };

    struct Reference {
        BoundingBox<Scalar> bbox;
        Vector3<Scalar>     center;
        size_t primitive_index;
    };

    struct Bin {
        BoundingBox<Scalar> bbox;
        BoundingBox<Scalar> accumulated_bbox;
        size_t entry;
        size_t exit;
    };

    struct ObjectSplit {
        Scalar cost;
        size_t index;
        int    axis;

        BoundingBox<Scalar> left_bbox;
        BoundingBox<Scalar> right_bbox;

        ObjectSplit(
            Scalar cost = std::numeric_limits<Scalar>::max(),
            size_t index = 1,
            int axis = 0,
            const BoundingBox<Scalar>& left_bbox = BoundingBox<Scalar>::empty(),
            const BoundingBox<Scalar>& right_bbox = BoundingBox<Scalar>::empty())
            : cost(cost), index(index), axis(axis), left_bbox(left_bbox), right_bbox(right_bbox)
        {}
    };

    struct SpatialSplit {
        Scalar cost;
        Scalar position;
        int    axis;

        SpatialSplit(
            Scalar cost = std::numeric_limits<Scalar>::max(),
            Scalar position = 0,
            int axis = 0)
            : cost(cost), position(position), axis(axis)
        {}
    };

    Builder& builder;

    const Primitive*     primitives;
    BoundingBox<Scalar>* accumulated_bboxes;
    std::vector<bool>    reference_marks;

    std::array<Reference* bvh_restrict, 3> references;

    size_t& reference_count;
    size_t  primitive_count;
    Scalar  spatial_threshold;

    static constexpr size_t bin_count = BinCount;
    std::array<Bin, bin_count> bins;

    ObjectSplit find_object_split(size_t begin, size_t end, bool is_sorted) const {
        if (!is_sorted) {
            // Sort references by the projection of their centers on this axis
            #pragma omp taskloop if (end - begin > builder.task_spawn_threshold) grainsize(1) default(shared)
            for (int axis = 0; axis < 3; ++axis) {
                std::sort(references[axis] + begin, references[axis] + end, [&] (const Reference& a, const Reference& b) {
                    return a.center[axis] < b.center[axis];
                });
            }
        }

        ObjectSplit best_split;
        for (int axis = 0; axis < 3; ++axis) {
            // Sweep from the right to the left to accumulate bounding boxes
            auto bbox = BoundingBox<Scalar>::empty();
            for (size_t i = end - 1; i > begin; --i) {
                bbox.extend(references[axis][i].bbox);
                accumulated_bboxes[i] = bbox;
            }

            // Sweep from the left to the right to compute the SAH cost
            bbox = BoundingBox<Scalar>::empty();
            for (size_t i = begin; i < end - 1; ++i) {
                bbox.extend(references[axis][i].bbox);
                auto cost =
                    static_cast<Scalar>(i + 1 - begin) * bbox.half_area() +
                    static_cast<Scalar>(end - (i + 1)) * accumulated_bboxes[i + 1].half_area();
                if (cost < best_split.cost)
                    best_split = ObjectSplit(cost, i + 1, axis, bbox, accumulated_bboxes[i + 1]);
            }
        }
        return best_split;
    }

    std::pair<WorkItem, WorkItem> allocate_children(
        Bvh& bvh,
        const WorkItem& item,
        size_t right_begin, size_t right_end,
        const BoundingBox<Scalar>& left_bbox,
        const BoundingBox<Scalar>& right_bbox,
        bool is_sorted)
    {
        auto& parent = bvh.nodes[item.node_index];

        // Allocate two nodes for the children
        size_t first_child;
        #pragma omp atomic capture
        { first_child = bvh.node_count; bvh.node_count += 2; }

        auto& left  = bvh.nodes[first_child + 0];
        auto& right = bvh.nodes[first_child + 1];
        parent.first_child_or_primitive = static_cast<IndexType>(first_child);
        parent.primitive_count          = 0;

        left.bounding_box_proxy()  = left_bbox;
        right.bounding_box_proxy() = right_bbox;

        // Allocate split space for the two children based on their SAH cost.
        // This assumes that reference ranges look like this:
        // - [item.begin...right_begin[ is the range of references on the left,
        // - [right_begin...right_end[ is the range of references on the right,
        // - [right_end...item.split_end[ is the free split space
        assert(item.begin < right_begin && right_begin < right_end && right_end <= item.split_end);
        auto remaining_split_count = item.split_end - right_end;
        auto left_cost  = left_bbox.half_area() * static_cast<Scalar>(right_begin - item.begin);
        auto right_cost = right_bbox.half_area() * static_cast<Scalar>(right_end - right_begin);
        auto left_split_ratio = left_cost + right_cost > 0 ? left_cost / (left_cost + right_cost) : Scalar(0.5);
        auto left_split_count = static_cast<size_t>(static_cast<Scalar>(remaining_split_count) * left_split_ratio);
        assert(left_split_count <= remaining_split_count);

        // Move references of the right child to leave some split space for the left one
        if (left_split_count > 0) {
            std::move_backward(references[0] + right_begin, references[0] + right_end, references[0] + right_end + left_split_count);
            std::move_backward(references[1] + right_begin, references[1] + right_end, references[1] + right_end + left_split_count);
            std::move_backward(references[2] + right_begin, references[2] + right_end, references[2] + right_end + left_split_count);
        }

        size_t left_end = right_begin;
        right_begin += left_split_count;
        right_end   += left_split_count;
        assert(right_begin < item.split_end);
        assert(right_end <= item.split_end);
        return std::make_pair(
            WorkItem(first_child + 0, item.begin,  left_end,  right_begin,    item.depth + 1, is_sorted),
            WorkItem(first_child + 1, right_begin, right_end, item.split_end, item.depth + 1, is_sorted));
    }

    std::pair<WorkItem, WorkItem> apply_object_split(Bvh& bvh, const ObjectSplit& split, const WorkItem& item) {
        int other_axis[2] = { (split.axis + 1) % 3, (split.axis + 2) % 3 };
        reference_marks.resize(primitive_count);
        for (size_t i = item.begin;  i < split.index; ++i)
            reference_marks[references[split.axis][i].primitive_index] = true;
        for (size_t i = split.index; i < item.end;    ++i)
            reference_marks[references[split.axis][i].primitive_index] = false;
        auto partition_predicate = [&] (const Reference& reference) { return reference_marks[reference.primitive_index]; };

        #pragma omp taskgroup
        {
            #pragma omp task if (item.work_size() > builder.task_spawn_threshold) default(shared)
            { std::stable_partition(references[other_axis[0]] + item.begin, references[other_axis[0]] + item.end, partition_predicate); }
            #pragma omp task if (item.work_size() > builder.task_spawn_threshold) default(shared)
            { std::stable_partition(references[other_axis[1]] + item.begin, references[other_axis[1]] + item.end, partition_predicate); }
        }

        return allocate_children(bvh, item, split.index, item.end, split.left_bbox, split.right_bbox, true);
    }

    std::optional<std::pair<Scalar, Scalar>>
    run_binning_pass(SpatialSplit& split, int axis, size_t begin, size_t end, Scalar min, Scalar max) {
        for (size_t i = 0; i < bin_count; ++i) {
            bins[i].bbox = BoundingBox<Scalar>::empty();
            bins[i].entry = 0;
            bins[i].exit  = 0;
        }

        // Split primitives and add the bounding box of the fragments to the bins
        auto bin_size = (max - min) / bin_count;
        auto inv_size = Scalar(1) / bin_size;
        for (size_t i = begin; i < end; ++i) {
            auto& reference = references[0][i];
            auto first_bin = std::min(bin_count - 1, size_t(std::max(Scalar(0), inv_size * (reference.bbox.min[axis] - min))));
            auto last_bin  = std::min(bin_count - 1, size_t(std::max(Scalar(0), inv_size * (reference.bbox.max[axis] - min))));
            auto current_bbox = reference.bbox;
            for (size_t j = first_bin; j < last_bin; ++j) {
                auto [left_bbox, right_bbox] = primitives[reference.primitive_index].split(
                    axis, min + static_cast<Scalar>(j + 1) * bin_size);
                bins[j].bbox.extend(left_bbox.shrink(current_bbox));
                current_bbox.shrink(right_bbox);
            }
            bins[last_bin].bbox.extend(current_bbox);
            bins[first_bin].entry++;
            bins[last_bin].exit++;
        }

        // Accumulate bounding boxes
        auto current_bbox = BoundingBox<Scalar>::empty();
        for (size_t i = bin_count; i > 0; --i)
            bins[i - 1].accumulated_bbox = current_bbox.extend(bins[i - 1].bbox);

        // Sweep and compute SAH cost
        size_t left_count = 0, right_count = end - begin;
        current_bbox = BoundingBox<Scalar>::empty();
        bool found = false;
        for (size_t i = 0; i < bin_count - 1; ++i) {
            left_count  += bins[i].entry;
            right_count -= bins[i].exit;
            current_bbox.extend(bins[i].bbox);

            auto cost =
                static_cast<Scalar>(left_count)  * current_bbox.half_area() +
                static_cast<Scalar>(right_count) * bins[i + 1].accumulated_bbox.half_area();
            if (cost < split.cost) {
                split.cost = cost;
                split.axis = axis;
                split.position = min + static_cast<Scalar>(i + 1) * bin_size;
                found = true;
            }
        }

        return found ? std::make_optional(std::make_pair(split.position - bin_size, split.position + bin_size)) : std::nullopt;
    }

    SpatialSplit find_spatial_split(const BoundingBox<Scalar>& node_bbox, size_t begin, size_t end, size_t binning_pass_count) {
        SpatialSplit split;
        for (int axis = 0; axis < 3; ++axis) {
            auto min = node_bbox.min[axis];
            auto max = node_bbox.max[axis];
            // Run several binning passes to get the best possible split
            for (size_t pass = 0; pass < binning_pass_count; ++pass) {
                auto next_bounds = run_binning_pass(split, axis, begin, end, min, max);
                if (next_bounds)
                    std::tie(min, max) = *next_bounds;
                else
                    break;
            }
        }
        return split;
    }

    std::pair<WorkItem, WorkItem> apply_spatial_split(Bvh& bvh, const SpatialSplit& split, const WorkItem& item) {
        size_t left_end    = item.begin;
        size_t right_begin = item.end;
        size_t right_end   = item.end;

        auto left_bbox  = BoundingBox<Scalar>::empty();
        auto right_bbox = BoundingBox<Scalar>::empty();

        // Choosing the references that are sorted on the split axis
        // is more efficient than the others, since fewers swaps are
        // necessary for primitives that are completely contained on
        // one side of the partition.
        auto references_to_split = references[split.axis];

        // Partition references such that:
        // - [item.begin...left_end[ is on the left,
        // - [left_end...right_begin[ is in between,
        // - [right_begin...item.end[ is on the right
        for (size_t i = item.begin; i < right_begin;) {
            auto& bbox = references_to_split[i].bbox;
            if (bbox.max[split.axis] <= split.position) {
                left_bbox.extend(bbox);
                std::swap(references_to_split[i++], references_to_split[left_end++]);
            } else if (bbox.min[split.axis] >= split.position) {
                right_bbox.extend(bbox);
                std::swap(references_to_split[i], references_to_split[--right_begin]);
            } else {
                i++;
            }
        }

        size_t left_count  = left_end  - item.begin;
        size_t right_count = right_end - right_begin;
        if ((left_count == 0 || right_count == 0) && left_end == right_begin) {
            // Sometimes, because of numerical imprecision,
            // the algorithm will report that a spatial split is
            // possible, but all references end up on the same side
            // when applying it. To counteract this, we simply put
            // half of the primitives of the left side in the right side
            // and continue as usual.
            if (left_count > 0) {
                left_end -= left_count / 2;
                right_begin = left_end;
            } else {
                left_end += right_count / 2;
                right_begin = left_end;
            }
            // Recompute the left and right bounding boxes
            left_bbox  = BoundingBox<Scalar>::empty();
            right_bbox = BoundingBox<Scalar>::empty();
            for (size_t i = item.begin; i < left_end; ++i)
                left_bbox.extend(references_to_split[i].bbox);
            for (size_t i = left_end; i < item.end; ++i)
                right_bbox.extend(references_to_split[i].bbox);
        }

        // Handle straddling references
        while (left_end < right_begin) {
            auto reference = references_to_split[left_end];
            auto [left_primitive_bbox, right_primitive_bbox] =
                primitives[reference.primitive_index].split(split.axis, split.position);
            left_primitive_bbox .shrink(reference.bbox);
            right_primitive_bbox.shrink(reference.bbox);

            // Make sure there is enough space to split that reference
            if (item.split_end - right_end > 0) {
                left_bbox .extend(left_primitive_bbox);
                right_bbox.extend(right_primitive_bbox);
                references_to_split[right_end++] = Reference {
                    right_primitive_bbox,
                    right_primitive_bbox.center(),
                    reference.primitive_index
                };
                references_to_split[left_end++] = Reference {
                    left_primitive_bbox,
                    left_primitive_bbox.center(),
                    reference.primitive_index
                };
                left_count++;
                right_count++;
            } else if (left_count < right_count) {
                left_bbox.extend(reference.bbox);
                left_end++;
                left_count++;
            } else {
                right_bbox.extend(reference.bbox);
                std::swap(references_to_split[--right_begin], references_to_split[left_end]);
                right_count++;
            }
        }

        std::copy(
            references_to_split + item.begin,
            references_to_split + right_end,
            references[(split.axis + 1) % 3] + item.begin);
        std::copy(
            references_to_split + item.begin,
            references_to_split + right_end,
            references[(split.axis + 2) % 3] + item.begin);

        assert(left_end == right_begin);
        assert(right_end <= item.split_end);
        return allocate_children(bvh, item, right_begin, right_end, left_bbox, right_bbox, false);
    }

public:
    using ReferenceType = Reference;
    using WorkItemType  = WorkItem;

    SpatialSplitBvhBuildTask(
        Builder& builder,
        const Primitive* primitives,
        BoundingBox<Scalar>* accumulated_bboxes,
        const std::array<Reference*, 3>& references,
        size_t& reference_count,
        size_t  primitive_count,
        Scalar spatial_threshold)
        : builder(builder)
        , primitives(primitives)
        , accumulated_bboxes(accumulated_bboxes)
        , references { references[0], references[1], references[2] }
        , reference_count(reference_count)
        , primitive_count(primitive_count)
        , spatial_threshold(spatial_threshold)
    {}

    SpatialSplitBvhBuildTask(const SpatialSplitBvhBuildTask& other)
        : builder(other.builder)
        , primitives(other.primitives)
        , accumulated_bboxes(other.accumulated_bboxes)
        , references(other.references)
        , reference_count(other.reference_count)
        , primitive_count(other.primitive_count)
        , spatial_threshold(other.spatial_threshold)
    {
        // Note: no need to copy reference marks as it is
        // the task's private data and should not be shared
    }

    std::optional<std::pair<WorkItem, WorkItem>> build(const WorkItem& item) {
        auto& bvh  = builder.bvh;
        auto& node = bvh.nodes[item.node_index];

        auto make_leaf = [&] (typename Bvh::Node& node, size_t begin, size_t end) {
            size_t primitive_count = end - begin;

            // Reserve space for the primitives
            size_t first_primitive;
            #pragma omp atomic capture
            { first_primitive = reference_count; reference_count += primitive_count; }

            // Copy the primitives indices from the references to the BVH
            for (size_t i = 0; i < primitive_count; ++i)
                bvh.primitive_indices[first_primitive + i] = references[0][begin + i].primitive_index;
            node.first_child_or_primitive = static_cast<IndexType>(first_primitive);
            node.primitive_count          = static_cast<IndexType>(primitive_count);
        };

        if (item.work_size() <= 1 || item.depth >= builder.max_depth) {
            make_leaf(node, item.begin, item.end);
            return std::nullopt;
        }

        ObjectSplit best_object_split = find_object_split(item.begin, item.end, item.is_sorted);

        // Find a spatial split when the size
        SpatialSplit best_spatial_split;
        auto overlap = BoundingBox<Scalar>(best_object_split.left_bbox).shrink(best_object_split.right_bbox).half_area();
        if (overlap > spatial_threshold && item.split_end - item.end > 0) {
            auto binning_pass_count = static_cast<SpatialSplitBvhBuilder<Bvh, Primitive, BinCount>&>(builder).binning_pass_count;
            best_spatial_split = find_spatial_split(node.bounding_box_proxy(), item.begin, item.end, binning_pass_count);
        }

        auto best_cost = std::min(best_spatial_split.cost, best_object_split.cost);
        bool use_spatial_split = best_cost < best_object_split.cost;

        // Make sure the cost of splitting does not exceed the cost of not splitting
        auto max_split_cost = node.bounding_box_proxy().half_area() *
            (static_cast<Scalar>(item.work_size()) - builder.traversal_cost);
        if (best_cost >= max_split_cost) {
            if (item.work_size() > builder.max_leaf_size) {
                // Fallback strategy: median split on the largest axis
                use_spatial_split = false;
                best_object_split.index = (item.begin + item.end) / 2;
                best_object_split.axis  = node.bounding_box_proxy().to_bounding_box().largest_axis();
                best_object_split.left_bbox  = BoundingBox<Scalar>::empty();
                best_object_split.right_bbox = BoundingBox<Scalar>::empty();
                for (size_t i = item.begin; i < best_object_split.index; ++i)
                    best_object_split.left_bbox.extend(references[best_object_split.axis][i].bbox);
                for (size_t i = best_object_split.index; i < item.end; ++i)
                    best_object_split.right_bbox.extend(references[best_object_split.axis][i].bbox);
            } else {
                make_leaf(node, item.begin, item.end);
                return std::nullopt;
            }
        }

        // Apply the (object/spatial) split
        return use_spatial_split
            ? std::make_optional(apply_spatial_split(bvh, best_spatial_split, item))
            : std::make_optional(apply_object_split(bvh, best_object_split, item));
    }
};

} // namespace bvh

#endif
