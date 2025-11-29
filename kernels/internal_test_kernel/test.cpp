#include "core/kernels/kernel_api.h"
#include "slp/slp.hpp"
#include <cstdio>

static const struct sxs_api_table_t *g_api = nullptr;

static const char *type_name(sxs_type_t type) {
  switch (type) {
  case SXS_TYPE_NONE:
    return "NONE";
  case SXS_TYPE_SOME:
    return "SOME";
  case SXS_TYPE_PAREN_LIST:
    return "PAREN_LIST";
  case SXS_TYPE_BRACE_LIST:
    return "BRACE_LIST";
  case SXS_TYPE_STRING:
    return "STRING";
  case SXS_TYPE_SYMBOL:
    return "SYMBOL";
  case SXS_TYPE_RUNE:
    return "RUNE";
  case SXS_TYPE_INT:
    return "INT";
  case SXS_TYPE_REAL:
    return "REAL";
  case SXS_TYPE_BRACKET_LIST:
    return "BRACKET_LIST";
  case SXS_TYPE_ERROR:
    return "ERROR";
  case SXS_TYPE_DATUM:
    return "DATUM";
  case SXS_TYPE_ABERRANT:
    return "ABERRANT";
  default:
    return "UNKNOWN";
  }
}

static sxs_object_t identity_int(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  if (g_api->list_size(list) < 2) {
    printf("TEST_API_INT:ERROR_NO_ARG\n");
    return g_api->create_int(0);
  }

  sxs_object_t arg_obj = g_api->list_at(list, 1);
  sxs_object_t evaled = g_api->eval(ctx, arg_obj);

  sxs_type_t type = g_api->get_type(evaled);
  if (type != SXS_TYPE_INT) {
    printf("TEST_API_INT:ERROR_WRONG_TYPE:%s\n", type_name(type));
    return g_api->create_int(0);
  }

  long long value = g_api->as_int(evaled);
  printf("TEST_API_INT:%lld\n", value);

  return g_api->create_int(value);
}

static sxs_object_t identity_real(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  if (g_api->list_size(list) < 2) {
    printf("TEST_API_REAL:ERROR_NO_ARG\n");
    return g_api->create_real(0.0);
  }

  sxs_object_t arg_obj = g_api->list_at(list, 1);
  sxs_object_t evaled = g_api->eval(ctx, arg_obj);

  sxs_type_t type = g_api->get_type(evaled);
  if (type != SXS_TYPE_REAL) {
    printf("TEST_API_REAL:ERROR_WRONG_TYPE:%s\n", type_name(type));
    return g_api->create_real(0.0);
  }

  double value = g_api->as_real(evaled);
  printf("TEST_API_REAL:%.2f\n", value);

  return g_api->create_real(value);
}

static sxs_object_t identity_str(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  if (g_api->list_size(list) < 2) {
    printf("TEST_API_STR:ERROR_NO_ARG\n");
    return g_api->create_string("");
  }

  sxs_object_t arg_obj = g_api->list_at(list, 1);
  sxs_object_t evaled = g_api->eval(ctx, arg_obj);

  sxs_type_t type = g_api->get_type(evaled);
  if (type != SXS_TYPE_STRING) {
    printf("TEST_API_STR:ERROR_WRONG_TYPE:%s\n", type_name(type));
    return g_api->create_string("");
  }

  const char *value = g_api->as_string(evaled);
  printf("TEST_API_STR:%s\n", value ? value : "");

  return g_api->create_string(value);
}

static sxs_object_t identity_none(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  if (g_api->list_size(list) < 2) {
    printf("TEST_API_NONE:ERROR_NO_ARG\n");
    return g_api->create_none();
  }

  sxs_object_t arg_obj = g_api->list_at(list, 1);
  sxs_object_t evaled = g_api->eval(ctx, arg_obj);

  sxs_type_t type = g_api->get_type(evaled);
  if (type == SXS_TYPE_SOME && g_api->some_has_value(evaled)) {
    evaled = g_api->some_get_value(evaled);
    type = g_api->get_type(evaled);
  }

  printf("TEST_API_NONE:%s\n", type_name(type));

  return evaled;
}

static sxs_object_t identity_symbol(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  if (g_api->list_size(list) < 2) {
    printf("TEST_API_SYMBOL:ERROR_NO_ARG\n");
    return g_api->create_none();
  }

  sxs_object_t arg_obj = g_api->list_at(list, 1);

  sxs_type_t type = g_api->get_type(arg_obj);

  if (type == SXS_TYPE_SOME) {
    if (g_api->some_has_value(arg_obj)) {
      sxs_object_t inner = g_api->some_get_value(arg_obj);
      sxs_type_t inner_type = g_api->get_type(inner);
      if (inner_type == SXS_TYPE_SYMBOL) {
        const char *symbol = g_api->as_symbol(inner);
        printf("TEST_API_SYMBOL:%s\n", symbol ? symbol : "");
        return arg_obj;
      }
    }
  }

  if (type != SXS_TYPE_SYMBOL) {
    printf("TEST_API_SYMBOL:ERROR_WRONG_TYPE:%s\n", type_name(type));
    return g_api->create_none();
  }

  const char *symbol = g_api->as_symbol(arg_obj);
  printf("TEST_API_SYMBOL:%s\n", symbol ? symbol : "");

  return arg_obj;
}

static sxs_object_t identity_list_p(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  if (g_api->list_size(list) < 2) {
    printf("TEST_API_LIST_P:ERROR_NO_ARG\n");
    return g_api->create_none();
  }

  sxs_object_t arg_obj = g_api->list_at(list, 1);
  sxs_object_t evaled = g_api->eval(ctx, arg_obj);

  sxs_type_t type = g_api->get_type(evaled);
  if (type == SXS_TYPE_SOME && g_api->some_has_value(evaled)) {
    evaled = g_api->some_get_value(evaled);
    type = g_api->get_type(evaled);
  }

  if (type != SXS_TYPE_PAREN_LIST) {
    printf("TEST_API_LIST_P:ERROR_WRONG_TYPE:%s\n", type_name(type));
    return g_api->create_none();
  }

  void *paren_list = g_api->as_list(evaled);
  size_t size = g_api->list_size(paren_list);
  printf("TEST_API_LIST_P:%s\n", type_name(type));

  return evaled;
}

static sxs_object_t identity_list_c(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  if (g_api->list_size(list) < 2) {
    printf("TEST_API_LIST_C:ERROR_NO_ARG\n");
    return g_api->create_none();
  }

  sxs_object_t arg_obj = g_api->list_at(list, 1);
  sxs_object_t evaled = g_api->eval(ctx, arg_obj);

  sxs_type_t type = g_api->get_type(evaled);
  if (type == SXS_TYPE_SOME && g_api->some_has_value(evaled)) {
    evaled = g_api->some_get_value(evaled);
    type = g_api->get_type(evaled);
  }

  if (type != SXS_TYPE_BRACE_LIST) {
    printf("TEST_API_LIST_C:ERROR_WRONG_TYPE:%s\n", type_name(type));
    return g_api->create_none();
  }

  void *brace_list = g_api->as_list(evaled);
  size_t size = g_api->list_size(brace_list);
  printf("TEST_API_LIST_C:%s\n", type_name(type));

  return evaled;
}

static sxs_object_t identity_list_b(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  if (g_api->list_size(list) < 2) {
    printf("TEST_API_LIST_B:ERROR_NO_ARG\n");
    return g_api->create_none();
  }

  sxs_object_t arg_obj = g_api->list_at(list, 1);
  sxs_object_t evaled = g_api->eval(ctx, arg_obj);

  sxs_type_t type = g_api->get_type(evaled);
  if (type == SXS_TYPE_SOME && g_api->some_has_value(evaled)) {
    evaled = g_api->some_get_value(evaled);
    type = g_api->get_type(evaled);
  }

  if (type != SXS_TYPE_BRACKET_LIST) {
    printf("TEST_API_LIST_B:ERROR_WRONG_TYPE:%s\n", type_name(type));
    return g_api->create_none();
  }

  void *bracket_list = g_api->as_list(evaled);
  size_t size = g_api->list_size(bracket_list);
  printf("TEST_API_LIST_B:%s\n", type_name(type));

  return evaled;
}

static sxs_object_t identity_some(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  if (g_api->list_size(list) < 2) {
    printf("TEST_API_SOME:ERROR_NO_ARG\n");
    return g_api->create_none();
  }

  sxs_object_t arg_obj = g_api->list_at(list, 1);
  sxs_object_t evaled = g_api->eval(ctx, arg_obj);

  sxs_type_t type = g_api->get_type(evaled);
  printf("TEST_API_SOME:%s\n", type_name(type));

  if (type == SXS_TYPE_SOME) {
    int has_value = g_api->some_has_value(evaled);
    printf("TEST_API_SOME:HAS_VALUE:%d\n", has_value);
  }

  return evaled;
}

static sxs_object_t identity_error(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  if (g_api->list_size(list) < 2) {
    printf("TEST_API_ERROR:ERROR_NO_ARG\n");
    return g_api->create_none();
  }

  sxs_object_t arg_obj = g_api->list_at(list, 1);
  sxs_object_t evaled = g_api->eval(ctx, arg_obj);

  sxs_type_t type = g_api->get_type(evaled);
  printf("TEST_API_ERROR:%s\n", type_name(type));

  return evaled;
}

static sxs_object_t identity_datum(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  if (g_api->list_size(list) < 2) {
    printf("TEST_API_DATUM:ERROR_NO_ARG\n");
    return g_api->create_none();
  }

  sxs_object_t arg_obj = g_api->list_at(list, 1);
  sxs_object_t evaled = g_api->eval(ctx, arg_obj);

  sxs_type_t type = g_api->get_type(evaled);
  printf("TEST_API_DATUM:%s\n", type_name(type));

  return evaled;
}

static sxs_object_t identity_aberrant(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  if (g_api->list_size(list) < 2) {
    printf("TEST_API_ABERRANT:ERROR_NO_ARG\n");
    return g_api->create_none();
  }

  sxs_object_t arg_obj = g_api->list_at(list, 1);
  sxs_object_t evaled = g_api->eval(ctx, arg_obj);

  sxs_type_t type = g_api->get_type(evaled);
  printf("TEST_API_ABERRANT:%s\n", type_name(type));

  return evaled;
}

extern "C" void kernel_init(sxs_registry_t registry,
                            const struct sxs_api_table_t *api) {
  g_api = api;
  api->register_function(registry, "identity_int", identity_int, SXS_TYPE_INT,
                         0);
  api->register_function(registry, "identity_real", identity_real,
                         SXS_TYPE_REAL, 0);
  api->register_function(registry, "identity_str", identity_str,
                         SXS_TYPE_STRING, 0);
  api->register_function(registry, "identity_none", identity_none,
                         SXS_TYPE_NONE, 0);
  api->register_function(registry, "identity_symbol", identity_symbol,
                         SXS_TYPE_SYMBOL, 0);
  api->register_function(registry, "identity_list_p", identity_list_p,
                         SXS_TYPE_PAREN_LIST, 0);
  api->register_function(registry, "identity_list_c", identity_list_c,
                         SXS_TYPE_BRACE_LIST, 0);
  api->register_function(registry, "identity_list_b", identity_list_b,
                         SXS_TYPE_BRACKET_LIST, 0);
  api->register_function(registry, "identity_some", identity_some,
                         SXS_TYPE_SOME, 0);
  api->register_function(registry, "identity_error", identity_error,
                         SXS_TYPE_ERROR, 0);
  api->register_function(registry, "identity_datum", identity_datum,
                         SXS_TYPE_DATUM, 0);
  api->register_function(registry, "identity_aberrant", identity_aberrant,
                         SXS_TYPE_ABERRANT, 0);
}
