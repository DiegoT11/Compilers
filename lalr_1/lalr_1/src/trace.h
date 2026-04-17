#ifndef TRACE_H
#define TRACE_H

#include <stdbool.h>
#include <stdio.h>
#include "grammar.h"
#include "parser.h"

/**
 * @brief Runs the LALR(1) parse with a full step-by-step trace.
 * @param g Parsed grammar (used to resolve symbol names and productions).
 * @param table ACTION / GOTO table produced by build_lalr1_parser_table().
 * @param out Destination stream for the trace.
 * @return true when the input is accepted, 
 *         false on syntax error or internal failure.
 */
bool parse_with_trace(const grammar *g, const parser_table *table, FILE *out);

#endif /* TRACE_H */