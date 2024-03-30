#ifdef _MSC_VER
#define NOMINMAX
#endif

#define TEST_NO_MAIN
#include "acutest.h"

#include "unit-primvar.h"
#include "primvar.hh"
#include "value-pprint.hh"
#include "usdGeom.hh"

using namespace tinyusdz::value;
using namespace tinyusdz::primvar;

void primvar_test(void) {

  // geom primvar
  {
    tinyusdz::GeomMesh mesh;
    std::vector<float> scalar_array = {1.0, 2.0, 3.0, 4.0};
    tinyusdz::Attribute attr;
    attr.set_value(scalar_array);
    tinyusdz::Property prop(attr, /* custom */false);

    mesh.props.emplace("primvars:myvar", prop);

    tinyusdz::GeomPrimvar primvar;
    TEST_CHECK(mesh.get_primvar("myvar", &primvar) == true);

    // non-existing primvar
    TEST_CHECK(mesh.get_primvar("myvar0", &primvar) == false);
    
  }

}
