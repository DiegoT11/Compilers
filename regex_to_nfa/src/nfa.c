#include "nfa.h"
#include <stdlib.h>
#include <string.h>

#define EPSILON_COL 0

alphabet new_alphabet(void)
{
    alphabet a;
    memset(a.char_to_col, -1, sizeof(a.char_to_col));
    a.size = 1; /* column 0 = epsilon */
    return a;
}

uint8_t add_symbol(alphabet *a, char c)
{
    if (a->char_to_col[(unsigned char)c] >= 0)
        return (uint8_t)a->char_to_col[(unsigned char)c];

    uint8_t col = (uint8_t)a->size++;
    a->char_to_col[(unsigned char)c] = col;
    return col;
}

states_manager new_states_manager(void)
{
    states_manager m;
    m.states = 0;
    m.transitions_count = 0;
    m.nfa_alphabet = new_alphabet();
    return m;
}

uint8_t new_state(states_manager *m)
{
    return m->states++;
}

void add_transition(states_manager *m, uint8_t from, uint8_t to, uint8_t col)
{
    temp_transition t;
    t.from       = from;
    t.to         = to;
    t.symbol_col = col;
    m->transitions[m->transitions_count++] = t;
}

/**
 * @brief Function to compute the epsilon closure for a given state in the NFA.
 * This function uses a depth-first search approach to find all states reachable
 * from the given state via epsilon transitions.
 *
 * @param state The state for which the epsilon closure is to be computed
 * @param epsilon_transitions An array representing the epsilon transitions for each state
 */
static uint64_t calculate_epsilon_closure(uint8_t state, uint64_t epsilon_transitions[MAX_STATES])
{
    uint64_t closure = 0;
    uint64_t stack   = (1ULL << state);

    while (stack)
    {
        uint8_t s = (uint8_t)__builtin_ctzll(stack);
        stack &= ~(1ULL << s);

        if (closure & (1ULL << s))
            continue;

        closure |= (1ULL << s);
        stack   |= epsilon_transitions[s];
    }

    return closure;
}

/**
 * @brief Function to calculate the epsilon closure for all states in the given NFA.
 * This function initializes a cache to store the epsilon closures and computes the closure
 * for each state using a depth-first search approach.

 * @param automaton Pointer to the NFA for which epsilon closures are to be calculated
 */
static void epsilon_closure(nfa *automaton)
{
    uint64_t epsilon_transitions[MAX_STATES] = {0};

    for (uint8_t s = 0; s < automaton->states; s++)
        epsilon_transitions[s] = automaton->transitions[s][EPSILON_COL];

    for (uint8_t s = 0; s < automaton->states; s++)
        automaton->epsilon_closure_cache[s] =
            calculate_epsilon_closure(s, epsilon_transitions);
}

nfa t_nfa_to_nfa(t_nfa t, states_manager m)
{
    nfa automaton;

    automaton.states = m.states;
    automaton.start_state = t.start;
    automaton.accept_states = (1ULL << t.accept);
    automaton.nfa_alphabet = m.nfa_alphabet;

    memset(automaton.transitions, 0, sizeof(automaton.transitions));

    for (int i = 0; i < m.transitions_count; i++)
    {
        temp_transition tr = m.transitions[i];
        automaton.transitions[tr.from][tr.symbol_col] |= (1ULL << tr.to);
    }

    epsilon_closure(&automaton);

    return automaton;
}