// Copyright 2022 The Draco Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef DRACO_METADATA_PROPERTY_TABLE_H_
#define DRACO_METADATA_PROPERTY_TABLE_H_

#include "draco/draco_features.h"

#ifdef DRACO_TRANSCODER_SUPPORTED

#include <memory>
#include <string>
#include <vector>

namespace draco {

// Describes a property table as defined in the EXT_structural_metadata glTF
// extension, including property table schema and table properties (columns).
class PropertyTable {
 public:
  // Describes property table schema in the form of a JSON object.
  struct Schema {
    // JSON object of the schema.
    // TODO(vytyaz): Consider using a third_party/json library. Currently there
    // is a conflict between Filament's assert_invariant() macro and JSON
    // library's assert_invariant() method that causes compile errors in Draco
    // visualization library.
    class Object {
     public:
      enum Type { OBJECT, ARRAY, STRING, INTEGER, BOOLEAN };

      // Constructors.
      Object() : Object("") {}
      explicit Object(const std::string& name)
          : name_(name), type_(OBJECT), integer_(0), boolean_(false) {}
      Object(const std::string& name, const std::string& value) : Object(name) {
        SetString(value);
      }
      Object(const std::string& name, const char* value) : Object(name) {
        SetString(value);
      }
      Object(const std::string& name, int value) : Object(name) {
        SetInteger(value);
      }
      Object(const std::string& name, bool value) : Object(name) {
        SetBoolean(value);
      }

      // Methods for comparing two objects.
      bool operator==(const Object& other) const;
      bool operator!=(const Object& other) const { return !(*this == other); }

      // Method for copying the object.
      void Copy(const Object& src);

      // Methods for getting object name and type.
      const std::string& GetName() const { return name_; }
      Type GetType() const { return type_; }

      // Methods for getting object value.
      const std::vector<Object>& GetObjects() const { return objects_; }
      const std::vector<Object>& GetArray() const { return array_; }
      const std::string& GetString() const { return string_; }
      int GetInteger() const { return integer_; }
      bool GetBoolean() const { return boolean_; }

      // Methods for setting object value.
      std::vector<Object>& SetObjects() {
        type_ = OBJECT;
        return objects_;
      }
      std::vector<Object>& SetArray() {
        type_ = ARRAY;
        return array_;
      }
      void SetString(const std::string& value) {
        type_ = STRING;
        string_ = value;
      }
      void SetInteger(int value) {
        type_ = INTEGER;
        integer_ = value;
      }
      void SetBoolean(bool value) {
        type_ = BOOLEAN;
        boolean_ = value;
      }

     private:
      std::string name_;
      Type type_;
      std::vector<Object> objects_;
      std::vector<Object> array_;
      std::string string_;
      int integer_;
      bool boolean_;
    };

    // Valid schema top-level JSON object name is "schema".
    Schema() : json("schema") {}

    // Methods for comparing two schemas.
    bool operator==(const Schema& other) const { return json == other.json; }
    bool operator!=(const Schema& other) const { return !(*this == other); }

    // Valid schema top-level JSON object is required to have child objects.
    bool Empty() const { return json.GetObjects().empty(); }

    // Top-level JSON object of the schema.
    Object json;
  };

  // Describes a property (column) of a property table.
  class Property {
   public:
    // Describes glTF buffer view data.
    struct Data {
      // Methods for comparing two data objects.
      bool operator==(const Data& other) const;
      bool operator!=(const Data& other) const { return !(*this == other); }

      // Buffer view data.
      std::vector<uint8_t> data;

      // Data target corresponds to the target property of the glTF bufferView
      // object and classifies the type or nature of the data.
      int target = 0;
    };

    // Describes offsets of the entries in property data when the data
    // represents an array of strings or an array of variable-length number
    // arrays.
    struct Offsets {
      // Methods for comparing two offsets.
      bool operator==(const Offsets& other) const;
      bool operator!=(const Offsets& other) const { return !(*this == other); }

      // Data containing the offset entries.
      Data data;

      // Data type of the offset entries.
      std::string type;
    };

    // Creates an empty property.
    Property();

    // Methods for comparing two properties.
    bool operator==(const Property& other) const;
    bool operator!=(const Property& other) const { return !(*this == other); }

    // Copies all data from |src| property.
    void Copy(const Property& src);

    // Name of this property.
    void SetName(const std::string& name);
    const std::string& GetName() const;

    // Property data stores one table column worth of data. For example, when
    // the data of type UINT8 is [11, 22] then the property values are 11 and 22
    // for the first and second table rows. See EXT_structural_metadata glTF
    // extension documentation for more details.
    Data& GetData();
    const Data& GetData() const;

    // Array offsets are used when property data contains a variable-length
    // number arrays. For example, when the data is [0, 1, 2, 3, 4] and the
    // array offsets are [0, 2, 5] for a two-row table, then the property value
    // arrays are [0, 1] and [2, 3, 4] for the first and second table rows,
    // respectively. See EXT_structural_metadata glTF extension documentation
    // for more details.
    const Offsets& GetArrayOffsets() const;
    Offsets& GetArrayOffsets();

    // String offsets are used when property data contains strings. For example,
    // when the data is "SeaLand" and the array offsets are [0, 3, 7] for a
    // two-row table, then the property strings are "Sea"  and "Land" for the
    // first and second table rows, respectively. See EXT_structural_metadata
    // glTF extension documentation for more details.
    const Offsets& GetStringOffsets() const;
    Offsets& GetStringOffsets();

   private:
    std::string name_;
    Data data_;
    Offsets array_offsets_;
    Offsets string_offsets_;
    // TODO(vytyaz): Support property value modifiers min, max, offset, scale.
  };

  // Creates an empty property table.
  PropertyTable();

  // Methods for comparing two property tables.
  bool operator==(const PropertyTable& other) const;
  bool operator!=(const PropertyTable& other) const {
    return !(*this == other);
  }

  // Copies all data from |src| property table.
  void Copy(const PropertyTable& src);

  // Name of this property table.
  void SetName(const std::string& value);
  const std::string& GetName() const;

  // Class of this property table.
  void SetClass(const std::string& value);
  const std::string& GetClass() const;

  // Number of rows in this property table.
  void SetCount(int count);
  int GetCount() const;

  // Table properties (columns).
  int AddProperty(std::unique_ptr<Property> property);
  int NumProperties() const;
  const Property& GetProperty(int index) const;
  Property& GetProperty(int index);
  void RemoveProperty(int index);

 private:
  std::string name_;
  std::string class_;
  int count_;
  std::vector<std::unique_ptr<Property>> properties_;
};

}  // namespace draco

#endif  // DRACO_TRANSCODER_SUPPORTED
#endif  // DRACO_METADATA_PROPERTY_TABLE_H_
