#ifndef REGEX_H
#define REGEX_H

// Operators symbols
/* Concatenation symbol. Ej. a.b for ab */
#define CONCATENATION_SYMBOL '.'
/* Kleene star symbol. Ej. a* for empty string, a, aa, aaa, etc. */
#define KLEENE_STAR_SYMBOL '*'
/* Alternation symbol. Ej. a|b for a or b */
#define ALTERNATION_SYMBOL '|'
/* Positive closure symbol. Ej. a+ for a, aa, aaa, etc. */
#define POSITIVE_CLOSURE_SYMBOL '+'
/* Optional symbol. Ej. a? for empty string or a */
#define OPTIONAL_SYMBOL '?'

// Hierarchy symbols
#define LEFT_PARENTHESIS_SYMBOL '('
#define RIGHT_PARENTHESIS_SYMBOL ')'

// Special symbols
/** Epsilon symbol. Represents the empty string. Using 240 because it's not a common
 * character in regex and is outside the standard ASCII range */
#define EPSILON_SYMBOL 240
/* Escape symbol. Used to escape special characters. Ej. \* for literal asterisk */
#define ESCAPE_SYMBOL '\\'

/**
 * @brief Enum to represent the different types of operators
 * in a regular expression. The values are assigned based on
 * their precedence, with lower values having higher precedence.
 */
typedef enum Item_Type
{
    ALTERNATION,
    CONCATENATION,
    OPTIONAL,
    POSITIVE_CLOSURE,
    KLEENE_STAR,
    OPERAND,
    L_PARENTHESIS,
    R_PARENTHESIS,
} item_type;

/**
 * @brief Struct to represent an item in the regex. It can be an operator or a symbol.
 * The value field represents the character of the item, and the type field
 * indicates the type of the item, which can be an operator or an operand.
 */
typedef struct Item
{
    char value;
    item_type type;
} item;

/**
 * @brief Struct to represent a regular expression (postfix or infix sequence)
 */
typedef struct Regex
{
    int size;
    item *items;
} regex;

/**
 * @brief Function to parse a regular expression string and convert it into a regex struct.
 *
 * @param regex_str The input regular expression as a string
 * @return A regex struct containing the size of the postfix items and the array of postfix items
 */
regex parse_regex(const char *regex_str);

/**
 * @brief Helper function to determine the type of an item based on its character value.
 *
 * @param c The character to check
 * @return An item_type corresponding to the character, or OPERAND if it's not an operator
 */
item_type get_item_type(char c);

/*
 * @brief Constructor to create a new item with the given value and operator status.
 *
 * @param value The character value of the item
 * @param type An item_type indicating the type of the operator
 * @return An item struct with the specified value and operator status
 */
item new_item(char value, item_type type);

/**
 * @brief Function to free the memory allocated for a regex struct, including its items array.
 * This function should be called when the regex struct is no longer needed to avoid memory leaks.

 * @param r The regex struct to free
 */
void free_regex(regex r);

#endif