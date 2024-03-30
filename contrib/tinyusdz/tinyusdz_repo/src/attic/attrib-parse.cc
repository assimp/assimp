
#if 0  // TODO: Remove
      if (type_name == "matrix4d") {
        double m[4][4];
        if (!ParseMatrix4d(m)) {
          PushError("Failed to parse value with type `matrix4d`.\n");
          return false;
        }

        std::cout << "matrix4d = \n";
        std::cout << m[0][0] << ", " << m[0][1] << ", " << m[0][2] << ", "
                  << m[0][3] << "\n";
        std::cout << m[1][0] << ", " << m[1][1] << ", " << m[1][2] << ", "
                  << m[1][3] << "\n";
        std::cout << m[2][0] << ", " << m[2][1] << ", " << m[2][2] << ", "
                  << m[2][3] << "\n";
        std::cout << m[3][0] << ", " << m[3][1] << ", " << m[3][2] << ", "
                  << m[3][3] << "\n";
      } else if (type_name == "bool") {
        if (array_qual) {
          // Assume people only use array access to vector<bool>
          std::vector<nonstd::optional<bool>> value;
          if (!ParseBasicTypeArray(&value)) {
            PushError(
                "Failed to parse array of string literal for `uniform "
                "bool[]`.\n");
          }
        } else {
          nonstd::optional<bool> value;
          if (!ReadBasicType(&value)) {
            PushError("Failed to parse value for `uniform bool`.\n");
          }
          if (value) {
            std::cout << "bool value = " << *value << "\n";
          }
        }
      } else if (type_name == "token") {
        if (array_qual) {
          if (!uniform_qual) {
            PushError("TODO: token[]\n");
            return false;
          }

          std::vector<nonstd::optional<std::string>> value;
          if (!ParseBasicTypeArray(&value)) {
            PushError(
                "Failed to parse array of string literal for `uniform "
                "token[]`.\n");
          }

          Variable var;
          Variable::Array arr;
          for (size_t i = 0; i < value.size(); i++) {
            Variable v;
            if (value[i]) {
              v.value = (*value[i]);
            } else {
              v.value = nonstd::monostate{};  // None
            }
            arr.values.push_back(v);
          }

          std::cout << "add token[] primattr: " << primattr_name << "\n";
          var.custom = custom_qual;
          (*props)[primattr_name] = var;
        } else {
          if (uniform_qual) {
            std::cout << "uniform_qual\n";
            std::string value;
            if (!ReadStringLiteral(&value)) {
              PushError(
                  "Failed to parse string literal for `uniform token`.\n");
            }
            std::cout << "StringLiteral = " << value << "\n";
          } else if (hasConnect(primattr_name)) {
            std::cout << "hasConnect\n";
            std::string value;  // TODO: Path
            if (!ReadPathIdentifier(&value)) {
              PushError("Failed to parse path identifier for `token`.\n");
            }
            std::cout << "Path identifier = " << value << "\n";

          } else if (hasOutputs(primattr_name)) {
            std::cout << "output\n";
            // Output node.
            // OK
          } else {
            std::cout << "??? " << primattr_name << "\n";
            std::string value;
            if (!ReadStringLiteral(&value)) {
              PushError("Failed to parse string literal for `token`.\n");
            }
          }
        }
      } else if (type_name == "int") {
        if (array_qual) {
          std::vector<nonstd::optional<int>> value;
          if (!ParseBasicTypeArray(&value)) {
            PushError("Failed to parse int array.\n");
          }
        } else {
          nonstd::optional<int> value;
          if (!ReadBasicType(&value)) {
            PushError("Failed to parse int value.\n");
          }
        }
      }  else if (type_name == "double2") {
        if (array_qual) {
          std::vector<std::array<double, 2>> values;
          if (!ParseTupleArray(&values)) {
            PushError("Failed to parse double2 array.\n");
          }
          std::cout << "double2 = \n";
          for (size_t i = 0; i < values.size(); i++) {
            std::cout << "(" << values[i][0] << ", " << values[i][1] << ")\n";
          }

          Variable::Array arr;
          for (size_t i = 0; i < values.size(); i++) {
            Variable v;
            v.value = values[i];
            arr.values.push_back(v);
          }

          Variable var;
          var.custom = custom_qual;
          (*props)[primattr_name] = var;
        } else {
          std::array<int, 2> value;
          if (!ParseBasicTypeTuple<int, 2>(&value)) {
            PushError("Failed to parse int2.\n");
          }
          std::cout << "int2 = (" << value[0] << ", " << value[1] << ")\n";

          Variable var;
          var.value = value;
          var.custom = custom_qual;

          (*props)[primattr_name] = var;
        }

      } else if (type_name == "float") {
        if (array_qual) {
          std::vector<nonstd::optional<float>> value;
          if (!ParseBasicTypeArray(&value)) {
            PushError("Failed to parse float array.\n");
          }
          std::cout << "float = \n";

          Variable::Array arr;
          for (size_t i = 0; i < value.size(); i++) {
            Variable v;
            if (value[i]) {
              std::cout << *value[i] << "\n";
              v.value = (*value[i]);
            } else {
              std::cout << "None\n";
              v.value = nonstd::monostate{};
            }
            arr.values.push_back(v);
          }
          Variable var;
          var.value = arr;
          (*props)[primattr_name] = var;
        } else if (hasConnect(primattr_name)) {
          std::string value;  // TODO: Path
          if (!ReadPathIdentifier(&value)) {
            PushError("Failed to parse path identifier for `token`.\n");
            return false;
          }
          std::cout << "Path identifier = " << value << "\n";
        } else {
          nonstd::optional<float> value;
          if (!ReadBasicType(&value)) {
            PushError("Failed to parse float.\n");
          }
          if (value) {
            std::cout << "float = " << *value << "\n";
          } else {
            std::cout << "float = None\n";
          }

          Variable var;
          if (value) {
            var.value = (*value);
          }

          (*props)[primattr_name] = var;
        }

        // optional: interpolation parameter
        std::map<std::string, Variable> meta;
        if (!ParseAttrMeta(&meta)) {
          PushError("Failed to parse PrimAttrib meta.");
          return false;
        }
        if (meta.count("interpolation")) {
          std::cout << "interpolation: "
                    << nonstd::get<std::string>(meta.at("interpolation").value)
                    << "\n";
        }
      } else if (type_name == "float2") {
        if (array_qual) {
          std::vector<std::array<float, 2>> value;
          if (!ParseTupleArray(&value)) {
            PushError("Failed to parse float2 array.\n");
          }
          std::cout << "float2 = \n";
          for (size_t i = 0; i < value.size(); i++) {
            std::cout << "(" << value[i][0] << ", " << value[i][1] << ")\n";
          }
        } else {
          std::array<float, 2> value;
          if (!ParseBasicTypeTuple<float, 2>(&value)) {
            PushError("Failed to parse float2.\n");
          }
          std::cout << "float2 = (" << value[0] << ", " << value[1] << ")\n";
        }

        // optional: interpolation parameter
        std::map<std::string, Variable> meta;
        if (!ParseAttrMeta(&meta)) {
          PushError("Failed to parse PrimAttr meta.");
          return false;
        }
        if (meta.count("interpolation")) {
          std::cout << "interpolation: "
                    << nonstd::get<std::string>(meta.at("interpolation").value)
                    << "\n";
        }

      } else if (type_name == "float3") {
        if (array_qual) {
          std::vector<std::array<float, 3>> value;
          if (!ParseTupleArray(&value)) {
            PushError("Failed to parse float3 array.\n");
          }
          std::cout << "float3 = \n";
          for (size_t i = 0; i < value.size(); i++) {
            std::cout << "(" << value[i][0] << ", " << value[i][1] << ", "
                      << value[i][2] << ")\n";
          }

          Variable var;
          var.value = value;
          var.custom = custom_qual;
          (*props)[primattr_name] = var;
        } else {
          std::array<float, 3> value;
          if (!ParseBasicTypeTuple<float, 3>(&value)) {
            PushError("Failed to parse float3.\n");
          }
          std::cout << "float3 = (" << value[0] << ", " << value[1] << ", "
                    << value[2] << ")\n";
        }

        std::map<std::string, Variable> meta;
        if (!ParseAttrMeta(&meta)) {
          PushError("Failed to parse PrimAttr meta.");
          return false;
        }
        if (meta.count("interpolation")) {
          std::cout << "interpolation: "
                    << nonstd::get<std::string>(meta.at("interpolation").value)
                    << "\n";
        }
      } else if (type_name == "float4") {
        if (array_qual) {
          std::vector<std::array<float, 4>> values;
          if (!ParseTupleArray(&values)) {
            PushError("Failed to parse float4 array.\n");
          }
          std::cout << "float4 = \n";
          for (size_t i = 0; i < values.size(); i++) {
            std::cout << "(" << values[i][0] << ", " << values[i][1] << ", "
                      << values[i][2] << ", " << values[i][3] << ")\n";
          }

          Variable::Array arr;
          for (size_t i = 0; i < values.size(); i++) {
            Variable v;
            v.value = values[i];
            arr.values.push_back(v);
          }

          Variable var;
          var.value = arr;
          var.custom = custom_qual;

          (*props)[primattr_name] = var;
        } else {
          std::array<float, 4> value;
          if (!ParseBasicTypeTuple<float, 4>(&value)) {
            PushError("Failed to parse float4.\n");
          }
          std::cout << "float4 = (" << value[0] << ", " << value[1] << ", "
                    << value[2] << ", " << value[3] << ")\n";
        }

        std::map<std::string, Variable> meta;
        if (!ParseAttrMeta(&meta)) {
          PushError("Failed to parse PrimAttr meta.");
          return false;
        }
        if (meta.count("interpolation")) {
          std::cout << "interpolation: "
                    << nonstd::get<std::string>(meta.at("interpolation").value)
                    << "\n";
        }
      } else if (type_name == "double") {
        if (array_qual) {
          std::vector<nonstd::optional<double>> values;
          if (!ParseBasicTypeArray(&values)) {
            PushError("Failed to parse double array.\n");
          }
          std::cout << "double = \n";
          for (size_t i = 0; i < values.size(); i++) {
            if (values[i]) {
              std::cout << *values[i] << "\n";
            } else {
              std::cout << "None\n";
            }
          }

          Variable::Array arr;
          for (size_t i = 0; i < values.size(); i++) {
            Variable v;
            if (values[i]) {
              v.value = *values[i];
            } else {
              v.value = nonstd::monostate{};
            }
            arr.values.push_back(v);
          }

          Variable var;
          var.value = arr;
          var.custom = custom_qual;
          (*props)[primattr_name] = var;

        } else if (hasConnect(primattr_name)) {
          std::string value;  // TODO: Path
          if (!ReadPathIdentifier(&value)) {
            PushError("Failed to parse path identifier for `token`.\n");
            return false;
          }
          std::cout << "Path identifier = " << value << "\n";
        } else {
          nonstd::optional<double> value;
          if (!ReadBasicType(&value)) {
            PushError("Failed to parse double.\n");
          }
          if (value) {
            std::cout << "double = " << *value << "\n";

            Variable var;
            var.value = Value(*value);
            var.custom = custom_qual;

            (*props)[primattr_name] = var;
          } else {
            std::cout << "double = None\n";
            // TODO: invalidate attr?
          }
        }

        // optional: interpolation parameter
        std::map<std::string, Variable> meta;
        if (!ParseAttrMeta(&meta)) {
          PushError("Failed to parse PrimAttr meta.");
          return false;
        }
        if (meta.count("interpolation")) {
          std::cout << "interpolation: "
                    << nonstd::get<std::string>(meta.at("interpolation").value)
                    << "\n";
        }
      } else if (type_name == "double2") {
        if (array_qual) {
          std::vector<std::array<double, 2>> values;
          if (!ParseTupleArray(&values)) {
            PushError("Failed to parse double2 array.\n");
          }
          std::cout << "double2 = \n";
          for (size_t i = 0; i < values.size(); i++) {
            std::cout << "(" << values[i][0] << ", " << values[i][1] << ")\n";
          }

          Variable::Array arr;
          for (size_t i = 0; i < values.size(); i++) {
            Variable v;
            v.value = values[i];
            arr.values.push_back(v);
          }

          Variable var;
          var.custom = custom_qual;
          (*props)[primattr_name] = var;
        } else {
          std::array<double, 2> value;
          if (!ParseBasicTypeTuple<double, 2>(&value)) {
            PushError("Failed to parse double2.\n");
          }
          std::cout << "double2 = (" << value[0] << ", " << value[1] << ")\n";

          Variable var;
          var.value = value;
          var.custom = custom_qual;

          (*props)[primattr_name] = var;
        }

        // optional: interpolation parameter
        std::map<std::string, Variable> meta;
        if (!ParseAttrMeta(&meta)) {
          PushError("Failed to parse PrimAttr meta.");
          return false;
        }
        if (meta.count("interpolation")) {
          std::cout << "interpolation: "
                    << nonstd::get<std::string>(meta.at("interpolation").value)
                    << "\n";
        }

      } else if (type_name == "double3") {
        if (array_qual) {
          std::vector<std::array<double, 3>> values;
          if (!ParseTupleArray(&values)) {
            PushError("Failed to parse double3 array.\n");
          }
          std::cout << "double3 = \n";
          for (size_t i = 0; i < values.size(); i++) {
            std::cout << "(" << values[i][0] << ", " << values[i][1] << ", "
                      << values[i][2] << ")\n";
          }

          Variable::Array arr;
          for (size_t i = 0; i < values.size(); i++) {
            Variable v;
            v.value = values[i];
            arr.values.push_back(v);
          }

          Variable var;
          var.value = arr;
          var.custom = custom_qual;
          (*props)[primattr_name] = var;

        } else {
          std::array<double, 3> value;
          if (!ParseBasicTypeTuple<double, 3>(&value)) {
            PushError("Failed to parse double3.\n");
          }
          std::cout << "double3 = (" << value[0] << ", " << value[1] << ", "
                    << value[2] << ")\n";

          Variable var;
          var.value = value;
          var.custom = custom_qual;
          (*props)[primattr_name] = var;
        }

        std::map<std::string, Variable> meta;
        if (!ParseAttrMeta(&meta)) {
          PushError("Failed to parse PrimAttr meta.");
          return false;
        }
        if (meta.count("interpolation")) {
          std::cout << "interpolation: "
                    << nonstd::get<std::string>(meta.at("interpolation").value)
                    << "\n";
        }

      } else if (type_name == "double4") {
        if (array_qual) {
          std::vector<std::array<double, 4>> values;
          if (!ParseTupleArray(&values)) {
            PushError("Failed to parse double4 array.\n");
          }
          std::cout << "double4 = \n";
          for (size_t i = 0; i < values.size(); i++) {
            std::cout << "(" << values[i][0] << ", " << values[i][1] << ", "
                      << values[i][2] << ", " << values[i][3] << ")\n";
          }

          Variable::Array arr;
          for (size_t i = 0; i < values.size(); i++) {
            Variable v;
            v.value = values[i];
            arr.values.push_back(v);
          }

          Variable var;
          var.value = arr;
          var.custom = custom_qual;
          (*props)[primattr_name] = var;
        } else {
          std::array<double, 4> value;
          if (!ParseBasicTypeTuple<double, 4>(&value)) {
            PushError("Failed to parse double4.\n");
          }
          std::cout << "double4 = (" << value[0] << ", " << value[1] << ", "
                    << value[2] << ", " << value[3] << ")\n";

          Variable var;
          var.value = value;
          var.custom = custom_qual;
          (*props)[primattr_name] = var;
        }

        std::map<std::string, Variable> meta;
        if (!ParseAttrMeta(&meta)) {
          PushError("Failed to parse PrimAttr meta.");
          return false;
        }
        if (meta.count("interpolation")) {
          std::cout << "interpolation: "
                    << nonstd::get<std::string>(meta.at("interpolation").value)
                    << "\n";
        }

      } else if (type_name == "color3f") {
        if (array_qual) {
          // TODO: connection
          std::vector<std::array<float, 3>> value;
          if (!ParseTupleArray(&value)) {
            PushError("Failed to parse color3f array.\n");
          }
          std::cout << "color3f = \n";
          for (size_t i = 0; i < value.size(); i++) {
            std::cout << "(" << value[i][0] << ", " << value[i][1] << ", "
                      << value[i][2] << ")\n";
          }

          Variable var;
          var.value = value;  // float3 array is the first-class type
          var.custom = custom_qual;
          (*props)[primattr_name] = var;

        } else if (hasConnect(primattr_name)) {
          std::string value;  // TODO: Path
          if (!ReadPathIdentifier(&value)) {
            PushError("Failed to parse path identifier for `token`.\n");
            return false;
          }
          std::cout << "Path identifier = " << value << "\n";
        } else {
          std::array<float, 3> value;
          if (!ParseBasicTypeTuple<float, 3>(&value)) {
            PushError("Failed to parse color3f.\n");
          }
          std::cout << "color3f = (" << value[0] << ", " << value[1] << ", "
                    << value[2] << ")\n";
        }

        std::map<std::string, Variable> meta;
        if (!ParseAttrMeta(&meta)) {
          PushError("Failed to parse PrimAttr meta.");
          return false;
        }
        if (meta.count("interpolation")) {
          std::cout << "interpolation: "
                    << nonstd::get<std::string>(meta.at("interpolation").value)
                    << "\n";
        }
        if (meta.count("customData")) {
          std::cout << "has customData\n";
        }

      } else if (type_name == "normal3f") {
        if (array_qual) {
          std::vector<std::array<float, 3>> value;
          if (!ParseTupleArray(&value)) {
            PushError("Failed to parse normal3f array.\n");
          }
          std::cout << "normal3f = \n";
          for (size_t i = 0; i < value.size(); i++) {
            std::cout << "(" << value[i][0] << ", " << value[i][1] << ", "
                      << value[i][2] << ")\n";
          }

          Variable var;
          var.value = value;  // float3 array is the first-class type
          var.custom = custom_qual;
          (*props)[primattr_name] = var;

        } else if (hasConnect(primattr_name)) {
          std::string value;  // TODO: Path
          if (!ReadPathIdentifier(&value)) {
            PushError("Failed to parse path identifier for `token`.\n");
            return false;
          }
          std::cout << "Path identifier = " << value << "\n";
        } else {
          std::array<float, 3> value;
          if (!ParseBasicTypeTuple<float, 3>(&value)) {
            PushError("Failed to parse normal3f.\n");
          }
          std::cout << "normal3f = (" << value[0] << ", " << value[1] << ", "
                    << value[2] << ")\n";

          Variable var;
          var.value = value;
          var.custom = custom_qual;
          (*props)[primattr_name] = var;
        }

        std::map<std::string, Variable> meta;
        if (!ParseAttrMeta(&meta)) {
          PushError("Failed to parse PrimAttr meta.");
          return false;
        }
        if (meta.count("interpolation")) {
          std::cout << "interpolation: "
                    << nonstd::get<std::string>(meta.at("interpolation").value)
                    << "\n";
        }

      } else if (type_name == "point3f") {
        if (array_qual) {
          std::vector<std::array<float, 3>> value;
          if (!ParseTupleArray(&value)) {
            PushError("Failed to parse point3f array.\n");
          }
          std::cout << "point3f = \n";
          for (size_t i = 0; i < value.size(); i++) {
            std::cout << "(" << value[i][0] << ", " << value[i][1] << ", "
                      << value[i][2] << ")\n";
          }

          Variable var;
          var.value = value;  // float3 array is the first-class type
          var.custom = custom_qual;
          (*props)[primattr_name] = var;

        } else {
          std::array<float, 3> value;
          if (!ParseBasicTypeTuple<float, 3>(&value)) {
            PushError("Failed to parse point3f.\n");
          }
          std::cout << "point3f = (" << value[0] << ", " << value[1] << ", "
                    << value[2] << ")\n";

          Variable var;
          var.value = value;
          var.custom = custom_qual;
          (*props)[primattr_name] = var;
        }

        std::map<std::string, Variable> meta;
        if (!ParseAttrMeta(&meta)) {
          PushError("Failed to parse PrimAttr meta.");
          return false;
        }
        if (meta.count("interpolation")) {
          std::cout << "interpolation: "
                    << nonstd::get<std::string>(meta.at("interpolation").value)
                    << "\n";
        }

      } else if (type_name == "texCoord2f") {
        if (array_qual) {
          std::vector<std::array<float, 2>> value;
          if (!ParseTupleArray(&value)) {
            PushError("Failed to parse texCoord2f array.\n");
          }
          std::cout << "texCoord2f = \n";
          for (size_t i = 0; i < value.size(); i++) {
            std::cout << "(" << value[i][0] << ", " << value[i][1] << ")\n";
          }
        } else {
          std::array<float, 2> value;
          if (!ParseBasicTypeTuple<float, 2>(&value)) {
            PushError("Failed to parse texCoord2f.\n");
          }
          std::cout << "texCoord2f = (" << value[0] << ", " << value[1]
                    << ")\n";
        }

        std::map<std::string, Variable> meta;
        if (!ParseAttrMeta(&meta)) {
          PushError("Failed to parse PrimAttr meta.");
          return false;
        }

        if (meta.count("interpolation")) {
          std::cout << "interpolation: "
                    << nonstd::get<std::string>(meta.at("interpolation").value)
                    << "\n";
        }

      } else if (type_name == "rel") {
        Rel rel;
        if (ParseRel(&rel)) {
          PushError("Failed to parse rel.\n");
        }

        std::cout << "rel: " << rel.path << "\n";

        // 'todos'

      } else {
        PushError("TODO: ParsePrimAttr: Implement value parser for type: " +
                   type_name + "\n");
        return false;
      }
#endif

