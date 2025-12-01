#include "alu.hpp"

static const struct pkg::kernel::api_table_s *g_api = nullptr;

static slp::slp_object_c alu_add(pkg::kernel::context_t ctx,
                                 const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_int(0);
  }

  auto a = g_api->eval(ctx, list.at(1)).as_int();
  auto b = g_api->eval(ctx, list.at(2)).as_int();

  return slp::slp_object_c::create_int(a + b);
}

static slp::slp_object_c alu_sub(pkg::kernel::context_t ctx,
                                 const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_int(0);
  }

  auto a = g_api->eval(ctx, list.at(1)).as_int();
  auto b = g_api->eval(ctx, list.at(2)).as_int();

  return slp::slp_object_c::create_int(a - b);
}

static slp::slp_object_c alu_mul(pkg::kernel::context_t ctx,
                                 const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_int(0);
  }

  auto a = g_api->eval(ctx, list.at(1)).as_int();
  auto b = g_api->eval(ctx, list.at(2)).as_int();

  return slp::slp_object_c::create_int(a * b);
}

static slp::slp_object_c alu_div(pkg::kernel::context_t ctx,
                                 const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_int(0);
  }

  auto a = g_api->eval(ctx, list.at(1)).as_int();
  auto b = g_api->eval(ctx, list.at(2)).as_int();

  if (b == 0) {
    return slp::slp_object_c::create_int(0);
  }

  return slp::slp_object_c::create_int(a / b);
}

static slp::slp_object_c alu_mod(pkg::kernel::context_t ctx,
                                 const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_int(0);
  }

  auto a = g_api->eval(ctx, list.at(1)).as_int();
  auto b = g_api->eval(ctx, list.at(2)).as_int();

  if (b == 0) {
    return slp::slp_object_c::create_int(0);
  }

  return slp::slp_object_c::create_int(a % b);
}

static slp::slp_object_c alu_add_r(pkg::kernel::context_t ctx,
                                   const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_real(0.0);
  }

  auto a = g_api->eval(ctx, list.at(1)).as_real();
  auto b = g_api->eval(ctx, list.at(2)).as_real();

  return slp::slp_object_c::create_real(a + b);
}

static slp::slp_object_c alu_sub_r(pkg::kernel::context_t ctx,
                                   const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_real(0.0);
  }

  auto a = g_api->eval(ctx, list.at(1)).as_real();
  auto b = g_api->eval(ctx, list.at(2)).as_real();

  return slp::slp_object_c::create_real(a - b);
}

static slp::slp_object_c alu_mul_r(pkg::kernel::context_t ctx,
                                   const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_real(0.0);
  }

  auto a = g_api->eval(ctx, list.at(1)).as_real();
  auto b = g_api->eval(ctx, list.at(2)).as_real();

  return slp::slp_object_c::create_real(a * b);
}

static slp::slp_object_c alu_div_r(pkg::kernel::context_t ctx,
                                   const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_real(0.0);
  }

  auto a = g_api->eval(ctx, list.at(1)).as_real();
  auto b = g_api->eval(ctx, list.at(2)).as_real();

  if (b == 0.0) {
    return slp::slp_object_c::create_real(0.0);
  }

  return slp::slp_object_c::create_real(a / b);
}

static slp::slp_object_c alu_eq(pkg::kernel::context_t ctx,
                                const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_int(0);
  }

  auto a = g_api->eval(ctx, list.at(1)).as_int();
  auto b = g_api->eval(ctx, list.at(2)).as_int();

  return slp::slp_object_c::create_int(a == b ? 1 : 0);
}

static slp::slp_object_c alu_eq_r(pkg::kernel::context_t ctx,
                                  const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_int(0);
  }

  auto a = g_api->eval(ctx, list.at(1)).as_real();
  auto b = g_api->eval(ctx, list.at(2)).as_real();

  return slp::slp_object_c::create_int(a == b ? 1 : 0);
}

extern "C" void kernel_init(pkg::kernel::registry_t registry,
                            const struct pkg::kernel::api_table_s *api) {
  g_api = api;
  api->register_function(registry, "add", alu_add, slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "sub", alu_sub, slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "mul", alu_mul, slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "div", alu_div, slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "mod", alu_mod, slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "add_r", alu_add_r, slp::slp_type_e::REAL,
                         0);
  api->register_function(registry, "sub_r", alu_sub_r, slp::slp_type_e::REAL,
                         0);
  api->register_function(registry, "mul_r", alu_mul_r, slp::slp_type_e::REAL,
                         0);
  api->register_function(registry, "div_r", alu_div_r, slp::slp_type_e::REAL,
                         0);
  api->register_function(registry, "eq", alu_eq, slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "eq_r", alu_eq_r, slp::slp_type_e::INTEGER,
                         0);
}
