#include "analyzer.h"

/**
 * @brief Finds a terminal identifier by terminal name.
 * @param g Parsed grammar.
 * @param name Terminal name to search.
 * @return Terminal id if found, otherwise -1.
 */
static int find_terminal_id(const grammar *g, const char *name)
{
	// TODO: Validate inputs and search terminal list to return the matching terminal id.
	if (g == NULL || name == NULL)
    {
        return -1;
    }

    return get_symbol_id_from_hash(name, &g->terminal_index);
}

/**
 * @brief Appends one symbol to a dynamically sized symbol array.
 * @param arr Target array pointer.
 * @param count Current element count; incremented on success.
 * @param text Symbol text to copy.
 * @param is_terminal Indicates terminal/non-terminal role.
 * @return true on success, false on allocation failure.
 */
static bool add_symbol_to_array(symbol **arr, int *count, const char *text, bool is_terminal)
{
	// TODO: Reallocate the array, duplicate symbol text, fill metadata, and increase count.
    // validate input
	if (arr == NULL || count == NULL || text == NULL)
    {
        return false;
    }

    // extend array in one element
    symbol *new_array = (symbol *)realloc(*arr, (*count + 1) * sizeof(symbol));
    if (new_array == NULL)
    {
        return false;
    }

    // update pointer
    *arr = new_array;

    // duplicate symbol text
    char *duplicate_text = strdup(text);
    if (duplicate_text == NULL)
    {
        return false;
    }

    (*arr)[*count].symbol = duplicate_text; // pointer to duplicate text
    (*arr)[*count].symbol_length = (int)strlen(text); // pre-calculated lenght
    (*arr)[*count].is_terminal = is_terminal; // symbol's rol

    // increase count after success
    (*count)++;
    return true;
}

/**
 * @brief Builds FIRST and nullable tables for all non-terminals.
 * @param g Parsed grammar.
 * @param first_table Output flattened table: non-terminal x terminal.
 * @param nullable Output nullable flags per non-terminal.
 * @param epsilon_id Output id of terminal "epsilon", or -1 if absent.
 * @return true when tables were built, false on invalid input or allocation error.
 */
static bool compute_first_tables(const grammar *g, bool **first_table, bool **nullable, int *epsilon_id)
{
	// TODO: Allocate FIRST/nullable tables and compute them with fixed-point propagation over productions.

    // validate input
	if (g == NULL || first_table == NULL || nullable == NULL || epsilon_id == NULL)
    {
        return false;
    }

    // search for epsilon to exclude it from propagating
    *epsilon_id = find_terminal_id(g, "epsilon");

    // initialize first_table in false
    *first_table = (bool *)calloc(g->num_non_terminals * g->num_terminals, sizeof(bool));
    if (*first_table == NULL)
    {
        return false;
    }

    // initialize nullable in false
    *nullable = (bool *)calloc(g->num_non_terminals, sizeof(bool));
    if (*nullable == NULL)
    {
        free(*first_table);
        return false;
    }

    bool changed = true;

    // until no production adds new symbols
    while (changed)
    {
        changed = false;

        // for every production A -> x1 x2 ... xn
        for (int p = 0; p < g->num_productions; p++)
        {
            
            production prod = g->productions[p];
            int A = prod.non_terminal_id;

            // assume every production is nullable until the first symbol doesn't
            bool all_nullable = true;

            // for every xi
            for (int j = 0; j < prod.production_length; j++)
            {
                int sym_id = prod.production_symbol_ids[j];
                bool is_terminal = sym_id < g->num_terminals;

                if (is_terminal)
                {   
                    // if there isn't in first[A]
                    if (!(*first_table)[A * g->num_terminals + sym_id])
                    {
                        // add it
                        (*first_table)[A * g->num_terminals + sym_id] = true;
                        changed = true;
                    }
                    // no longer nullable, stop
                    all_nullable = false;
                    break;
                }
                else
                {
                    int Xi = sym_id - g->num_terminals;
                    
                    // for every terminal t in first[xi]
                    for (int t = 0; t < g->num_terminals; t++)
                    {
                        // it's nullable
                        if (t == *epsilon_id)
                        {
                            continue;
                        }
                        // if t there isn't in first[xi]
                        if ((*first_table)[Xi * g->num_terminals + t] &&
                            !(*first_table)[A  * g->num_terminals + t])
                        {
                            // add it
                            (*first_table)[A * g->num_terminals + t] = true;
                            changed = true;
                        }
                    }

                    // xi isn't nullable, the prod can't derivate on epsilon
                    if (!(*nullable)[Xi])
                    {
                        all_nullable = false;
                        break;
                    }
                }
            }
            // every A symbols' production are nullable 
            if (all_nullable && !(*nullable)[A])
            {
                // A nullable
                (*nullable)[A] = true;
                changed = true;
            }
        }
    }
    return true;
}

/**
 * @brief Builds FOLLOW table for all non-terminals.
 * @param g Parsed grammar.
 * @param first_table FIRST table from compute_first_tables.
 * @param nullable Nullable flags from compute_first_tables.
 * @param epsilon_id Terminal id for "epsilon", or -1.
 * @param out_follow Output flattened table: non-terminal x (terminals + '$').
 * @param out_follow_cols Output number of columns for out_follow.
 * @return true on success, false on allocation error or invalid input.
 */
static bool compute_follow_table(
	const grammar *g,
	const bool *first_table,
	const bool *nullable,
	int epsilon_id,
	bool **out_follow,
	int *out_follow_cols)
{
	// validate input: grammar, first_table, nullable, out_follow, and out_follow_cols must be non-null
    if (!g || !first_table || !nullable || !out_follow || !out_follow_cols) 
        return false;

    int cols = g->num_terminals + 1; // +1 for $
    *out_follow_cols = cols;

	// initialize follow_table in false		
    *out_follow = calloc(g->num_non_terminals * cols, sizeof(bool));
	// allocation failure
    if (!*out_follow)
        return false;

	// The end marker '$' is represented as the last column in the follow table.
    int dollar_col = cols - 1;

    // FOLLOW(start) = $
    (*out_follow)[0 * cols + dollar_col] = true;

    bool changed = true; 

	// until no production adds new symbols to any FOLLOW set
    while (changed)
    {
        changed = false;

        for (int p = 0; p < g->num_productions; p++) // for every production A -> x1 x2 ... xn
        {
			// get production and its non-terminal A
            production prod = g->productions[p]; 
            int A = prod.non_terminal_id;

            for (int i = 0; i < prod.production_length; i++) // for every xi
            {
				// if xi is a non-terminal B
                int sym = prod.production_symbol_ids[i];
				// if it's a terminal, skip
                if (sym < g->num_terminals)
                    continue;
				// it's a non-terminal, get its index B
                int B = sym - g->num_terminals;

                bool beta_nullable = true;

				// for every symbol xj in beta = xi+1 ... xn
                for (int j = i + 1; j < prod.production_length; j++)
                {
					// get the next symbol after B
                    int next = prod.production_symbol_ids[j];
					// if next is a terminal
                    if (next < g->num_terminals)
                    {// if next isn't epsilon and isn't already in follow[B], add it to follow[B]
                        if (next != epsilon_id &&
                            !(*out_follow)[B * cols + next])
                        {// add it to follow[B]
                            (*out_follow)[B * cols + next] = true;
                            changed = true;
                        }

                        beta_nullable = false;
                        break;
                    }
                    else // next is a non-terminal
                    {
						// get next's index Xi
                        int Xi = next - g->num_terminals;
						// for every terminal t in first[xi]
                        for (int t = 0; t < g->num_terminals; t++)
                        {
                            if (t == epsilon_id)
                                continue;
							// if t is in first[xi] and isn't already in follow[B], add it to follow[B]
                            if (first_table[Xi * g->num_terminals + t] &&
                                !(*out_follow)[B * cols + t])
                            {
                                (*out_follow)[B * cols + t] = true;
                                changed = true;
                            }
                        }
						// if xi isn't nullable, stop processing beta
                        if (!nullable[Xi])
                        {
                            beta_nullable = false;
                            break;
                        }
                    }
                }

                if (beta_nullable)
                {
                    for (int t = 0; t < cols; t++)
                    {	// if t is in follow[A] and isn't already in follow[B], add it to follow[B]
                        if ((*out_follow)[A * cols + t] &&
                            !(*out_follow)[B * cols + t])
                        {
                            (*out_follow)[B * cols + t] = true;
                            changed = true;
                        }
                    }
                }
            }
        }
    }

    return true;
}

/**
 * @brief Collects FIRST symbols for one non-terminal from the computed table.
 * @param g Parsed grammar.
 * @param non_terminal_id Non-terminal index.
 * @param first_table FIRST table.
 * @param nullable Nullable flags.
 * @param epsilon_id Terminal id for "epsilon", or -1.
 * @param out_first Output array with FIRST symbols.
 * @return Number of collected symbols, or 0 on error.
 */
static int collect_first_for_non_terminal(
	const grammar *g,
	int non_terminal_id,
	const bool *first_table,
	const bool *nullable,
	int epsilon_id,
	symbol **out_first)
{
	// validate input
	if(!g || !first_table || !nullable || !out_first || non_terminal_id < 0 || non_terminal_id >= g->num_non_terminals)
		return 0;

	int count = 0;
	// for every terminal t, if t in first[non_terminal_id], add it to out_first
	for (int t = 0; t < g->num_terminals; t++)
	{// if t is in first[non_terminal_id]
		if (first_table[non_terminal_id * g->num_terminals + t])
		{ // add it to out_first
			if (!add_symbol_to_array(out_first, &count, g->terminals[t].symbol, true))
			{ // on allocation failure, free the collected symbols and return 0
				free_symbol_array(*out_first, count);
				*out_first = NULL;
				return 0;
			}
		}
	}
	// if non_terminal_id is nullable and epsilon is in the grammar, add epsilon to out_first
	if (nullable[non_terminal_id] && epsilon_id != -1)
	{
		if (!add_symbol_to_array(out_first, &count, g->terminals[epsilon_id].symbol, true))
		{ // on allocation failure, free the collected symbols and return 0
			free_symbol_array(*out_first, count);
			*out_first = NULL;
			return 0;
		}
	}
	// return the number of collected symbols
	return count;
}

/**
 * @brief Collects FOLLOW symbols for one non-terminal from the computed table.
 * @param g Parsed grammar.
 * @param non_terminal_id Non-terminal index.
 * @param follow_table FOLLOW table.
 * @param follow_cols Number of columns in follow_table.
 * @param out_follow Output array with FOLLOW symbols.
 * @return Number of collected symbols, or 0 on error.
 */
static int collect_follow_for_non_terminal(
	const grammar *g,
	int non_terminal_id,
	const bool *follow_table,
	int follow_cols,
	symbol **out_follow)
{
	// validate input
	if(!g || !follow_table || !out_follow || non_terminal_id < 0 || non_terminal_id >= g->num_non_terminals)
		return 0;

	int count = 0;
	// for every terminal t, if t in follow[non_terminal_id], add it to out_follow
	for (int t = 0; t < follow_cols; t++)
	{// if t is in follow[non_terminal_id]
		if (follow_table[non_terminal_id * follow_cols + t])
		{ // add it to out_follow
			const char *symbol_name;
			bool is_terminal;
			if (t == follow_cols - 1)
			{ // it's the end marker $
				symbol_name = "$";
				is_terminal = true;
			}
			else
			{ // it's a terminal from the grammar
				symbol_name = g->terminals[t].symbol;
				is_terminal = true;
			}

			if (!add_symbol_to_array(out_follow, &count, symbol_name, is_terminal))
			{ // on allocation failure, free the collected symbols and return 0
				free_symbol_array(*out_follow, count);
				*out_follow = NULL;
				return 0;
			}
		}
	}
	// return the number of collected symbols
	return count;
}

/**
 * @brief Computes FIRST set for one non-terminal by index.
 * @param g Parsed grammar.
 * @param non_terminal_id Non-terminal index in g->non_terminals.
 * @param out_first Output array with FIRST symbols.
 * @return Number of symbols in out_first, or 0 on error.
 */
int compute_first_for_non_terminal(const grammar *g, int non_terminal_id, symbol **out_first)
{
	// TODO: Validate inputs, compute shared FIRST tables, and collect FIRST for the requested non-terminal.
}

/**
 * @brief Computes FOLLOW set for one non-terminal by index.
 * @param g Parsed grammar.
 * @param non_terminal_id Non-terminal index in g->non_terminals.
 * @param out_follow Output array with FOLLOW symbols.
 * @return Number of symbols in out_follow, or 0 on error.
 */
int compute_follow_for_non_terminal(const grammar *g, int non_terminal_id, symbol **out_follow)
{
	// TODO: Validate inputs, compute FIRST/nullable and FOLLOW tables, then collect FOLLOW for the target non-terminal.
}

/**
 * @brief Computes FIRST set for the start symbol.
 * @param g Parsed grammar.
 * @param out_first Output array with FIRST(start) terminals.
 * @return Number of symbols in out_first, or 0 on error.
 */
int compute_first_for_start_symbol(const grammar *g, symbol **out_first)
{
	// TODO: Delegate FIRST computation to the generic non-terminal function using the start-symbol index.
}

/**
 * @brief Computes FOLLOW set for the start symbol.
 * @param g Parsed grammar.
 * @param out_follow Output array with FOLLOW(start) terminals.
 * @return Number of symbols in out_follow, or 0 on error.
 */
int compute_follow_for_start_symbol(const grammar *g, symbol **out_follow)
{
	// TODO: Delegate FOLLOW computation to the generic non-terminal function using the start-symbol index.
}

/**
 * @brief Frees a symbol array and each duplicated symbol string.
 * @param symbols Symbol array to release.
 * @param count Number of initialized entries.
 * @return This function does not return a value.
 */
void free_symbol_array(symbol *symbols, int count){
	if (!symbols)
    return;

	for (int i = 0; i < count; i++)
	{
    	free(symbols[i].symbol);
	}

	free(symbols);
}
