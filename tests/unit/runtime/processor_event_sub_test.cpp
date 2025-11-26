#include <atomic>
#include <chrono>
#include <filesystem>
#include <kvds/datastore.hpp>
#include <record/record.hpp>
#include <runtime/entity/entity.hpp>
#include <runtime/events/events.hpp>
#include <runtime/processor.hpp>
#include <runtime/session/session.hpp>
#include <snitch/snitch.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <thread>

namespace {
void ensure_db_cleanup(const std::string &path) {
  std::filesystem::remove_all(path);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

std::string get_unique_test_path(const std::string &base) {
  static std::atomic<int> counter{0};
  return base + "_" + std::to_string(counter.fetch_add(1)) + "_" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count());
}

auto create_test_logger() {
  auto logger = spdlog::get("processor_event_sub_test");
  if (!logger) {
    logger = spdlog::stdout_color_mt("processor_event_sub_test");
  }
  return logger;
}

class test_accessor_c : public runtime::runtime_accessor_if {
public:
  void raise_warning(const char *message) override {}
  void raise_error(const char *message) override {}
};

runtime::session_c *
create_test_session(runtime::events::event_system_c &event_system,
                    kvds::datastore_c &data_ds, runtime::entity_c *entity) {
  return new runtime::session_c("test_session", "test_entity", "test_scope",
                                *entity, &data_ds, &event_system);
}
} // namespace

TEST_CASE("core/event/sub with handler body executes on event",
          "[unit][runtime][processor]") {
  auto logger = create_test_logger();
  runtime::events::event_system_c event_system(logger.get(), 2, 100);

  auto accessor = std::make_shared<test_accessor_c>();
  event_system.initialize(accessor);

  kvds::datastore_c data_ds;
  std::string data_test_path =
      get_unique_test_path("/tmp/processor_test_event_sub_handler");
  ensure_db_cleanup(data_test_path);
  CHECK(data_ds.open(data_test_path));

  kvds::datastore_c entity_ds;
  std::string entity_test_path =
      get_unique_test_path("/tmp/processor_test_event_sub_handler_entity");
  ensure_db_cleanup(entity_test_path);
  CHECK(entity_ds.open(entity_test_path));

  record::record_manager_c entity_manager(entity_ds, logger);
  auto entity_opt = entity_manager.get_or_create<runtime::entity_c>("user1");
  REQUIRE(entity_opt.has_value());
  auto entity = std::move(entity_opt.value());
  entity->grant_permission("test_scope", runtime::permission::READ_WRITE);
  entity->grant_topic_permission(300, runtime::topic_permission::PUBSUB);
  entity->save();

  runtime::processor_c processor(logger.get(), event_system);
  runtime::session_c *session =
      create_test_session(event_system, data_ds, entity.get());

  SECTION("handler executes and can use $data binding") {
    runtime::execution_request_s sub_request{*session,
                                             R"((core/event/sub $CHANNEL_A 300 :str {
      (core/kv/set received_data $data)
      (core/util/log "Received event:" $data)
    }))",
                                             "sub_req"};

    runtime::events::event_s sub_event;
    sub_event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    sub_event.topic_identifier = 0;
    sub_event.payload = sub_request;

    processor.consume_event(sub_event);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto producer = event_system.get_event_producer_for_category(
        runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A);
    auto writer = producer->get_topic_writer_for_topic(300);

    runtime::events::event_s data_event;
    data_event.category =
        runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A;
    data_event.topic_identifier = 300;
    data_event.payload = std::string("\"test message\"");

    writer->write_event(data_event);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::string received;
    CHECK(session->get_store()->get("received_data", received));
    CHECK(received == "test message");
  }

  SECTION("handler with multiple statements executes in order") {
    runtime::execution_request_s sub_request{*session,
                                             R"((core/event/sub $CHANNEL_A 300 :str {
      (core/kv/set step1 "first")
      (core/kv/set step2 "second")
      (core/kv/set data_copy $data)
    }))",
                                             "multi_req"};

    runtime::events::event_s sub_event;
    sub_event.category =
        runtime::events::event_category_e::RUNTIME_EXECUTION_REQUEST;
    sub_event.topic_identifier = 0;
    sub_event.payload = sub_request;

    processor.consume_event(sub_event);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto producer = event_system.get_event_producer_for_category(
        runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A);
    auto writer = producer->get_topic_writer_for_topic(300);

    runtime::events::event_s data_event;
    data_event.category =
        runtime::events::event_category_e::RUNTIME_BACKCHANNEL_A;
    data_event.topic_identifier = 300;
    data_event.payload = std::string("\"event data\"");

    writer->write_event(data_event);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::string step1, step2, data_copy;
    CHECK(session->get_store()->get("step1", step1));
    CHECK(step1 == "first");
    CHECK(session->get_store()->get("step2", step2));
    CHECK(step2 == "second");
    CHECK(session->get_store()->get("data_copy", data_copy));
    CHECK(data_copy == "event data");
  }

  delete session;
  event_system.shutdown();
  ensure_db_cleanup(data_test_path);
  ensure_db_cleanup(entity_test_path);
}
