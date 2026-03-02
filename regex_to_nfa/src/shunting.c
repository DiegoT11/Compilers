#include "shunting.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/**
 * @brief Returns the precedence level of a regex operator.
 *
 * Higher value = higher precedence in infix parsing.
 *
 * Precedence levels:
 *   3 → Unary postfix operators (*, +, ?)
 *   2 → Concatenation
 *   1 → Alternation (|)
 *   0 → Non-operators / operands
 *
 * @param t The item_type operator
 * @return Integer precedence level
 */
static int precedence(item_type t)
{
    switch (t)
    {
        case ALTERNATION:      return 1;
        case CONCATENATION:    return 2;
        case OPTIONAL:         return 3;
        case POSITIVE_CLOSURE: return 3;
        case KLEENE_STAR:      return 3;
        default:               return 0;
    }
}

// Step 1 — TOKENIZATION

/**
 * @brief Function to convert a regex string into an array of items.
 * This function iterates through the input regex string, creates items
 * for each character, and determines if they are operators or not.
 *
 * @param regex_str The input regular expression as a string
 * @param out_size A pointer to an integer where the size of the output array will be stored
 * @return An array of items representing the regex
 */
item *itemize_regex(const char *regex_str, int *out_size)
{
    if (!regex_str)
    {
        if (out_size) *out_size = 0;
        return NULL;
    }

    int len = (int)strlen(regex_str);

    item *items = malloc(sizeof(item) * (len + 1));
    if (!items)
    {
        if (out_size) *out_size = 0;
        return NULL;
    }

    int n = 0;

    for (int i = 0; i < len; i++)
    {
        char c = regex_str[i];

        if (c == ESCAPE_SYMBOL && i + 1 < len)
            items[n++] = new_item(regex_str[++i], OPERAND);
        else
            items[n++] = new_item(c, get_item_type(c));
    }

    if (out_size)
        *out_size = n;

    return items;
}

// Step 2 — IMPLICIT CONCATENATION TO EXPLICIT CONCATENATION

/**
 * @brief Determines if an item can appear before an implicit concatenation.
 *
 * Concatenation is needed after: operand, ')', or postfix unary operator
 */
static bool needs_concat_after(item_type t)
{
    return t == OPERAND ||
           t == KLEENE_STAR ||
           t == POSITIVE_CLOSURE ||
           t == OPTIONAL ||
           t == R_PARENTHESIS;
}

/**
 * @brief Determines if an item can appear after an implicit concatenation.
 *
 * Concatenation is needed before: operand or '('
 */
static bool needs_concat_before(item_type t)
{
    return t == OPERAND ||
           t == L_PARENTHESIS;
}

/**
 * @brief Inserts explicit CONCATENATION operators where needed.
 *
 * First it scan adjacent item pairs (i, i+1). If left can end expression AND right can start expression,
 * insert CONCATENATION operator between them.
 *
 * @param items Input item sequence (implicit concat)
 * @param size Number of input items
 * @param out_size Output number of items
 * @return New array with explicit concatenation
 */
static item *implicit_to_explicit_concatenation(const item *items, int size, int *out_size)
{
    /* worst case: a concat between every pair of tokens */
    item *out = malloc(sizeof(item) * (size * 2 + 1));
    int n = 0;

    for (int i = 0; i < size; i++)
    {
        out[n++] = items[i];

        /* Check if implicit concatenation exists between i and i+1 */
        if (i + 1 < size &&
            needs_concat_after(items[i].type) &&
            needs_concat_before(items[i + 1].type))
        {
            out[n++] = new_item(CONCATENATION_SYMBOL, CONCATENATION);
        }
    }

    *out_size = n;
    return out;
}

// Step 3 — SHUNTING-YARD ALGORITHM

/**
 * @brief Converts infix regex items into postfix notation.
 *
 * Implements Dijkstra's Shunting-Yard algorithm:
 *   - Operands go directly to output
 *   - Operators use precedence and stack
 *   - Parentheses control grouping
 *
 * @param items Infix item sequence
 * @param size Number of items
 * @param out_size Output size
 * @return Postfix item array
 */
static item *shunting_yard(const item *items, int size, int *out_size)
{
    item *output   = malloc(sizeof(item) * (size + 1));
    item *op_stack = malloc(sizeof(item) * (size + 1));

    int out_top = 0;
    int op_top  = 0;

    for (int i = 0; i < size; i++)
    {
        item it = items[i];

        switch (it.type)
        {
        case OPERAND:
            /* Operands go directly to output */
            output[out_top++] = it;
            break;

        case L_PARENTHESIS:
            /* Push '(' to stack */
            op_stack[op_top++] = it;
            break;

        case R_PARENTHESIS:
            /* Pop until matching '(' */
            while (op_top > 0 &&
                   op_stack[op_top - 1].type != L_PARENTHESIS)
            {
                output[out_top++] = op_stack[--op_top];
            }

            /* Discard '(' */
            if (op_top > 0)
                op_top--;
            break;

        default: /* Operator */
            /*
             * Pop operators with higher/equal precedence
             * (left-associative behavior)
             */
            while (op_top > 0 &&
                   op_stack[op_top - 1].type != L_PARENTHESIS &&
                   precedence(op_stack[op_top - 1].type) >= precedence(it.type))
            {
                output[out_top++] = op_stack[--op_top];
            }

            op_stack[op_top++] = it;
            break;
        }
    }

    /* Flush remaining operators */
    while (op_top > 0)
        output[out_top++] = op_stack[--op_top];

    free(op_stack);

    *out_size = out_top;
    return output;
}

regex parse_regex(const char *regex_str)
{
    int n1, n2, n3;

    /* Step 1 — tokenize */
    item *t1 = itemize_regex(regex_str, &n1);

    /* Step 2 — explicit concatenation */
    item *t2 = implicit_to_explicit_concatenation(t1, n1, &n2);
    free(t1);

    /* Step 3 — shunting yard */
    item *t3 = shunting_yard(t2, n2, &n3);
    free(t2);

    return (regex){ .items = t3, .size = n3 };
}