#ifndef BVH_PRIMITIVE_INTERSECTORS_HPP
#define BVH_PRIMITIVE_INTERSECTORS_HPP

#include <optional>

#include "bvh/ray.hpp"

namespace bvh {

/// Base class for primitive intersectors.
template <typename Bvh, typename Primitive, bool Permuted, bool AnyHit>
struct PrimitiveIntersector {
    PrimitiveIntersector(const Bvh& bvh, const Primitive* primitives)
        : bvh(bvh), primitives(primitives)
    {}

    std::pair<const Primitive&, size_t> primitive_at(size_t index) const {
        index = Permuted ? index : bvh.primitive_indices[index];
        return std::pair<const Primitive&, size_t> { primitives[index], index };
    }

    const Bvh& bvh;
    const Primitive* primitives = nullptr;

    static constexpr bool any_hit = AnyHit;

protected:
    ~PrimitiveIntersector() {}
};

/// An intersector that looks for the closest intersection.
template <typename Bvh, typename Primitive, bool Permuted = false>
struct ClosestPrimitiveIntersector : public PrimitiveIntersector<Bvh, Primitive, Permuted, false> {
    using Scalar       = typename Primitive::ScalarType;
    using Intersection = typename Primitive::IntersectionType;

    struct Result {
        size_t       primitive_index;
        Intersection intersection;

        Scalar distance() const { return intersection.distance(); }
    };

    ClosestPrimitiveIntersector(const Bvh& bvh, const Primitive* primitives)
        : PrimitiveIntersector<Bvh, Primitive, Permuted, false>(bvh, primitives)
    {}

    std::optional<Result> intersect(size_t index, const Ray<Scalar>& ray) const {
        auto [p, i] = this->primitive_at(index);
        if (auto hit = p.intersect(ray))
            return std::make_optional(Result { i, *hit });
        return std::nullopt;
    }
};

/// An intersector that exits after the first hit and only stores the distance to the primitive.
template <typename Bvh, typename Primitive, bool Permuted = false>
struct AnyPrimitiveIntersector : public PrimitiveIntersector<Bvh, Primitive, Permuted, true> {
    using Scalar = typename Primitive::ScalarType;

    struct Result {
        Scalar t;
        Scalar distance() const { return t; }
    };

    AnyPrimitiveIntersector(const Bvh& bvh, const Primitive* primitives)
        : PrimitiveIntersector<Bvh, Primitive, Permuted, true>(bvh, primitives)
    {}

    std::optional<Result> intersect(size_t index, const Ray<Scalar>& ray) const {
        auto [p, i] = this->primitive_at(index);
        if (auto hit = p.intersect(ray))
            return std::make_optional(Result { hit->distance() });
        return std::nullopt;
    }
};

} // namespace bvh

#endif
