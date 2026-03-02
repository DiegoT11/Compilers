#ifndef NFA_H
#define NFA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>   
#include "regex.h"

#define MAX_STATES 64
#define MAX_TRANSITIONS (MAX_STATES * MAX_STATES)

/**
 * @brief Struct to represent an alphabet. It contains an array of symbols and a mapping from characters
 * to their corresponding column index in the symbols array. The symbol_count field keeps track of how many
 * unique symbols are in the alphabet.
 */
struct alphabet
{
    /* Array of symbols in the alphabet. The index of each
    symbol corresponds to its column in the transition table. */
    char symbols[256];
    /* Mapping from character to column index in the symbols array */
    int char_to_col[256];
    /* Number of symbols in the alphabet */
    int size;      
} alphabet;

/**
 * @brief Represents a temporary transition in an NFA under construction.
 * Each transition connects a source state to a destination state using
 * a symbol identified by its column index in the alphabet.
 */
typedef struct temp_transition
{
    /* Source state of the transition */
    uint8_t from;

    /* Destination state of the transition */
    uint8_t to;

    /* Column index of the symbol in the alphabet */
    uint8_t symbol_col;
} temp_transition;

/**
 * @brief Represents a minimal NFA fragment with a single entry and exit state.
 * Used during Thompson construction to compose larger NFAs from smaller ones.
 */
typedef struct temp_nfa
{
    /* Start (entry) state of the NFA fragment */
    uint8_t start;

    /* Accept (exit) state of the NFA fragment */
    uint8_t accept;
} t_nfa;

/**
 * @brief Manages the set of states and transitions of an NFA during construction.
 * It keeps track of how many states exist, the list of transitions created so far,
 * and the alphabet associated with the automaton.
 */
typedef struct states_manager
{
    /* Total number of states currently allocated in the NFA */
    uint8_t states;

    /* Array of transitions between states */
    temp_transition transitions[MAX_TRANSITIONS];

    /* Number of transitions stored in the transitions array */
    int transitions_count;

    /* Alphabet used by the NFA */
    alphabet nfa_alphabet;
} states_manager;

/**
 * @brief Struct to represent a non-deterministic finite automaton (NFA). It contains the start state,
 * a bitset representing the accept states, the total number of states, the alphabet used by the NFA,
 * a transition table, and a cache for epsilon closures. The transition table is a 2D array where each
 * entry is a bitset representing the set of states reachable from the current state on the given symbol.
 */
typedef struct NFA
{
     /* State id for the start state */
    uint8_t start_state;
    /* Bitset representing accept states */
    uint64_t accept_states;
    /* Number of states in the NFA */
    uint8_t states;
    /* Alphabet used by the NFA */
    alphabet nfa_alphabet;
    /** Transition table. Each entry is a bitset representing the set of
     * states reachable from the current state on the given symbol. A
     * MAX_STATES x nfa_alphabet.symbol_count matrix. */
    uint64_t transitions[MAX_STATES][MAX_STATES]; 
    /* Cache for epsilon closures. Each entry is a bitset representing
    the epsilon closure of the corresponding state. */
    uint64_t epsilon_closure_cache[MAX_STATES];  
} nfa;

// Public API

/**
 * @brief Function to create a new alphabet. This function initializes an alphabet struct with
 * default values, including setting the epsilon symbol and initializing the character-to-column
 * mapping.
 */
alphabet new_alphabet(void);

/**
 * @brief Function to add a symbol to the alphabet. This function checks if the symbol is already
 * in the alphabet, and if not, it adds the symbol to the symbols array and updates the
 * character-to-column mapping.
 *
 * @param a Pointer to the alphabet struct to which the symbol should be added
 * @param c The symbol to add to the alphabet
 * @return The column index of the added symbol in the alphabet
 */
uint8_t  add_symbol(alphabet *a, char c);

/**
 * @brief Struct to manage states and transitions during NFA construction. It keeps track
 * of the next available state ID, the list of states, the list of transitions, and the
 * alphabet used by the NFAs being constructed.
 */
states_manager new_states_manager(void);

/**
 * @brief Function to create a new state in the states manager. This function assigns a new state ID,
 * adds it to the list of states, and returns the new state ID.
 *
 * @param m Pointer to the states_manager struct that manages the states
 * @return The ID of the newly created state
 */
uint8_t new_state(states_manager *m);

/**
 * @brief Function to add a transition to the states manager. This function creates a new transition
 * struct and adds it to the list of transitions.
 * @param m Pointer to the states_manager struct that manages the states and transitions
 * @param from The state from which the transition originates
 * @param to The state to which the transition leads
 * @param col The column index of the symbol in the alphabet
 */
void add_transition(states_manager *m, uint8_t from, uint8_t to, uint8_t col);

/**
 * @brief Function to convert a temporary NFA representation (t_nfa) into the final NFA struct. This function
 * takes the start and end states from the temporary NFA, initializes the transition table based on the
 * transitions stored in the states manager, and calculates the epsilon closures for all states.
 *
 * @param t The temporary NFA representation containing the start and end states
 * @param m The states_manager struct that contains the transitions and alphabet information
 * @return An NFA struct representing the final non-deterministic finite automaton
 */
nfa t_nfa_to_nfa(t_nfa t, states_manager m);

nfa regex_to_nfa(const regex r);

/**
 * @brief Function to check if a given input string matches the language defined by the NFA.
 * This function simulates the NFA on the input string and returns true if the NFA accepts
 * the string, and false otherwise.
 * @param automaton The NFA to simulate
 * @param input The input string to check against the NFA
 * @param input_length The length of the input string
 * @return true if the NFA accepts the input string, false otherwise
 */
bool match_nfa(nfa automaton, const char *input, size_t input_length);

#endif