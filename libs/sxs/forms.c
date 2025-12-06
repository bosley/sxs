#include "forms.h"
#include "map/map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_FORMS_CAPACITY 32

static form_definition_t *create_form_definition(const char *name,
                                                 form_type_e type,
                                                 slp_type_e list_constraint,
                                                 bool is_variadic) {
  form_definition_t *def = malloc(sizeof(form_definition_t));
  if (!def) {
    return NULL;
  }

  def->name = malloc(strlen(name) + 1);
  if (!def->name) {
    free(def);
    return NULL;
  }
  strcpy(def->name, name);

  def->types = malloc(sizeof(form_type_e));
  if (!def->types) {
    free(def->name);
    free(def);
    return NULL;
  }
  def->types[0] = type;
  def->type_count = 1;
  def->list_constraint = list_constraint;
  def->is_variadic = is_variadic;

  return def;
}

static bool add_form(symbol_forms_t *forms, form_definition_t *def) {
  if (!forms || !def) {
    return false;
  }

  if (forms->count >= forms->capacity) {
    size_t new_capacity = forms->capacity * 2;
    form_definition_t **new_forms =
        realloc(forms->forms, sizeof(form_definition_t *) * new_capacity);
    if (!new_forms) {
      return false;
    }
    forms->forms = new_forms;
    forms->capacity = new_capacity;
  }

  forms->forms[forms->count++] = def;
  return true;
}

symbol_forms_t *sxs_forms_new(void) {
  symbol_forms_t *forms = malloc(sizeof(symbol_forms_t));
  if (!forms) {
    return NULL;
  }

  forms->forms = malloc(sizeof(form_definition_t *) * INITIAL_FORMS_CAPACITY);
  if (!forms->forms) {
    free(forms);
    return NULL;
  }
  forms->count = 0;
  forms->capacity = INITIAL_FORMS_CAPACITY;

  map_init(&forms->form_map);

  struct {
    const char *name;
    form_type_e type;
    bool is_variadic;
  } base_forms[] = {
      {"none", FORM_TYPE_NONE, false},
      {"int", FORM_TYPE_INTEGER, false},
      {"real", FORM_TYPE_REAL, false},
      {"symbol", FORM_TYPE_SYMBOL, false},
      {"list-s", FORM_TYPE_LIST_S, false},
      {"list-p", FORM_TYPE_LIST_P, false},
      {"list-b", FORM_TYPE_LIST_B, false},
      {"list-c", FORM_TYPE_LIST_C, false},
      {"some", FORM_TYPE_SOME, false},
      {"fn", FORM_TYPE_FN, false},
      {"any", FORM_TYPE_ANY, false},
      {"none..", FORM_TYPE_NONE, true},
      {"int..", FORM_TYPE_INTEGER_VARIADIC, true},
      {"real..", FORM_TYPE_REAL_VARIADIC, true},
      {"symbol..", FORM_TYPE_SYMBOL_VARIADIC, true},
      {"list-s..", FORM_TYPE_LIST_S_VARIADIC, true},
      {"list-p..", FORM_TYPE_LIST_P_VARIADIC, true},
      {"list-b..", FORM_TYPE_LIST_B_VARIADIC, true},
      {"list-c..", FORM_TYPE_LIST_C_VARIADIC, true},
      {"some..", FORM_TYPE_SOME_VARIADIC, true},
      {"fn..", FORM_TYPE_FN_VARIADIC, true},
      {"any..", FORM_TYPE_ANY_VARIADIC, true},
  };

  for (size_t i = 0; i < sizeof(base_forms) / sizeof(base_forms[0]); i++) {
    form_definition_t *def =
        create_form_definition(base_forms[i].name, base_forms[i].type,
                               SLP_TYPE_NONE, base_forms[i].is_variadic);
    if (!def || !add_form(forms, def)) {
      sxs_forms_free(forms);
      return NULL;
    }

    if (map_set(&forms->form_map, def->name, def) != 0) {
      sxs_forms_free(forms);
      return NULL;
    }
  }

  return forms;
}

void sxs_forms_free(symbol_forms_t *forms) {
  if (!forms) {
    return;
  }

  map_deinit(&forms->form_map);

  if (forms->forms) {
    for (size_t i = 0; i < forms->count; i++) {
      if (forms->forms[i]) {
        if (forms->forms[i]->name) {
          free(forms->forms[i]->name);
        }
        if (forms->forms[i]->types) {
          free(forms->forms[i]->types);
        }
        free(forms->forms[i]);
      }
    }
    free(forms->forms);
  }

  free(forms);
}

form_type_e sxs_forms_get_form_type(slp_object_t *obj) {
  if (!obj) {
    return FORM_TYPE_NONE;
  }

  switch (obj->type) {
  case SLP_TYPE_NONE:
    return FORM_TYPE_NONE;
  case SLP_TYPE_INTEGER:
    return FORM_TYPE_INTEGER;
  case SLP_TYPE_REAL:
    return FORM_TYPE_REAL;
  case SLP_TYPE_SYMBOL:
    return FORM_TYPE_SYMBOL;
  case SLP_TYPE_LIST_S:
    return FORM_TYPE_LIST_S;
  case SLP_TYPE_LIST_P:
    return FORM_TYPE_LIST_P;
  case SLP_TYPE_LIST_B:
    return FORM_TYPE_LIST_B;
  case SLP_TYPE_LIST_C:
    return FORM_TYPE_LIST_C;
  case SLP_TYPE_QUOTED:
    return FORM_TYPE_SOME;
  case SLP_TYPE_BUILTIN:
  case SLP_TYPE_LAMBDA:
    return FORM_TYPE_FN;
  case SLP_TYPE_ERROR:
    return FORM_TYPE_NONE;
  default:
    return FORM_TYPE_NONE;
  }
}

bool sxs_forms_is_symbol_known_form(symbol_forms_t *forms,
                                    slp_object_t *symbol) {
  if (!forms || !symbol || symbol->type != SLP_TYPE_SYMBOL) {
    return false;
  }

  if (!symbol->value.buffer || symbol->value.buffer->count == 0) {
    return false;
  }

  if (symbol->value.buffer->data[0] != ':') {
    return false;
  }

  return sxs_forms_lookup(forms, symbol) != NULL;
}

form_definition_t *sxs_forms_lookup(symbol_forms_t *forms,
                                    slp_object_t *symbol) {
  if (!forms || !symbol || symbol->type != SLP_TYPE_SYMBOL) {
    return NULL;
  }

  if (!symbol->value.buffer || symbol->value.buffer->count < 2) {
    return NULL;
  }

  if (symbol->value.buffer->data[0] != ':') {
    return NULL;
  }

  size_t name_len = symbol->value.buffer->count - 1;
  const char *name_start = (const char *)&symbol->value.buffer->data[1];

  char *key = malloc(name_len + 1);
  if (!key) {
    return NULL;
  }
  memcpy(key, name_start, name_len);
  key[name_len] = '\0';

  void **result = map_get(&forms->form_map, key);
  free(key);

  if (!result) {
    return NULL;
  }

  return (form_definition_t *)*result;
}

const char *sxs_forms_get_form_type_name(form_type_e type) {
  switch (type) {
  case FORM_TYPE_NONE:
    return "none";
  case FORM_TYPE_INTEGER:
    return "int";
  case FORM_TYPE_REAL:
    return "real";
  case FORM_TYPE_SYMBOL:
    return "symbol";
  case FORM_TYPE_LIST_S:
    return "list-s";
  case FORM_TYPE_LIST_P:
    return "list-p";
  case FORM_TYPE_LIST_B:
    return "list-b";
  case FORM_TYPE_LIST_C:
    return "list-c";
  case FORM_TYPE_SOME:
    return "some";
  case FORM_TYPE_FN:
    return "fn";
  case FORM_TYPE_ANY:
    return "any";
  case FORM_TYPE_INTEGER_VARIADIC:
    return "int..";
  case FORM_TYPE_REAL_VARIADIC:
    return "real..";
  case FORM_TYPE_SYMBOL_VARIADIC:
    return "symbol..";
  case FORM_TYPE_LIST_S_VARIADIC:
    return "list-s..";
  case FORM_TYPE_LIST_P_VARIADIC:
    return "list-p..";
  case FORM_TYPE_LIST_B_VARIADIC:
    return "list-b..";
  case FORM_TYPE_LIST_C_VARIADIC:
    return "list-c..";
  case FORM_TYPE_SOME_VARIADIC:
    return "some..";
  case FORM_TYPE_FN_VARIADIC:
    return "fn..";
  case FORM_TYPE_ANY_VARIADIC:
    return "any..";
  default:
    return "unknown";
  }
}
