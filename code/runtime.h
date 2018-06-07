#ifndef BUCKET_RUNTIME_H
#define BUCKET_RUNTIME_H

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

typedef bool          bucket_bool_t;
typedef int64_t       bucket_int_t;

#define BUCKET_INT_MIN  INT64_MIN
#define BUCKET_INT_MAX  INT64_MAX

bucket_bool_t bucket_bool_and(const bucket_bool_t*, bucket_bool_t);
bucket_bool_t bucket_bool_or(const bucket_bool_t*, bucket_bool_t);
bucket_bool_t bucket_bool_not(const bucket_bool_t*);

bucket_int_t bucket_int_add(const bucket_int_t*, bucket_int_t);
bucket_int_t bucket_int_sub(const bucket_int_t*, bucket_int_t);
bucket_int_t bucket_int_mul(const bucket_int_t*, bucket_int_t);
bucket_int_t bucket_int_div(const bucket_int_t*, bucket_int_t);
bucket_int_t bucket_int_mod(const bucket_int_t*, bucket_int_t);
bucket_bool_t bucket_int_lt(const bucket_int_t*, bucket_int_t);
bucket_bool_t bucket_int_le(const bucket_int_t*, bucket_int_t);
bucket_bool_t bucket_int_eq(const bucket_int_t*, bucket_int_t);
bucket_bool_t bucket_int_ne(const bucket_int_t*, bucket_int_t);
bucket_bool_t bucket_int_ge(const bucket_int_t*, bucket_int_t);
bucket_bool_t bucket_int_gt(const bucket_int_t*, bucket_int_t);

#endif
