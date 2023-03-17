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
#ifndef DRACO_METADATA_STRUCTURAL_METADATA_H_
#define DRACO_METADATA_STRUCTURAL_METADATA_H_

#include "draco/draco_features.h"

#ifdef DRACO_TRANSCODER_SUPPORTED

#include <memory>
#include <string>
#include <vector>

#include "draco/metadata/property_table.h"

namespace draco {

// Holds data associated with EXT_structural_metadata glTF extension.
class StructuralMetadata {
 public:
  StructuralMetadata();

  // Methods for comparing two structural metadata objects.
  bool operator==(const StructuralMetadata &other) const;
  bool operator!=(const StructuralMetadata &other) const {
    return !(*this == other);
  }

  // Copies |src| structural metadata into this object.
  void Copy(const StructuralMetadata &src);

  // Property table schema.
  void SetPropertyTableSchema(const PropertyTable::Schema &schema);
  const PropertyTable::Schema &GetPropertyTableSchema() const;

  // Property tables.
  int AddPropertyTable(std::unique_ptr<PropertyTable> property_table);
  int NumPropertyTables() const;
  const PropertyTable &GetPropertyTable(int index) const;
  PropertyTable &GetPropertyTable(int index);
  void RemovePropertyTable(int index);

 private:
  // Property table schema and property tables.
  PropertyTable::Schema property_table_schema_;
  std::vector<std::unique_ptr<PropertyTable>> property_tables_;
};

}  // namespace draco

#endif  // DRACO_TRANSCODER_SUPPORTED
#endif  // DRACO_METADATA_STRUCTURAL_METADATA_H_
