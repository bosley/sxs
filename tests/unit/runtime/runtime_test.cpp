#include <atomic>
#include <chrono>
#include <filesystem>
#include <runtime/runtime.hpp>
#include <snitch/snitch.hpp>
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
} // namespace

TEST_CASE("runtime basic operations", "[unit][runtime]") {
  std::string test_path = get_unique_test_path("/tmp/runtime_test_basic");
  ensure_db_cleanup(test_path);

  runtime::options_s options;
  options.runtime_root_path = test_path;
  runtime::runtime_c rt(options);

  SECTION("initialize and shutdown") {
    CHECK(rt.initialize());
    CHECK(rt.is_running());
    CHECK_FALSE(rt.initialize());
    CHECK(rt.shutdown());
    CHECK_FALSE(rt.is_running());
    CHECK_FALSE(rt.shutdown());
  }

  ensure_db_cleanup(test_path);
}
