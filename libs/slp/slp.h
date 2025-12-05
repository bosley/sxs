#ifndef SXS_SLP_SLP_H
#define SXS_SLP_SLP_H

#define SLP_PROCESSOR_COUNT 16 // for consistence . dont change
#define SLP_REGISTER_COUNT 32  // for consistence . dont change

#include "buffer/buffer.h"

typedef enum slp_type_e {
  SLP_TYPE_NONE = 0,
  SLP_TYPE_INTEGER,
  SLP_TYPE_REAL,
  SLP_TYPE_SYMBOL,
  SLP_TYPE_LIST_P,  // ()
  SLP_TYPE_LIST_C,  // {}
  SLP_TYPE_LIST_B,  // []
  SLP_TYPE_LIST_Q,  // ''
  SLP_TYPE_LIST_S,  // ""
  SLP_TYPE_QUOTED,  // ' prefixed
  SLP_TYPE_BUILTIN, // A C function
  SLP_TYPE_LAMBDA,  // A lambda function
} slp_type_e;

typedef struct slp_cell_s {

} slp_type_t;

/*

command based language. not list centric. not homoiconicm ?


think of it as lisp with an optional "outer set?"


our runtime offers us "registers" to store things
top level object results are discarded and freed, unless in repl then
it would show

we dont have "Variable storage" the same way that you would in a higher language
we cant just "Set x" we have pre-allocated bins of objects that we can assign to
by index into that bin

Our builtins are "mapped" to 1-byte instruction ops so we use raw aski to
invoke! Neat!

All builtin symbols are 1 char

+ 1 2 3

(+
    1
    2
    3)


(@ 0 (+ 1 2 3)) ; Sum 3 ints and store into object storage slot 0 - thats what
"@" does store-at ; @ works for store and load. one param means load, 2 params
means store, 3 params ; means CAS (compare and swap) for atomic updating

(@ 0 5 420)     ; if 0 == 5, swap-in 420

(o (@ 0))       ; output - load spot 0


(@ 0 (F (LHS :int RHS :int) :int
    (+ LHS RHS)
))

((@ 0) 60 9) ; Call the function

*/

#endif