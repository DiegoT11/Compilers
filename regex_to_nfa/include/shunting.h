#ifndef SHUNTING_H
#define SHUNTING_H

#include "regex.h"

/**
 * @brief Parses a regular expression string into postfix regex items.
 *
 * Converts the input regex into a postfix representation suitable
 * for Thompson NFA construction.
 *
 * @param regex_str The input regular expression
 * @return A regex struct containing postfix items and their count
 */
regex parse_regex(const char *regex_str);

#endif