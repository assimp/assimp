#ifndef BVH_SINGLE_RAY_TRAVERSAL_HPP
#define BVH_SINGLE_RAY_TRAVERSAL_HPP

#include <cassert>

#include "bvh/bvh.hpp"
#include "bvh/ray.hpp"
#include "bvh/node_intersectors.hpp"
#include "bvh/utilities.hpp"

namespace bvh {

/// Single ray traversal algorithm, using the provided ray-node intersector.
template <typename Bvh, size_t StackSize = 64, typename NodeIntersector = FastNodeIntersector<Bvh>>
class SingleRayTraverser {
public:
    static constexpr size_t stack_size = StackSize;

private:
    using Scalar = typename Bvh::ScalarType;

    struct Stack {
        using Element = typename Bvh::IndexType;

        Element elements[stack_size];
        size_t size = 0;

        void push(const Element& t) {
            assert(size < stack_size);
            elements[size++] = t;
        }

        Element pop() {
            assert(!empty());
            return elements[--size];
        }

        bool empty() const { return size == 0; }
    };

    template <typename PrimitiveIntersector, typename Statistics>
    bvh_always_inline
    std::optional<typename PrimitiveIntersector::Result>& intersect_leaf(
        const typename Bvh::Node& node,
        Ray<Scalar>& ray,
        std::optional<typename PrimitiveIntersector::Result>& best_hit,
        PrimitiveIntersector& primitive_intersector,
        Statistics& statistics) const
    {
        assert(node.is_leaf());
        size_t begin = node.first_child_or_primitive;
        size_t end   = begin + node.primitive_count;
        statistics.intersections += end - begin;
        for (size_t i = begin; i < end; ++i) {
            if (auto hit = primitive_intersector.intersect(i, ray)) {
                best_hit = hit;
                if (primitive_intersector.any_hit)
                    return best_hit;
                ray.tmax = hit->distance();
            }
        }
        return best_hit;
    }

    template <typename PrimitiveIntersector, typename Statistics>
    bvh_always_inline
    std::optional<typename PrimitiveIntersector::Result>
    intersect(Ray<Scalar> ray, PrimitiveIntersector& primitive_intersector, Statistics& statistics) const {
        auto best_hit = std::optional<typename PrimitiveIntersector::Result>(std::nullopt);

        // If the root is a leaf, intersect it and return
        if (bvh_unlikely(bvh.nodes[0].is_leaf()))
            return intersect_leaf(bvh.nodes[0], ray, best_hit, primitive_intersector, statistics);

        NodeIntersector node_intersector(ray);

        // This traversal loop is eager, because it immediately processes leaves instead of pushing them on the stack.
        // This is generally beneficial for performance because intersections will likely be found which will
        // allow to cull more subtrees with the ray-box test of the traversal loop.
        Stack stack;
        auto* left_child = &bvh.nodes[bvh.nodes[0].first_child_or_primitive];
        while (true) {
            statistics.traversal_steps++;

            auto* right_child = left_child + 1;
            auto distance_left  = node_intersector.intersect(*left_child,  ray);
            auto distance_right = node_intersector.intersect(*right_child, ray);

            if (distance_left.first <= distance_left.second) {
                if (bvh_unlikely(left_child->is_leaf())) {
                    if (intersect_leaf(*left_child, ray, best_hit, primitive_intersector, statistics) &&
                        primitive_intersector.any_hit)
                        break;
                    left_child = nullptr;
                }
            } else
                left_child = nullptr;

            if (distance_right.first <= distance_right.second) {
                if (bvh_unlikely(right_child->is_leaf())) {
                    if (intersect_leaf(*right_child, ray, best_hit, primitive_intersector, statistics) &&
                        primitive_intersector.any_hit)
                        break;
                    right_child = nullptr;
                }
            } else
                right_child = nullptr;

            if (left_child) {
                if (right_child) {
                    if (distance_left.first > distance_right.first)
                        std::swap(left_child, right_child);
                    stack.push(right_child->first_child_or_primitive);
                }
                left_child = &bvh.nodes[left_child->first_child_or_primitive];
            } else if (right_child) {
                left_child = &bvh.nodes[right_child->first_child_or_primitive];
            } else {
                if (stack.empty())
                    break;
                left_child = &bvh.nodes[stack.pop()];
            }
        }

        return best_hit;
    }

    const Bvh& bvh;

public:
    /// Statistics collected during traversal.
    struct Statistics {
        size_t traversal_steps = 0;
        size_t intersections   = 0;
    };

    SingleRayTraverser(const Bvh& bvh)
        : bvh(bvh)
    {}

    /// Intersects the BVH with the given ray and intersector.
    template <typename PrimitiveIntersector>
    bvh_always_inline
    std::optional<typename PrimitiveIntersector::Result>
    traverse(const Ray<Scalar>& ray, PrimitiveIntersector& intersector) const {
        struct {
            struct Empty {
                Empty& operator ++ (int)    { return *this; }
                Empty& operator ++ ()       { return *this; }
                Empty& operator += (size_t) { return *this; }
            } traversal_steps, intersections;
        } statistics;
        return intersect(ray, intersector, statistics);
    }

    /// Intersects the BVH with the given ray and intersector.
    /// Record statistics on the number of traversal and intersection steps.
    template <typename PrimitiveIntersector>
    bvh_always_inline
    std::optional<typename PrimitiveIntersector::Result>
    traverse(const Ray<Scalar>& ray, PrimitiveIntersector& primitive_intersector, Statistics& statistics) const {
        return intersect(ray, primitive_intersector, statistics);
    }
};

} // namespace bvh

#endif
