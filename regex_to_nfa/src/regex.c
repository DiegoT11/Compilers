#include "regex.h"
#include <stdlib.h>

/*
 * @brief Constructor to create a new item with the given value and operator status.
 *
 * @param value The character value of the item
 * @param type An item_type indicating the type of the operator
 * @return An item struct with the specified value and operator status
 */
item new_item(char value, item_type type)
{
    item it;
    it.value = value;
    it.type = type;
    return it;
}

/**
 * @brief Helper function to determine the type of an item based on its character value.
 *
 * @param c The character to check
 * @return An item_type corresponding to the character, or OPERAND if it's not an operator
 */
item_type get_item_type(char c)
{
    switch (c) {
        case ALTERNATION_SYMBOL: return ALTERNATION;
        case CONCATENATION_SYMBOL: return CONCATENATION;
        case OPTIONAL_SYMBOL: return OPTIONAL;
        case POSITIVE_CLOSURE_SYMBOL: return POSITIVE_CLOSURE;
        case KLEENE_STAR_SYMBOL: return KLEENE_STAR;
        case LEFT_PARENTHESIS_SYMBOL: return L_PARENTHESIS;
        case RIGHT_PARENTHESIS_SYMBOL: return R_PARENTHESIS;
        default: return OPERAND;
    }
}

/**
 * @brief Frees the memory associated with a regex struct.
 *
 * Releases the dynamically allocated array of items contained in the regex.
 * This function does not free the struct itself only the internal items buffer.
 *
 * @param r The regex whose internal memory will be released
 */
void free_regex(regex r)
{
    if (r.items)
        free(r.items);
}