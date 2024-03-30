#include "simple-type-reflection.hh"

namespace simple_type_reflection {

Error* TypeMismatchError(std::string expected_type, std::string actual_type) {
  return new Error(Error::TYPE_MISMATCH,
                   "Type mismatch error: type `" + expected_type +
                       "` expected but got type `" + actual_type + "`");
}

Error* RequiredFieldMissingError() {
  return new Error(Error::TYPE_MISMATCH, "Required field(s) is missing: ");
}

Error* UnknownFieldError(std::string field_name) {
  return new Error(Error::UNKNOWN_FIELD,
                   "Unknown field with name: `" + field_name + "`");
}

Error* ArrayLengthMismatchError() {
  return new Error(Error::ARRAY_LENGTH_MISMATCH, "Array length mismatch");
}

Error* ArrayElementError(size_t idx) {
  return new Error(Error::ARRAY_ELEMENT,
                   "Error at array element at index " + std::to_string(idx));
}

Error* ObjectMemberError(std::string key) {
  return new Error(Error::OBJECT_MEMBER,
                   "Error at object member with name `" + key + "`");
}

Error* DuplicateKeyError(std::string key) {
  return new Error(Error::DUPLICATE_KEYS, "Duplicated key name `" + key + "`");
}

IHandler::~IHandler() {}

BaseHandler::BaseHandler() {}
BaseHandler::~BaseHandler() {}

bool BaseHandler::set_out_of_range(const char* actual_type) {
  the_error.reset(new Error(Error::NUMBER_OUT_OF_RANGE,
                            "Number out-of-range: type `" + type_name() +
                                "`, actual_type `" + actual_type + "`"));
  return false;
}

bool BaseHandler::set_type_mismatch(const char* actual_type) {
  the_error.reset(new Error(Error::TYPE_MISMATCH,
                            "Type mismatch error: type `" + type_name() +
                                "` expected but got type `" + actual_type +
                                "`"));
  return false;
}

ObjectHandler::ObjectHandler() {}

ObjectHandler::~ObjectHandler() {}

std::string ObjectHandler::type_name() const { return "object"; }

bool ObjectHandler::precheck(const char* actual_type) {
  if (depth <= 0) {
    the_error.reset(TypeMismatchError(type_name(), actual_type));
    return false;
  }
  if (current && current->handler && current->handler->is_parsed()) {
    if (flags & Flags::AllowDuplicateKey) {
      current->handler->prepare_for_reuse();
    } else {
      the_error.reset(DuplicateKeyError(current_name));
      return false;
    }
  }
  return true;
}

bool ObjectHandler::postcheck(bool success) {
  if (!success) {
    the_error.reset(ObjectMemberError(current_name));
  }
  return success;
}

void ObjectHandler::set_missing_required(const std::string& name) {
  if (!the_error || the_error->error_type != Error::MISSING_REQUIRED)
    the_error.reset(RequiredFieldMissingError());

  // Add error message
  the_error->error_msg += " " + name + ", ";
}

#define POSTCHECK(x) (!current || !(current->handler) || postcheck(x))

bool ObjectHandler::Double(double value) {
  if (!precheck("double")) return false;
  return POSTCHECK(current->handler->Double(value));
}

bool ObjectHandler::Short(short value) {
  if (!precheck("short")) return false;
  return POSTCHECK(current->handler->Short(value));
}

bool ObjectHandler::Ushort(unsigned short value) {
  if (!precheck("unsigned short")) return false;
  return POSTCHECK(current->handler->Ushort(value));
}

bool ObjectHandler::Int(int value) {
  if (!precheck("int")) return false;
  return POSTCHECK(current->handler->Int(value));
}

bool ObjectHandler::Uint(unsigned value) {
  if (!precheck("unsigned")) return false;
  return POSTCHECK(current->handler->Uint(value));
}

bool ObjectHandler::Bool(bool value) {
  if (!precheck("bool")) return false;
  return POSTCHECK(current->handler->Bool(value));
}

bool ObjectHandler::Int64(std::int64_t value) {
  if (!precheck("std::int64_t")) return false;
  return POSTCHECK(current->handler->Int64(value));
}

bool ObjectHandler::Uint64(std::uint64_t value) {
  if (!precheck("std::uint64_t")) return false;
  return POSTCHECK(current->handler->Uint64(value));
}

bool ObjectHandler::Null() {
  if (!precheck("null")) return false;
  return POSTCHECK(current->handler->Null());
}

bool ObjectHandler::StartArray() {
  if (!precheck("array")) return false;
  return POSTCHECK(current->handler->StartArray());
}

bool ObjectHandler::EndArray(SizeType sz) {
  if (!precheck("array")) return false;
  return POSTCHECK(current->handler->EndArray(sz));
}

bool ObjectHandler::String(const char* str, SizeType sz, bool copy) {
  if (!precheck("string")) return false;
  return POSTCHECK(current->handler->String(str, sz, copy));
}

bool ObjectHandler::Key(const char* str, SizeType sz, bool copy) {
  if (depth <= 0) {
    the_error.reset(new Error(Error::CORRUPTED_DOM, "Corrupted DOM"));
    return false;
  }
  if (depth == 1) {
    current_name.assign(str, sz);
    auto it = internals.find(current_name);
    if (it == internals.end()) {
      current = nullptr;
      if ((flags & Flags::DisallowUnknownKey)) {
        the_error.reset(UnknownFieldError(str));  // sz?
        return false;
      }
    } else if (it->second.flags & Flags::IgnoreRead) {
      current = nullptr;
    } else {
      current = &it->second;
    }
    return true;
  } else {
    return POSTCHECK(current->handler->Key(str, sz, copy));
  }
}

bool ObjectHandler::StartObject() {
  ++depth;
  if (depth > 1) {
    return POSTCHECK(current->handler->StartObject());
  }
  return true;
}

bool ObjectHandler::EndObject(SizeType sz) {
  --depth;
  if (depth > 0) {
    return POSTCHECK(current->handler->EndObject(sz));
  }
  for (auto&& pair : internals) {
    if (pair.second.handler && !(pair.second.flags & Flags::Optional) &&
        !pair.second.handler->is_parsed()) {
      set_missing_required(pair.first);
    }
  }
  if (!the_error) {
    this->parsed = true;
    return true;
  }
  return false;
}

void ObjectHandler::reset() {
  current = nullptr;
  current_name.clear();
  depth = 0;
  for (auto&& pair : internals) {
    if (pair.second.handler) pair.second.handler->prepare_for_reuse();
  }
}

void ObjectHandler::add_handler(std::string&& name,
                                ObjectHandler::FlaggedHandler&& fh) {
  internals.emplace(std::move(name), std::move(fh));
}

// bool ObjectHandler::reap_error(ErrorStack& stack)
//{
//    if (!the_error)
//        return false;
//    stack.push(the_error.release());
//    if (current && current->handler)
//        current->handler->reap_error(stack);
//    return true;
//}

bool ObjectHandler::write(IHandler* output) const {
  SizeType count = 0;
  if (!output->StartObject()) return false;

  for (auto&& pair : internals) {
    if (!pair.second.handler || (pair.second.flags & Flags::IgnoreWrite))
      continue;
    if (!output->Key(pair.first.data(),
                     static_cast<simple_serialize::SizeType>(pair.first.size()),
                     true))
      return false;
    if (!pair.second.handler->write(output)) return false;
    ++count;
  }
  return output->EndObject(count);
}

// void ObjectHandler::generate_schema(Value& output, MemoryPoolAllocator&
// alloc) const
//{
//    output.SetObject();
//    output.AddMember(rapidjson::StringRef("type"),
//    rapidjson::StringRef("object"), alloc);
//
//    Value properties(rapidjson::kObjectType);
//    Value required(rapidjson::kArrayType);
//    for (auto&& pair : internals)
//    {
//        Value schema;
//        if (pair.second.handler)
//            pair.second.handler->generate_schema(schema, alloc);
//        else
//            std::abort();
//        Value key;
//        key.SetString(pair.first.c_str(),
//        static_cast<SizeType>(pair.first.size()), alloc);
//        properties.AddMember(key, schema, alloc);
//        if (!(pair.second.flags & Flags::Optional))
//        {
//            key.SetString(pair.first.c_str(),
//            static_cast<SizeType>(pair.first.size()), alloc);
//            required.PushBack(key, alloc);
//        }
//    }
//    output.AddMember(rapidjson::StringRef("properties"), properties, alloc);
//    if (!required.Empty())
//    {
//        output.AddMember(rapidjson::StringRef("required"), required, alloc);
//    }
//    output.AddMember(rapidjson::StringRef("additionalProperties"),
//                     !(get_flags() & Flags::DisallowUnknownKey),
//                     alloc);
//}

Handler<bool>::Handler(bool* value) : m_value(value) {}
Handler<bool>::~Handler() {}
Handler<short>::~Handler() {}
Handler<unsigned short>::~Handler() {}
Handler<int>::~Handler() {}
Handler<unsigned>::~Handler() {}
Handler<int64_t>::~Handler() {}
Handler<uint64_t>::~Handler() {}
Handler<float>::~Handler() {}
Handler<double>::~Handler() {}
Handler<char>::~Handler() {}
Handler<std::string>::~Handler() {}

bool Parse::SetValue(bool b, BaseHandler& handler) const {
  return handler.Bool(b);
}

bool Parse::SetValue(short i, BaseHandler& handler) const {
  return handler.Short(i);
}

bool Parse::SetValue(unsigned short i, BaseHandler& handler) const {
  return handler.Ushort(i);
}

bool Parse::SetValue(int i, BaseHandler& handler) const {
  return handler.Int(i);
}

bool Parse::SetValue(unsigned int i, BaseHandler& handler) const {
  return handler.Uint(i);
}

bool Parse::SetValue(int64_t i, BaseHandler& handler) const {
  return handler.Int64(i);
}

bool Parse::SetValue(uint64_t i, BaseHandler& handler) const {
  return handler.Uint64(i);
}

bool Parse::SetValue(float f, BaseHandler& handler) const {
  return handler.Double(double(f));
}

bool Parse::SetValue(double f, BaseHandler& handler) const {
  return handler.Double(f);
}

bool Parse::SetValue(const std::string& s, BaseHandler& handler) const {
  return handler.String(s.c_str(), s.size(), /* not used */ false);
}

}  // namespace simple_type_reflection
