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

#include "builtin.hxx"
#include <cinttypes>
#include <cstdio>
#include <cstdlib>

bucket_bool_t bucket_bool_and(const bucket_bool_t* a, bucket_bool_t b)
{
  return *a && b;
}

bucket_bool_t bucket_bool_or(const bucket_bool_t* a, bucket_bool_t b)
{
  return *a || b;
}

bucket_bool_t bucket_bool_not(const bucket_bool_t* a)
{
  return !*a;
}

void bucket_bool_print(const bucket_bool_t* a)
{
  [[maybe_unused]] auto result = std::printf("%s", *a ? "true" : "false");
  #ifdef BUCKET_BUILTIN_UBCHECK
  if (result < 0) std::abort();
  #endif
}

bucket_int_t bucket_int_add(const bucket_int_t* a, bucket_int_t b) {
  #ifdef BUCKET_BUILTIN_UBCHECK
  if (
    ((b > 0) && (b > std::numeric_limits<bucket_int_t>::max() - *a)) ||
    ((b < 0) && (b < std::numeric_limits<bucket_int_t>::min() - *a))
  ) std::abort();
  #endif
  return *a + b;
}

bucket_int_t bucket_int_sub(const bucket_int_t* a, bucket_int_t b) {
  #ifdef BUCKET_BUILTIN_UBCHECK
  if (
    ((b < 0) && (*a > std::numeric_limits<bucket_int_t>::max() + b)) ||
    ((b > 0) && (*a < std::numeric_limits<bucket_int_t>::min() + b))
  ) std::abort();
  #endif
  return *a - b;
}

bucket_int_t bucket_int_mul(const bucket_int_t* a, bucket_int_t b) {
  #ifdef BUCKET_BUILTIN_UBCHECK
  if (
    (b > std::numeric_limits<bucket_int_t>::max() / *a) ||
    (b < std::numeric_limits<bucket_int_t>::min() / *a) ||
    ((b == -1) && (*a == std::numeric_limits<bucket_int_t>::min())) ||
    ((*a == -1) && (b == std::numeric_limits<bucket_int_t>::min()))
  ) std::abort();
  #endif
  return *a * b;
}

bucket_int_t bucket_int_div(const bucket_int_t* a, bucket_int_t b) {
  #ifdef BUCKET_BUILTIN_UBCHECK
  if (b == 0) std::abort();
  #endif
  return *a / b;
}

bucket_int_t bucket_int_mod(const bucket_int_t* a, bucket_int_t b)
{
  #ifdef BUCKET_BUILTIN_UBCHECK
  if (b == 0) std::abort();
  #endif
  return *a % b;
}

bucket_bool_t bucket_int_lt(const bucket_int_t* a, bucket_int_t b)
{
  return *a < b;
}

bucket_bool_t bucket_int_le(const bucket_int_t* a, bucket_int_t b)
{
  return *a <= b;
}

bucket_bool_t bucket_int_eq(const bucket_int_t* a, bucket_int_t b)
{
  return *a == b;
}

bucket_bool_t bucket_int_ne(const bucket_int_t* a, bucket_int_t b)
{
  return *a != b;
}

bucket_bool_t bucket_int_gt(const bucket_int_t* a, bucket_int_t b)
{
  return *a > b;
}

bucket_bool_t bucket_int_ge(const bucket_int_t* a, bucket_int_t b)
{
  return *a >= b;
}

void bucket_int_print(const bucket_int_t* a)
{
  [[maybe_unused]] auto result = std::printf("%" PRId64, *a);
  #ifdef BUCKET_BUILTIN_UBCHECK
  if (result < 0) std::abort();
  #endif
}

bucket_real_t bucket_real_add(const bucket_real_t* a, bucket_real_t b)
{
  return *a + b;
}

bucket_real_t bucket_real_sub(const bucket_real_t* a, bucket_real_t b)
{
  return *a - b;
}

bucket_real_t bucket_real_mul(const bucket_real_t* a, bucket_real_t b)
{
  return *a * b;
}

bucket_real_t bucket_real_div(const bucket_real_t* a, bucket_real_t b)
{
  return *a / b;
}

bucket_bool_t bucket_real_lt(const bucket_real_t* a, bucket_real_t b)
{
  return *a < b;
}

bucket_bool_t bucket_real_le(const bucket_real_t* a, bucket_real_t b)
{
  return *a <= b;
}

bucket_bool_t bucket_real_eq(const bucket_real_t* a, bucket_real_t b)
{
  return *a == b;
}

bucket_bool_t bucket_real_ne(const bucket_real_t* a, bucket_real_t b)
{
  return *a != b;
}

bucket_bool_t bucket_real_gt(const bucket_real_t* a, bucket_real_t b)
{
  return *a > b;
}

bucket_bool_t bucket_real_ge(const bucket_real_t* a, bucket_real_t b)
{
  return *a >= b;
}

void bucket_real_print(const bucket_real_t* a)
{
  [[maybe_unused]] auto result = std::printf("%f", *a);
  #ifdef BUCKET_BUILTIN_UBCHECK
  if (result < 0) std::abort();
  #endif
}

bucket_byte_t bucket_byte_and(const bucket_byte_t* a, bucket_byte_t b)
{
  return *a & b;
}
bucket_byte_t bucket_byte_or(const bucket_byte_t* a, bucket_byte_t b)
{
  return *a | b;
}

bucket_byte_t bucket_byte_xor(const bucket_byte_t* a, bucket_byte_t b)
{
  return *a ^ b;
}

bucket_byte_t bucket_byte_not(const bucket_byte_t* a)
{
  return ~*a;
}

bucket_byte_t bucket_byte_lshift(const bucket_byte_t* a, bucket_int_t b)
{
  #ifdef BUCKET_BUILTIN_UBCHECK
  if (b < 0) std::abort();
  #endif
  return static_cast<bucket_byte_t>(*a << b);
}

bucket_byte_t bucket_byte_rshift(const bucket_byte_t* a, bucket_int_t b)
{
  #ifdef BUCKET_BUILTIN_UBCHECK
  if (b < 0) std::abort();
  #endif
  return static_cast<bucket_byte_t>(*a >> b);
}

void bucket_byte_print(const bucket_byte_t* a)
{
  [[maybe_unused]] auto result = std::printf("%" PRIu8, *a);
  #ifdef BUCKET_BUILTIN_UBCHECK
  if (result < 0) std::abort();
  #endif
}

void bucket_system_test(void)
{
  [[maybe_unused]] auto result = std::printf("%s", "<bucket_system_test>");
  #ifdef BUCKET_BUILTIN_UBCHECK
  if (result < 0) std::abort();
  #endif
}
