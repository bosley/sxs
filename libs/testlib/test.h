#ifndef SXS_TEST_FRAMEWORK_H
#define SXS_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT(condition)                                                      \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "Assertion failed: %s\n", #condition);                   \
      fprintf(stderr, "  at %s:%d in %s\n", __FILE__, __LINE__, __func__);     \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#define ASSERT_EQ(a, b)                                                        \
  do {                                                                         \
    if ((a) != (b)) {                                                          \
      fprintf(stderr, "Assertion failed: %s == %s\n", #a, #b);                 \
      fprintf(stderr, "  at %s:%d in %s\n", __FILE__, __LINE__, __func__);     \
      fprintf(stderr, "  values: %ld != %ld\n", (long)(a), (long)(b));         \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#define ASSERT_NEQ(a, b)                                                       \
  do {                                                                         \
    if ((a) == (b)) {                                                          \
      fprintf(stderr, "Assertion failed: %s != %s\n", #a, #b);                 \
      fprintf(stderr, "  at %s:%d in %s\n", __FILE__, __LINE__, __func__);     \
      fprintf(stderr, "  values: %ld == %ld\n", (long)(a), (long)(b));         \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#define ASSERT_TRUE(condition) ASSERT(condition)

#define ASSERT_FALSE(condition) ASSERT(!(condition))

#define ASSERT_STR_EQ(a, b)                                                    \
  do {                                                                         \
    if (strcmp((a), (b)) != 0) {                                               \
      fprintf(stderr, "Assertion failed: %s == %s\n", #a, #b);                 \
      fprintf(stderr, "  at %s:%d in %s\n", __FILE__, __LINE__, __func__);     \
      fprintf(stderr, "  values: \"%s\" != \"%s\"\n", (a), (b));               \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#define ASSERT_NULL(ptr)                                                       \
  do {                                                                         \
    if ((ptr) != NULL) {                                                       \
      fprintf(stderr, "Assertion failed: %s is NULL\n", #ptr);                 \
      fprintf(stderr, "  at %s:%d in %s\n", __FILE__, __LINE__, __func__);     \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#define ASSERT_NOT_NULL(ptr)                                                   \
  do {                                                                         \
    if ((ptr) == NULL) {                                                       \
      fprintf(stderr, "Assertion failed: %s is not NULL\n", #ptr);             \
      fprintf(stderr, "  at %s:%d in %s\n", __FILE__, __LINE__, __func__);     \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#endif
