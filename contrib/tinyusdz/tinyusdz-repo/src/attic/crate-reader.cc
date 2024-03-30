#if 0
bool CrateReader::ParseAttribute(const FieldValuePairVector &fvs,
                                 PrimAttrib *attr,
                                 const std::string &prop_name) {
  bool success = false;

  DCOUT("fvs.size = " << fvs.size());

  bool has_connection{false};

  Variability variability{Variability::Varying};
  Interpolation interpolation{Interpolation::Invalid};

  // Check if required field exists.
  if (!HasFieldValuePair(fvs, kTypeName, kToken)) {
    PUSH_ERROR(
        "\"typeName\" field with `token` type must exist for Attribute data.");
    return false;
  }

  if (!HasField(kDefault)) {
    PUSH_ERROR("\"default\" field must exist for Attribute data.");
    return false;
  }

  //
  // Parse properties
  //
  for (const auto &fv : fvs) {
    DCOUT("===  fvs.first " << fv.first
                            << ", second: " << fv.second.type_name());
    if ((fv.first == "typeName") && (fv.second.type_name() == "Token")) {
      attr->set_type_name(fv.second.value<value::token>().str());
      DCOUT("typeName: " << attr->type_name());
    } else if (fv.first == "default") {
      // Nothing to do at there. Process `default` in the later
      continue;
    } else if (fv.first == "targetPaths") {
      // e.g. connection to Material.
      const ListOp<Path> paths = fv.second.value<ListOp<Path>>();

      DCOUT("ListOp<Path> = " << to_string(paths));
      // Currently we only support single explicit path.
      if ((paths.GetExplicitItems().size() == 1)) {
        const Path &path = paths.GetExplicitItems()[0];
        (void)path;

        DCOUT("full path: " << path.full_path_name());
        //DCOUT("local path: " << path.local_path_name());

        primvar::PrimVar var;
        var.set_scalar(path);
        attr->set_var(std::move(var));

        has_connection = true;

      } else {
        return false;
      }
    } else if (fv.first == "connectionPaths") {
      // e.g. connection to texture file.
      const ListOp<Path> paths = fv.second.value<ListOp<Path>>();

      DCOUT("ListOp<Path> = " << to_string(paths));

      // Currently we only support single explicit path.
      if ((paths.GetExplicitItems().size() == 1)) {
        const Path &path = paths.GetExplicitItems()[0];
        (void)path;

        DCOUT("full path: " << path.full_path_name());
        //DCOUT("local path: " << path.local_path_name());

        primvar::PrimVar var;
        var.set_scalar(path);
        attr->set_var(std::move(var));

        has_connection = true;

      } else {
        return false;
      }
    } else if ((fv.first == "variablity") &&
               (fv.second.type_name() == "Variability")) {
      variability = fv.second.value<Variability>();
    } else if ((fv.first == "interpolation") &&
               (fv.second.type_name() == "Token")) {
      interpolation =
          InterpolationFromString(fv.second.value<value::token>().str());
    } else {
      DCOUT("TODO: name: " << fv.first
                           << ", type: " << fv.second.type_name());
    }
  }

  attr->variability = variability;
  attr->meta.interpolation = interpolation;

  //
  // Decode value(stored in "default" field)
  //
  const auto fvRet = GetFieldValuePair(fvs, kDefault);
  if (!fvRet) {
    // This code path should not happen. Just in case.
    PUSH_ERROR("`default` field not found.");
    return false;
  }
  const auto fv = fvRet.value();

  auto add1DArraySuffix = [](const std::string &a) -> std::string {
    return a + "[]";
  };

  {
    if (fv.first == "default") {
      attr->name = prop_name;

      DCOUT("fv.second.type_name = " << fv.second.type_name());

#define PROC_SCALAR(__tyname, __ty)                             \
  }                                                             \
  else if (fv.second.type_name() == __tyname) {               \
    auto ret = fv.second.get_value<__ty>();                     \
    if (!ret) {                                                 \
      PUSH_ERROR("Failed to decode " << __tyname << " value."); \
      return false;                                             \
    }                                                           \
    primvar::PrimVar var; \
    var.set_scalar(ret.value()); \
    attr->set_var(std::move(var));                          \
    success = true;

#define PROC_ARRAY(__tyname, __ty)                                  \
  }                                                                 \
  else if (fv.second.type_name() == add1DArraySuffix(__tyname)) { \
    auto ret = fv.second.get_value<std::vector<__ty>>();            \
    if (!ret) {                                                     \
      PUSH_ERROR("Failed to decode " << __tyname << "[] value.");   \
      return false;                                                 \
    }                                                               \
    primvar::PrimVar var; \
    var.set_scalar(ret.value()); \
    attr->set_var(std::move(var));                          \
    success = true;

      if (0) {  // dummy
        PROC_SCALAR(value::kFloat, float)
        PROC_SCALAR(value::kBool, bool)
        PROC_SCALAR(value::kInt, int)
        PROC_SCALAR(value::kFloat2, value::float2)
        PROC_SCALAR(value::kFloat3, value::float3)
        PROC_SCALAR(value::kFloat4, value::float4)
        PROC_SCALAR(value::kHalf2, value::half2)
        PROC_SCALAR(value::kHalf3, value::half3)
        PROC_SCALAR(value::kHalf4, value::half4)
        PROC_SCALAR(value::kToken, value::token)
        PROC_SCALAR(value::kAssetPath, value::AssetPath)

        PROC_SCALAR(value::kMatrix2d, value::matrix2d)
        PROC_SCALAR(value::kMatrix3d, value::matrix3d)
        PROC_SCALAR(value::kMatrix4d, value::matrix4d)

        // It seems `token[]` is defined as `TokenVector` in CrateData.
        // We tret it as scalar
        PROC_SCALAR("TokenVector", std::vector<value::token>)

        // TODO(syoyo): Use constexpr concat
        PROC_ARRAY(value::kInt, int32_t)
        PROC_ARRAY(value::kUInt, uint32_t)
        PROC_ARRAY(value::kFloat, float)
        PROC_ARRAY(value::kFloat2, value::float2)
        PROC_ARRAY(value::kFloat3, value::float3)
        PROC_ARRAY(value::kFloat4, value::float4)
        PROC_ARRAY(value::kToken, value::token)

        PROC_ARRAY(value::kMatrix2d, value::matrix2d)
        PROC_ARRAY(value::kMatrix3d, value::matrix3d)
        PROC_ARRAY(value::kMatrix4d, value::matrix4d)

        PROC_ARRAY(value::kPoint3h, value::point3h)
        PROC_ARRAY(value::kPoint3f, value::point3f)
        PROC_ARRAY(value::kPoint3d, value::point3d)

        PROC_ARRAY(value::kVector3h, value::vector3h)
        PROC_ARRAY(value::kVector3f, value::vector3f)
        PROC_ARRAY(value::kVector3d, value::vector3d)

        PROC_ARRAY(value::kNormal3h, value::normal3h)
        PROC_ARRAY(value::kNormal3f, value::normal3f)
        PROC_ARRAY(value::kNormal3d, value::normal3d)

        // PROC_ARRAY("Vec2fArray", value::float2)
        // PROC_ARRAY("Vec3fArray", value::float3)
        // PROC_ARRAY("Vec4fArray", value::float4)
        // PROC_ARRAY("IntArray", int)
        // PROC_ARRAY(kTokenArray, value::token)

      } else {
        PUSH_ERROR("TODO: " + fv.second.type_name());
      }
    }
  }

  if (!success && has_connection) {
    // Attribute has a connection(has a path and no `default` field)
    success = true;
  }

  return success;
}
#endif

