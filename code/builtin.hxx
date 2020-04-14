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

#ifndef BUCKET_BUILTIN_HXX
#define BUCKET_BUILTIN_HXX

#include <cstdint>
#include <limits>

using bucket_bool_t = bool;
using bucket_int_t  = std::int64_t;
using bucket_byte_t = std::uint8_t;
using bucket_rune_t = std::uint32_t;
using bucket_real_t = double;

static_assert(std::numeric_limits<bucket_real_t>::is_iec559);

extern "C" {

bucket_bool_t bucket_bool_and(const bucket_bool_t*, bucket_bool_t);
bucket_bool_t bucket_bool_or(const bucket_bool_t*, bucket_bool_t);
bucket_bool_t bucket_bool_not(const bucket_bool_t*);
void bucket_bool_print(const bucket_bool_t*);

bucket_int_t bucket_int_add(const bucket_int_t*, bucket_int_t);
bucket_int_t bucket_int_sub(const bucket_int_t*, bucket_int_t);
bucket_int_t bucket_int_mul(const bucket_int_t*, bucket_int_t);
bucket_int_t bucket_int_div(const bucket_int_t*, bucket_int_t);
bucket_int_t bucket_int_mod(const bucket_int_t*, bucket_int_t);
bucket_bool_t bucket_int_lt(const bucket_int_t*, bucket_int_t);
bucket_bool_t bucket_int_le(const bucket_int_t*, bucket_int_t);
bucket_bool_t bucket_int_eq(const bucket_int_t*, bucket_int_t);
bucket_bool_t bucket_int_ne(const bucket_int_t*, bucket_int_t);
bucket_bool_t bucket_int_gt(const bucket_int_t*, bucket_int_t);
bucket_bool_t bucket_int_ge(const bucket_int_t*, bucket_int_t);
void bucket_int_print(const bucket_int_t*);

bucket_real_t bucket_real_add(const bucket_real_t*, bucket_real_t);
bucket_real_t bucket_real_sub(const bucket_real_t*, bucket_real_t);
bucket_real_t bucket_real_mul(const bucket_real_t*, bucket_real_t);
bucket_real_t bucket_real_div(const bucket_real_t*, bucket_real_t);
bucket_bool_t bucket_real_lt(const bucket_real_t*, bucket_real_t);
bucket_bool_t bucket_real_le(const bucket_real_t*, bucket_real_t);
bucket_bool_t bucket_real_eq(const bucket_real_t*, bucket_real_t);
bucket_bool_t bucket_real_ne(const bucket_real_t*, bucket_real_t);
bucket_bool_t bucket_real_gt(const bucket_real_t*, bucket_real_t);
bucket_bool_t bucket_real_ge(const bucket_real_t*, bucket_real_t);
void bucket_real_print(const bucket_real_t*);

bucket_byte_t bucket_byte_and(const bucket_byte_t*, bucket_byte_t);
bucket_byte_t bucket_byte_or(const bucket_byte_t*, bucket_byte_t);
bucket_byte_t bucket_byte_xor(const bucket_byte_t*, bucket_byte_t);
bucket_byte_t bucket_byte_not(const bucket_byte_t*);
bucket_byte_t bucket_byte_lshift(const bucket_byte_t*, bucket_int_t);
bucket_byte_t bucket_byte_rshift(const bucket_byte_t*, bucket_int_t);
void bucket_byte_print(const bucket_byte_t*);

void bucket_system_test(void);

}

#endif
