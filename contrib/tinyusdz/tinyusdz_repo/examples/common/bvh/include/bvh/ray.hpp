#ifndef BVH_RAY_HPP
#define BVH_RAY_HPP

#include "bvh/vector.hpp"

namespace bvh {

/// A ray, defined by an origin and a direction, with minimum and maximum distances along the direction from the origin.
template <typename Scalar>
struct Ray {
    Vector3<Scalar> origin;
    Vector3<Scalar> direction;
    Scalar tmin;
    Scalar tmax;

    Ray() = default;
    Ray(const Vector3<Scalar>& origin,
        const Vector3<Scalar>& direction,
        Scalar tmin = Scalar(0),
        Scalar tmax = std::numeric_limits<Scalar>::max())
        : origin(origin), direction(direction), tmin(tmin), tmax(tmax)
    {}
};

} // namespace bvh

#endif
