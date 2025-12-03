#ifndef SXS_TYPES_TYPES_H
#define SXS_TYPES_TYPES_H

#include <stdint.h>

/*
    Types are produced by a scanner off a buffer. The types listed here
    do not own the data they point to. they point into a buffer. the owner
    of the buffer is to maintain/manage that memory

    static_base_e represents the MOST BASIC data possible. no lists. no dynamic
   implications just shit we can infer from a small state machine off of a
   buffer

    start
        - if buffer at pos = [- or +] check next. if int -> switch to int parse
   state and keep sign, else: symbol
        - if while reading int we get "." -> real reading state (still consuming
   ints)
        - if white space \n \t ' ' etc, we are done
        - if parsing ints and non-whitespace non-peiord shows up (special case
   for real) -> error
*/
typedef enum slp_static_base_e {
  SLP_STATIC_BASE_NONE = 0,
  SLP_STATIC_BASE_INTEGER,
  SLP_STATIC_BASE_REAL,
  SLP_STATIC_BASE_SYMBOL,
} slp_static_base_e;

/*
    The static types and base types - really everything uses buffers. We don't
   own the buffers. We typedef like this to visually remind us not to try
   freeing them
*/
typedef void *slp_ref_none_t;
typedef int64_t *slp_ref_integer_t;
typedef double *slp_ref_real_t;
typedef char *slp_ref_symbol_t;

typedef union slp_static_union_s {
  slp_ref_none_t none;
  slp_ref_integer_t integer;
  slp_ref_real_t real;
  slp_ref_symbol_t symbol;
} slp_static_union_t;

typedef struct slp_static_type_s {
  slp_static_base_e base;
  slp_static_union_t data; // union of unowned pointers into buffer region that
                           // matched the type
} slp_static_type_t;

#endif