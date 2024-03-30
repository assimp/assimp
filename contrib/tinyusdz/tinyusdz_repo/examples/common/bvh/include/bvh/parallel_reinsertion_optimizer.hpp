#ifndef BVH_PARALLEL_REINSERTION_OPTIMIZER_HPP
#define BVH_PARALLEL_REINSERTION_OPTIMIZER_HPP

#include <cassert>

#include "bvh/bvh.hpp"
#include "bvh/sah_based_algorithm.hpp"
#include "bvh/hierarchy_refitter.hpp"

namespace bvh {

/// Optimization that tries to re-insert BVH nodes in such a way that the
/// SAH cost of the tree decreases after the re-insertion. Inspired from the
/// article "Parallel Reinsertion for Bounding Volume Hierarchy Optimization",
/// by D. Meister and J. Bittner.
template <typename Bvh>
class ParallelReinsertionOptimizer :
    public SahBasedAlgorithm<Bvh>,
    protected HierarchyRefitter<Bvh>
{
    using Scalar    = typename Bvh::ScalarType;
    using Insertion = std::pair<size_t, Scalar>;

    using SahBasedAlgorithm<Bvh>::compute_cost;
    using HierarchyRefitter<Bvh>::bvh;
    using HierarchyRefitter<Bvh>::parents;
    using HierarchyRefitter<Bvh>::refit_in_parallel;

public:
    ParallelReinsertionOptimizer(Bvh& bvh)
        : HierarchyRefitter<Bvh>(bvh)
    {}

private:
    static constexpr Insertion invalid_insertion { size_t{0}, Scalar(0) };
    static constexpr bool is_valid_insertion(const Insertion& insertion) { return insertion.second > 0; }

    std::array<size_t, 6> get_conflicts(size_t in, size_t out) {
        // Return an array of re-insertion conflicts for the given nodes
        auto parent_in = parents[in];
        return std::array<size_t, 6> {
            in,
            bvh.sibling(in),
            parent_in,
            parent_in == 0 ? in : parents[parent_in],
            out,
            out == 0 ? out : parents[out],
        };
    }

    void reinsert(size_t in, size_t out) {
        auto sibling_in   = bvh.sibling(in);
        auto parent_in    = parents[in];
        auto sibling_node = bvh.nodes[sibling_in];
        auto out_node     = bvh.nodes[out];

        // Re-insert it into the destination
        bvh.nodes[out].bounding_box_proxy().extend(bvh.nodes[in].bounding_box_proxy());
        bvh.nodes[out].first_child_or_primitive = static_cast<typename Bvh::IndexType>(std::min(in, sibling_in));
        bvh.nodes[out].primitive_count = 0;
        bvh.nodes[sibling_in] = out_node;
        bvh.nodes[parent_in] = sibling_node;

        // Update parent-child indices
        if (!out_node.is_leaf()) {
            parents[out_node.first_child_or_primitive + 0] = sibling_in;
            parents[out_node.first_child_or_primitive + 1] = sibling_in;
        }
        if (!sibling_node.is_leaf()) {
            parents[sibling_node.first_child_or_primitive + 0] = parent_in;
            parents[sibling_node.first_child_or_primitive + 1] = parent_in;
        }
        parents[sibling_in] = out;
        parents[in] = out;
    }

    Insertion search(size_t in) {
        bool   down  = true;
        size_t pivot = parents[in];
        size_t out   = bvh.sibling(in);
        size_t out_best = out;

        auto bbox_in = bvh.nodes[in].bounding_box_proxy();
        auto bbox_parent = bvh.nodes[pivot].bounding_box_proxy();
        auto bbox_pivot = BoundingBox<Scalar>::empty();

        Scalar d = 0;
        Scalar d_best = 0;
        const Scalar d_bound = bbox_parent.half_area() - bbox_in.half_area();

        // Perform a search to find a re-insertion position for the given node
        while (true) {
            auto bbox_out = bvh.nodes[out].bounding_box_proxy().to_bounding_box();
            auto bbox_merged = BoundingBox<Scalar>(bbox_in).extend(bbox_out);
            if (down) {
                auto d_direct = bbox_parent.half_area() - bbox_merged.half_area();
                if (d_best < d_direct + d) {
                    d_best = d_direct + d;
                    out_best = out;
                }
                d = d + bbox_out.half_area() - bbox_merged.half_area();
                if (bvh.nodes[out].is_leaf() || d_bound + d <= d_best)
                    down = false;
                else
                    out = bvh.nodes[out].first_child_or_primitive;
            } else {
                d = d - bbox_out.half_area() + bbox_merged.half_area();
                if (pivot == parents[out]) {
                    bbox_pivot.extend(bbox_out);
                    out = pivot;
                    bbox_out = bvh.nodes[out].bounding_box_proxy();
                    if (out != parents[in]) {
                        bbox_merged = BoundingBox<Scalar>(bbox_in).extend(bbox_pivot);
                        auto d_direct = bbox_parent.half_area() - bbox_merged.half_area();
                        if (d_best < d_direct + d) {
                            d_best = d_direct + d;
                            out_best = out;
                        }
                        d = d + bbox_out.half_area() - bbox_pivot.half_area();
                    }
                    if (out == 0)
                        break;
                    out = bvh.sibling(pivot);
                    pivot = parents[out];
                    down = true;
                } else {
                    if (bvh.is_left_sibling(out)) {
                        down = true;
                        out = bvh.sibling(out);
                    } else {
                        out = parents[out];
                    }
                }
            }
        }

        if (in == out_best || bvh.sibling(in) == out_best || parents[in] == out_best)
            return invalid_insertion;
        return Insertion { out_best, d_best };
    }

public:
    void optimize(size_t u = 9, Scalar threshold = Scalar(0.1)) {
        auto locks = std::make_unique<std::atomic<uint64_t>[]>(bvh.node_count);
        auto outs  = std::make_unique<Insertion[]>(bvh.node_count);

        auto old_cost = compute_cost(bvh);
        for (size_t iteration = 0; ; ++iteration) {
            size_t first_node = iteration % u + 1;

            #pragma omp parallel
            {
                // Clear the locks
                #pragma omp for nowait
                for (size_t i = 0; i < bvh.node_count; i++)
                    locks[i] = 0;

                // Search for insertion candidates
                #pragma omp for
                for (size_t i = first_node; i < bvh.node_count; i += u)
                    outs[i] = search(i);

                // Resolve topological conflicts with locking
                #pragma omp for
                for (size_t i = first_node; i < bvh.node_count; i += u) {
                    if (!is_valid_insertion(outs[i]))
                        continue;
                    // Encode locks into 64 bits using the highest 32 bits for the cost and the
                    // lowest 32 bits for the index of the node requesting the re-insertion.
                    // This takes advantage of the fact that IEEE-754 floats can be compared
                    // with regular integer comparisons.
                    auto lock = (uint64_t(as<uint32_t>(float(outs[i].second))) << 32) | (uint64_t(i) & UINT64_C(0xFFFFFFFF));
                    for (auto c : get_conflicts(i, outs[i].first))
                        atomic_max(locks[c], lock);
                }

                // Check the locks to disable conflicting re-insertions
                #pragma omp for
                for (size_t i = first_node; i < bvh.node_count; i += u) {
                    if (!is_valid_insertion(outs[i]))
                        continue;
                    auto conflicts = get_conflicts(i, outs[i].first);
                    // Make sure that this node owns all the locks for each and every conflicting node
                    bool is_conflict_free = std::all_of(conflicts.begin(), conflicts.end(), [&] (size_t j) {
                        return (locks[j] & UINT64_C(0xFFFFFFFF)) == i;
                    });
                    if (!is_conflict_free)
                        outs[i] = invalid_insertion;
                }

                // Perform the reinsertions
                #pragma omp for
                for (size_t i = first_node; i < bvh.node_count; i += u) {
                    if (is_valid_insertion(outs[i]))
                        reinsert(i, outs[i].first);
                }

                // Update the bounding boxes of each node in the tree
                refit_in_parallel([] (typename Bvh::Node&) {});
            }

            // Compare the old SAH cost to the new one and decrease the number
            // of nodes that are ignored during the optimization if the change
            // in cost is below the threshold.
            auto new_cost = compute_cost(bvh);
            if (std::abs(new_cost - old_cost) <= threshold || iteration >= u) {
                if (u <= 1)
                    break;
                u = u - 1;
                iteration = 0;
            }
            old_cost = new_cost;
        }
    }
};

} // namespace bvh

#endif
