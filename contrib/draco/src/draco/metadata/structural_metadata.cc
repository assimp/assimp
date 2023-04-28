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
#include "draco/metadata/structural_metadata.h"

#include <memory>
#include <utility>

#ifdef DRACO_TRANSCODER_SUPPORTED

namespace draco {

StructuralMetadata::StructuralMetadata() {}

bool StructuralMetadata::operator==(const StructuralMetadata &other) const {
  return property_table_schema_ == other.property_table_schema_ &&
         property_tables_ == other.property_tables_;
}

void StructuralMetadata::Copy(const StructuralMetadata &src) {
  property_table_schema_.json.Copy(src.property_table_schema_.json);
  property_tables_.resize(src.property_tables_.size());
  for (int i = 0; i < property_tables_.size(); ++i) {
    property_tables_[i] = std::unique_ptr<PropertyTable>(new PropertyTable());
    property_tables_[i]->Copy(*src.property_tables_[i]);
  }
}

void StructuralMetadata::SetPropertyTableSchema(
    const PropertyTable::Schema &schema) {
  property_table_schema_ = schema;
}

const PropertyTable::Schema &StructuralMetadata::GetPropertyTableSchema()
    const {
  return property_table_schema_;
}

int StructuralMetadata::AddPropertyTable(
    std::unique_ptr<PropertyTable> property_table) {
  property_tables_.push_back(std::move(property_table));
  return property_tables_.size() - 1;
}

int StructuralMetadata::NumPropertyTables() const {
  return property_tables_.size();
}

const PropertyTable &StructuralMetadata::GetPropertyTable(int index) const {
  return *property_tables_[index];
}

PropertyTable &StructuralMetadata::GetPropertyTable(int index) {
  return *property_tables_[index];
}

void StructuralMetadata::RemovePropertyTable(int index) {
  property_tables_.erase(property_tables_.begin() + index);
}

}  // namespace draco

#endif  // DRACO_TRANSCODER_SUPPORTED
