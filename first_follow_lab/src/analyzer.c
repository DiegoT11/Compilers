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
	if (arr == NULL || count == NULL || text == NULL)
    {
        return false;
    }

    symbol *new_array = (symbol *)realloc(*arr, (*count + 1) * sizeof(symbol));
    if (new_array == NULL)
    {
        return false;
    }

    *arr = new_array;

    char *duplicate_text = strdup(text);
    if (duplicate_text == NULL)
    {
        return false;
    }

    (*arr)[*count].symbol = duplicate_text;
    (*arr)[*count].symbol_length = (int)strlen(text);
    (*arr)[*count].is_terminal = is_terminal;

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
	if (g == NULL || first_table == NULL || nullable == NULL || epsilon_id == NULL)
    {
        return false;
    }

    *epsilon_id = find_terminal_id(g, "epsilon");

    *first_table = (bool *)calloc(g->num_non_terminals * g->num_terminals, sizeof(bool));
    if (*first_table == NULL)
    {
        return false;
    }

    *nullable = (bool *)calloc(g->num_non_terminals, sizeof(bool));
    if (*nullable == NULL)
    {
        free(*first_table);
        return false;
    }

    bool changed = true;
    while (changed)
    {
        changed = false;

        for (int p = 0; p < g->num_productions; p++)
        {
            production prod = g->productions[p];
            int A = prod.non_terminal_id;

            bool all_nullable = true;

            for (int j = 0; j < prod.production_length; j++)
            {
                int sym_id = prod.production_symbol_ids[j];
                bool is_terminal = sym_id < g->num_terminals;

                if (es_terminal)
                {
                    if (!(*first_table)[A * g->num_terminals + sym_id])
                    {
                        (*first_table)[A * g->num_terminals + sym_id] = true;
                        changed = true;
                    }
                    all_nullable = false;
                    break;
                }
                else
                {

                    int Xi = sym_id - g->num_terminals;

                    for (int t = 0; t < g->num_terminals; t++)
                    {
                        if (t == *epsilon_id)
                        {
                            continue;
                        }
                        if ((*first_table)[Xi * g->num_terminals + t] &&
                            !(*first_table)[A  * g->num_terminals + t])
                        {
                            (*first_table)[A * g->num_terminals + t] = true;
                            changed = true;
                        }
                    }

                    if (!(*nullable)[Xi])
                    {
                        all_nullable = false;
                        break;
                    }
                }
            }

            if (all_nullable && !(*nullable)[A])
            {
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
	// TODO: Allocate FOLLOW structures and propagate FOLLOW sets right-to-left until convergence.
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
	// TODO: Read one FIRST row, append terminal symbols, and include epsilon when nullable.
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
	// TODO: Read one FOLLOW row, append terminal symbols, and include end marker '$' when present.
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
