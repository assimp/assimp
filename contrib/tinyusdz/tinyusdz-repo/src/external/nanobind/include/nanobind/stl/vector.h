#pragma once

#include "detail/nb_list.h"
#include <vector>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <typename Type, typename Alloc> struct type_caster<std::vector<Type, Alloc>>
 : list_caster<std::vector<Type, Alloc>, Type> { };

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
