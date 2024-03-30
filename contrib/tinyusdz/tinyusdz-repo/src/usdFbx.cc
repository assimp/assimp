// SPDX-License-Identifier: MIT
//
#include <string>

#include "tinyusdz.hh"
#include "io-util.hh"
#include "usdFbx.hh"

//#include "math-util.inc"

#ifdef TINYUSDZ_USE_USDFBX

#include "external/OpenFBX/src/ofbx.h"

#endif

namespace tinyusdz {

namespace usdFbx {

namespace {



}

bool ReadFbxFromFile(const std::string &filepath, tinyusdz::GPrim *prim, std::string *err)
{
#if !defined(TINYUSDZ_USE_USDFBX)
  (void)filepath;
  (void)prim;
  if (err) {
    (*err) = "usdFbx is disabled in this build.\n";
  }
  return false;
#else

  std::vector<uint8_t> buf;
  if (!io::ReadWholeFile(&buf, err, filepath, /* filesize max */ 0,
                         /* user_ptr */ nullptr)) {
    return false;
  }

  std::string str(reinterpret_cast<const char *>(buf.data()), buf.size());

  return ReadFbxFromString(str, prim, err);

#endif

}


bool ReadFbxFromString(const std::string &str, tinyusdz::GPrim *prim, std::string *err)
{
#if !defined(TINYUSDZ_USE_USDFBX)
  (void)str;
  (void)prim;
  if (err) {
    (*err) = "usdFbx is disabled in this build.\n";
  }
  return false;
#else

  (void)str;
  (void)prim;
  if (err) {
    (*err) = "TODO: Implement usdFbx importer.\n";
  }

  return false;
#endif
}

} // namespace usdFbx

} // tinyusdz
