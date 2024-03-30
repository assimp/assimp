// SPDX-License-Identifier: MIT
// 
// Built-in .obj import plugIn.
// Import only. Writing scene data as .obj is not supported.
//
// example usage 
//
// def "mesh" (
//   prepend references = @bunny.obj@
// )
// {
//    ...
// }

#pragma once

#include "tinyusdz.hh"

namespace tinyusdz {

namespace usdObj {

bool ReadObjFromString(const std::string &str, GPrim *prim, std::string *err = nullptr);
bool ReadObjFromFile(const std::string &filepath, GPrim *prim, std::string *err = nullptr);

} // namespace usdObj

} // namespace tinyusdz
