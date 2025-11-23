#include <runtime/session/session.hpp>
#include <snitch/snitch.hpp>

TEST_CASE("session basic operations", "[unit][runtime][session]") {
  runtime::session_c session;

  SECTION("session creation") {
    CHECK(session.get_id() == "");
    CHECK_FALSE(session.is_active());
  }
}

