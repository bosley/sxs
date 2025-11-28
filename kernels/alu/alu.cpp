#include "alu.hpp"

// the kernel call api ensures the length and type of the parameters are correct so we just have to do the work

static const struct sxs_api_table_t *g_api = nullptr;

static sxs_object_t alu_add(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  size_t size = g_api->list_size(list);

  if (size < 3) {
    return g_api->create_int(0);
  }

  sxs_object_t a_obj = g_api->list_at(list, 1);
  sxs_object_t b_obj = g_api->list_at(list, 2);

  sxs_object_t evaled_a = g_api->eval(ctx, a_obj);
  sxs_object_t evaled_b = g_api->eval(ctx, b_obj);

  long long a = g_api->as_int(evaled_a);
  long long b = g_api->as_int(evaled_b);

  return g_api->create_int(a + b);
}

static sxs_object_t alu_sub(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  size_t size = g_api->list_size(list);

  if (size < 3) {
    return g_api->create_int(0);
  }

  sxs_object_t a_obj = g_api->list_at(list, 1);
  sxs_object_t b_obj = g_api->list_at(list, 2);

  sxs_object_t evaled_a = g_api->eval(ctx, a_obj);
  sxs_object_t evaled_b = g_api->eval(ctx, b_obj);

  long long a = g_api->as_int(evaled_a);
  long long b = g_api->as_int(evaled_b);

  return g_api->create_int(a - b);
}

static sxs_object_t alu_mul(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  size_t size = g_api->list_size(list);

  if (size < 3) {
    return g_api->create_int(0);
  }

  sxs_object_t a_obj = g_api->list_at(list, 1);
  sxs_object_t b_obj = g_api->list_at(list, 2);

  sxs_object_t evaled_a = g_api->eval(ctx, a_obj);
  sxs_object_t evaled_b = g_api->eval(ctx, b_obj);

  long long a = g_api->as_int(evaled_a);
  long long b = g_api->as_int(evaled_b);

  return g_api->create_int(a * b);
}

static sxs_object_t alu_div(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  size_t size = g_api->list_size(list);

  if (size < 3) {
    return g_api->create_int(0);
  }

  sxs_object_t a_obj = g_api->list_at(list, 1);
  sxs_object_t b_obj = g_api->list_at(list, 2);

  sxs_object_t evaled_a = g_api->eval(ctx, a_obj);
  sxs_object_t evaled_b = g_api->eval(ctx, b_obj);

  long long a = g_api->as_int(evaled_a);
  long long b = g_api->as_int(evaled_b);

  if (b == 0) {
    return g_api->create_int(0);
  }

  return g_api->create_int(a / b);
}

static sxs_object_t alu_mod(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  size_t size = g_api->list_size(list);

  if (size < 3) {
    return g_api->create_int(0);
  }

  sxs_object_t a_obj = g_api->list_at(list, 1);
  sxs_object_t b_obj = g_api->list_at(list, 2);

  sxs_object_t evaled_a = g_api->eval(ctx, a_obj);
  sxs_object_t evaled_b = g_api->eval(ctx, b_obj);

  long long a = g_api->as_int(evaled_a);
  long long b = g_api->as_int(evaled_b);

  if (b == 0) {
    return g_api->create_int(0);
  }

  return g_api->create_int(a % b);
}

static sxs_object_t alu_add_r(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  size_t size = g_api->list_size(list);

  if (size < 3) {
    return g_api->create_real(0.0);
  }

  sxs_object_t a_obj = g_api->list_at(list, 1);
  sxs_object_t b_obj = g_api->list_at(list, 2);

  sxs_object_t evaled_a = g_api->eval(ctx, a_obj);
  sxs_object_t evaled_b = g_api->eval(ctx, b_obj);

  double a = g_api->as_real(evaled_a);
  double b = g_api->as_real(evaled_b);

  return g_api->create_real(a + b);
}

static sxs_object_t alu_sub_r(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  size_t size = g_api->list_size(list);

  if (size < 3) {
    return g_api->create_real(0.0);
  }

  sxs_object_t a_obj = g_api->list_at(list, 1);
  sxs_object_t b_obj = g_api->list_at(list, 2);

  sxs_object_t evaled_a = g_api->eval(ctx, a_obj);
  sxs_object_t evaled_b = g_api->eval(ctx, b_obj);

  double a = g_api->as_real(evaled_a);
  double b = g_api->as_real(evaled_b);

  return g_api->create_real(a - b);
}

static sxs_object_t alu_mul_r(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  size_t size = g_api->list_size(list);

  if (size < 3) {
    return g_api->create_real(0.0);
  }

  sxs_object_t a_obj = g_api->list_at(list, 1);
  sxs_object_t b_obj = g_api->list_at(list, 2);

  sxs_object_t evaled_a = g_api->eval(ctx, a_obj);
  sxs_object_t evaled_b = g_api->eval(ctx, b_obj);

  double a = g_api->as_real(evaled_a);
  double b = g_api->as_real(evaled_b);

  return g_api->create_real(a * b);
}

static sxs_object_t alu_div_r(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  size_t size = g_api->list_size(list);

  if (size < 3) {
    return g_api->create_real(0.0);
  }

  sxs_object_t a_obj = g_api->list_at(list, 1);
  sxs_object_t b_obj = g_api->list_at(list, 2);

  sxs_object_t evaled_a = g_api->eval(ctx, a_obj);
  sxs_object_t evaled_b = g_api->eval(ctx, b_obj);

  double a = g_api->as_real(evaled_a);
  double b = g_api->as_real(evaled_b);

  if (b == 0.0) {
    return g_api->create_real(0.0);
  }

  return g_api->create_real(a / b);
}

extern "C" void kernel_init(sxs_registry_t registry,
                            const struct sxs_api_table_t *api) {
  g_api = api;
  api->register_function(registry, "add", alu_add, SXS_TYPE_INT, 0);
  api->register_function(registry, "sub", alu_sub, SXS_TYPE_INT, 0);
  api->register_function(registry, "mul", alu_mul, SXS_TYPE_INT, 0);
  api->register_function(registry, "div", alu_div, SXS_TYPE_INT, 0);
  api->register_function(registry, "mod", alu_mod, SXS_TYPE_INT, 0);
  api->register_function(registry, "add_r", alu_add_r, SXS_TYPE_REAL, 0);
  api->register_function(registry, "sub_r", alu_sub_r, SXS_TYPE_REAL, 0);
  api->register_function(registry, "mul_r", alu_mul_r, SXS_TYPE_REAL, 0);
  api->register_function(registry, "div_r", alu_div_r, SXS_TYPE_REAL, 0);
}
