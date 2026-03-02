#include "thompson.h"
#include "nfa.h"

#define EPSILON_COL 0

// Internal building blocks for NFAs corresponding to regex constructs

/**
 * @brief Function to create a new NFA that represents a single symbol. This function creates a new NFA
 * with a start state and an end state, and adds a transition from the start state to the end state on
 * the given symbol.
 *
 * @param m Pointer to the states_manager struct that manages the states and transitions
 * @param c The symbol for which the NFA should be created
 * @return A new NFA struct representing the given symbol
 */
static t_nfa symbol_nfa(states_manager *m, char c)
{
    uint8_t start  = new_state(m);
    uint8_t accept = new_state(m);

    if ((unsigned char)c == EPSILON_SYMBOL)
        c = EPSILON_COL;
    else
        c = add_symbol(&m->nfa_alphabet, c);

    add_transition(m, start, accept, c);

    t_nfa nfa = {
        .start  = start,
        .accept = accept
    };

    return nfa;
}

/**
 * @brief Function to create a new NFA that represents the concatenation of two NFAs. This function
 * takes two NFAs as input, creates a new NFA that has the start state of the first NFA and the end
 * state of the second NFA, and adds an epsilon transition from the end state of the first NFA to
 * the start state of the second NFA.
 *
 * @param m Pointer to the states_manager struct that manages the states and transitions
 * @param a Pointer to the first NFA
 * @param b Pointer to the second NFA
 * @return A new NFA struct representing the concatenation of the two input NFAs
 */
static t_nfa concat_nfa(states_manager *m, t_nfa *a, t_nfa *b)
{
    /* merge a.accept with b.start via epsilon */
    add_transition(m, a->accept, b->start, EPSILON_COL);

    t_nfa result;
    result.start  = a->start;
    result.accept = b->accept;
    return result;
}

/**
 * @brief Function to create a new NFA that represents the union of two NFAs. This function takes two NFAs
 * as input, creates a new NFA that has a new start state and a new end state, and adds epsilon transitions
 * from the new start state to the start states of both input NFAs, and from the end states of both input
 * NFAs to the new end state.
 *
 * @param m Pointer to the states_manager struct that manages the states and transitions
 * @param a Pointer to the first NFA
 * @param b Pointer to the second NFA
 * @return A new NFA struct representing the union of the two input NFAs
 */
static t_nfa union_nfa(states_manager *m, t_nfa *a, t_nfa *b)
{
    uint8_t s = new_state(m);
    uint8_t f = new_state(m);

    add_transition(m, s, a->start, EPSILON_COL);
    add_transition(m, s, b->start, EPSILON_COL);
    add_transition(m, a->accept, f, EPSILON_COL);
    add_transition(m, b->accept, f, EPSILON_COL);

    t_nfa result;
    result.start  = s;
    result.accept = f;
    return result;
}

/**
 * @brief Function to create a new NFA that represents the Kleene closure of an NFA. This function
 * takes an NFA as input, creates a new NFA that has a new start state and a new end state, and adds
 * epsilon transitions from the new start state to the start state of the input NFA, from
 * the end state of the input NFA to the start state of the input NFA, from the end state of the input
 * NFA to the new end state, and from the new start state to the new end state (to allow for the empty
 * string).
 *
 * @param m Pointer to the states_manager struct that manages the states and transitions
 * @param a Pointer to the input NFA
 * @return A new NFA struct representing the Kleene closure of the input NFA
 */
static t_nfa kleene_closure_nfa(states_manager *m, t_nfa *a)
{
    uint8_t s = new_state(m);
    uint8_t f = new_state(m);

    add_transition(m, s, a->start, EPSILON_COL);
    add_transition(m, a->accept, a->start, EPSILON_COL); /* loop */
    add_transition(m, a->accept, f, EPSILON_COL);
    add_transition(m, s, f, EPSILON_COL);                /* zero times */

    t_nfa result;
    result.start  = s;
    result.accept = f;
    return result;
}

/**
 * @brief Function to create a new NFA that represents the positive closure of an NFA.
 * This function takes an NFA as input, creates a new NFA that has a new start state and
 * a new end state, and adds epsilon transitions from the new start state to the start
 * state of the input NFA, from the end state of the input NFA to the start state of the
 * input NFA, and from the end state of the input NFA to the new end state.
 *
 * @param m Pointer to the states_manager struct that manages the states and transitions
 * @param a Pointer to the input NFA
 * @return A new NFA struct representing the positive closure of the input NFA
 */
static t_nfa positive_closure_nfa(states_manager *m, t_nfa *a)
{
    uint8_t s = new_state(m);
    uint8_t f = new_state(m);

    add_transition(m, s, a->start, EPSILON_COL);
    add_transition(m, a->accept, a->start, EPSILON_COL); /* loop */
    add_transition(m, a->accept, f, EPSILON_COL);

    t_nfa result;
    result.start  = s;
    result.accept = f;
    return result;
}

/**
 * @brief Function to create a new NFA that represents the optionality of an NFA. This function takes
 * an NFA as input, creates a new NFA that adds epsilon transitions from the start state of the input
 * NFA to the end state (to allow for the empty string).
 *
 * @param m Pointer to the states_manager struct that manages the states and transitions
 * @param a Pointer to the input NFA
 * @return A new NFA struct representing the optionality of the input NFA
 */
static t_nfa optional_nfa(states_manager *m, t_nfa *a)
{
    uint8_t s = new_state(m);
    uint8_t f = new_state(m);

    add_transition(m, s, a->start, EPSILON_COL);
    add_transition(m, s, f, EPSILON_COL);        /* zero times */
    add_transition(m, a->accept, f, EPSILON_COL);

    t_nfa result;
    result.start  = s;
    result.accept = f;
    return result;
}

// Main function to convert a regex to an NFA using Thompson's construction

nfa regex_to_nfa(const regex r)
{
    states_manager manager = new_states_manager();

    t_nfa stack[MAX_STATES];
    int top = 0;

    for (int i = 0; i < r.size; i++)
    {
        item it = r.items[i];

        if (it.type == OPERAND)
        {
            stack[top++] = symbol_nfa(&manager, it.value);
        }
        else if (it.type == CONCATENATION)
        {
            t_nfa b = stack[--top];
            t_nfa a = stack[--top];
            stack[top++] = concat_nfa(&manager, &a, &b);
        }
        else if (it.type == ALTERNATION)
        {
            t_nfa b = stack[--top];
            t_nfa a = stack[--top];
            stack[top++] = union_nfa(&manager, &a, &b);
        }
        else if (it.type == KLEENE_STAR)
        {
            t_nfa a = stack[--top];
            stack[top++] = kleene_closure_nfa(&manager, &a);
        }
        else if (it.type == POSITIVE_CLOSURE)
        {
            t_nfa a = stack[--top];
            stack[top++] = positive_closure_nfa(&manager, &a);
        }
        else if (it.type == OPTIONAL)
        {
            t_nfa a = stack[--top];
            stack[top++] = optional_nfa(&manager, &a);
        }
    }

    return t_nfa_to_nfa(stack[0], manager);
}