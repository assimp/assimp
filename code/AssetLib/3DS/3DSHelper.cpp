#include "3DSHelper.h"

namespace Assimp {
namespace D3DS {

// Helper function implementation inside 3DSHelper.h
bool operator&(Discreet3DS::AnimatedKey lhs, Discreet3DS::AnimatedKey rhs) {
    return static_cast<uint16_t>(lhs) & static_cast<uint16_t>(rhs);
}

} // namespace D3DS
} // namespace Assimp
