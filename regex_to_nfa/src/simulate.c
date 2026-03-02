#include "simulate.h"
#include "nfa.h"

bool match_nfa(nfa automaton, const char *input, size_t input_length)
{
    uint64_t current = automaton.epsilon_closure_cache[automaton.start_state];

    for (size_t i = 0; i < input_length; i++)
    {
        char c = input[i];
        int col = automaton.nfa_alphabet.char_to_col[(unsigned char)c];
        if (col < 0)
            return false;

        uint64_t next = 0;

        for (uint8_t s = 0; s < automaton.states; s++)
        {
            if (current & (1ULL << s))
            {
                next |= automaton.transitions[s][col];
            }
        }

        uint64_t expanded = 0;
        for (uint8_t s = 0; s < automaton.states; s++)
        {
            if (next & (1ULL << s))
                expanded |= automaton.epsilon_closure_cache[s];
        }

        current = expanded;
        if (!current)
            return false;
    }

    return (current & automaton.accept_states) != 0;
}