#include "miscellaneous.hxx"

#ifdef BUCKET_DEBUG

void bucket_assert_fail(const char* assertion, const char* file,
  unsigned line, const char* function)
{
  throw make_error<AssertionError>(file, ": Assertion failed: '", assertion,
    "' in function: '", function, "'\n");
}

#endif
