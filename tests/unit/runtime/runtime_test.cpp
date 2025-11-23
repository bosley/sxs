#include <runtime/runtime.hpp>
#include <snitch/snitch.hpp>

TEST_CASE("runtime basic operations", "[unit][runtime]") {
  runtime::options_s options;
  runtime::runtime_c rt(options);

  SECTION("initialize and shutdown") {
    CHECK(rt.initialize());
    CHECK(rt.is_running());
    CHECK_FALSE(rt.initialize());
    CHECK(rt.shutdown());
    CHECK_FALSE(rt.is_running());
    CHECK_FALSE(rt.shutdown());
  }
}
