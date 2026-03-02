#ifndef SIMULATE_H
#define SIMULATE_H

#include "nfa.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Function to check if a given input string matches the language defined by the NFA.
 * This function simulates the NFA on the input string and returns true if the NFA accepts
 * the string, and false otherwise.
 *
 * @param automaton The NFA to simulate
 * @param input The input string to check against the NFA
 * @param input_length The length of the input string
 * @return true if the NFA accepts the input string, false otherwise
 */
bool match_nfa(nfa automaton, const char *input, size_t input_length);

#endif