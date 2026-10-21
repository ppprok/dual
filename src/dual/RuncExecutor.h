#pragma once

#include <stdint.h>

/*
 * C ABI of librunc: portion-wise enumeration of the minimal covers of a Boolean matrix (RUNCM).
 *
 * Contract:
 * - A handle comes from init_dualizer() (NULL if out of memory) and is freed by release_dualizer()
 *   (NULL is ignored). A handle must not be used from several threads at once; different handles
 *   are independent. No function throws or aborts on bad input.
 * - set_input_row(h, row_num, ones_num, ones) sets the ones_num bits ones[0..ones_num) in row
 *   row_num (0-based). Rows may be given in any order and a row may be set several times (bits are
 *   added). The matrix has max(row_num)+1 rows and max(column)+1 columns; rows never given are
 *   zero rows. ones_num == 0 just declares the row (ones may be NULL then). Returns RUNC_OK, or
 *   RUNC_ERROR_INVALID_ARGUMENT (NULL handle, row_num < 0 or == INT32_MAX, ones_num < 0, ones == NULL with
 *   ones_num > 0, a column < 0 or == INT32_MAX) and leaves the input unchanged. Changing the input
 *   restarts the enumeration: the next enumerate_covers() begins with the first cover again.
 * - Semantics of the input: no rows -> exactly one cover, the empty one; a zero row -> no covers.
 * - enumerate_covers(h, buffer_size, covers) continues the enumeration and writes records
 *   [k, c_1, ..., c_k] (cover size, then its column indices) into covers[0..buffer_size). It
 *   returns the number of records written (> 0), 0 when the enumeration is finished (every later
 *   call returns 0 until the input changes), or a negative error code:
 *   RUNC_ERROR_INVALID_ARGUMENT (NULL handle or buffer), RUNC_ERROR_BUFFER_TOO_SMALL
 *   (buffer_size < number of columns + 1, the size of the largest possible record; nothing is
 *   written and the enumeration does not advance), RUNC_ERROR_INTERNAL (e.g. out of memory; the
 *   enumeration is reset). Nothing is written past buffer_size.
 */

#define RUNC_OK 0
#define RUNC_ERROR_INVALID_ARGUMENT (-1)
#define RUNC_ERROR_BUFFER_TOO_SMALL (-2)
#define RUNC_ERROR_INTERNAL (-3)

typedef void* DualizerHandler;

#ifdef __cplusplus
extern "C" {
#endif

DualizerHandler init_dualizer(void);
void release_dualizer(DualizerHandler dualizer);
int32_t set_input_row(DualizerHandler dualizer, int32_t row_num, int32_t ones_num, const int32_t* ones);
int32_t enumerate_covers(DualizerHandler dualizer, uint32_t buffer_size, uint32_t* covers);

#ifdef __cplusplus
}
#endif
