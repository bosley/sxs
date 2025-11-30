#include <cassert>
#include <cstdio>
#include <kernel_api.hpp>
#include <slp/slp.hpp>

static const struct pkg::kernel::api_table_s *g_test_api = nullptr;

static slp::slp_object_c test_eval(pkg::kernel::context_t ctx,
                                   const slp::slp_object_c &obj) {
  auto list = obj.as_list();
  if (list.size() > 0) {
    return list.at(0);
  }
  return slp::slp_object_c::create_none();
}

static void test_register_function(pkg::kernel::registry_t registry,
                                   const char *name,
                                   pkg::kernel::kernel_fn_t function,
                                   slp::slp_type_e return_type, int variadic) {
  printf("Registered function: %s\n", name);
}

extern "C" void kernel_init(pkg::kernel::registry_t registry,
                            const struct pkg::kernel::api_table_s *api);

void test_basic_operations() {
  printf("\n=== Testing Basic Operations ===\n");

  auto list = slp::slp_object_c::create_int(5);
  printf("Created test integer: 5\n");
  assert(list.type() == slp::slp_type_e::INTEGER);
  assert(list.as_int() == 5);

  auto str = slp::slp_object_c::create_string("hello");
  printf("Created test string: hello\n");
  assert(str.type() == slp::slp_type_e::DQ_LIST);
  assert(str.as_string().to_string() == "hello");

  slp::slp_object_c items[3] = {slp::slp_object_c::create_int(1),
                                slp::slp_object_c::create_int(2),
                                slp::slp_object_c::create_int(3)};
  auto paren = slp::slp_object_c::create_paren_list(items, 3);
  printf("Created test paren list (1 2 3)\n");
  assert(paren.type() == slp::slp_type_e::PAREN_LIST);
  assert(paren.as_list().size() == 3);

  printf("Basic operations test: PASSED\n");
}

void test_list_types() {
  printf("\n=== Testing List Types ===\n");

  slp::slp_object_c items[2] = {slp::slp_object_c::create_int(1),
                                slp::slp_object_c::create_int(2)};

  auto paren = slp::slp_object_c::create_paren_list(items, 2);
  assert(paren.type() == slp::slp_type_e::PAREN_LIST);
  printf("Paren list type: OK\n");

  auto bracket = slp::slp_object_c::create_bracket_list(items, 2);
  assert(bracket.type() == slp::slp_type_e::BRACKET_LIST);
  printf("Bracket list type: OK\n");

  auto brace = slp::slp_object_c::create_brace_list(items, 2);
  assert(brace.type() == slp::slp_type_e::BRACE_LIST);
  printf("Brace list type: OK\n");

  printf("List types test: PASSED\n");
}

void test_object_equality() {
  printf("\n=== Testing Object Equality ===\n");

  auto i1 = slp::slp_object_c::create_int(42);
  auto i2 = slp::slp_object_c::create_int(42);
  auto i3 = slp::slp_object_c::create_int(43);

  assert(i1.type() == i2.type());
  assert(i1.as_int() == i2.as_int());
  assert(i1.as_int() != i3.as_int());
  printf("Integer equality: OK\n");

  auto r1 = slp::slp_object_c::create_real(3.14);
  auto r2 = slp::slp_object_c::create_real(3.14);
  auto r3 = slp::slp_object_c::create_real(2.71);

  assert(r1.type() == r2.type());
  assert(r1.as_real() == r2.as_real());
  assert(r1.as_real() != r3.as_real());
  printf("Real equality: OK\n");

  auto s1 = slp::slp_object_c::create_string("test");
  auto s2 = slp::slp_object_c::create_string("test");
  auto s3 = slp::slp_object_c::create_string("other");

  assert(s1.type() == s2.type());
  assert(s1.as_string().to_string() == s2.as_string().to_string());
  assert(s1.as_string().to_string() != s3.as_string().to_string());
  printf("String equality: OK\n");

  printf("Object equality test: PASSED\n");
}

void test_empty_list_operations() {
  printf("\n=== Testing Empty List Operations ===\n");

  slp::slp_object_c empty_items[0];
  auto empty = slp::slp_object_c::create_paren_list(empty_items, 0);

  assert(empty.type() == slp::slp_type_e::PAREN_LIST);
  assert(empty.as_list().size() == 0);
  assert(empty.as_list().empty());
  printf("Empty list creation: OK\n");

  printf("Empty list operations test: PASSED\n");
}

void test_kernel_init() {
  printf("\n=== Testing Kernel Initialization ===\n");

  pkg::kernel::api_table_s test_api;
  test_api.register_function = test_register_function;
  test_api.eval = test_eval;

  kernel_init(nullptr, &test_api);

  printf("Kernel initialization test: PASSED\n");
}

int main() {
  printf("===========================================\n");
  printf("    Forge Kernel Unit Tests\n");
  printf("===========================================\n");

  test_basic_operations();
  test_list_types();
  test_object_equality();
  test_empty_list_operations();
  test_kernel_init();

  printf("\n===========================================\n");
  printf("    All Unit Tests PASSED\n");
  printf("===========================================\n");

  return 0;
}
