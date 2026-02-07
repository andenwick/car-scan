/**
 * hex_utils.h — Internal header for hex conversion utilities.
 *
 * This is a PRIVATE header (lives in src/, not include/).
 * Other modules within the library can use these helpers, but
 * outside code should use the public API in obd.h instead.
 */

#ifndef HEX_UTILS_H
#define HEX_UTILS_H

#include <obd/obd_types.h>

/**
 * Convert a single hex character ('0'-'9', 'A'-'F', 'a'-'f') to its
 * numeric value (0-15). Returns -1 if the character isn't valid hex.
 *
 * Examples: '0'→0, '9'→9, 'A'→10, 'f'→15, 'G'→-1
 */
int hex_char_to_nibble(char c);

/**
 * Check if a character is a whitespace character we should skip.
 * Spaces, tabs, carriage returns, and newlines all count.
 */
int is_whitespace(char c);

#endif /* HEX_UTILS_H */
