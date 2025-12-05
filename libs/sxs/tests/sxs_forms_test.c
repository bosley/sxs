#include "../forms.h"
#include "buffer/buffer.h"
#include "testlib/test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static slp_object_t *create_test_symbol(const char *text) {
  slp_object_t *obj = malloc(sizeof(slp_object_t));
  ASSERT_NOT_NULL(obj);

  obj->type = SLP_TYPE_SYMBOL;
  size_t len = strlen(text);
  obj->value.buffer = slp_buffer_new(len);
  ASSERT_NOT_NULL(obj->value.buffer);

  slp_buffer_copy_to(obj->value.buffer, (uint8_t *)text, len);

  return obj;
}

static void test_forms_create_destroy(void) {
  symbol_forms_t *forms = sxs_forms_new();
  ASSERT_NOT_NULL(forms);
  ASSERT_NOT_NULL(forms->forms);
  ASSERT_TRUE(forms->count > 0);
  ASSERT_TRUE(forms->capacity >= forms->count);

  sxs_forms_free(forms);
}

static void test_forms_get_form_type(void) {
  slp_object_t obj;

  obj.type = SLP_TYPE_INTEGER;
  ASSERT_EQ(sxs_forms_get_form_type(&obj), FORM_TYPE_INTEGER);

  obj.type = SLP_TYPE_REAL;
  ASSERT_EQ(sxs_forms_get_form_type(&obj), FORM_TYPE_REAL);

  obj.type = SLP_TYPE_SYMBOL;
  ASSERT_EQ(sxs_forms_get_form_type(&obj), FORM_TYPE_SYMBOL);

  obj.type = SLP_TYPE_LIST_S;
  ASSERT_EQ(sxs_forms_get_form_type(&obj), FORM_TYPE_LIST_S);

  obj.type = SLP_TYPE_LIST_P;
  ASSERT_EQ(sxs_forms_get_form_type(&obj), FORM_TYPE_LIST_P);

  obj.type = SLP_TYPE_LIST_B;
  ASSERT_EQ(sxs_forms_get_form_type(&obj), FORM_TYPE_LIST_B);

  obj.type = SLP_TYPE_LIST_C;
  ASSERT_EQ(sxs_forms_get_form_type(&obj), FORM_TYPE_LIST_C);

  obj.type = SLP_TYPE_QUOTED;
  ASSERT_EQ(sxs_forms_get_form_type(&obj), FORM_TYPE_SOME);

  obj.type = SLP_TYPE_BUILTIN;
  ASSERT_EQ(sxs_forms_get_form_type(&obj), FORM_TYPE_FN);

  obj.type = SLP_TYPE_LAMBDA;
  ASSERT_EQ(sxs_forms_get_form_type(&obj), FORM_TYPE_FN);

  obj.type = SLP_TYPE_NONE;
  ASSERT_EQ(sxs_forms_get_form_type(&obj), FORM_TYPE_NONE);

  ASSERT_EQ(sxs_forms_get_form_type(NULL), FORM_TYPE_NONE);
}

static void test_forms_lookup_base_forms(void) {
  symbol_forms_t *forms = sxs_forms_new();
  ASSERT_NOT_NULL(forms);

  slp_object_t *int_sym = create_test_symbol(":int");
  form_definition_t *def = sxs_forms_lookup(forms, int_sym);
  ASSERT_NOT_NULL(def);
  ASSERT_STR_EQ(def->name, "int");
  ASSERT_EQ(def->type_count, 1);
  ASSERT_EQ(def->types[0], FORM_TYPE_INTEGER);
  ASSERT_FALSE(def->is_variadic);
  slp_object_free(int_sym);

  slp_object_t *real_sym = create_test_symbol(":real");
  def = sxs_forms_lookup(forms, real_sym);
  ASSERT_NOT_NULL(def);
  ASSERT_STR_EQ(def->name, "real");
  ASSERT_EQ(def->types[0], FORM_TYPE_REAL);
  slp_object_free(real_sym);

  slp_object_t *symbol_sym = create_test_symbol(":symbol");
  def = sxs_forms_lookup(forms, symbol_sym);
  ASSERT_NOT_NULL(def);
  ASSERT_STR_EQ(def->name, "symbol");
  ASSERT_EQ(def->types[0], FORM_TYPE_SYMBOL);
  slp_object_free(symbol_sym);

  slp_object_t *any_sym = create_test_symbol(":any");
  def = sxs_forms_lookup(forms, any_sym);
  ASSERT_NOT_NULL(def);
  ASSERT_STR_EQ(def->name, "any");
  ASSERT_EQ(def->types[0], FORM_TYPE_ANY);
  slp_object_free(any_sym);

  sxs_forms_free(forms);
}

static void test_forms_lookup_variadic_forms(void) {
  symbol_forms_t *forms = sxs_forms_new();
  ASSERT_NOT_NULL(forms);

  slp_object_t *int_var_sym = create_test_symbol(":int..");
  form_definition_t *def = sxs_forms_lookup(forms, int_var_sym);
  ASSERT_NOT_NULL(def);
  ASSERT_STR_EQ(def->name, "int..");
  ASSERT_EQ(def->type_count, 1);
  ASSERT_EQ(def->types[0], FORM_TYPE_INTEGER_VARIADIC);
  ASSERT_TRUE(def->is_variadic);
  slp_object_free(int_var_sym);

  slp_object_t *real_var_sym = create_test_symbol(":real..");
  def = sxs_forms_lookup(forms, real_var_sym);
  ASSERT_NOT_NULL(def);
  ASSERT_STR_EQ(def->name, "real..");
  ASSERT_EQ(def->types[0], FORM_TYPE_REAL_VARIADIC);
  ASSERT_TRUE(def->is_variadic);
  slp_object_free(real_var_sym);

  slp_object_t *any_var_sym = create_test_symbol(":any..");
  def = sxs_forms_lookup(forms, any_var_sym);
  ASSERT_NOT_NULL(def);
  ASSERT_STR_EQ(def->name, "any..");
  ASSERT_EQ(def->types[0], FORM_TYPE_ANY_VARIADIC);
  ASSERT_TRUE(def->is_variadic);
  slp_object_free(any_var_sym);

  sxs_forms_free(forms);
}

static void test_forms_is_symbol_known_form(void) {
  symbol_forms_t *forms = sxs_forms_new();
  ASSERT_NOT_NULL(forms);

  slp_object_t *int_sym = create_test_symbol(":int");
  ASSERT_TRUE(sxs_forms_is_symbol_known_form(forms, int_sym));
  slp_object_free(int_sym);

  slp_object_t *unknown_sym = create_test_symbol(":unknown");
  ASSERT_FALSE(sxs_forms_is_symbol_known_form(forms, unknown_sym));
  slp_object_free(unknown_sym);

  slp_object_t *no_colon_sym = create_test_symbol("int");
  ASSERT_FALSE(sxs_forms_is_symbol_known_form(forms, no_colon_sym));
  slp_object_free(no_colon_sym);

  slp_object_t *int_var_sym = create_test_symbol(":int..");
  ASSERT_TRUE(sxs_forms_is_symbol_known_form(forms, int_var_sym));
  slp_object_free(int_var_sym);

  ASSERT_FALSE(sxs_forms_is_symbol_known_form(forms, NULL));
  ASSERT_FALSE(sxs_forms_is_symbol_known_form(NULL, int_sym));

  sxs_forms_free(forms);
}

static void test_forms_lookup_invalid_inputs(void) {
  symbol_forms_t *forms = sxs_forms_new();
  ASSERT_NOT_NULL(forms);

  ASSERT_NULL(sxs_forms_lookup(NULL, NULL));
  ASSERT_NULL(sxs_forms_lookup(forms, NULL));

  slp_object_t *int_sym = create_test_symbol(":int");
  ASSERT_NULL(sxs_forms_lookup(NULL, int_sym));
  slp_object_free(int_sym);

  slp_object_t non_symbol;
  non_symbol.type = SLP_TYPE_INTEGER;
  non_symbol.value.integer = 42;
  ASSERT_NULL(sxs_forms_lookup(forms, &non_symbol));

  sxs_forms_free(forms);
}

static void test_forms_all_base_forms_registered(void) {
  symbol_forms_t *forms = sxs_forms_new();
  ASSERT_NOT_NULL(forms);

  const char *base_form_names[] = {":none",   ":int",    ":real",   ":symbol",
                                   ":list-s", ":list-p", ":list-b", ":list-c",
                                   ":some",   ":fn",     ":any"};

  for (size_t i = 0; i < sizeof(base_form_names) / sizeof(base_form_names[0]);
       i++) {
    slp_object_t *sym = create_test_symbol(base_form_names[i]);
    ASSERT_TRUE(sxs_forms_is_symbol_known_form(forms, sym));
    form_definition_t *def = sxs_forms_lookup(forms, sym);
    ASSERT_NOT_NULL(def);
    slp_object_free(sym);
  }

  const char *variadic_form_names[] = {":none..",   ":int..",    ":real..",
                                       ":symbol..", ":list-s..", ":list-p..",
                                       ":list-b..", ":list-c..", ":some..",
                                       ":fn..",     ":any.."};

  for (size_t i = 0;
       i < sizeof(variadic_form_names) / sizeof(variadic_form_names[0]); i++) {
    slp_object_t *sym = create_test_symbol(variadic_form_names[i]);
    ASSERT_TRUE(sxs_forms_is_symbol_known_form(forms, sym));
    form_definition_t *def = sxs_forms_lookup(forms, sym);
    ASSERT_NOT_NULL(def);
    ASSERT_TRUE(def->is_variadic);
    slp_object_free(sym);
  }

  ASSERT_EQ(forms->count, 22);

  sxs_forms_free(forms);
}

int main(void) {
  test_forms_create_destroy();
  test_forms_get_form_type();
  test_forms_lookup_base_forms();
  test_forms_lookup_variadic_forms();
  test_forms_is_symbol_known_form();
  test_forms_lookup_invalid_inputs();
  test_forms_all_base_forms_registered();

  printf("All sxs_forms tests passed!\n");
  return 0;
}
