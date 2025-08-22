#include <snitch/snitch.hpp>
#include <quanta/quanta.hpp>

using namespace pkg::quanta;

TEST_CASE("quanta kvds test", "[unit][quanta]") {
    quanta_config_c config;
    config.type = quanta_store_type_e::MEMORY;
    quanta_store_c store(config);

    CHECK(store.open());
    CHECK(store.close());
}