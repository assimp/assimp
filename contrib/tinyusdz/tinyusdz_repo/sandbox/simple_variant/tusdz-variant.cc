#include <iostream>
//#include <utility>
//#include <typeinfo>
//#include <type_traits>
#include <string>

#include "value-type.hh"

namespace tinyusdz {

// Simpole variant template Based on
// https://gist.github.com/tibordp/6909880#file-variant-cc-L148
//
// Modification:
//   - Use TypeTrait::type_id so only types which defines TypeTrait are
//   supported.
//   - Remove std::swap
//   https://gist.github.com/tibordp/6909880?permalink_comment_id=1831344#gistcomment-1831344

// Equivalent to std::aligned_storage
template <unsigned int Len, unsigned int Align>
struct aligned_storage {
  struct type {
    alignas(Align) unsigned char data[Len];
  };
};

template <size_t arg1, size_t... others>
struct static_max;

template <size_t arg>
struct static_max<arg> {
  static const size_t value = arg;
};

template <size_t arg1, size_t arg2, size_t... others>
struct static_max<arg1, arg2, others...> {
  static const size_t value = arg1 >= arg2 ? static_max<arg1, others...>::value
                                           : static_max<arg2, others...>::value;
};

template <typename... Ts>
struct variant_helper;

template <typename F, typename... Ts>
struct variant_helper<F, Ts...> {
  inline static void destroy(size_t id, void* data) {
    if (id == value::TypeTrait<F>::type_id)
      reinterpret_cast<F*>(data)->~F();
    else
      variant_helper<Ts...>::destroy(id, data);
  }

  inline static void move(size_t old_t, void* old_v, void* new_v) {
    if (old_t == value::TypeTrait<F>::type_id)
      new (new_v) F(std::move(*reinterpret_cast<F*>(old_v)));
    else
      variant_helper<Ts...>::move(old_t, old_v, new_v);
  }

  inline static void copy(size_t old_t, const void* old_v, void* new_v) {
    if (old_t == value::TypeTrait<F>::type_id)
      new (new_v) F(*reinterpret_cast<const F*>(old_v));
    else
      variant_helper<Ts...>::copy(old_t, old_v, new_v);
  }
};

template <>
struct variant_helper<> {
  inline static void destroy(size_t id, void* data) {}
  inline static void move(size_t old_t, void* old_v, void* new_v) {}
  inline static void copy(size_t old_t, const void* old_v, void* new_v) {}
};

template <typename...>
struct is_one_of {
  static constexpr bool value = false;
};

template <typename T, typename S, typename... Ts>
struct is_one_of<T, S, Ts...> {
  static constexpr bool value =
      std::is_same<T, S>::value || is_one_of<T, Ts...>::value;
};

template <typename... Ts>
struct variant {
 private:
  static const size_t data_size = static_max<sizeof(Ts)...>::value;
  static const size_t data_align = static_max<alignof(Ts)...>::value;

  using data_t = typename std::aligned_storage<data_size, data_align>::type;

  using helper_t = variant_helper<Ts...>;

  static inline size_t invalid_type() {
    return value::TypeTrait<void>::type_id;
  }

  size_t type_id;
  data_t data;

 public:
  variant() : type_id(invalid_type()) {}

  variant(const variant<Ts...>& old) : type_id(old.type_id) {
    helper_t::copy(old.type_id, &old.data, &data);
  }

  variant(variant<Ts...>&& old) : type_id(old.type_id) {
    helper_t::move(old.type_id, &old.data, &data);
  }

  // Serves as both the move and the copy asignment operator.
  variant<Ts...>& operator=(variant<Ts...>& rhs) {
    // std::swap + aligned_storage = undefined behavior with non-trivially
    // copyable types
    // https://gist.github.com/tibordp/6909880?permalink_comment_id=1831344#gistcomment-1831344
    // std::swap(type_id, old.type_id);
    // std::swap(data, old.data);

    helper_t::destroy(type_id, &data);
    type_id = rhs.type_id;
    helper_t::copy(rhs.type_id, &rhs.data, &data);

    return *this;
  }

  template <typename T>
  void is() {
    return (type_id == value::TypeTrait<T>::type_id);
  }

  bool valid() { return (type_id != invalid_type()); }

  // template<typename T, typename... Args>
  template <typename T, typename... Args,
            typename =
                typename std::enable_if<is_one_of<T, Ts...>::value, void>::type>
  void set(Args&&... args) {
    // First we destroy the current contents
    helper_t::destroy(type_id, &data);
    new (&data) T(std::forward<Args>(args)...);
    type_id = value::TypeTrait<T>::type_id;
  }

  // template<typename T>
  template <typename T, typename... Args,
            typename =
                typename std::enable_if<is_one_of<T, Ts...>::value, void>::type>
  T& get() {
    // It is a dynamic_cast-like behaviour
    if (type_id == value::TypeTrait<T>::type_id)
      return *reinterpret_cast<T*>(&data);
    else
      throw std::bad_cast();
  }

  ~variant() { helper_t::destroy(type_id, &data); }
};

}  // namespace tinyusdz

int main(int argc, char** argv) {
  tinyusdz::variant<bool, std::string> a;
  a.set<bool>(true);
  a.set<std::string>("bora");

  tinyusdz::variant<bool, std::string> b = a;

  std::cout << a.get<std::string>() << "\n";
  std::cout << b.get<std::string>() << "\n";

  return 0;
}
