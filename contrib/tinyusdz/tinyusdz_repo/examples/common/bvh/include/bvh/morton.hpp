#ifndef BVH_MORTON_HPP
#define BVH_MORTON_HPP

#include <cstddef>
#include <climits>
#include <cassert>

#include "bvh/utilities.hpp"

namespace bvh {

/// Split an unsigned integer such that its bits are spaced by 2 zeros.
/// For instance, morton_split(0b00110010) = 0b000000001001000000001000.
template <typename Morton>
Morton morton_split(Morton x) {
    constexpr size_t bit_count = sizeof(Morton) * CHAR_BIT;
    constexpr size_t log_bits = round_up_log2(bit_count);
    auto mask = Morton(-1) >> (bit_count / 2);
    x &= mask;
    for (size_t i = log_bits - 1, n = size_t{1} << i; i > 0; --i, n >>= 1) {
        mask = (mask | (mask << n)) & ~(mask << (n / 2));
        x = (x | (x << n)) & mask;
    }
    return x;
}

/// Morton-encode three unsigned integers into one.
template <typename Morton>
Morton morton_encode(Morton x, Morton y, Morton z) {
    return morton_split(x)       |
          (morton_split(y) << 1) |
          (morton_split(z) << 2);
}

template <typename Morton, typename Scalar>
class MortonEncoder {
    Vector3<Scalar> world_to_grid;
    Vector3<Scalar> grid_offset;
    size_t grid_dim;

public:
    static constexpr size_t max_grid_dim = 1 << (sizeof(Morton) * CHAR_BIT / 3);

    MortonEncoder(const BoundingBox<Scalar>& bbox, size_t grid_dim = max_grid_dim)
        : grid_dim(grid_dim)
    {
        assert(grid_dim <= max_grid_dim);
        world_to_grid = Scalar(grid_dim) * bbox.diagonal().inverse();
        grid_offset   = -bbox.min * world_to_grid;
    }

    /// Morton-encode a 3D point into one unsigned integer.
    Morton encode(const Vector3<Scalar>& point) const {
        auto grid_position = point * world_to_grid + grid_offset;
        Morton x = std::min(Morton(grid_dim - 1), Morton(std::max(grid_position[0], Scalar(0))));
        Morton y = std::min(Morton(grid_dim - 1), Morton(std::max(grid_position[1], Scalar(0))));
        Morton z = std::min(Morton(grid_dim - 1), Morton(std::max(grid_position[2], Scalar(0))));
        return morton_encode(x, y, z);
    }
};

} // namespace bvh

#endif
