#include "../slp.h"
#include "test.h"

int main(void) {
  ASSERT_TRUE(1 == 1);
  ASSERT_FALSE(0 == 1);
  ASSERT_EQ(42, 42);
  ASSERT_NEQ(1, 2);

  return 0;
}
