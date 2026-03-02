#ifndef THOMPSON_H
#define THOMPSON_H

#include "regex.h"
#include "nfa.h"

/**
 * @brief Convert a regular expression represented as a regex struct into an NFA.
 * This function uses a stack-based approach to construct the NFA from the postfix
 * representation of the regex.
 *
 * @param r The input regular expression as a regex struct
 * @return An NFA struct representing the non-deterministic finite automaton
 * for the given regex.
 */
nfa regex_to_nfa(const regex r);

#endif