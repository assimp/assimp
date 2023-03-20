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
#include "draco/metadata/property_table.h"

#include <memory>
#include <utility>
#include <vector>

#include "draco/core/draco_test_base.h"
#include "draco/core/draco_test_utils.h"

namespace {

#ifdef DRACO_TRANSCODER_SUPPORTED

TEST(PropertyTableTest, TestPropertyDataDefaults) {
  // Test construction of an empty property data.
  draco::PropertyTable::Property::Data data;
  ASSERT_TRUE(data.data.empty());
  ASSERT_EQ(data.target, 0);
}

TEST(PropertyTableTest, TestPropertyDefaults) {
  // Test construction of an empty property table property.
  draco::PropertyTable::Property property;
  ASSERT_TRUE(property.GetName().empty());
  ASSERT_TRUE(property.GetData().data.empty());
  {
    const auto &offsets = property.GetArrayOffsets();
    ASSERT_TRUE(offsets.type.empty());
    ASSERT_TRUE(offsets.data.data.empty());
    ASSERT_EQ(offsets.data.target, 0);
  }
  {
    const auto &offsets = property.GetStringOffsets();
    ASSERT_TRUE(offsets.type.empty());
    ASSERT_TRUE(offsets.data.data.empty());
    ASSERT_EQ(offsets.data.target, 0);
  }
}

TEST(PropertyTableTest, TestPropertyTableDefaults) {
  // Test construction of an empty property table.
  draco::PropertyTable table;
  ASSERT_TRUE(table.GetName().empty());
  ASSERT_TRUE(table.GetClass().empty());
  ASSERT_EQ(table.GetCount(), 0);
  ASSERT_EQ(table.NumProperties(), 0);
}

TEST(PropertyTableTest, TestSchemaDefaults) {
  // Test construction of an empty property table schema.
  draco::PropertyTable::Schema schema;
  ASSERT_TRUE(schema.Empty());
  ASSERT_EQ(schema.json.GetName(), "schema");
  ASSERT_EQ(schema.json.GetType(),
            draco::PropertyTable::Schema::Object::OBJECT);
  ASSERT_TRUE(schema.json.GetObjects().empty());
  ASSERT_TRUE(schema.json.GetArray().empty());
  ASSERT_TRUE(schema.json.GetString().empty());
  ASSERT_EQ(schema.json.GetInteger(), 0);
  ASSERT_FALSE(schema.json.GetBoolean());
}

TEST(PropertyTableTest, TestSchemaObjectDefaultConstructor) {
  // Test construction of an empty property table schema object.
  draco::PropertyTable::Schema::Object object;
  ASSERT_TRUE(object.GetName().empty());
  ASSERT_EQ(object.GetType(), draco::PropertyTable::Schema::Object::OBJECT);
  ASSERT_TRUE(object.GetObjects().empty());
  ASSERT_TRUE(object.GetArray().empty());
  ASSERT_TRUE(object.GetString().empty());
  ASSERT_EQ(object.GetInteger(), 0);
  ASSERT_FALSE(object.GetBoolean());
}

TEST(PropertyTableTest, TestSchemaObjectNamedConstructor) {
  // Test construction of a named property table schema object.
  draco::PropertyTable::Schema::Object object("Flexible Demeanour");
  ASSERT_EQ(object.GetName(), "Flexible Demeanour");
  ASSERT_EQ(object.GetType(), draco::PropertyTable::Schema::Object::OBJECT);
  ASSERT_TRUE(object.GetObjects().empty());
}

TEST(PropertyTableTest, TestSchemaObjectStringConstructor) {
  // Test construction of property table schema object storing a string.
  draco::PropertyTable::Schema::Object object("Flexible Demeanour", "GCU");
  ASSERT_EQ(object.GetName(), "Flexible Demeanour");
  ASSERT_EQ(object.GetType(), draco::PropertyTable::Schema::Object::STRING);
  ASSERT_EQ(object.GetString(), "GCU");
}

TEST(PropertyTableTest, TestSchemaObjectIntegerConstructor) {
  // Test construction of property table schema object storing an integer.
  draco::PropertyTable::Schema::Object object("Flexible Demeanour", 12);
  ASSERT_EQ(object.GetName(), "Flexible Demeanour");
  ASSERT_EQ(object.GetType(), draco::PropertyTable::Schema::Object::INTEGER);
  ASSERT_EQ(object.GetInteger(), 12);
}

TEST(PropertyTableTest, TestSchemaObjectBooleanConstructor) {
  // Test construction of property table schema object storing a boolean.
  draco::PropertyTable::Schema::Object object("Flexible Demeanour", true);
  ASSERT_EQ(object.GetName(), "Flexible Demeanour");
  ASSERT_EQ(object.GetType(), draco::PropertyTable::Schema::Object::BOOLEAN);
  ASSERT_TRUE(object.GetBoolean());
}

TEST(PropertyTableTest, TestSchemaObjectSettersAndGetters) {
  // Test value setters and getters of property table schema object.
  typedef draco::PropertyTable::Schema::Object Object;
  Object object;
  ASSERT_EQ(object.GetType(), Object::OBJECT);

  object.SetArray().push_back(Object("entry", 12));
  ASSERT_EQ(object.GetType(), Object::ARRAY);
  ASSERT_EQ(object.GetArray().size(), 1);
  ASSERT_EQ(object.GetArray()[0].GetName(), "entry");
  ASSERT_EQ(object.GetArray()[0].GetInteger(), 12);

  object.SetObjects().push_back(Object("object", 9));
  ASSERT_EQ(object.GetType(), Object::OBJECT);
  ASSERT_EQ(object.GetObjects().size(), 1);
  ASSERT_EQ(object.GetObjects()[0].GetName(), "object");
  ASSERT_EQ(object.GetObjects()[0].GetInteger(), 9);

  object.SetString("matter");
  ASSERT_EQ(object.GetType(), Object::STRING);
  ASSERT_EQ(object.GetString(), "matter");

  object.SetInteger(5);
  ASSERT_EQ(object.GetType(), Object::INTEGER);
  ASSERT_EQ(object.GetInteger(), 5);

  object.SetBoolean(true);
  ASSERT_EQ(object.GetType(), Object::BOOLEAN);
  ASSERT_EQ(object.GetBoolean(), true);
}

TEST(PropertyTableTest, TestSchemaCompare) {
  typedef draco::PropertyTable::Schema Schema;
  // Test comparison of two schema objects.
  {
    // Compare the same empty schema object.
    Schema a;
    ASSERT_TRUE(a == a);
    ASSERT_FALSE(a != a);
  }
  {
    // Compare two empty schema objects.
    Schema a;
    Schema b;
    ASSERT_TRUE(a == b);
    ASSERT_FALSE(a != b);
  }
  {
    // Compare two schema objects with different JSON objects.
    Schema a;
    Schema b;
    a.json.SetBoolean(true);
    b.json.SetBoolean(false);
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
}

TEST(PropertyTableTest, TestSchemaObjectCompare) {
  // Test comparison of two schema JSON objects.
  typedef draco::PropertyTable::Schema::Object Object;
  {
    // Compare the same object.
    Object a;
    ASSERT_TRUE(a == a);
    ASSERT_FALSE(a != a);
  }
  {
    // Compare two default objects.
    Object a;
    Object b;
    ASSERT_TRUE(a == b);
    ASSERT_FALSE(a != b);
  }
  {
    // Compare two objects with different names.
    Object a("one");
    Object b("two");
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
  {
    // Compare two objects with different types.
    Object a;
    Object b;
    a.SetInteger(1);
    b.SetString("one");
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
  {
    // Compare two identical string-type objects.
    Object a;
    Object b;
    a.SetString("one");
    b.SetString("one");
    ASSERT_TRUE(a == b);
    ASSERT_FALSE(a != b);
  }
  {
    // Compare two different string-type objects.
    Object a;
    Object b;
    a.SetString("one");
    b.SetString("two");
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
  {
    // Compare two identical integer-type objects.
    Object a;
    Object b;
    a.SetInteger(1);
    b.SetInteger(1);
    ASSERT_TRUE(a == b);
    ASSERT_FALSE(a != b);
  }
  {
    // Compare two different integer-type objects.
    Object a;
    Object b;
    a.SetInteger(1);
    b.SetInteger(2);
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
  {
    // Compare two identical boolean-type objects.
    Object a;
    Object b;
    a.SetBoolean(true);
    b.SetBoolean(true);
    ASSERT_TRUE(a == b);
    ASSERT_FALSE(a != b);
  }
  {
    // Compare two different boolean-type objects.
    Object a;
    Object b;
    a.SetBoolean(true);
    b.SetBoolean(false);
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
  {
    // Compare two identical object-type objects.
    Object a;
    Object b;
    a.SetObjects().emplace_back("one");
    b.SetObjects().emplace_back("one");
    ASSERT_TRUE(a == b);
    ASSERT_FALSE(a != b);
  }
  {
    // Compare two different object-type objects.
    Object a;
    Object b;
    a.SetObjects().emplace_back("one");
    b.SetObjects().emplace_back("two");
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
  {
    // Compare two object-type objects with different counts.
    Object a;
    Object b;
    a.SetObjects().emplace_back("one");
    b.SetObjects().emplace_back("one");
    b.SetObjects().emplace_back("two");
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
  {
    // Compare two identical array-type objects.
    Object a;
    Object b;
    a.SetArray().emplace_back("", 1);
    b.SetArray().emplace_back("", 1);
    ASSERT_TRUE(a == b);
    ASSERT_FALSE(a != b);
  }
  {
    // Compare two different array-type objects.
    Object a;
    Object b;
    a.SetArray().emplace_back("", 1);
    b.SetArray().emplace_back("", 2);
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
  {
    // Compare two array-type objects with different counts.
    Object a;
    Object b;
    a.SetArray().emplace_back("", 1);
    b.SetArray().emplace_back("", 1);
    b.SetArray().emplace_back("", 2);
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
}

TEST(PropertyTableTest, TestPropertySettersAndGetters) {
  // Test setter and getter methods of the property table property.
  draco::PropertyTable::Property property;
  property.SetName("Unfortunate Conflict Of Evidence");
  property.GetData().data.push_back(2);

  // Check that property members can be accessed via getters.
  ASSERT_EQ(property.GetName(), "Unfortunate Conflict Of Evidence");
  ASSERT_EQ(property.GetData().data.size(), 1);
  ASSERT_EQ(property.GetData().data[0], 2);
}

TEST(PropertyTableTest, TestPropertyTableSettersAndGetters) {
  // Test setter and getter methods of the property table.
  draco::PropertyTable table;
  table.SetName("Just Read The Instructions");
  table.SetClass("General Contact Unit");
  table.SetCount(456);
  {
    std::unique_ptr<draco::PropertyTable::Property> property(
        new draco::PropertyTable::Property());
    property->SetName("Determinist");
    ASSERT_EQ(table.AddProperty(std::move(property)), 0);
  }
  {
    std::unique_ptr<draco::PropertyTable::Property> property(
        new draco::PropertyTable::Property());
    property->SetName("Revisionist");
    ASSERT_EQ(table.AddProperty(std::move(property)), 1);
  }

  // Check that property table members can be accessed via getters.
  ASSERT_EQ(table.GetName(), "Just Read The Instructions");
  ASSERT_EQ(table.GetClass(), "General Contact Unit");
  ASSERT_EQ(table.GetCount(), 456);
  ASSERT_EQ(table.NumProperties(), 2);
  ASSERT_EQ(table.GetProperty(0).GetName(), "Determinist");
  ASSERT_EQ(table.GetProperty(1).GetName(), "Revisionist");

  // Check that proeprties can be removed.
  table.RemoveProperty(0);
  ASSERT_EQ(table.NumProperties(), 1);
  ASSERT_EQ(table.GetProperty(0).GetName(), "Revisionist");
  table.RemoveProperty(0);
  ASSERT_EQ(table.NumProperties(), 0);
}

TEST(PropertyTableTest, TestPropertyCopy) {
  // Test that property table property can be copied.
  draco::PropertyTable::Property property;
  property.SetName("Unfortunate Conflict Of Evidence");
  property.GetData().data.push_back(2);

  // Make a copy.
  draco::PropertyTable::Property copy;
  copy.Copy(property);

  // Check the copy.
  ASSERT_EQ(copy.GetName(), "Unfortunate Conflict Of Evidence");
  ASSERT_EQ(copy.GetData().data.size(), 1);
  ASSERT_EQ(copy.GetData().data[0], 2);
}

TEST(PropertyTableTest, TestPropertyTableCopy) {
  // Test that property table can be copied.
  draco::PropertyTable table;
  table.SetName("Just Read The Instructions");
  table.SetClass("General Contact Unit");
  table.SetCount(456);
  {
    std::unique_ptr<draco::PropertyTable::Property> property(
        new draco::PropertyTable::Property());
    property->SetName("Determinist");
    table.AddProperty(std::move(property));
  }
  {
    std::unique_ptr<draco::PropertyTable::Property> property(
        new draco::PropertyTable::Property());
    property->SetName("Revisionist");
    table.AddProperty(std::move(property));
  }

  // Make a copy.
  draco::PropertyTable copy;
  copy.Copy(table);

  // Check the copy.
  ASSERT_EQ(copy.GetName(), "Just Read The Instructions");
  ASSERT_EQ(copy.GetClass(), "General Contact Unit");
  ASSERT_EQ(copy.GetCount(), 456);
  ASSERT_EQ(copy.NumProperties(), 2);
  ASSERT_EQ(copy.GetProperty(0).GetName(), "Determinist");
  ASSERT_EQ(copy.GetProperty(1).GetName(), "Revisionist");
}

TEST(PropertyTableTest, TestPropertyDataCompare) {
  // Test comparison of two property data objects.
  typedef draco::PropertyTable::Property::Data Data;
  {
    // Compare the same data object.
    Data a;
    ASSERT_TRUE(a == a);
    ASSERT_FALSE(a != a);
  }
  {
    // Compare two default data objects.
    Data a;
    Data b;
    ASSERT_TRUE(a == b);
    ASSERT_FALSE(a != b);
  }
  {
    // Compare two data objects with different targets.
    Data a;
    Data b;
    a.target = 1;
    b.target = 2;
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
  {
    // Compare two data objects with different data vectors.
    Data a;
    Data b;
    a.data = {1};
    a.data = {2};
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
}

TEST(PropertyTableTest, TestPropertyOffsets) {
  // Test comparison of two property offsets.
  typedef draco::PropertyTable::Property::Offsets Offsets;
  {
    // Compare the same offsets object.
    Offsets a;
    ASSERT_TRUE(a == a);
    ASSERT_FALSE(a != a);
  }
  {
    // Compare two default offsets objects.
    Offsets a;
    Offsets b;
    ASSERT_TRUE(a == b);
    ASSERT_FALSE(a != b);
  }
  {
    // Compare two offsets objects with different types.
    Offsets a;
    Offsets b;
    a.type = 1;
    b.type = 2;
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
  {
    // Compare two offsets objects with different data objects.
    Offsets a;
    Offsets b;
    a.data.target = 1;
    b.data.target = 2;
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
}

TEST(PropertyTableTest, TestPropertyCompare) {
  // Test comparison of two properties.
  typedef draco::PropertyTable::Property Property;
  {
    // Compare the same property object.
    Property a;
    ASSERT_TRUE(a == a);
    ASSERT_FALSE(a != a);
  }
  {
    // Compare two default property objects.
    Property a;
    Property b;
    ASSERT_TRUE(a == b);
    ASSERT_FALSE(a != b);
  }
  {
    // Compare two property objects with different names.
    Property a;
    Property b;
    a.SetName("one");
    b.SetName("two");
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
  {
    // Compare two property objects with different data.
    Property a;
    Property b;
    a.GetData().target = 1;
    b.GetData().target = 2;
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
  {
    // Compare two property objects with different array offsets.
    Property a;
    Property b;
    a.GetArrayOffsets().data.target = 1;
    b.GetArrayOffsets().data.target = 2;
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
  {
    // Compare two property objects with different string offsets.
    Property a;
    Property b;
    a.GetStringOffsets().data.target = 1;
    b.GetStringOffsets().data.target = 2;
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
}

TEST(PropertyTableTest, TestPropertyTableCompare) {
  // Test comparison of two property tables.
  typedef draco::PropertyTable PropertyTable;
  typedef draco::PropertyTable::Property Property;
  {
    // Compare the same property table object.
    PropertyTable a;
    ASSERT_TRUE(a == a);
    ASSERT_FALSE(a != a);
  }
  {
    // Compare two default property tables.
    PropertyTable a;
    PropertyTable b;
    ASSERT_TRUE(a == b);
    ASSERT_FALSE(a != b);
  }
  {
    // Compare two property tables with different names.
    PropertyTable a;
    PropertyTable b;
    a.SetName("one");
    b.SetName("two");
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
  {
    // Compare two property tables with different classes.
    PropertyTable a;
    PropertyTable b;
    a.SetClass("one");
    b.SetClass("two");
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
  {
    // Compare two property tables with different counts.
    PropertyTable a;
    PropertyTable b;
    a.SetCount(1);
    b.SetCount(2);
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
  {
    // Compare two property tables with identical properties.
    PropertyTable a;
    PropertyTable b;
    a.AddProperty(std::unique_ptr<Property>(new Property));
    b.AddProperty(std::unique_ptr<Property>(new Property));
    ASSERT_TRUE(a == b);
    ASSERT_FALSE(a != b);
  }
  {
    // Compare two property tables with different number of properties.
    PropertyTable a;
    PropertyTable b;
    a.AddProperty(std::unique_ptr<Property>(new Property));
    b.AddProperty(std::unique_ptr<Property>(new Property));
    b.AddProperty(std::unique_ptr<Property>(new Property));
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
  {
    // Compare two property tables with different properties.
    PropertyTable a;
    PropertyTable b;
    std::unique_ptr<Property> p1(new Property);
    std::unique_ptr<Property> p2(new Property);
    p1->SetName("one");
    p2->SetName("two");
    a.AddProperty(std::move(p1));
    b.AddProperty(std::move(p2));
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
  }
}

#endif  // DRACO_TRANSCODER_SUPPORTED

}  // namespace
