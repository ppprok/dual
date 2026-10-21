#pragma once

#include <stdint.h>

/// @file
/// @brief C ABI of `librunc`: portion-wise enumeration of the minimal covers of a Boolean matrix
///        (RUNCM). Used from Python by `src/pyrunc/runc.py` via `ctypes`.
///
/// Contract:
/// - A handle comes from `init_dualizer()` (NULL if out of memory) and is freed by
///   `release_dualizer()` (NULL is ignored). The library owns the handle; the caller owns all
///   buffers it passes and the library keeps no pointer to them after a call returns. A handle must
///   not be used from several threads at once; different handles are independent. No function
///   throws or aborts on bad input.
/// - Call order: `init_dualizer()`, then `set_input_row()` for the rows of the matrix, then
///   `enumerate_covers()` repeatedly until it returns 0, then `release_dualizer()`. Calling
///   `set_input_row()` between portions is allowed and restarts the enumeration.
/// - Semantics of the input: no rows -> exactly one cover, the empty one; a zero row -> no covers.
///
/// Example:
/// @code
/// DualizerHandler h = init_dualizer();
/// int32_t row0[] = {0, 1}, row1[] = {1, 2};
/// set_input_row(h, 0, 2, row0);
/// set_input_row(h, 1, 2, row1);
/// uint32_t buf[64];
/// for (int32_t n; (n = enumerate_covers(h, 64, buf)) > 0;) {
///     for (uint32_t *p = buf; n--; p += 1 + p[0])
///         use_cover(p + 1, p[0]);  // covers {1} and {0, 2}
/// }
/// release_dualizer(h);
/// @endcode

/// Success.
#define RUNC_OK 0
/// A NULL handle or buffer, or an invalid row number, count or column index.
#define RUNC_ERROR_INVALID_ARGUMENT (-1)
/// The buffer passed to `enumerate_covers()` cannot hold the largest possible record.
#define RUNC_ERROR_BUFFER_TOO_SMALL (-2)
/// Internal error, e.g. out of memory; the enumeration is reset.
#define RUNC_ERROR_INTERNAL (-3)

/// Opaque handle of a dualizer created by `init_dualizer()`.
typedef void* DualizerHandler;

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Create a dualizer with an empty input matrix.
/// @return A new handle, or NULL if out of memory. Free it with `release_dualizer()`.
DualizerHandler init_dualizer(void);

/// @brief Free a dualizer created by `init_dualizer()`; NULL is ignored.
/// @param dualizer Handle to free; it must not be used afterwards.
void release_dualizer(DualizerHandler dualizer);

/// @brief Add ones to a row of the input matrix.
///
/// Sets the bits `ones[0..ones_num)` in row @p row_num. Rows may be given in any order and a row
/// may be set several times (bits are added). The matrix has `max(row_num) + 1` rows and
/// `max(column) + 1` columns; rows never given are zero rows. Changing the input restarts the
/// enumeration: the next `enumerate_covers()` begins with the first cover again.
/// @param dualizer Handle from `init_dualizer()`.
/// @param row_num 0-based row index, `0 <= row_num < INT32_MAX`.
/// @param ones_num Number of column indices in @p ones; 0 just declares the row.
/// @param ones 0-based column indices, each in `[0, INT32_MAX)`; may be NULL if `ones_num == 0`.
///        The array is copied.
/// @return `RUNC_OK`; `RUNC_ERROR_INVALID_ARGUMENT` (NULL handle, `row_num < 0` or `== INT32_MAX`,
///         `ones_num < 0`, `ones == NULL` with `ones_num > 0`, a column `< 0` or `== INT32_MAX`),
///         the input is unchanged then; `RUNC_ERROR_INTERNAL` if out of memory.
int32_t set_input_row(DualizerHandler dualizer, int32_t row_num, int32_t ones_num, const int32_t* ones);

/// @brief Continue the enumeration and write the next portion of minimal covers.
///
/// Writes records `[k, c_1, ..., c_k]` (cover size, then its 0-based column indices in the order
/// the algorithm added them) one after another into `covers[0..buffer_size)`, as long as there is
/// room for another record of the largest possible size (number of columns + 1). Nothing is
/// written past @p buffer_size. Each minimal cover is returned exactly once over all calls.
/// @param dualizer Handle from `init_dualizer()`.
/// @param buffer_size Number of `uint32_t` words in @p covers; at least the number of columns + 1.
/// @param covers Caller-owned output buffer.
/// @return The number of records written (> 0); 0 when the enumeration is finished (every later
///         call returns 0 until the input changes); or a negative error code:
///         `RUNC_ERROR_INVALID_ARGUMENT` (NULL handle or buffer), `RUNC_ERROR_BUFFER_TOO_SMALL`
///         (`buffer_size` < number of columns + 1; nothing is written and the enumeration does not
///         advance), `RUNC_ERROR_INTERNAL` (e.g. out of memory; the enumeration is reset).
int32_t enumerate_covers(DualizerHandler dualizer, uint32_t buffer_size, uint32_t* covers);

#ifdef __cplusplus
}
#endif
