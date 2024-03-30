//
// Simple single-file static type reflection
// library supporting frequently used STL containers. Code is based on
// StaticJSON: https://github.com/netheril96/StaticJSON
//
// MIT license
//
// Copyright (c) 2014 Siyuan Ren (netheril96@gmail.com)
//
// Modification: Copyright (c) 2020-Present Syoyo Fujita
//

#pragma once

#include <cmath>
#include <limits>
#include <memory>

//
// STL types
//
#include <array>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>

// TODO: deque, tuple

namespace simple_type_relection {

// TODO:
// [ ] EnumHandler
// [ ] Better error report
// [ ] std::optional type

struct NonMobile {
  NonMobile() {}
  ~NonMobile() {}
  NonMobile(const NonMobile&) = delete;
  NonMobile(NonMobile&&) = delete;
  NonMobile& operator=(const NonMobile&) = delete;
  NonMobile& operator=(NonMobile&&) = delete;
};

typedef std::size_t SizeType;

struct Error {
  int error_type;
  static const int SUCCESS = 0, OBJECT_MEMBER = 1, ARRAY_ELEMENT = 2,
                   MISSING_REQUIRED = 3, TYPE_MISMATCH = 4,
                   NUMBER_OUT_OF_RANGE = 5, ARRAY_LENGTH_MISMATCH = 6,
                   UNKNOWN_FIELD = 7, DUPLICATE_KEYS = 8, CORRUPTED_DOM = 9,
                   TOO_DEEP_RECURSION = 10, INVALID_ENUM = 11, CUSTOM = -1;

  std::string error_msg;

  Error(int ty, std::string msg) : error_type(ty), error_msg(msg) {}
};

Error* TypeMismatchError(std::string expected_type, std::string actual_type);
Error* RequiredFieldMissingError();
Error* UnknownFieldError(std::string field_name);
Error* ArrayElementError(size_t n);
Error* ArrayLengthMismatchError();
Error* ObjectMemberError(std::string key);
Error* DuplicateKeyError(std::string key);

class IHandler {
 public:
  IHandler() {}

  virtual ~IHandler();

  virtual bool Null() = 0;

  virtual bool Bool(bool) = 0;

  virtual bool Short(short) = 0;

  virtual bool Ushort(unsigned short) = 0;

  virtual bool Int(int) = 0;

  virtual bool Uint(unsigned) = 0;

  virtual bool Int64(std::int64_t) = 0;

  virtual bool Uint64(std::uint64_t) = 0;

  virtual bool Double(double) = 0;

  virtual bool String(const char*, SizeType, bool) = 0;

  virtual bool StartObject() = 0;

  virtual bool Key(const char*, SizeType, bool) = 0;

  virtual bool EndObject(SizeType) = 0;

  virtual bool StartArray() = 0;

  virtual bool EndArray(SizeType) = 0;

  // virtual bool RawNumber(const char*, SizeType, bool);

  virtual void prepare_for_reuse() = 0;
};

class BaseHandler : public IHandler
//, private NonMobile
{
  friend class NullableHandler;

 protected:
  std::unique_ptr<Error> the_error;
  bool parsed = false;

 protected:
  bool set_out_of_range(const char* actual_type);
  bool set_type_mismatch(const char* actual_type);

  virtual void reset() {}

 public:
  BaseHandler();

  virtual ~BaseHandler() override;

  virtual std::string type_name() const = 0;

  virtual bool Null() override { return set_type_mismatch("null"); }

  virtual bool Bool(bool) override { return set_type_mismatch("bool"); }

  virtual bool Short(short) override { return set_type_mismatch("short"); }

  virtual bool Ushort(unsigned short) override {
    return set_type_mismatch("ushort");
  }

  virtual bool Int(int) override { return set_type_mismatch("int"); }

  virtual bool Uint(unsigned) override { return set_type_mismatch("unsigned"); }

  virtual bool Int64(std::int64_t) override {
    return set_type_mismatch("int64_t");
  }

  virtual bool Uint64(std::uint64_t) override {
    return set_type_mismatch("uint64_t");
  }

  virtual bool Double(double) override { return set_type_mismatch("double"); }

  virtual bool String(const char*, SizeType, bool) override {
    return set_type_mismatch("string");
  }

  virtual bool StartObject() override { return set_type_mismatch("object"); }

  virtual bool Key(const char*, SizeType, bool) override {
    return set_type_mismatch("object");
  }

  virtual bool EndObject(SizeType) override {
    return set_type_mismatch("object");
  }

  virtual bool StartArray() override { return set_type_mismatch("array"); }

  virtual bool EndArray(SizeType) override {
    return set_type_mismatch("array");
  }

  virtual bool has_error() const { return !the_error; }

  bool is_parsed() const { return parsed; }

  void prepare_for_reuse() override {
    the_error.reset();
    parsed = false;
    reset();
  }

  virtual bool write(IHandler* output) const = 0;

  // virtual void generate_schema(Value& output, MemoryPoolAllocator& alloc)
  // const = 0;
};

struct Flags {
  static const unsigned Default = 0x0, AllowDuplicateKey = 0x1, Optional = 0x2,
                        IgnoreRead = 0x4, IgnoreWrite = 0x8,
                        DisallowUnknownKey = 0x10;
};

// Forward declaration
template <class T>
class Handler;

class ObjectHandler : public BaseHandler {
 protected:
  struct FlaggedHandler {
    std::unique_ptr<BaseHandler> handler;
    unsigned flags;
  };

 protected:
  std::map<std::string, FlaggedHandler> internals;
  FlaggedHandler* current = nullptr;
  std::string current_name;
  int depth = 0;
  unsigned flags = Flags::Default;

 protected:
  bool precheck(const char* type);
  bool postcheck(bool success);
  void set_missing_required(const std::string& name);
  void add_handler(std::string&&, FlaggedHandler&&);
  void reset() override;

 public:
  ObjectHandler();

  ~ObjectHandler() override;

  std::string type_name() const override;

  virtual bool Null() override;

  virtual bool Bool(bool) override;

  virtual bool Short(short) override;

  virtual bool Ushort(unsigned short) override;

  virtual bool Int(int) override;

  virtual bool Uint(unsigned) override;

  virtual bool Int64(std::int64_t) override;

  virtual bool Uint64(std::uint64_t) override;

  virtual bool Double(double) override;

  virtual bool String(const char*, SizeType, bool) override;

  virtual bool StartObject() override;

  virtual bool Key(const char*, SizeType, bool) override;

  virtual bool EndObject(SizeType) override;

  virtual bool StartArray() override;

  virtual bool EndArray(SizeType) override;

  // virtual bool reap_error(ErrorStack&) override;

  virtual bool write(IHandler* output) const override;

  // virtual void generate_schema(Value& output, MemoryPoolAllocator& alloc)
  // const override;

  unsigned get_flags() const { return flags; }

  void set_flags(unsigned f) { flags = f; }

  template <class T>
  void add_property(std::string name, T* pointer,
                    unsigned flags_ = Flags::Default) {
    FlaggedHandler fh;
    fh.handler.reset(new Handler<T>(pointer));
    fh.flags = flags_;
    add_handler(std::move(name), std::move(fh));
  }
};

template <class T>
struct Converter {
  typedef T shadow_type;

  static std::string from_shadow(const shadow_type& shadow, T& value) {
    (void)shadow;
    (void)value;
    return nullptr;
  }

  static void to_shadow(const T& value, shadow_type& shadow) {
    (void)shadow;
    (void)value;
  }

  static std::string type_name() { return "T"; }

  static constexpr bool has_specialized_type_name = false;
};

template <class T>
void init(T* t, ObjectHandler* h) {
  t->simple_serialize_init(h);
}

template <class T>
class ObjectTypeHandler : public ObjectHandler {
 public:
  explicit ObjectTypeHandler(T* t) { init(t, this); }
};

template <class T>
class ConversionHandler : public BaseHandler {
 private:
  typedef typename Converter<T>::shadow_type shadow_type;
  typedef Handler<shadow_type> internal_type;

 private:
  shadow_type shadow;
  internal_type internal;
  T* m_value;

 protected:
  bool postprocess(bool success) {
    if (!success) {
      return false;
    }
    if (!internal.is_parsed()) return true;
    this->parsed = true;
    auto err = Converter<T>::from_shadow(shadow, *m_value);
    if (err) {
      this->the_error.swap(err);
      return false;
    }
    return true;
  }

  void reset() override {
    shadow = shadow_type();
    internal.prepare_for_reuse();
  }

 public:
  explicit ConversionHandler(T* t) : shadow(), internal(&shadow), m_value(t) {}

  std::string type_name() const override {
    // if (Converter<T>::has_specialized_type_name)
    //  return Converter<T>::type_name();
    return internal.type_name();
  }

  bool Null() override { return postprocess(internal.Null()); }

  bool Bool(bool b) override { return postprocess(internal.Bool(b)); }

  bool Short(short i) override { return postprocess(internal.Short(i)); }

  bool Ushort(unsigned short u) override {
    return postprocess(internal.Ushort(u));
  }

  bool Int(int i) override { return postprocess(internal.Int(i)); }

  bool Uint(unsigned u) override { return postprocess(internal.Uint(u)); }

  bool Int64(std::int64_t i) override { return postprocess(internal.Int64(i)); }

  bool Uint64(std::uint64_t u) override {
    return postprocess(internal.Uint64(u));
  }

  bool Double(double d) override { return postprocess(internal.Double(d)); }

  bool String(const char* str, SizeType size, bool copy) override {
    return postprocess(internal.String(str, size, copy));
  }

  bool StartObject() override { return postprocess(internal.StartObject()); }

  bool Key(const char* str, SizeType size, bool copy) override {
    return postprocess(internal.Key(str, size, copy));
  }

  bool EndObject(SizeType sz) override {
    return postprocess(internal.EndObject(sz));
  }

  bool StartArray() override { return postprocess(internal.StartArray()); }

  bool EndArray(SizeType sz) override {
    return postprocess(internal.EndArray(sz));
  }

  bool has_error() const override {
    return BaseHandler::has_error() || internal.has_error();
  }

  // bool reap_error(ErrorStack& errs) override
  //{
  //    return BaseHandler::reap_error(errs) || internal.reap_error(errs);
  //}

  virtual bool write(IHandler* output) const override {
    Converter<T>::to_shadow(*m_value, const_cast<shadow_type&>(shadow));
    return internal.write(output);
  }

  // void generate_schema(Value& output, MemoryPoolAllocator& alloc) const
  // override
  //{
  //    return internal.generate_schema(output, alloc);
  //}
};

namespace helper {
template <class T, bool no_conversion>
class DispatchHandler;
template <class T>
class DispatchHandler<T, true>
    : public ::simple_serialize::ObjectTypeHandler<T> {
 public:
  explicit DispatchHandler(T* t)
      : ::simple_serialize::ObjectTypeHandler<T>(t) {}
};

template <class T>
class DispatchHandler<T, false>
    : public ::simple_serialize::ConversionHandler<T> {
 public:
  explicit DispatchHandler(T* t)
      : ::simple_serialize::ConversionHandler<T>(t) {}
};
}  // namespace helper

template <class T>
class Handler
    : public helper::DispatchHandler<
          T, std::is_same<typename Converter<T>::shadow_type, T>::value> {
 public:
  typedef helper::DispatchHandler<
      T, std::is_same<typename Converter<T>::shadow_type, T>::value>
      base_type;
  // explicit Handler(T* t) : base_type(t) {}
  explicit Handler(T* t);
};

// ---- primitive types ----

template <class IntType>
class IntegerHandler : public BaseHandler {
  static_assert(std::is_arithmetic<IntType>::value,
                "Only arithmetic types are allowed");

 protected:
  IntType* m_value;

  template <class AnotherIntType>
  static constexpr
      typename std::enable_if<std::is_integral<AnotherIntType>::value,
                              bool>::type
      is_out_of_range(AnotherIntType a) {
    typedef typename std::common_type<IntType, AnotherIntType>::type CommonType;
    typedef typename std::numeric_limits<IntType> this_limits;
    typedef typename std::numeric_limits<AnotherIntType> that_limits;

    // The extra logic related to this_limits::min/max allows the compiler to
    // short circuit this check at compile time. For instance, a `uint32_t`
    // will NEVER be out of range for an `int64_t`
    return (
        (this_limits::is_signed == that_limits::is_signed)
            ? ((CommonType(this_limits::min()) > CommonType(a) ||
                CommonType(this_limits::max()) < CommonType(a)))
            : (this_limits::is_signed)
                  ? (CommonType(this_limits::max()) < CommonType(a))
                  : (a < 0 || CommonType(a) > CommonType(this_limits::max())));
  }

  template <class FloatType>
  static constexpr
      typename std::enable_if<std::is_floating_point<FloatType>::value,
                              bool>::type
      is_out_of_range(FloatType f) {
    // return static_cast<FloatType>(static_cast<IntType>(f)) != f;
    return std::isfinite(f);
  }

  template <class ReceiveNumType>
  bool receive(ReceiveNumType r, const char* actual_type) {
    if (is_out_of_range(r)) return set_out_of_range(actual_type);
    *m_value = static_cast<IntType>(r);
    this->parsed = true;
    return true;
  }

 public:
  explicit IntegerHandler(IntType* value) : m_value(value) {}

  virtual bool Short(short i) override { return receive(i, "short"); }

  virtual bool Ushort(unsigned short i) override {
    return receive(i, "unsigned short");
  }

  virtual bool Int(int i) override { return receive(i, "int"); }

  virtual bool Uint(unsigned i) override { return receive(i, "unsigned int"); }

  virtual bool Int64(std::int64_t i) override {
    return receive(i, "std::int64_t");
  }

  virtual bool Uint64(std::uint64_t i) override {
    return receive(i, "std::uint64_t");
  }

  virtual bool Double(double d) override { return receive(d, "double"); }

  virtual bool write(IHandler* output) const override {
    if (std::numeric_limits<IntType>::is_signed) {
      return output->Int64(int64_t(*m_value));
    } else {
      return output->Uint64(uint64_t(*m_value));
    }
  }

  // virtual void generate_schema(Value& output, MemoryPoolAllocator& alloc)
  // const override
  //{
  //    output.SetObject();
  //    output.AddMember(rapidjson::StringRef("type"),
  //    rapidjson::StringRef("integer"), alloc); Value minimum, maximum; if
  //    (std::numeric_limits<IntType>::is_signed)
  //    {
  //        minimum.SetInt64(std::numeric_limits<IntType>::min());
  //        maximum.SetInt64(std::numeric_limits<IntType>::max());
  //    }
  //    else
  //    {
  //        minimum.SetUint64(std::numeric_limits<IntType>::min());
  //        maximum.SetUint64(std::numeric_limits<IntType>::max());
  //    }
  //    output.AddMember(rapidjson::StringRef("minimum"), minimum, alloc);
  //    output.AddMember(rapidjson::StringRef("maximum"), maximum, alloc);
  //}
};

template <>
class Handler<std::nullptr_t> : public BaseHandler {
 public:
  explicit Handler(std::nullptr_t*);

  bool Null() override {
    this->parsed = true;
    return true;
  }

  std::string type_name() const override { return "null"; }

  bool write(IHandler* output) const override { return output->Null(); }

  // void generate_schema(Value& output, MemoryPoolAllocator& alloc) const
  // override
  //{
  //    output.SetObject();
  //    output.AddMember(rapidjson::StringRef("type"),
  //    rapidjson::StringRef("null"), alloc);
  //}
};

template <>
class Handler<bool> : public BaseHandler {
 private:
  bool* m_value;

 public:
  explicit Handler(bool* value);
  ~Handler() override;

  bool Bool(bool v) override {
    *m_value = v;
    this->parsed = true;
    return true;
  }

  std::string type_name() const override { return "bool"; }

  bool write(IHandler* output) const override { return output->Bool(*m_value); }

  // void generate_schema(Value& output, MemoryPoolAllocator& alloc) const
  // override
  //{
  //    output.SetObject();
  //    output.AddMember(rapidjson::StringRef("type"),
  //    rapidjson::StringRef("boolean"), alloc);
  //}
};

template <>
class Handler<short> : public IntegerHandler<short> {
 public:
  explicit Handler(short* i) : IntegerHandler<short>(i) {}
  ~Handler() override;

  std::string type_name() const override { return "short"; }

  bool write(IHandler* output) const override {
    return output->Short(*m_value);
  }
};

template <>
class Handler<unsigned short> : public IntegerHandler<unsigned short> {
 public:
  explicit Handler(unsigned short* i) : IntegerHandler<unsigned short>(i) {}
  ~Handler() override;

  std::string type_name() const override { return "unsigned short"; }

  bool write(IHandler* output) const override {
    return output->Ushort(*m_value);
  }
};

template <>
class Handler<int> : public IntegerHandler<int> {
 public:
  explicit Handler(int* i) : IntegerHandler<int>(i) {}
  ~Handler() override;

  std::string type_name() const override { return "int"; }

  bool write(IHandler* output) const override { return output->Int(*m_value); }
};

template <>
class Handler<unsigned int> : public IntegerHandler<unsigned int> {
 public:
  explicit Handler(unsigned* i) : IntegerHandler<unsigned>(i) {}
  ~Handler() override;

  std::string type_name() const override { return "unsigned int"; }

  bool write(IHandler* output) const override { return output->Uint(*m_value); }
};

template <>
class Handler<int64_t> : public IntegerHandler<int64_t> {
 public:
  explicit Handler(int64_t* i) : IntegerHandler<int64_t>(i) {}
  ~Handler() override;

  std::string type_name() const override { return "int64"; }
};

template <>
class Handler<uint64_t> : public IntegerHandler<uint64_t> {
 public:
  explicit Handler(uint64_t* i) : IntegerHandler<uint64_t>(i) {}
  ~Handler() override;

  std::string type_name() const override { return "unsigned int64"; }
};

// char is an alias for bool to work around the stupid `std::vector<bool>`
template <>
class Handler<char> : public BaseHandler {
 private:
  char* m_value;

 public:
  explicit Handler(char* i) : m_value(i) {}
  ~Handler() override;

  std::string type_name() const override { return "bool"; }

  bool Bool(bool v) override {
    *this->m_value = v;
    this->parsed = true;
    return true;
  }

  bool write(IHandler* out) const override { return out->Bool(*m_value != 0); }

  // void generate_schema(Value& output, MemoryPoolAllocator& alloc) const
  // override
  //{
  //    output.SetObject();
  //    output.AddMember(rapidjson::StringRef("type"),
  //    rapidjson::StringRef("boolean"), alloc);
  //}
};

template <>
class Handler<double> : public BaseHandler {
 private:
  double* m_value;

 public:
  explicit Handler(double* v) : m_value(v) {}
  ~Handler() override;

  bool Short(short i) override {
    *m_value = i;
    this->parsed = true;
    return true;
  }

  bool Ushort(unsigned short i) override {
    *m_value = i;
    this->parsed = true;
    return true;
  }

  bool Int(int i) override {
    *m_value = i;
    this->parsed = true;
    return true;
  }

  bool Uint(unsigned i) override {
    *m_value = i;
    this->parsed = true;
    return true;
  }

  bool Int64(std::int64_t i) override {
    *m_value = static_cast<double>(i);
    if (static_cast<decltype(i)>(*m_value) != i)
      return set_out_of_range("std::int64_t");
    this->parsed = true;
    return true;
  }

  bool Uint64(std::uint64_t i) override {
    *m_value = static_cast<double>(i);
    if (static_cast<decltype(i)>(*m_value) != i)
      return set_out_of_range("std::uint64_t");
    this->parsed = true;
    return true;
  }

  bool Double(double d) override {
    *m_value = d;
    this->parsed = true;
    return true;
  }

  std::string type_name() const override { return "double"; }

  bool write(IHandler* out) const override { return out->Double(*m_value); }

  // void generate_schema(Value& output, MemoryPoolAllocator& alloc) const
  // override
  //{
  //    output.SetObject();
  //    output.AddMember(rapidjson::StringRef("type"),
  //    rapidjson::StringRef("number"), alloc);
  //}
};

template <>
class Handler<float> : public BaseHandler {
 private:
  float* m_value;

 public:
  explicit Handler(float* v) : m_value(v) {}
  ~Handler() override;

  bool Short(short i) override {
    *m_value = i;
    this->parsed = true;
    return true;
  }

  bool Ushort(unsigned short i) override {
    *m_value = i;
    this->parsed = true;
    return true;
  }

  bool Int(int i) override {
    *m_value = static_cast<float>(i);
    if (static_cast<decltype(i)>(*m_value) != i) return set_out_of_range("int");
    this->parsed = true;
    return true;
  }

  bool Uint(unsigned i) override {
    *m_value = static_cast<float>(i);
    if (static_cast<decltype(i)>(*m_value) != i)
      return set_out_of_range("unsigned int");
    this->parsed = true;
    return true;
  }

  bool Int64(std::int64_t i) override {
    *m_value = static_cast<float>(i);
    if (static_cast<decltype(i)>(*m_value) != i)
      return set_out_of_range("std::int64_t");
    this->parsed = true;
    return true;
  }

  bool Uint64(std::uint64_t i) override {
    *m_value = static_cast<float>(i);
    if (static_cast<decltype(i)>(*m_value) != i)
      return set_out_of_range("std::uint64_t");
    this->parsed = true;
    return true;
  }

  bool Double(double d) override {
    *m_value = static_cast<float>(d);
    this->parsed = true;
    return true;
  }

  std::string type_name() const override { return "float"; }

  bool write(IHandler* out) const override {
    return out->Double(double(*m_value));
  }

  // void generate_schema(Value& output, MemoryPoolAllocator& alloc) const
  // override
  //{
  //    output.SetObject();
  //    output.AddMember(rapidjson::StringRef("type"),
  //    rapidjson::StringRef("number"), alloc);
  //}
};

template <>
class Handler<std::string> : public BaseHandler {
 private:
  std::string* m_value;

 public:
  explicit Handler(std::string* v) : m_value(v) {}
  ~Handler() override;

  bool String(const char* str, SizeType length, bool) override {
    m_value->assign(str, length);
    this->parsed = true;
    return true;
  }

  std::string type_name() const override { return "string"; }

  bool write(IHandler* out) const override {
    return out->String(m_value->data(), SizeType(m_value->size()), true);
  }

  // void generate_schema(Value& output, MemoryPoolAllocator& alloc) const
  // override
  //{
  //    output.SetObject();
  //    output.AddMember(rapidjson::StringRef("type"),
  //    rapidjson::StringRef("string"), alloc);
  //}
};

//
// STL types
//

template <class ArrayType>
class ArrayHandler : public BaseHandler {
 public:
  typedef typename ArrayType::value_type ElementType;

 protected:
  ElementType element;
  Handler<ElementType> internal;
  ArrayType* m_value;
  int depth = 0;

 protected:
  void set_element_error() {
    the_error.reset(ArrayElementError(m_value->size()));
  }

  bool precheck(const char* type) {
    if (depth <= 0) {
      the_error.reset(TypeMismatchError(type_name(), type));
      return false;
    }
    return true;
  }

  bool postcheck(bool success) {
    if (!success) {
      set_element_error();
      return false;
    }
    if (internal.is_parsed()) {
      m_value->emplace_back(std::move(element));
      element = ElementType();
      internal.prepare_for_reuse();
    }
    return true;
  }

  void reset() override {
    element = ElementType();
    internal.prepare_for_reuse();
    depth = 0;
  }

 public:
  explicit ArrayHandler(ArrayType* value)
      : element(), internal(&element), m_value(value) {}

  bool Null() override {
    return precheck("null") && postcheck(internal.Null());
  }

  bool Bool(bool b) override {
    return precheck("bool") && postcheck(internal.Bool(b));
  }

  bool Short(short i) override {
    return precheck("short") && postcheck(internal.Short(i));
  }

  bool Ushort(unsigned short i) override {
    return precheck("unsigned short") && postcheck(internal.Ushort(i));
  }

  bool Int(int i) override {
    return precheck("int") && postcheck(internal.Int(i));
  }

  bool Uint(unsigned i) override {
    return precheck("unsigned") && postcheck(internal.Uint(i));
  }

  bool Int64(std::int64_t i) override {
    return precheck("int64_t") && postcheck(internal.Int64(i));
  }

  bool Uint64(std::uint64_t i) override {
    return precheck("uint64_t") && postcheck(internal.Uint64(i));
  }

  bool Double(double d) override {
    return precheck("double") && postcheck(internal.Double(d));
  }

  bool String(const char* str, SizeType length, bool copy) override {
    return precheck("string") && postcheck(internal.String(str, length, copy));
  }

  bool Key(const char* str, SizeType length, bool copy) override {
    return precheck("object") && postcheck(internal.Key(str, length, copy));
  }

  bool StartObject() override {
    return precheck("object") && postcheck(internal.StartObject());
  }

  bool EndObject(SizeType length) override {
    return precheck("object") && postcheck(internal.EndObject(length));
  }

  bool StartArray() override {
    ++depth;
    if (depth > 1)
      return postcheck(internal.StartArray());
    else
      m_value->clear();
    return true;
  }

  bool EndArray(SizeType length) override {
    --depth;

    // When depth >= 1, this event should be forwarded to the element
    if (depth > 0) return postcheck(internal.EndArray(length));

    this->parsed = true;
    return true;
  }

  // bool reap_error(ErrorStack& stk) override
  //{
  //    if (!the_error)
  //        return false;
  //    stk.push(the_error.release());
  //    internal.reap_error(stk);
  //    return true;
  //}

  bool write(IHandler* output) const override {
    if (!output->StartArray()) return false;
    for (auto&& e : *m_value) {
      Handler<ElementType> h(&e);
      if (!h.write(output)) return false;
    }
    return output->EndArray(
        static_cast<simple_serialize::SizeType>(m_value->size()));
  }

  // void generate_schema(Value& output, MemoryPoolAllocator& alloc) const
  // override
  //{
  //    output.SetObject();
  //    output.AddMember(rapidjson::StringRef("type"),
  //    rapidjson::StringRef("array"), alloc); Value items;
  //    internal.generate_schema(items, alloc);
  //    output.AddMember(rapidjson::StringRef("items"), items, alloc);
  //}
};

template <class T>
class Handler<std::vector<T>> : public ArrayHandler<std::vector<T>> {
 public:
  explicit Handler(std::vector<T>* value)
      : ArrayHandler<std::vector<T>>(value) {}

  std::string type_name() const override {
    return "std::vector<" + this->internal.type_name() + ">";
  }
};

#if 0
template <class T>
class Handler<std::deque<T>> : public ArrayHandler<std::deque<T>>
{
public:
    explicit Handler(std::deque<T>* value) : ArrayHandler<std::deque<T>>(value) {}

    std::string type_name() const override
    {
        return "std::deque<" + this->internal.type_name() + ">";
    }
};
#endif

template <class T>
class Handler<std::list<T>> : public ArrayHandler<std::list<T>> {
 public:
  explicit Handler(std::list<T>* value) : ArrayHandler<std::list<T>>(value) {}

  std::string type_name() const override {
    return "std::list<" + this->internal.type_name() + ">";
  }
};

template <class T, size_t N>
class Handler<std::array<T, N>> : public BaseHandler {
 protected:
  T element;
  Handler<T> internal;
  std::array<T, N>* m_value;
  size_t count = 0;
  int depth = 0;

 protected:
  void set_element_error() { the_error.reset(ArrayElementError(count)); }

  void set_length_error() { the_error.reset(ArrayLengthMismatchError()); }

  bool precheck(const char* type) {
    if (depth <= 0) {
      the_error.reset(TypeMismatchError(type_name(), type));
      return false;
    }
    return true;
  }

  bool postcheck(bool success) {
    if (!success) {
      set_element_error();
      return false;
    }
    if (internal.is_parsed()) {
      if (count >= N) {
        set_length_error();
        return false;
      }
      (*m_value)[count] = std::move(element);
      ++count;
      element = T();
      internal.prepare_for_reuse();
    }
    return true;
  }

  void reset() override {
    element = T();
    internal.prepare_for_reuse();
    depth = 0;
    count = 0;
  }

 public:
  explicit Handler(std::array<T, N>* value)
      : element(), internal(&element), m_value(value) {}

  bool Null() override {
    return precheck("null") && postcheck(internal.Null());
  }

  bool Bool(bool b) override {
    return precheck("bool") && postcheck(internal.Bool(b));
  }

  bool Short(short i) override {
    return precheck("short") && postcheck(internal.Short(i));
  }

  bool Ushort(unsigned short i) override {
    return precheck("unsigned short") && postcheck(internal.Ushort(i));
  }

  bool Int(int i) override {
    return precheck("int") && postcheck(internal.Int(i));
  }

  bool Uint(unsigned i) override {
    return precheck("unsigned") && postcheck(internal.Uint(i));
  }

  bool Int64(std::int64_t i) override {
    return precheck("int64_t") && postcheck(internal.Int64(i));
  }

  bool Uint64(std::uint64_t i) override {
    return precheck("uint64_t") && postcheck(internal.Uint64(i));
  }

  bool Double(double d) override {
    return precheck("double") && postcheck(internal.Double(d));
  }

  bool String(const char* str, SizeType length, bool copy) override {
    return precheck("string") && postcheck(internal.String(str, length, copy));
  }

  bool Key(const char* str, SizeType length, bool copy) override {
    return precheck("object") && postcheck(internal.Key(str, length, copy));
  }

  bool StartObject() override {
    return precheck("object") && postcheck(internal.StartObject());
  }

  bool EndObject(SizeType length) override {
    return precheck("object") && postcheck(internal.EndObject(length));
  }

  bool StartArray() override {
    ++depth;
    if (depth > 1) return postcheck(internal.StartArray());
    return true;
  }

  bool EndArray(SizeType length) override {
    --depth;

    // When depth >= 1, this event should be forwarded to the element
    if (depth > 0) return postcheck(internal.EndArray(length));
    if (count != N) {
      set_length_error();
      return false;
    }
    this->parsed = true;
    return true;
  }

  // bool reap_error(ErrorStack& stk) override
  //{
  //    if (!the_error)
  //        return false;
  //    stk.push(the_error.release());
  //    internal.reap_error(stk);
  //    return true;
  //}

  bool write(IHandler* output) const override {
    if (!output->StartArray()) return false;
    for (auto&& e : *m_value) {
      Handler<T> h(&e);
      if (!h.write(output)) return false;
    }
    return output->EndArray(
        static_cast<simple_serialize::SizeType>(m_value->size()));
  }

  // void generate_schema(Value& output, MemoryPoolAllocator& alloc) const
  // override
  //{
  //    output.SetObject();
  //    output.AddMember(rapidjson::StringRef("type"),
  //    rapidjson::StringRef("array"), alloc); Value items;
  //    internal.generate_schema(items, alloc);
  //    output.AddMember(rapidjson::StringRef("items"), items, alloc);
  //    output.AddMember(rapidjson::StringRef("minItems"),
  //    static_cast<uint64_t>(N), alloc);
  //    output.AddMember(rapidjson::StringRef("maxItems"),
  //    static_cast<uint64_t>(N), alloc);
  //}

  std::string type_name() const override {
    return "std::array<" + internal.type_name() + ", " + std::to_string(N) +
           ">";
  }
};

#if 0
template <class PointerType>
class PointerHandler : public BaseHandler
{
public:
    typedef typename std::pointer_traits<PointerType>::element_type ElementType;

protected:
    mutable PointerType* m_value;
    mutable std::unique_ptr<Handler<ElementType>> internal_handler;
    int depth = 0;

protected:
    explicit PointerHandler(PointerType* value) : m_value(value) {}

    void initialize()
    {
        if (!internal_handler)
        {
            m_value->reset(new ElementType());
            internal_handler.reset(new Handler<ElementType>(m_value->get()));
        }
    }

    void reset() override
    {
        depth = 0;
        internal_handler.reset();
        m_value->reset();
    }

    bool postcheck(bool success)
    {
        if (success)
            this->parsed = internal_handler->is_parsed();
        return success;
    }

public:
    bool Null() override
    {
        if (depth == 0)
        {
            m_value->reset();
            this->parsed = true;
            return true;
        }
        else
        {
            initialize();
            return postcheck(internal_handler->Null());
        }
    }

    bool write(IHandler* out) const override
    {
        if (!m_value || !m_value->get())
        {
            return out->Null();
        }
        if (!internal_handler)
        {
            internal_handler.reset(new Handler<ElementType>(m_value->get()));
        }
        return internal_handler->write(out);
    }

    void generate_schema(Value& output, MemoryPoolAllocator& alloc) const override
    {
        const_cast<PointerHandler<PointerType>*>(this)->initialize();
        output.SetObject();
        Value anyOf(rapidjson::kArrayType);
        Value nullDescriptor(rapidjson::kObjectType);
        nullDescriptor.AddMember(rapidjson::StringRef("type"), rapidjson::StringRef("null"), alloc);
        Value descriptor;
        internal_handler->generate_schema(descriptor, alloc);
        anyOf.PushBack(nullDescriptor, alloc);
        anyOf.PushBack(descriptor, alloc);
        output.AddMember(rapidjson::StringRef("anyOf"), anyOf, alloc);
    }

    bool Bool(bool b) override
    {
        initialize();
        return postcheck(internal_handler->Bool(b));
    }

    bool Int(int i) override
    {
        initialize();
        return postcheck(internal_handler->Int(i));
    }

    bool Uint(unsigned i) override
    {
        initialize();
        return postcheck(internal_handler->Uint(i));
    }

    bool Int64(std::int64_t i) override
    {
        initialize();
        return postcheck(internal_handler->Int64(i));
    }

    bool Uint64(std::uint64_t i) override
    {
        initialize();
        return postcheck(internal_handler->Uint64(i));
    }

    bool Double(double i) override
    {
        initialize();
        return postcheck(internal_handler->Double(i));
    }

    bool String(const char* str, SizeType len, bool copy) override
    {
        initialize();
        return postcheck(internal_handler->String(str, len, copy));
    }

    bool Key(const char* str, SizeType len, bool copy) override
    {
        initialize();
        return postcheck(internal_handler->Key(str, len, copy));
    }

    bool StartObject() override
    {
        initialize();
        ++depth;
        return internal_handler->StartObject();
    }

    bool EndObject(SizeType len) override
    {
        initialize();
        --depth;
        return postcheck(internal_handler->EndObject(len));
    }

    bool StartArray() override
    {
        initialize();
        ++depth;
        return postcheck(internal_handler->StartArray());
    }

    bool EndArray(SizeType len) override
    {
        initialize();
        --depth;
        return postcheck(internal_handler->EndArray(len));
    }

    bool has_error() const override { return internal_handler && internal_handler->has_error(); }

    bool reap_error(ErrorStack& stk) override
    {
        return internal_handler && internal_handler->reap_error(stk);
    }
};

template <class T, class Deleter>
class Handler<std::unique_ptr<T, Deleter>> : public PointerHandler<std::unique_ptr<T, Deleter>>
{
public:
    explicit Handler(std::unique_ptr<T, Deleter>* value)
        : PointerHandler<std::unique_ptr<T, Deleter>>(value)
    {
    }

    std::string type_name() const override
    {
        if (this->internal_handler)
        {
            return "std::unique_ptr<" + this->internal_handler->type_name() + ">";
        }
        return "std::unique_ptr";
    }
};

template <class T>
class Handler<std::shared_ptr<T>> : public PointerHandler<std::shared_ptr<T>>
{
public:
    explicit Handler(std::shared_ptr<T>* value) : PointerHandler<std::shared_ptr<T>>(value) {}

    std::string type_name() const override
    {
        if (this->internal_handler)
        {
            return "std::shared_ptr<" + this->internal_handler->type_name() + ">";
        }
        return "std::shared_ptr";
    }
};
#endif

template <class MapType>
class MapHandler : public BaseHandler {
 protected:
  typedef typename MapType::mapped_type ElementType;

 protected:
  ElementType element;
  Handler<ElementType> internal_handler;
  MapType* m_value;
  std::string current_key;
  int depth = 0;

 protected:
  void reset() override {
    element = ElementType();
    current_key.clear();
    internal_handler.prepare_for_reuse();
    depth = 0;
  }

  bool precheck(const char* type) {
    if (depth <= 0) {
      set_type_mismatch(type);
      return false;
    }
    return true;
  }

  bool postcheck(bool success) {
    if (!success) {
      the_error.reset(ObjectMemberError(current_key));
    } else {
      if (internal_handler.is_parsed()) {
        m_value->emplace(std::move(current_key), std::move(element));
        element = ElementType();
        internal_handler.prepare_for_reuse();
      }
    }
    return success;
  }

 public:
  explicit MapHandler(MapType* value)
      : element(), internal_handler(&element), m_value(value) {}

  bool Null() override {
    return precheck("null") && postcheck(internal_handler.Null());
  }

  bool Bool(bool b) override {
    return precheck("bool") && postcheck(internal_handler.Bool(b));
  }

  bool Short(short i) override {
    return precheck("short") && postcheck(internal_handler.Short(i));
  }

  bool Ushort(unsigned short i) override {
    return precheck("unsigned short") && postcheck(internal_handler.Ushort(i));
  }

  bool Int(int i) override {
    return precheck("int") && postcheck(internal_handler.Int(i));
  }

  bool Uint(unsigned i) override {
    return precheck("unsigned") && postcheck(internal_handler.Uint(i));
  }

  bool Int64(std::int64_t i) override {
    return precheck("int64_t") && postcheck(internal_handler.Int64(i));
  }

  bool Uint64(std::uint64_t i) override {
    return precheck("uint64_t") && postcheck(internal_handler.Uint64(i));
  }

  bool Double(double d) override {
    return precheck("double") && postcheck(internal_handler.Double(d));
  }

  bool String(const char* str, SizeType length, bool copy) override {
    return precheck("string") &&
           postcheck(internal_handler.String(str, length, copy));
  }

  bool Key(const char* str, SizeType length, bool copy) override {
    if (depth > 1) return postcheck(internal_handler.Key(str, length, copy));

    current_key.assign(str, length);
    return true;
  }

  bool StartArray() override {
    return precheck("array") && postcheck(internal_handler.StartArray());
  }

  bool EndArray(SizeType length) override {
    return precheck("array") && postcheck(internal_handler.EndArray(length));
  }

  bool StartObject() override {
    ++depth;
    if (depth > 1)
      return postcheck(internal_handler.StartObject());
    else
      m_value->clear();
    return true;
  }

  bool EndObject(SizeType length) override {
    --depth;
    if (depth > 0) return postcheck(internal_handler.EndObject(length));
    this->parsed = true;
    return true;
  }

  // bool reap_error(ErrorStack& errs) override
  //{
  //    if (!this->the_error)
  //        return false;

  //    errs.push(this->the_error.release());
  //    internal_handler.reap_error(errs);
  //    return true;
  //}

  bool write(IHandler* out) const override {
    if (!out->StartObject()) return false;
    for (auto&& pair : *m_value) {
      if (!out->Key(pair.first.data(), static_cast<SizeType>(pair.first.size()),
                    true))
        return false;
      Handler<ElementType> h(&pair.second);
      if (!h.write(out)) return false;
    }
    return out->EndObject(static_cast<SizeType>(m_value->size()));
  }

  // void generate_schema(Value& output, MemoryPoolAllocator& alloc) const
  // override
  //{
  //    Value internal_schema;
  //    internal_handler.generate_schema(internal_schema, alloc);
  //    output.SetObject();
  //    output.AddMember(rapidjson::StringRef("type"),
  //    rapidjson::StringRef("object"), alloc);

  //    Value empty_obj(rapidjson::kObjectType);
  //    output.AddMember(rapidjson::StringRef("properties"), empty_obj, alloc);
  //    output.AddMember(rapidjson::StringRef("additionalProperties"),
  //    internal_schema, alloc);
  //}
};

template <class T, class Hash, class Equal>
class Handler<std::unordered_map<std::string, T, Hash, Equal>>
    : public MapHandler<std::unordered_map<std::string, T, Hash, Equal>> {
 public:
  explicit Handler(std::unordered_map<std::string, T, Hash, Equal>* value)
      : MapHandler<std::unordered_map<std::string, T, Hash, Equal>>(value) {}

  std::string type_name() const override {
    return "std::unordered_map<std::string, " +
           this->internal_handler.type_name() + ">";
  }
};

template <class T, class Hash, class Equal>
class Handler<std::map<std::string, T, Hash, Equal>>
    : public MapHandler<std::map<std::string, T, Hash, Equal>> {
 public:
  explicit Handler(std::map<std::string, T, Hash, Equal>* value)
      : MapHandler<std::map<std::string, T, Hash, Equal>>(value) {}

  std::string type_name() const override {
    return "std::map<std::string, " + this->internal_handler.type_name() + ">";
  }
};

template <class T, class Hash, class Equal>
class Handler<std::unordered_multimap<std::string, T, Hash, Equal>>
    : public MapHandler<std::unordered_multimap<std::string, T, Hash, Equal>> {
 public:
  explicit Handler(std::unordered_multimap<std::string, T, Hash, Equal>* value)
      : MapHandler<std::unordered_multimap<std::string, T, Hash, Equal>>(
            value) {}

  std::string type_name() const override {
    return "std::unordered_mulitimap<std::string, " +
           this->internal_handler.type_name() + ">";
  }
};

template <class T, class Hash, class Equal>
class Handler<std::multimap<std::string, T, Hash, Equal>>
    : public MapHandler<std::multimap<std::string, T, Hash, Equal>> {
 public:
  explicit Handler(std::multimap<std::string, T, Hash, Equal>* value)
      : MapHandler<std::multimap<std::string, T, Hash, Equal>>(value) {}

  std::string type_name() const override {
    return "std::multimap<std::string, " + this->internal_handler.type_name() +
           ">";
  }
};

#if 0
template <std::size_t N>
class TupleHander : public BaseHandler
{
protected:
    std::array<std::unique_ptr<BaseHandler>, N> handlers;
    std::size_t index = 0;
    int depth = 0;

    bool postcheck(bool success)
    {
        if (!success)
        {
            the_error.reset(new error::ArrayElementError(index));
            return false;
        }
        if (handlers[index]->is_parsed())
        {
            ++index;
        }
        return true;
    }

protected:
    void reset() override
    {
        index = 0;
        depth = 0;
        for (auto&& h : handlers)
            h->prepare_for_reuse();
    }

public:
    bool Null() override
    {
        if (index >= N)
            return true;
        return postcheck(handlers[index]->Null());
    }

    bool Bool(bool b) override
    {
        if (index >= N)
            return true;
        return postcheck(handlers[index]->Bool(b));
    }

    bool Int(int i) override
    {
        if (index >= N)
            return true;
        return postcheck(handlers[index]->Int(i));
    }

    bool Uint(unsigned i) override
    {
        if (index >= N)
            return true;
        return postcheck(handlers[index]->Uint(i));
    }

    bool Int64(std::int64_t i) override
    {
        if (index >= N)
            return true;
        return postcheck(handlers[index]->Int64(i));
    }

    bool Uint64(std::uint64_t i) override
    {
        if (index >= N)
            return true;
        return postcheck(handlers[index]->Uint64(i));
    }

    bool Double(double d) override
    {
        if (index >= N)
            return true;
        return postcheck(handlers[index]->Double(d));
    }

    bool String(const char* str, SizeType length, bool copy) override
    {
        if (index >= N)
            return true;
        return postcheck(handlers[index]->String(str, length, copy));
    }

    bool Key(const char* str, SizeType length, bool copy) override
    {
        if (index >= N)
            return true;
        return postcheck(handlers[index]->Key(str, length, copy));
    }

    bool StartArray() override
    {
        if (++depth > 1)
        {
            if (index >= N)
                return true;
            return postcheck(handlers[index]->StartArray());
        }
        return true;
    }

    bool EndArray(SizeType length) override
    {
        if (--depth > 0)
        {
            if (index >= N)
                return true;
            return postcheck(handlers[index]->EndArray(length));
        }
        this->parsed = true;
        return true;
    }

    bool StartObject() override
    {
        if (index >= N)
            return true;
        return postcheck(handlers[index]->StartObject());
    }

    bool EndObject(SizeType length) override
    {
        if (index >= N)
            return true;
        return postcheck(handlers[index]->EndObject(length));
    }

    bool reap_error(ErrorStack& errs) override
    {
        if (!this->the_error)
            return false;

        errs.push(this->the_error.release());
        for (auto&& h : handlers)
            h->reap_error(errs);
        return true;
    }

    bool write(IHandler* out) const override
    {
        if (!out->StartArray())
            return false;
        for (auto&& h : handlers)
        {
            if (!h->write(out))
                return false;
        }
        return out->EndArray(N);
    }

    //void generate_schema(Value& output, MemoryPoolAllocator& alloc) const override
    //{
    //    output.SetObject();
    //    output.AddMember(rapidjson::StringRef("type"), rapidjson::StringRef("array"), alloc);
    //    Value items(rapidjson::kArrayType);
    //    for (auto&& h : handlers)
    //    {
    //        Value item;
    //        h->generate_schema(item, alloc);
    //        items.PushBack(item, alloc);
    //    }
    //    output.AddMember(rapidjson::StringRef("items"), items, alloc);
    //}
};

namespace nonpublic
{
    template <std::size_t index, std::size_t N, typename Tuple>
    struct TupleIniter
    {
        void operator()(std::unique_ptr<BaseHandler>* handlers, Tuple& t) const
        {
            handlers[index].reset(
                new Handler<typename std::tuple_element<index, Tuple>::type>(&std::get<index>(t)));
            TupleIniter<index + 1, N, Tuple>{}(handlers, t);
        }
    };

    template <std::size_t N, typename Tuple>
    struct TupleIniter<N, N, Tuple>
    {
        void operator()(std::unique_ptr<BaseHandler>* handlers, Tuple& t) const
        {
            (void)handlers;
            (void)t;
        }
    };
}

template <typename... Ts>
class Handler<std::tuple<Ts...>> : public TupleHander<std::tuple_size<std::tuple<Ts...>>::value>
{
private:
    static const std::size_t N = std::tuple_size<std::tuple<Ts...>>::value;

public:
    explicit Handler(std::tuple<Ts...>* t)
    {
        nonpublic::TupleIniter<0, N, std::tuple<Ts...>> initer;
        initer(this->handlers.data(), *t);
    }

    std::string type_name() const override
    {
        std::string str = "std::tuple<";
        for (auto&& h : this->handlers)
        {
            str += h->type_name();
            str += ", ";
        }
        str.pop_back();
        str.pop_back();
        str += '>';
        return str;
    }
};
#endif

class Parse {
 public:
  bool SetValue(bool, BaseHandler& handler) const;
  bool SetValue(short, BaseHandler& handler) const;
  bool SetValue(unsigned short, BaseHandler& handler) const;
  bool SetValue(int, BaseHandler& handler) const;
  bool SetValue(unsigned int, BaseHandler& handler) const;
  bool SetValue(int64_t, BaseHandler& handler) const;
  bool SetValue(uint64_t, BaseHandler& handler) const;
  bool SetValue(float f, BaseHandler& handler) const;
  bool SetValue(double f, BaseHandler& handler) const;
  bool SetValue(char, BaseHandler& handler) const;
  bool SetValue(const std::string& s, BaseHandler& handler) const;

  template <typename T>
  bool SetValue(const std::vector<T>& v, BaseHandler& handler) const {
    if (!handler.StartArray()) {
      return false;
    }

    for (size_t i = 0; i < v.size(); i++) {
      if (!SetValue(v[i], handler)) {
        return false;
      }
    }

    return handler.EndArray(v.size());
  }

  template <typename T, size_t N>
  bool SetValue(const std::array<T, N>& v, BaseHandler& handler) const {
    if (!handler.StartArray()) {
      return false;
    }

    for (size_t i = 0; i < N; i++) {
      if (!SetValue(v[i], handler)) {
        return false;
      }
    }

    return handler.EndArray(v.size());
  }

  template <typename T>
  bool SetValue(const std::map<std::string, T>& m, BaseHandler& handler) const {
    if (!handler.StartObject()) {
      return false;
    }

    for (auto item : m) {
      if (!handler.Key(item.first.c_str(), item.first.size(),
                       /* copy(not used) */ true)) {
        return false;
      }

      if (!SetValue(item.second, handler)) {
        return false;
      }
    }

    return handler.EndObject(m.size());
  }

  template <class T, class Hash, class Equal>
  bool SetValue(const std::unordered_map<std::string, T, Hash, Equal>& m,
                BaseHandler& handler) const {
    if (!handler.StartObject()) {
      return false;
    }

    for (auto item : m) {
      if (!handler.Key(item.first)) {
        return false;
      }

      if (!SetValue(item.second, handler)) {
        return false;
      }
    }

    return handler.EndObject(m.size());
  }
};

}  // namespace simple_type_relection
