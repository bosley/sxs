#include <runtime/entity/entity.hpp>
#include <snitch/snitch.hpp>

TEST_CASE("entity basic operations", "[unit][runtime][entity]") {
  runtime::entity_c entity;

  SECTION("entity creation") {
    CHECK(entity.get_id() == "");
  }
}

