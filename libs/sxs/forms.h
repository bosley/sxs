#ifndef SXS_FORMS_H
#define SXS_FORMS_H

#include "slp/slp.h"
#include <stdbool.h>
#include <stddef.h>

/*
Forms dictate what an argument must "evaluate" to at runtime not "what they are
when calling" meaning that if a function states it takes a "some" (quoted) then
''a must be passed so that the first eval yields a quoted

forms are basically types, but extendable in list-form

initial forms:

int
real
symbol
list-s ""
list-p ()
list-b []
list-c {}
some   '
none

- Specail type meaning "doesnt matter" -
any


.form list :list-p :list-b :list-c          ; means a form of any of these can
be known as list

.form numeric :int :real

.form person [:str :int :str]               ; means it requires "str int and
str" t make a person

*/

typedef enum form_type_e {
  FORM_TYPE_NONE = 0,
  FORM_TYPE_INTEGER,
  FORM_TYPE_REAL,
  FORM_TYPE_SYMBOL,
  FORM_TYPE_LIST_S,
  FORM_TYPE_LIST_P,
  FORM_TYPE_LIST_B,
  FORM_TYPE_LIST_C,
  FORM_TYPE_SOME,
  FORM_TYPE_FN,
  FORM_TYPE_ANY,
  FORM_TYPE_INTEGER_VARIADIC,
  FORM_TYPE_REAL_VARIADIC,
  FORM_TYPE_SYMBOL_VARIADIC,
  FORM_TYPE_LIST_S_VARIADIC,
  FORM_TYPE_LIST_P_VARIADIC,
  FORM_TYPE_LIST_B_VARIADIC,
  FORM_TYPE_LIST_C_VARIADIC,
  FORM_TYPE_SOME_VARIADIC,
  FORM_TYPE_FN_VARIADIC,
  FORM_TYPE_ANY_VARIADIC,
} form_type_e;

typedef struct form_definition_s {
  char *name;
  form_type_e *types;
  size_t type_count;
  slp_type_e list_constraint;
  bool is_variadic;
} form_definition_t;

typedef struct symbol_forms_s {
  form_definition_t **forms;
  size_t count;
  size_t capacity;
} symbol_forms_t;

symbol_forms_t *sxs_forms_create(void);
void sxs_forms_destroy(symbol_forms_t *forms);
form_type_e sxs_forms_get_form_type(slp_object_t *obj);
bool sxs_forms_is_symbol_known_form(symbol_forms_t *forms,
                                    slp_object_t *symbol);
form_definition_t *sxs_forms_lookup(symbol_forms_t *forms,
                                    slp_object_t *symbol);
const char *sxs_forms_get_form_type_name(form_type_e type);

#endif