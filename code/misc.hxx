#ifndef BUCKET_MISC_HXX
#define BUCKET_MISC_HXX

#include <algorithm>
#include <alloca.h>
#include <cassert>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <csignal>

namespace misc {

namespace details {

template <typename> struct is_ordinary_signed_integer               : std::false_type {};
template <> struct is_ordinary_signed_integer<signed char>          : std::true_type  {};
template <> struct is_ordinary_signed_integer<short>                : std::true_type  {};
template <> struct is_ordinary_signed_integer<int>                  : std::true_type  {};
template <> struct is_ordinary_signed_integer<long>                 : std::true_type  {};
template <> struct is_ordinary_signed_integer<long long>            : std::true_type  {};

template <typename> struct is_ordinary_unsigned_integer             : std::false_type {};
template <> struct is_ordinary_unsigned_integer<unsigned char>      : std::true_type  {};
template <> struct is_ordinary_unsigned_integer<unsigned short>     : std::true_type  {};
template <> struct is_ordinary_unsigned_integer<unsigned>           : std::true_type  {};
template <> struct is_ordinary_unsigned_integer<unsigned long>      : std::true_type  {};
template <> struct is_ordinary_unsigned_integer<unsigned long long> : std::true_type  {};

template <typename T>
inline constexpr bool is_ordinary_signed_integer_v = is_ordinary_signed_integer<T>::value;

template <typename T>
inline constexpr bool is_ordinary_unsigned_integer_v = is_ordinary_unsigned_integer<T>::value;

template <typename T, typename Enable = void>
struct ConcatenateHelper;

template <>
struct ConcatenateHelper<char> {
  static constexpr std::size_t sizeAsString(char) {
    return 1;
  }
  static char* writeAsString(char* buf, char value) {
    *buf++ = value;
    return buf;
  }
};

template <>
struct ConcatenateHelper<bool> {
  static constexpr std::size_t sizeAsString(bool value) {
    return (value ? sizeof("true") : sizeof("false")) - 1;
  }
  static char* writeAsString(char* buf, bool value) {
    if (value) {
      *buf++ = 't';
      *buf++ = 'r';
      *buf++ = 'u';
      *buf++ = 'e';
    }
    else {
      *buf++ = 'f';
      *buf++ = 'a';
      *buf++ = 'l';
      *buf++ = 's';
      *buf++ = 'e';
    }
    return buf;
  }
};


template <typename T>
struct ConcatenateHelper<T, std::enable_if_t<is_ordinary_unsigned_integer_v<T>>> {
  static constexpr std::size_t sizeAsString(T value) {
    std::size_t result = 0;
    do {
      value /= 10;
      result++;
    } while (value);
    return result;
  }
  static char* writeAsString(char* buf, T value)
  {
    auto buf_start = buf;
    do {
      *buf++ = value % 10 + '0';
      value /= 10;
    } while (value);
    std::reverse(buf_start, buf);
    return buf;
  }
};

template <typename T>
struct ConcatenateHelper<T, std::enable_if_t<is_ordinary_signed_integer_v<T>>> {
  static constexpr std::size_t sizeAsString(T value) {
    std::size_t result = value < 0 ? 1 : 0;
    do {
      value /= 10;
      result++;
    } while (value);
    return result;
  }
  static char* writeAsString(char* buf, T value) {
    if (value < 0) {
      *buf++ = '-';
      value = -value;
    }
    auto buf_start = buf;
    do {
      *buf++ = value % 10 + '0';
      value /= 10;
    } while (value);
    std::reverse(buf_start, buf);
    return buf;
  }
};

template <std::size_t N>
struct ConcatenateHelper<const char (&) [N]> {
  static constexpr std::size_t sizeAsString(const char (&) [N]) {
    return N - 1;
  }
  static char* writeAsString(char* buf, const char (&value) [N]) {
    return std::copy(value, value + N - 1, buf);
  }
};

template <>
struct ConcatenateHelper<std::string_view> {
  static constexpr std::size_t sizeAsString(std::string_view value) {
    return value.size();
  }
  static char* writeAsString(char* buf, std::string_view value) {
    return std::copy(value.begin(), value.end(), buf);
  }
};

template <typename T, typename Enable = void>
struct TypeConverter;

template <>
struct TypeConverter<char> {
  using type = char;
};

template <>
struct TypeConverter<bool> {
  using type = bool;
};

template <>
struct TypeConverter<bool&> {
  using type = bool;
};

template <>
struct TypeConverter<const bool&> {
  using type = bool;
};

template <typename T>
struct TypeConverter<T, std::enable_if_t<is_ordinary_unsigned_integer_v<T>>> {
  using type = T;
};

template <typename T>
struct TypeConverter<T, std::enable_if_t<is_ordinary_signed_integer_v<T>>> {
  using type = T;
};

template <typename T>
struct TypeConverter<T&, std::enable_if_t<is_ordinary_unsigned_integer_v<T>>> {
  using type = T;
};

template <typename T>
struct TypeConverter<T&, std::enable_if_t<is_ordinary_signed_integer_v<T>>> {
  using type = T;
};

template <typename T>
struct TypeConverter<const T&, std::enable_if_t<is_ordinary_unsigned_integer_v<T>>> {
  using type = T;
};

template <typename T>
struct TypeConverter<const T&, std::enable_if_t<is_ordinary_signed_integer_v<T>>> {
  using type = T;
};

template <>
struct TypeConverter<std::string_view> {
  using type = std::string_view;
};

template <std::size_t N>
struct TypeConverter<const char (&) [N]> {
  using type = const char (&) [N];
};

template <>
struct TypeConverter<char*> {
  using type = std::string_view;
};

template <>
struct TypeConverter<char*&> {
  using type = std::string_view;
};

template <>
struct TypeConverter<const char*> {
  using type = std::string_view;
};

template <>
struct TypeConverter<const char*&> {
  using type = std::string_view;
};

template <>
struct TypeConverter<std::string> {
  using type = std::string_view;
};

template <>
struct TypeConverter<std::string&> {
  using type = std::string_view;
};

template <>
struct TypeConverter<const std::string&> {
  using type = std::string_view;
};

template <typename T>
using convert_type_t = typename TypeConverter<T>::type;

template <typename... Args>
struct RecursiveConcatenateHelper;

template <typename First, typename... Rest>
struct RecursiveConcatenateHelper<First, Rest...> {
  static constexpr std::size_t sizeAsString(First&& first, Rest&&... rest) {
    return ConcatenateHelper<convert_type_t<First>>::sizeAsString(std::forward<First>(first)) +
           RecursiveConcatenateHelper<Rest...>::sizeAsString(std::forward<Rest>(rest)...);
  }
  static char* writeAsString(char* buf, First&& first, Rest&&... rest) {
    return RecursiveConcatenateHelper<Rest...>::writeAsString(
        ConcatenateHelper<convert_type_t<First>>::writeAsString(buf, std::forward<First>(first)),
      std::forward<Rest>(rest)...
    );
  }
};

template <>
struct RecursiveConcatenateHelper<> {
  static constexpr std::size_t sizeAsString() {
    return 0;
  }
  static char* writeAsString(char* buf) {
    return buf;
  }
};

template <typename... Args>
constexpr std::size_t sizeAsString(Args&&... args) {
  return RecursiveConcatenateHelper<Args...>::sizeAsString(std::forward<Args>(args)...);
}

template <typename... Args>
char* writeAsString(char* buf, Args&&... args) {
  return RecursiveConcatenateHelper<Args...>::writeAsString(buf, std::forward<Args>(args)...);
}

}

template <typename FunctionType, typename... Args>
decltype(auto) concatenateAndCall(FunctionType&& function, Args&&... args) noexcept(std::is_nothrow_invocable_v<FunctionType, std::string_view>) {
  static_assert(std::is_invocable_v<FunctionType, std::string_view>);
  std::size_t size_as_string = details::sizeAsString(std::forward<Args>(args)...);
  char* buf = static_cast<char*>(alloca(size_as_string));
  [[maybe_unused]] char* ptr = details::writeAsString(buf, std::forward<Args>(args)...);
  assert(ptr - buf == static_cast<std::ptrdiff_t>(size_as_string));
  return std::invoke(std::forward<FunctionType>(function), std::string_view(buf, size_as_string));
}

template <typename... Args>
[[noreturn]] void error(Args&&... args) {
  concatenateAndCall(
    [](std::string_view string){
      assert(string.find('\0') == string.size() - 1);
      std::raise(SIGSEGV);
      throw std::runtime_error(string.data());
    },
    std::forward<Args>(args)...,
    '\0'
  );
  #ifdef __clang__
  __builtin_unreachable();
  #endif
}

template <typename... Args>
std::string concatenate(Args&&... args) {
  return concatenateAndCall(
    [](std::string_view string){
      assert(string.find('\0') == string.size() - 1);
      return std::string(string);
    },
    std::forward<Args>(args)...,
    '\0'
  );
}

}

#endif
