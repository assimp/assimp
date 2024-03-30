// TODO: merge into api-tutorial example.
#include "usda-writer.hh"

#include <iostream>

//
// create simple scene composed of Xform and Mesh.
//
void SimpleScene(tinyusdz::Stage *stage)
{
  //
  // tinyusdz currently does not provide scene construction API yet, so edit parameters directly.
  //
  // TinyUSDZ does not use mutex, smart pointers(e.g. shared_ptr) and C++ exception.
  // API is not multi-thread safe, thus if you need to manipulate a scene in multi-threaded context,
  // The app must take care of resource locks in app layer.
  //
  tinyusdz::Xform xform;
  xform.name = "root";

  tinyusdz::XformOp op;
  op.op_type = tinyusdz::XformOp::OpType::Translate;
  tinyusdz::value::double3 translate;
  translate[0] = 1.0;
  translate[1] = 2.0;
  translate[2] = 3.0;
  op.set_value(translate);

  xform.xformOps.push_back(op);

  tinyusdz::GeomMesh mesh;
  mesh.name = "quad";

  {
    std::vector<tinyusdz::value::point3f> pts;
    pts.push_back({0.0f, 0.0f, 0.0f});

    pts.push_back({1.0f, 0.0f, 0.0f});

    pts.push_back({1.0f, 1.0f, 0.0f});

    pts.push_back({0.0f, 1.0f, 0.0f});

    mesh.points.set_value(pts);
  }

  {
    // quad plane composed of 2 triangles.
    std::vector<int> indices;
    std::vector<int> counts;
    counts.push_back(3);
    counts.push_back(3);
    mesh.faceVertexCounts.set_value(counts);

    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);

    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(3);

    mesh.faceVertexIndices.set_value(indices);
  }

  // primvar and custom attribute can be added to generic Property container `props`
  {
    // primvar is simply an attribute with prefix `primvars:`
    //
    // texCoord2f[] primvars:uv = [ ... ] ( interpolation = "vertex" )
    // int[] primvars:uv:indices = [ ... ]
    //
    {
      tinyusdz::Attribute uvAttr;
      std::vector<tinyusdz::value::texcoord2f> uvs;

      uvs.push_back({0.0f, 0.0f});
      uvs.push_back({1.0f, 0.0f});
      uvs.push_back({1.0f, 1.0f});
      uvs.push_back({0.0f, 1.0f});

      // Fast path. Set the value directly to Attribute.
      uvAttr.set_value(uvs);

      // or we can first build primvar::PrimVar
      //tinyusdz::primvar::PrimVar uvVar;
      //uvVar.set_value(uvs);
      //uvAttr.set_var(std::move(uvVar));

      // Currently `interpolation` is described in Attribute metadataum.
      tinyusdz::AttrMeta meta;
      meta.interpolation = tinyusdz::Interpolation::Vertex;
      uvAttr.metas() = meta;

      tinyusdz::Property uvProp(uvAttr, /* custom*/false);

      mesh.props.emplace("primvars:uv", uvProp);

      // ----------------------

      tinyusdz::Attribute uvIndexAttr;
      std::vector<int> uvIndices;

      // FIXME: Validate
      uvIndices.push_back(0);
      uvIndices.push_back(1);
      uvIndices.push_back(2);
      uvIndices.push_back(3);


      tinyusdz::primvar::PrimVar uvIndexVar;
      uvIndexVar.set_value(uvIndices);
      uvIndexAttr.set_var(std::move(uvIndexVar));

      tinyusdz::Property uvIndexProp(uvIndexAttr, /* custom*/false);
      mesh.props.emplace("primvars:uv:indices", uvIndexProp);

    }

    // `custom uniform double myvalue = 3.0 ( hidden = 0 )`
    {
      tinyusdz::Attribute attrib;
      double myvalue = 3.0;
      tinyusdz::primvar::PrimVar var;
      var.set_value(myvalue);
      attrib.set_var(std::move(var));

      attrib.variability() = tinyusdz::Variability::Uniform;

      tinyusdz::AttrMeta meta;
      meta.hidden = false;
      attrib.metas() = meta;

      tinyusdz::Property prop(attrib, /* custom*/true);

      mesh.props.emplace("myvalue", prop);
    }

  }

  tinyusdz::Prim meshPrim(mesh);
  tinyusdz::Prim xformPrim(xform);

  // [Xform]
  //  |
  //  +- [Mesh]
  //
  xformPrim.children().emplace_back(std::move(meshPrim));

  stage->root_prims().emplace_back(std::move(xformPrim));
}

int main(int argc, char **argv)
{
  tinyusdz::Stage stage; // empty scene

  SimpleScene(&stage);

  std::string warn;
  std::string err;
  bool ret = tinyusdz::usda::SaveAsUSDA("output.usda", stage, &warn, &err);

  if (warn.size()) {
    std::cout << "WARN: " << warn << "\n";
  }

  if (err.size()) {
    std::cerr << "ERR: " << err << "\n";
  }

  return ret ? EXIT_SUCCESS : -1;
}
