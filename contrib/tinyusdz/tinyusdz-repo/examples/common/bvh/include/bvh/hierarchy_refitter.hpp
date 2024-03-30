#ifndef BVH_HIERARCHY_REFITTER_HPP
#define BVH_HIERARCHY_REFITTER_HPP

#include "bvh/bvh.hpp"
#include "bvh/bottom_up_algorithm.hpp"
#include "bvh/platform.hpp"

namespace bvh {

template <typename Bvh>
class HierarchyRefitter : public BottomUpAlgorithm<Bvh> {
protected:
    using BottomUpAlgorithm<Bvh>::bvh;
    using BottomUpAlgorithm<Bvh>::traverse_in_parallel;
    using BottomUpAlgorithm<Bvh>::parents;

    template <typename UpdateLeaf>
    void refit_in_parallel(const UpdateLeaf& update_leaf) {
        bvh::assert_in_parallel();

        // Refit every node of the tree in parallel
        traverse_in_parallel(
            [&] (size_t i) { update_leaf(bvh.nodes[i]); },
            [&] (size_t i) {
                auto& node = bvh.nodes[i];
                auto first_child = node.first_child_or_primitive;
                node.bounding_box_proxy() = bvh.nodes[first_child + 0]
                    .bounding_box_proxy()
                    .to_bounding_box()
                    .extend(bvh.nodes[first_child + 1].bounding_box_proxy());
            });
    }

public:
    HierarchyRefitter(Bvh& bvh)
        : BottomUpAlgorithm<Bvh>(bvh)
    {}

    template <typename UpdateLeaf>
    void refit(const UpdateLeaf& update_leaf) {
        #pragma omp parallel
        {
            refit_in_parallel(update_leaf);
        }
    }
};

} // namespace bvh

#endif
