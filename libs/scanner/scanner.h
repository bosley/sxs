#ifndef SLP_SCANNER_H
#define SLP_SCANNER_H

#include "buffer/buffer.h"
#include "types/types.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct slp_scanner_s {
  slp_buffer_t *buffer;
  size_t position;
} slp_scanner_t;

slp_scanner_t *slp_scanner_new(slp_buffer_t *buffer, size_t position);

void slp_scanner_free(slp_scanner_t *scanner);

typedef struct slp_scanner_static_type_result_s {
  bool success;
  size_t start_position;
  size_t error_position;
  slp_static_type_t data;
} slp_scanner_static_type_result_t;

typedef struct slp_scanner_stop_symbols_s {
  uint8_t *symbols;
  size_t count;
} slp_scanner_stop_symbols_t;

/*
reads from buffer:

  ints (with sign)
  real
  symbols

Terminates by default on all whitespace.
Optionally accepts additional stop symbols that will terminate parsing
without consuming the stop character. stop_symbols may be NULL.
Stop symbols must NOT include '.', '+', or '-'.

Failure to read an int, real, r symbol will result in an error - indicated in
the result Success will automatically move the position of the scanner
*/
slp_scanner_static_type_result_t
slp_scanner_read_static_base_type(slp_scanner_t *scanner,
                                  slp_scanner_stop_symbols_t *stop_symbols);

/*
A start/end range to view into a buffer some grouping of data (a subset)
*/
typedef struct slp_scanner_find_group_result_s {
  bool success;
  size_t index_of_start_symbol;
  size_t index_of_closing_symbol;
} slp_scanner_find_group_result_t;

/*
scans the buffer from the current position
if the current position does not match must_start_with, return failure
(a user could check themselves, but we're completionists)
must_end_with is the byte that says "stop here"
if can_escape_with is not NULL then any byte matching must_end_with may be
skipped over and included into the group iff immediatly preceeded by the
*can_escape_with

consume_leading_ws when true will skip any leading whitespace before checking
for must_start_with. when false the check happens at the current position.
the returned index_of_start_symbol will point to the delimiter not the
whitespace

this provides the ability for us to get a view into something of a list "form"
without limiting us to JUST processing () [] {} etc. We can say parse this
group:   !a b +1 2$ where ! is start and $ is end then it also means we can do
"hello \"world!\"!" my providing '\' as the escape byte and having start AND end
both be: " Naturally this means we can also find () [] {} || <> etc provided
that the buffer position starts with the must_start_with so this enforces/
implies that the caller is doing something specific, and we just don't need to
know

*/
slp_scanner_find_group_result_t slp_scanner_find_group(slp_scanner_t *scanner,
                                                       uint8_t must_start_with,
                                                       uint8_t must_end_with,
                                                       uint8_t *can_escape_with,
                                                       bool consume_leading_ws);

/*
Skips all leading whitespace and returns true if the next non-whitespace byte is
found returns false if the end of the buffer is reached
*/
bool slp_scanner_goto_next_non_white(slp_scanner_t *scanner);

/*
Skips to the next byte matching the given target_byte and returns true if found
returns false if the end of the buffer is reached
*/
bool slp_scanner_goto_next_target(slp_scanner_t *scanner, uint8_t target_byte);

#endif