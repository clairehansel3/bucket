// Copyright (C) 2020  Claire Hansel
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef BUCKET_MISCELLANEOUS_HXX
#define BUCKET_MISCELLANEOUS_HXX

#include <algorithm>
#ifndef BUCKET_NO_ALLOCA
#include <alloca.h>
#endif
#ifdef BUCKET_DEBUG
#define _GNU_SOURCE
#include <boost/stacktrace.hpp>
#endif
#include <cassert>
#include <cstddef>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#ifdef BUCKET_DEBUG
  #define BUCKET_UNREACHABLE() exception("internal compiler", \
                               "reached supposedly unreachable code")
#else
  #if defined(__clang__) || defined(__GNUC__)
    #define BUCKET_UNREACHABLE() __builtin_unreachable()
  #else
    #define BUCKET_UNREACHABLE() (void)0
  #endif
#endif

#ifdef BUCKET_HIGHLIGHTING_OFF
  #define BUCKET_BOLD  ""
  #define BUCKET_BLACK ""
  #define BUCKET_RED   ""
#else
  #define BUCKET_BOLD  "\033[1;37m"
  #define BUCKET_BLACK "\033[m"
  #define BUCKET_RED   "\033[0;31m"
#endif

namespace details {

// There is no template in type_traits to determine if a type is an integer, so
// the type_traits like templates 'is_signed_integer' and 'is_unsigned_integer'
// are defined.

template <typename> struct is_signed_integer               : std::false_type {};
template <> struct is_signed_integer<signed char>          : std::true_type  {};
template <> struct is_signed_integer<short>                : std::true_type  {};
template <> struct is_signed_integer<int>                  : std::true_type  {};
template <> struct is_signed_integer<long>                 : std::true_type  {};
template <> struct is_signed_integer<long long>            : std::true_type  {};

template <typename> struct is_unsigned_integer             : std::false_type {};
template <> struct is_unsigned_integer<unsigned char>      : std::true_type  {};
template <> struct is_unsigned_integer<unsigned short>     : std::true_type  {};
template <> struct is_unsigned_integer<unsigned>           : std::true_type  {};
template <> struct is_unsigned_integer<unsigned long>      : std::true_type  {};
template <> struct is_unsigned_integer<unsigned long long> : std::true_type  {};

template <typename T>
inline constexpr bool is_signed_integer_v = is_signed_integer<T>::value;

template <typename T>
inline constexpr bool is_unsigned_integer_v = is_unsigned_integer<T>::value;

// ConcatenateHelper is a class template that implements two functions for a
// type T: lengthAsString and writeString. If a value of type T is converted to
// a string, lengthAsString returns that string's length. The function
// writeString converts a value of type T to a string and writes the string to a
// buffer guarenteed to be the right length or longer.


template <typename T, typename Enable = void>
struct ConcatenateHelper
{
  static constexpr std::size_t lengthAsString(T value);
  static char* writeString(char* buf, T value);
};

// char specialization

template <>
struct ConcatenateHelper<char>
{
  static constexpr std::size_t lengthAsString(char)
  {
    return 1;
  }
  static char* writeString(char* buf, char value)
  {
    *buf++ = value;
    return buf;
  }
};

// bool specialization

template <>
struct ConcatenateHelper<bool>
{
  static constexpr std::size_t lengthAsString(bool value)
  {
    return (value ? sizeof("true") : sizeof("false")) - 1;
  }
  static char* writeString(char* buf, bool value)
  {
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

// unsigned integer specialization

template <typename T>
struct ConcatenateHelper<T, std::enable_if_t<is_unsigned_integer_v<T>>>
{
  static constexpr std::size_t lengthAsString(T value)
  {
    std::size_t result = 0;
    do {
      value /= 10;
      result++;
    } while (value);
    return result;
  }
  static char* writeString(char* buf, T value)
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

// signed integer specialization

template <typename T>
struct ConcatenateHelper<T, std::enable_if_t<is_signed_integer_v<T>>>
{
  static constexpr std::size_t lengthAsString(T value)
  {
    std::size_t result = value < 0 ? 1 : 0;
    do {
      value /= 10;
      result++;
    } while (value);
    return result;
  }
  static char* writeString(char* buf, T value)
  {
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

// fixed size character array specialization

template <std::size_t N>
struct ConcatenateHelper<const char (&) [N]>
{
  static constexpr std::size_t lengthAsString(const char (&) [N])
  {
    return N - 1;
  }
  static char* writeString(char* buf, const char (&value) [N])
  {
    return std::copy(value, value + N - 1, buf);
  }
};

// string specialization

template <>
struct ConcatenateHelper<std::string_view>
{
  static constexpr std::size_t lengthAsString(std::string_view value)
  {
    return value.size();
  }
  static char* writeString(char* buf, std::string_view value)
  {
    return std::copy(value.begin(), value.end(), buf);
  }
};

// void* specialization
template <>
struct ConcatenateHelper<void*>
{
  static std::size_t lengthAsString(void* value)
  {
    std::size_t int_value = reinterpret_cast<std::size_t>(value);
    std::size_t length = 0;
    while (int_value != 0)
    {
      ++length;
      int_value /= 16;
    }
    std::size_t length2 = int_value / 16;
    assert(length == length2);
    return length2 + 2; // for '0x'
  }
  static char* writeString(char* buf, void* value)
  {
    std::size_t int_value = reinterpret_cast<std::size_t>(value);
    *buf++ = '0';
    *buf++ = 'x';
    while (int_value != 0) {
      int temp = int_value % 16;
      if (temp < 10) {
        *buf++ = static_cast<char>(temp) + '0';
      }
      else {
        *buf++ = static_cast<char>(temp) + 'a';
      }
      int_value /= 16;
    }
    return buf;
  }
};

// ConcatenateTypeConverter is a class template which for a type T chooses the
// right ConcatenateHelper. Often T is something like a reference or a redundant
// type (e.g. const char* which is basically the same as std::string_view).

template <typename T, typename Enable = void>
struct TypeConverter;

template <>
struct TypeConverter<char>
{
  using type = char;
};

template <>
struct TypeConverter<bool>
{
  using type = bool;
};

template <>
struct TypeConverter<bool&>
{
  using type = bool;
};

template <>
struct TypeConverter<const bool&>
{
  using type = bool;
};

template <typename T>
struct TypeConverter<T, std::enable_if_t<is_unsigned_integer_v<T>>>
{
  using type = T;
};

template <typename T>
struct TypeConverter<T, std::enable_if_t<is_signed_integer_v<T>>>
{
  using type = T;
};

template <typename T>
struct TypeConverter<T&, std::enable_if_t<is_unsigned_integer_v<T>>>
{
  using type = T;
};

template <typename T>
struct TypeConverter<T&, std::enable_if_t<is_signed_integer_v<T>>>
{
  using type = T;
};

template <typename T>
struct TypeConverter<const T&, std::enable_if_t<is_unsigned_integer_v<T>>>
{
  using type = T;
};

template <typename T>
struct TypeConverter<const T&, std::enable_if_t<is_signed_integer_v<T>>>
{
  using type = T;
};

template <>
struct TypeConverter<std::string_view>
{
  using type = std::string_view;
};

template <>
struct TypeConverter<std::string_view&>
{
  using type = std::string_view;
};

template <std::size_t N>
struct TypeConverter<const char (&) [N]>
{
  using type = const char (&) [N];
};

template <>
struct TypeConverter<char*>
{
  using type = std::string_view;
};

template <>
struct TypeConverter<char*&>
{
  using type = std::string_view;
};

template <>
struct TypeConverter<const char*>
{
  using type = std::string_view;
};

template <>
struct TypeConverter<const char*&>
{
  using type = std::string_view;
};

template <>
struct TypeConverter<const char* const&>
{
  using type = std::string_view;
};

template <>
struct TypeConverter<std::string>
{
  using type = std::string_view;
};

template <>
struct TypeConverter<std::string&>
{
  using type = std::string_view;
};

template <>
struct TypeConverter<const std::string&>
{
  using type = std::string_view;
};

template <>
struct TypeConverter<void*>
{
  using type = void*;
};

template <typename T>
using convert_type_t = typename TypeConverter<T>::type;

// RecursiveConcatenateHelper has two functions lengthAsString and writeString
// which are similar to the two functions of ConcatenateHelper except that they
// can take an arbitrary number of arguments of different types and the types
// are converted using TypeConverter to before the associated ConcatenateHelper
// template is chosen.

template <typename... Args>
struct RecursiveConcatenateHelper;

template <typename First, typename... Rest>
struct RecursiveConcatenateHelper<First, Rest...>
{
  static constexpr std::size_t lengthAsString(First&& first, Rest&&... rest)
  {
    return ConcatenateHelper<convert_type_t<First>>::lengthAsString(
      std::forward<First>(first))
    + RecursiveConcatenateHelper<Rest...>::lengthAsString(
      std::forward<Rest>(rest)...);
  }
  static char* writeString(char* buf, First&& first, Rest&&... rest)
  {
    return RecursiveConcatenateHelper<Rest...>::writeString(
        ConcatenateHelper<convert_type_t<First>>::writeString(
          buf, std::forward<First>(first)
        ),
      std::forward<Rest>(rest)...
    );
  }
};

template <>
struct RecursiveConcatenateHelper<>
{
  static constexpr std::size_t lengthAsString()
  {
    return 0;
  }
  static char* writeString(char* buf)
  {
    return buf;
  }
};

// The functions lengthAsString and writeString from RecursiveConcatenateHelper
// are converted to functions for convenience.

template <typename... Args>
constexpr std::size_t lengthAsString(Args&&... args)
{
  return RecursiveConcatenateHelper<Args...>::lengthAsString(
    std::forward<Args>(args)...
  );
}

template <typename... Args>
char* writeString(char* buf, Args&&... args)
{
  return RecursiveConcatenateHelper<Args...>::writeString(
    buf, std::forward<Args>(args)...
  );
}

}

template <typename FunctionType, typename... Args>
decltype(auto) concatenateAndCall(FunctionType&& function, Args&&... args)
  noexcept(std::is_nothrow_invocable_v<FunctionType, std::string_view>)
// This function is a lot but take a deep breath. As arguments it takes a
// function and then an arbitrary number of additional arguments. The total
// length of the string formed when all the additional arguments are converted
// to strings and concatenated is computed without allocating any memory. Then
// this string (note: not a std::string, just a character array) is actually
// created on the stack. The function passed to concatenateAndCall as the first
// argument is called with a std::string_view which is a view of the stack
// allocated string. concatenateAndCall returns the result of this function.
// it is important to note that the std::string_view passed to the function will
// be deallocated after the function returns and so please don't do anything
// like concatenateAndCall([](std::string_view s){return s;}, ...). Instead
// do something like concatenateAndCall([](std::string_view s){return
// std::string(s);}, ...).
{
  static_assert(std::is_invocable_v<FunctionType, std::string_view>);
  std::size_t size_as_string = details::lengthAsString(
    std::forward<Args>(args)...
  );
  #ifdef BUCKET_USE_ALLOCA
  char* buf = static_cast<char*>(alloca(size_as_string));
  #else
  auto buf_uptr = std::make_unique<char[]>(size_as_string);
  char* buf = buf_uptr.get();
  #endif
  [[maybe_unused]] char* ptr = details::writeString(
    buf, std::forward<Args>(args)...
  );
  assert(ptr - buf == static_cast<std::ptrdiff_t>(size_as_string));
  return std::invoke(std::forward<FunctionType>(function), std::string_view(
    buf, size_as_string)
  );
}

template <typename... Args>
std::string concatenate(Args&&... args) {
// Converts an arbitrary number of arguments to strings, concatenates them, and
// then returns the result.
  return concatenateAndCall(
    [](std::string_view string){
      auto result = std::string(string);
      return result;
    },
    std::forward<Args>(args)...
  );
}

template <typename... Args>
[[noreturn]] void exception(std::string_view name, Args&&... args)
// Converts an arbitrary number of arguments to strings, concatenates them, and
// then throws a runtime error created with the concatenated string.
{
  concatenateAndCall(
    [](std::string_view string){
      assert(string.find('\0') == string.size() - 1);
      throw std::runtime_error(string.data());
    },
    "" BUCKET_BOLD BUCKET_RED,
    name,
    " error" BUCKET_BLACK ": ",
    std::forward<Args>(args)...,
    #ifdef BUCKET_DEBUG
    "\n" BUCKET_BOLD BUCKET_RED "stack trace" BUCKET_BLACK ":\n",
    boost::stacktrace::to_string(boost::stacktrace::stacktrace()),
    #endif
    '\0'
  );
  BUCKET_UNREACHABLE();
}

template <typename... Args>
[[noreturn]] void error(std::string_view name, Args&&... args)
// Converts an arbitrary number of arguments to strings, concatenates them, and
// then throws a runtime error created with the concatenated string.
{
  concatenateAndCall(
    [](std::string_view string){
      std::cout << string;
    },
    "" BUCKET_BOLD BUCKET_RED,
    name,
    " error" BUCKET_BLACK ": ",
    std::forward<Args>(args)...
    #ifdef BUCKET_DEBUG
    ,
    "\n" BUCKET_BOLD BUCKET_RED "stack trace" BUCKET_BLACK ":\n",
    boost::stacktrace::to_string(boost::stacktrace::stacktrace())
    #endif
  );
  std::terminate();
}

template <typename... Args>
void print(Args&&... args)
// Converts an arbitrary number of arguments to strings, concatenates them, and
// then throws a runtime error created with the concatenated string.
{
  concatenateAndCall(
    [](std::string_view string){
      std::cout << string;
    },
    std::forward<Args>(args)...
  );
}

#endif
