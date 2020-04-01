#include "runtime.h"
#include <assert.h>
#include <stdio.h>

bucket_bool_t bucket_bool_and(const bucket_bool_t* a, bucket_bool_t b) {
  return *a && b;
}

bucket_bool_t bucket_bool_or(const bucket_bool_t* a, bucket_bool_t b) {
  return *a || b;
}

bucket_bool_t bucket_bool_not(const bucket_bool_t* a) {
  return !*a;
}

bucket_int_t bucket_int_add(const bucket_int_t* a, bucket_int_t b) {
  assert(!(
    ((b > 0) && (b > BUCKET_INT_MAX - *a)) ||
    ((b < 0) && (b < BUCKET_INT_MIN - *a))
  ));
  return *a + b;
}

bucket_int_t bucket_int_sub(const bucket_int_t* a, bucket_int_t b) {
  assert(!(
    ((b < 0) && (*a > BUCKET_INT_MAX + b)) ||
    ((b > 0) && (*a < BUCKET_INT_MIN + b))
  ));
  return *a - b;
}

bucket_int_t bucket_int_mul(const bucket_int_t* a, bucket_int_t b) {
  assert(!(
    (b > BUCKET_INT_MAX / *a) ||
    (b < BUCKET_INT_MIN / *a) ||
    ((b == -1) && (*a == BUCKET_INT_MIN)) ||
    ((*a == -1) && (b == BUCKET_INT_MIN))
  ));
  return *a * b;
}

bucket_int_t bucket_int_div(const bucket_int_t* a, bucket_int_t b) {
  assert(b);
  return *a / b;
}

bucket_int_t bucket_int_mod(const bucket_int_t* a, bucket_int_t b) {
  assert(b);
  return *a % b;
}

bucket_bool_t bucket_int_lt(const bucket_int_t* a, bucket_int_t b) {
  return *a < b;
}

bucket_bool_t bucket_int_le(const bucket_int_t* a, bucket_int_t b) {
  return *a <= b;
}

bucket_bool_t bucket_int_eq(const bucket_int_t* a, bucket_int_t b) {
  return *a == b;
}

bucket_bool_t bucket_int_ne(const bucket_int_t* a, bucket_int_t b) {
  return *a != b;
}

bucket_bool_t bucket_int_ge(const bucket_int_t* a, bucket_int_t b) {
  return *a >= b;
}

bucket_bool_t bucket_int_gt(const bucket_int_t* a, bucket_int_t b) {
  return *a > b;
}

bucket_nil_t bucket_int_print(const bucket_int_t* a) {
  printf("%lld", *a);
}
