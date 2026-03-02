#ifndef SHUNTING_H
#define SHUNTING_H

#include "regex.h"

/**
 * @brief Function to parse a regular expression string and convert it into a regex struct.
 * This function performs several steps: 
 * 1. Itemizes the regex string into an array of items (tokens).
 * 2. Converts implicit concatenation in the regex string to explicit concatenation.
 * 3. Converts the infix notation of the regex to postfix notation using the shunting yard algorithm.
 *
 * @param regex_str The input regular expression as a string
 * @return A regex struct containing the size of the postfix items and the array of postfix items
 */
regex parse_regex(const char *regex_str);

#endif