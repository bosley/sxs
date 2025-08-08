#include <iostream>

#include "bytes/bytes.hpp"
#include "types/lifetime.hpp"
#include "types/monotonic_counter.hpp"
#include "types/shared_obj.hpp"
#include "types/view.hpp"

void views() {

  if (EXTRA_DEBUG) {
    std::cout << "EXTRA_DEBUG is enabled" << std::endl;
  }

  std::cout << __func__ << std::endl;

  uint16_t *x = new uint16_t[10]();

  x[5] = 99;
  x[9] = 1;

  pkg::types::view_c<uint16_t> v(x, 10);

  auto iter = v.iter();

  while (iter->has_next()) {
    auto *xv = iter->next();
    std::cout << "cur=" << *xv << std::endl;
  }

  delete[] x;
}

void shared() {

  std::cout << __func__ << std::endl;

  class some_object_c : public pkg::types::shared_c {
  public:
    some_object_c(const std::size_t id) : id(id) {
      std::cout << id << " has been created" << std::endl;
    }
    ~some_object_c() { std::cout << id << " has been deleted" << std::endl; }
    const std::size_t id{0};
  };

  using so_ptr = pkg::types::shared_obj_c<some_object_c>;

  {
    // Should be freed after this anonymous scope
    auto ptr = pkg::types::make_shared<some_object_c>(42);
  }

  std::vector<so_ptr> created;

  for (auto x = 0; x < 10; x++) {
    created.emplace_back(pkg::types::make_shared<some_object_c>(x + 10));
  }

  std::cout << "Leaving " << __func__ << std::endl;
}

void lifetime() {
  class cb_c final : public pkg::types::lifetime_c::observer_if {
  public:
    void death_ind() override { std::cout << "anonymous lifetime end\n"; }
  };

  class tagged_cb_c final : public pkg::types::lifetime_tagged_c::observer_if {
  public:
    void death_ind(const std::size_t tag) override {
      std::cout << "tagged lifetime end: " << tag << "\n";
    }
  };

  class a_c final : public pkg::types::lifetime_c {
  public:
    a_c() = delete;
    a_c(pkg::types::lifetime_c::observer_if &obs)
        : pkg::types::lifetime_c(obs) {
      std::cout << "Created anonymous lifetime\n";
    }
  };

  class b_c final : public pkg::types::lifetime_tagged_c {
  public:
    b_c() = delete;
    b_c(pkg::types::lifetime_tagged_c::observer_if &obs, const std::size_t id)
        : pkg::types::lifetime_tagged_c(obs, id) {
      std::cout << "Created tagged lifetime: " << id << "\n";
    }
  };

  cb_c cba;
  tagged_cb_c cbb;

  a_c a(cba);
  b_c b(cbb, 0);
  b_c c(cbb, 1);
  b_c d(cbb, 2);
  b_c e(cbb, 3);

  std::cout << "Leaving " << __func__ << std::endl;
}

void byte_tool_stuff() {

  std::string x = "cuppa";
  std::vector<uint8_t> dest;

  // Specify the <type> as the encoding for the string length in the vector
  pkg::bytes::pack_string_into<uint32_t>(x, dest);

  std::cout << x << x.size() << " , " << dest.size() << std::endl;
}

void counter() {
  pkg::types::monotonic_counter_c<uint8_t, 1> counter(250);
  for (uint16_t i = 250; i < std::numeric_limits<uint8_t>::max() + 5; i++) {
    std::cout << (int)counter.next() << std::endl;
  }
}

int main(int argc, char **argv) {

  std::cout << "Basic example...\n";
  views();
  shared();
  lifetime();
  byte_tool_stuff();
  counter();

  return 0;
}