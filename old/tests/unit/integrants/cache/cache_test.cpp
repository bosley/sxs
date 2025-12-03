#include <atomic>
#include <cache/cache.hpp>
#include <snitch/snitch.hpp>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace pkg::cache;

TEST_CASE("cache basic construction", "[unit][cache][construction]") {
  SECTION("integer cache") {
    cache_c<int, 10> cache(0);
    CHECK(cache.size() == 10);
    for (std::size_t i = 0; i < cache.size(); ++i) {
      CHECK(cache[i] == 0);
    }
  }

  SECTION("string cache") {
    cache_c<std::string, 5> cache("default");
    CHECK(cache.size() == 5);
    for (std::size_t i = 0; i < cache.size(); ++i) {
      CHECK(cache[i] == "default");
    }
  }

  SECTION("double cache") {
    cache_c<double, 8> cache(3.14);
    CHECK(cache.size() == 8);
    for (std::size_t i = 0; i < cache.size(); ++i) {
      CHECK(cache[i] == 3.14);
    }
  }

  SECTION("size 1 cache") {
    cache_c<int, 1> cache(42);
    CHECK(cache.size() == 1);
    CHECK(cache[0] == 42);
  }

  SECTION("large cache") {
    cache_c<int, 1000> cache(99);
    CHECK(cache.size() == 1000);
    CHECK(cache[0] == 99);
    CHECK(cache[500] == 99);
    CHECK(cache[999] == 99);
  }
}

TEST_CASE("cache indexing operations", "[unit][cache][indexing]") {
  SECTION("valid indexing") {
    cache_c<int, 10> cache(0);
    cache[0] = 10;
    cache[5] = 50;
    cache[9] = 90;

    CHECK(cache[0] == 10);
    CHECK(cache[5] == 50);
    CHECK(cache[9] == 90);
  }

  SECTION("out of bounds throws") {
    cache_c<int, 5> cache(0);
    CHECK_THROWS_AS(cache[5], std::out_of_range);
    CHECK_THROWS_AS(cache[10], std::out_of_range);
    CHECK_THROWS_AS(cache[100], std::out_of_range);
  }

  SECTION("modify through index") {
    cache_c<std::string, 3> cache("init");
    cache[0] = "first";
    cache[1] = "second";
    cache[2] = "third";

    CHECK(cache[0] == "first");
    CHECK(cache[1] == "second");
    CHECK(cache[2] == "third");
  }

  SECTION("increment through index") {
    cache_c<int, 5> cache(0);
    for (std::size_t i = 0; i < 5; ++i) {
      cache[i] = static_cast<int>(i * 10);
    }

    for (std::size_t i = 0; i < 5; ++i) {
      CHECK(cache[i] == static_cast<int>(i * 10));
    }
  }
}

TEST_CASE("cache const operations", "[unit][cache][const]") {
  SECTION("const indexing") {
    cache_c<int, 5> cache(42);
    const auto &const_cache = cache;

    CHECK(const_cache[0] == 42);
    CHECK(const_cache[4] == 42);
  }

  SECTION("const out of bounds throws") {
    cache_c<int, 5> cache(0);
    const auto &const_cache = cache;
    CHECK_THROWS_AS(const_cache[5], std::out_of_range);
  }

  SECTION("const size") {
    cache_c<int, 7> cache(0);
    const auto &const_cache = cache;
    CHECK(const_cache.size() == 7);
  }
}

TEST_CASE("cache iterators", "[unit][cache][iterator]") {
  SECTION("iterate over cache") {
    cache_c<int, 5> cache(0);
    for (std::size_t i = 0; i < 5; ++i) {
      cache[i] = static_cast<int>(i);
    }

    int expected = 0;
    for (int value : cache) {
      CHECK(value == expected);
      ++expected;
    }
    CHECK(expected == 5);
  }

  SECTION("modify through iterator") {
    cache_c<int, 3> cache(0);
    int counter = 100;
    for (int &value : cache) {
      value = counter++;
    }

    CHECK(cache[0] == 100);
    CHECK(cache[1] == 101);
    CHECK(cache[2] == 102);
  }

  SECTION("const iterators") {
    cache_c<int, 4> cache(5);
    const auto &const_cache = cache;

    int count = 0;
    for (const int &value : const_cache) {
      CHECK(value == 5);
      ++count;
    }
    CHECK(count == 4);
  }

  SECTION("begin and end pointers") {
    cache_c<int, 5> cache(0);
    auto *begin_ptr = cache.begin();
    auto *end_ptr = cache.end();

    CHECK(end_ptr - begin_ptr == 5);
  }
}

TEST_CASE("cache range creation", "[unit][cache][range]") {
  SECTION("valid range at start") {
    cache_c<int, 10> cache(0);
    auto r = cache.range<5>(0);
    CHECK(r.size() == 5);
  }

  SECTION("valid range at middle") {
    cache_c<int, 10> cache(0);
    auto r = cache.range<3>(5);
    CHECK(r.size() == 3);
  }

  SECTION("valid range at end") {
    cache_c<int, 10> cache(0);
    auto r = cache.range<2>(8);
    CHECK(r.size() == 2);
  }

  SECTION("full cache range") {
    cache_c<int, 5> cache(0);
    auto r = cache.range<5>(0);
    CHECK(r.size() == 5);
  }

  SECTION("single element range") {
    cache_c<int, 10> cache(0);
    auto r = cache.range<1>(7);
    CHECK(r.size() == 1);
  }

  SECTION("range exceeds cache bounds throws") {
    cache_c<int, 10> cache(0);
    CHECK_THROWS_AS((cache.range<5>(6)), std::out_of_range);
    CHECK_THROWS_AS((cache.range<3>(8)), std::out_of_range);
  }

  SECTION("default start index") {
    cache_c<int, 10> cache(0);
    auto r = cache.range<5>();
    CHECK(r.size() == 5);
  }
}

TEST_CASE("cache range operations", "[unit][cache][range]") {
  SECTION("access through range") {
    cache_c<int, 10> cache(0);
    for (std::size_t i = 0; i < 10; ++i) {
      cache[i] = static_cast<int>(i * 10);
    }

    auto r = cache.range<5>(2);
    CHECK(r[0] == 20);
    CHECK(r[1] == 30);
    CHECK(r[2] == 40);
    CHECK(r[3] == 50);
    CHECK(r[4] == 60);
  }

  SECTION("modify through range affects cache") {
    cache_c<int, 10> cache(0);
    auto r = cache.range<3>(5);
    r[0] = 100;
    r[1] = 200;
    r[2] = 300;

    CHECK(cache[5] == 100);
    CHECK(cache[6] == 200);
    CHECK(cache[7] == 300);
  }

  SECTION("range out of bounds throws") {
    cache_c<int, 10> cache(0);
    auto r = cache.range<5>(0);
    CHECK_THROWS_AS(r[5], std::out_of_range);
    CHECK_THROWS_AS(r[10], std::out_of_range);
  }

  SECTION("range with strings") {
    cache_c<std::string, 10> cache("default");
    auto r = cache.range<3>(2);
    r[0] = "first";
    r[1] = "second";
    r[2] = "third";

    CHECK(cache[2] == "first");
    CHECK(cache[3] == "second");
    CHECK(cache[4] == "third");
  }

  SECTION("multiple ranges on same cache") {
    cache_c<int, 10> cache(0);
    auto r1 = cache.range<3>(0);
    auto r2 = cache.range<3>(5);

    r1[0] = 10;
    r2[0] = 50;

    CHECK(cache[0] == 10);
    CHECK(cache[5] == 50);
  }

  SECTION("overlapping ranges") {
    cache_c<int, 10> cache(0);
    auto r1 = cache.range<5>(0);
    auto r2 = cache.range<5>(3);

    r1[3] = 100;
    CHECK(r2[0] == 100);

    r2[0] = 200;
    CHECK(r1[3] == 200);
  }
}

TEST_CASE("cache range iterators", "[unit][cache][range][iterator]") {
  SECTION("iterate over range") {
    cache_c<int, 10> cache(0);
    for (std::size_t i = 0; i < 10; ++i) {
      cache[i] = static_cast<int>(i);
    }

    auto r = cache.range<5>(3);
    int expected = 3;
    for (int *ptr : r) {
      CHECK(*ptr == expected);
      ++expected;
    }
    CHECK(expected == 8);
  }

  SECTION("modify through range iterator") {
    cache_c<int, 10> cache(0);
    auto r = cache.range<4>(2);

    int value = 100;
    for (int *ptr : r) {
      *ptr = value++;
    }

    CHECK(cache[2] == 100);
    CHECK(cache[3] == 101);
    CHECK(cache[4] == 102);
    CHECK(cache[5] == 103);
  }
}

TEST_CASE("cache range const operations", "[unit][cache][range][const]") {
  SECTION("const range access") {
    cache_c<int, 10> cache(42);
    const auto r = cache.range<5>(0);
    CHECK(r[0] == 42);
    CHECK(r[4] == 42);
  }

  SECTION("const range out of bounds") {
    cache_c<int, 10> cache(0);
    const auto r = cache.range<5>(0);
    CHECK_THROWS_AS(r[5], std::out_of_range);
  }

  SECTION("const range size") {
    cache_c<int, 10> cache(0);
    const auto r = cache.range<7>(0);
    CHECK(r.size() == 7);
  }
}

TEST_CASE("cache with complex types", "[unit][cache][complex]") {
  SECTION("cache of strings") {
    cache_c<std::string, 5> cache("test");
    cache[0] = "hello";
    cache[1] = "world";
    cache[2] = "foo";

    CHECK(cache[0] == "hello");
    CHECK(cache[1] == "world");
    CHECK(cache[2] == "foo");
    CHECK(cache[3] == "test");
  }

  SECTION("range with strings") {
    cache_c<std::string, 10> cache("init");
    auto r = cache.range<3>(2);
    r[0] = "a";
    r[1] = "b";
    r[2] = "c";

    CHECK(cache[2] == "a");
    CHECK(cache[3] == "b");
    CHECK(cache[4] == "c");
  }

  SECTION("cache of doubles") {
    cache_c<double, 5> cache(0.0);
    cache[0] = 1.1;
    cache[1] = 2.2;
    cache[2] = 3.3;

    auto r = cache.range<3>(0);
    CHECK(r[0] == 1.1);
    CHECK(r[1] == 2.2);
    CHECK(r[2] == 3.3);
  }
}

TEST_CASE("cache edge cases", "[unit][cache][edge]") {
  SECTION("single element cache") {
    cache_c<int, 1> cache(99);
    CHECK(cache.size() == 1);
    CHECK(cache[0] == 99);
    CHECK_THROWS_AS(cache[1], std::out_of_range);

    auto r = cache.range<1>(0);
    CHECK(r.size() == 1);
    CHECK(r[0] == 99);
  }

  SECTION("single element range on large cache") {
    cache_c<int, 100> cache(0);
    cache[50] = 42;
    auto r = cache.range<1>(50);
    CHECK(r[0] == 42);
  }

  SECTION("range at exact boundary") {
    cache_c<int, 10> cache(0);
    cache[9] = 999;
    auto r = cache.range<1>(9);
    CHECK(r[0] == 999);
  }

  SECTION("zero value initialization") {
    cache_c<int, 5> cache(0);
    for (std::size_t i = 0; i < 5; ++i) {
      CHECK(cache[i] == 0);
    }
  }

  SECTION("negative values") {
    cache_c<int, 5> cache(-1);
    for (std::size_t i = 0; i < 5; ++i) {
      CHECK(cache[i] == -1);
    }

    cache[2] = -999;
    CHECK(cache[2] == -999);
  }
}

TEST_CASE("cache range equality", "[unit][cache][range][equality]") {
  SECTION("ranges are equal to themselves") {
    cache_c<int, 10> cache(0);
    auto r1 = cache.range<5>(0);
    CHECK(r1 == r1);
  }
}

TEST_CASE("cache boundary validation", "[unit][cache][boundary]") {
  SECTION("access at boundaries") {
    cache_c<int, 10> cache(5);
    CHECK(cache[0] == 5);
    CHECK(cache[9] == 5);
    CHECK_THROWS_AS(cache[10], std::out_of_range);
  }

  SECTION("range at start boundary") {
    cache_c<int, 10> cache(0);
    auto r = cache.range<1>(0);
    CHECK(r[0] == 0);
  }

  SECTION("range at end boundary") {
    cache_c<int, 10> cache(0);
    auto r = cache.range<1>(9);
    CHECK(r.size() == 1);
  }

  SECTION("range exceeding by one") {
    cache_c<int, 10> cache(0);
    CHECK_THROWS_AS((cache.range<1>(10)), std::out_of_range);
  }
}

TEST_CASE("cache multiple ranges interactions", "[unit][cache][range][multi]") {
  SECTION("independent ranges") {
    cache_c<int, 20> cache(0);
    auto r1 = cache.range<5>(0);
    auto r2 = cache.range<5>(10);

    for (std::size_t i = 0; i < 5; ++i) {
      r1[i] = static_cast<int>(i);
      r2[i] = static_cast<int>(i + 100);
    }

    CHECK(cache[0] == 0);
    CHECK(cache[4] == 4);
    CHECK(cache[10] == 100);
    CHECK(cache[14] == 104);
  }

  SECTION("consecutive ranges") {
    cache_c<int, 10> cache(0);
    auto r1 = cache.range<3>(0);
    auto r2 = cache.range<3>(3);
    auto r3 = cache.range<4>(6);

    r1[0] = 1;
    r2[0] = 2;
    r3[0] = 3;

    CHECK(cache[0] == 1);
    CHECK(cache[3] == 2);
    CHECK(cache[6] == 3);
  }
}

TEST_CASE("cache stress tests", "[unit][cache][stress]") {
  SECTION("large cache with many operations") {
    cache_c<int, 1000> cache(0);

    for (std::size_t i = 0; i < 1000; ++i) {
      cache[i] = static_cast<int>(i);
    }

    for (std::size_t i = 0; i < 1000; ++i) {
      CHECK(cache[i] == static_cast<int>(i));
    }
  }

  SECTION("many small ranges") {
    cache_c<int, 100> cache(0);

    for (std::size_t i = 0; i < 100; i += 10) {
      auto r = cache.range<10>(i);
      for (std::size_t j = 0; j < 10; ++j) {
        r[j] = static_cast<int>(i + j);
      }
    }

    for (std::size_t i = 0; i < 100; ++i) {
      CHECK(cache[i] == static_cast<int>(i));
    }
  }

  SECTION("alternating access patterns") {
    cache_c<int, 50> cache(0);

    for (std::size_t i = 0; i < 50; i += 2) {
      cache[i] = static_cast<int>(i);
    }

    auto r = cache.range<25>(0);
    for (std::size_t i = 0; i < 25; i += 2) {
      CHECK(r[i] == static_cast<int>(i));
    }
    for (std::size_t i = 1; i < 25; i += 2) {
      CHECK(r[i] == 0);
    }
  }
}

TEST_CASE("cache thread safety - concurrent writes", "[unit][cache][thread]") {
  SECTION("multiple threads writing to different indices") {
    cache_c<int, 100> cache(0);
    std::vector<std::thread> threads;

    for (int t = 0; t < 10; ++t) {
      threads.emplace_back([&cache, t]() {
        for (int i = 0; i < 10; ++i) {
          std::size_t idx = t * 10 + i;
          cache[idx] = static_cast<int>(idx);
        }
      });
    }

    for (auto &thread : threads) {
      thread.join();
    }

    for (std::size_t i = 0; i < 100; ++i) {
      CHECK(cache[i] == static_cast<int>(i));
    }
  }

  SECTION("multiple threads writing to overlapping indices") {
    cache_c<int, 50> cache(0);
    std::atomic<int> completed{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < 5; ++t) {
      threads.emplace_back([&cache, &completed, t]() {
        for (int i = 0; i < 100; ++i) {
          cache[t] = t + 1;
        }
        completed.fetch_add(1);
      });
    }

    for (auto &thread : threads) {
      thread.join();
    }

    CHECK(completed.load() == 5);
    for (int t = 0; t < 5; ++t) {
      CHECK(cache[t] >= 1);
      CHECK(cache[t] <= 6);
    }
  }
}

TEST_CASE("cache thread safety - concurrent reads", "[unit][cache][thread]") {
  SECTION("multiple threads reading simultaneously") {
    cache_c<int, 100> cache(42);
    std::atomic<int> read_count{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < 10; ++t) {
      threads.emplace_back([&cache, &read_count]() {
        for (std::size_t i = 0; i < 100; ++i) {
          int val = cache[i];
          if (val == 42) {
            read_count.fetch_add(1);
          }
        }
      });
    }

    for (auto &thread : threads) {
      thread.join();
    }

    CHECK(read_count.load() == 1000);
  }
}

TEST_CASE("cache thread safety - mixed operations", "[unit][cache][thread]") {
  SECTION("concurrent reads and writes") {
    cache_c<int, 100> cache(0);
    std::atomic<bool> start{false};
    std::atomic<int> valid_reads{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < 100; ++i) {
      cache[i] = i;
    }

    for (int t = 0; t < 5; ++t) {
      threads.emplace_back([&cache, &start, t]() {
        while (!start.load()) {
        }
        for (int i = 0; i < 100; ++i) {
          cache[t * 20 + (i % 20)] = t * 1000 + i;
        }
      });
    }

    for (int t = 0; t < 5; ++t) {
      threads.emplace_back([&cache, &start, &valid_reads]() {
        while (!start.load()) {
        }
        int sum = 0;
        for (std::size_t i = 0; i < 100; ++i) {
          sum += cache[i];
        }
        if (sum >= 0) {
          valid_reads.fetch_add(1);
        }
      });
    }

    start.store(true);

    for (auto &thread : threads) {
      thread.join();
    }

    CHECK(valid_reads.load() == 5);
  }
}

TEST_CASE("cache thread safety - range concurrent access",
          "[unit][cache][thread]") {
  SECTION("concurrent range operations") {
    cache_c<int, 100> cache(0);
    std::vector<std::thread> threads;

    for (int t = 0; t < 5; ++t) {
      threads.emplace_back([&cache, t]() {
        auto r = cache.range<20>(t * 20);
        for (std::size_t i = 0; i < 20; ++i) {
          r[i] = t * 100 + static_cast<int>(i);
        }
      });
    }

    for (auto &thread : threads) {
      thread.join();
    }

    for (int t = 0; t < 5; ++t) {
      auto r = cache.range<20>(t * 20);
      for (std::size_t i = 0; i < 20; ++i) {
        CHECK(r[i] == t * 100 + static_cast<int>(i));
      }
    }
  }

  SECTION("overlapping range concurrent access") {
    cache_c<int, 50> cache(0);
    std::atomic<int> completed{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < 4; ++t) {
      threads.emplace_back([&cache, &completed, t]() {
        auto r = cache.range<20>(t * 10);
        for (int i = 0; i < 50; ++i) {
          for (std::size_t j = 0; j < 20; ++j) {
            r[j] = t;
          }
        }
        completed.fetch_add(1);
      });
    }

    for (auto &thread : threads) {
      thread.join();
    }

    CHECK(completed.load() == 4);
  }
}

TEST_CASE("cache thread safety - stress test", "[unit][cache][thread]") {
  SECTION("heavy concurrent load") {
    cache_c<int, 1000> cache(0);
    std::atomic<int> operations{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < 20; ++t) {
      threads.emplace_back([&cache, &operations, t]() {
        int local_ops = 0;
        for (int i = 0; i < 1000; ++i) {
          std::size_t idx = (t * 50 + (i % 50)) % 1000;
          cache[idx] = t;
          int val = cache[idx];
          if (val >= 0) {
            local_ops++;
          }
        }
        operations.fetch_add(local_ops);
      });
    }

    for (auto &thread : threads) {
      thread.join();
    }

    CHECK(operations.load() == 20000);
  }
}

TEST_CASE("cache thread safety - with_lock helper", "[unit][cache][thread]") {
  SECTION("bulk operations with with_lock") {
    cache_c<int, 100> cache(0);
    std::vector<std::thread> threads;

    for (int t = 0; t < 5; ++t) {
      threads.emplace_back([&cache, t]() {
        cache.with_lock([t](std::array<int, 100> &data) {
          for (std::size_t i = t * 20; i < (t + 1) * 20; ++i) {
            data[i] = t * 100 + static_cast<int>(i);
          }
        });
      });
    }

    for (auto &thread : threads) {
      thread.join();
    }

    for (int t = 0; t < 5; ++t) {
      for (std::size_t i = t * 20; i < (t + 1) * 20; ++i) {
        CHECK(cache[i] == t * 100 + static_cast<int>(i));
      }
    }
  }

  SECTION("read-only operations with with_lock") {
    cache_c<int, 100> cache(5);
    std::atomic<int> sum{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < 10; ++t) {
      threads.emplace_back([&cache, &sum]() {
        cache.with_lock([&sum](const std::array<int, 100> &data) {
          int local_sum = 0;
          for (const auto &val : data) {
            local_sum += val;
          }
          sum.fetch_add(local_sum);
        });
      });
    }

    for (auto &thread : threads) {
      thread.join();
    }

    CHECK(sum.load() == 5000);
  }
}
