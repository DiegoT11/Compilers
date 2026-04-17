#include "trace.h"
#include "scanner.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// external lexer symbols
extern int   yylex(void);
extern char *yytext;

/**
 * @brief Stack structure used exclusively for the traced parser execution.
 *
 * This structure replicates the behavior of the parser stack from the
 * original implementation but is defined locally to avoid coupling
 * with internal parser logic.
 * It was introduced to allow controlled inspection and printing of
 * stack states at each parsing step.
 */
typedef struct
{
    int *states;
    int  size;
    int  capacity;
} trace_stack;

/**
 * @brief Initializes the trace stack.
 * @param s Pointer to the stack structure.
 * @return true on success, false on memory allocation failure.
 *
 * Initializes the stack with an initial capacity and pushes
 * the initial parser state (state 0).
 */
static bool ts_init(trace_stack *s)
{
    s->capacity = 64;
    s->size     = 0;
    s->states   = (int *)malloc((size_t)s->capacity * sizeof(int));
    if (!s->states) { s->capacity = 0; return false; }
    s->states[s->size++] = 0;  
    return true;
}

/**
 * @brief Frees all memory associated with the trace stack.
 * @param s Pointer to the stack structure.
 *
 * Releases allocated memory and resets the stack fields.
 */
static void ts_free(trace_stack *s)
{
    free(s->states);
    s->states   = NULL;
    s->size     = s->capacity = 0;
}

/**
 * @brief Pushes a new state onto the stack.
 * @param s Pointer to the stack structure.
 * @param state Parser state to push.
 * @return true on success, 
 *         false on allocation failure.
 *
 * Automatically resizes the stack when capacity is exceeded.
 */
static bool ts_push(trace_stack *s, int state)
{
    if (s->size >= s->capacity)
    {
        int  nc  = s->capacity * 2;
        int *buf = (int *)realloc(s->states, (size_t)nc * sizeof(int));
        if (!buf) return false;
        s->states   = buf;
        s->capacity = nc;
    }
    s->states[s->size++] = state;
    return true;
}

/**
 * @brief Pops multiple states from the stack.
 * @param s Pointer to the stack structure.
 * @param count Number of states to remove.
 * @return true on success, false if the operation is invalid.
 *
 * Used during reduce actions to remove RHS symbols.
 */
static bool ts_pop(trace_stack *s, int count)
{
    if (!s || count < 0 || s->size - count < 0) return false;
    s->size -= count;
    return true;
}

/**
 * @brief Returns the top state of the stack.
 * @param s Pointer to the stack structure.
 * @return Top state, or -1 if the stack is empty.
 */
static int ts_top(const trace_stack *s)
{
    return (s && s->size > 0) ? s->states[s->size - 1] : -1;
}

/**
 * @brief Token representation used during traced parsing.
 *
 * This structure stores the lexer token, its corresponding grammar
 * terminal id, and the lexeme string.
 * It mirrors the original token_stream structure but is defined locally
 * to keep the trace module self-contained.
 */
typedef struct
{
    int         lexer_token;
    int         terminal_id;
    const char *lexeme;
} ts_token;

/**
 * @brief Maps a symbol name to its terminal id (local version).
 * @param g Parsed grammar.
 * @param name Symbol name.
 * @return Terminal id or -1 if not found.
 *
 * This function duplicates the behavior of the original lookup
 * helper but is defined locally to avoid dependencies with main.c.
 */
static int find_terminal_id_local(const grammar *g, const char *name)
{
    if (!g || !name) return -1;
    for (int i = 0; i < g->num_terminals; i++)
        if (strcmp(g->terminals[i].symbol, name) == 0)
            return i;
    return -1;
}

/**
 * @brief Maps lexer token ids to grammar terminal ids.
 * @param g Parsed grammar.
 * @param lexer_token Token returned by yylex().
 * @param lexeme Lexeme text from yytext.
 * @return Terminal id, g->num_terminals for EOF, or -1 if unmapped.
 *
 * This function was introduced as a shared utility between main.c and
 * the trace module to unify how lexer tokens are interpreted by the parser.
 */
int map_lexer_token_to_terminal_id(const grammar *g, int lexer_token, const char *lexeme)
{
    if (!g || !lexeme) return -1;

    if (lexer_token == TOK_EOF)
        return g->num_terminals;

    if (lexer_token == TOK_ERROR)
        return -1;

    // lexeme match keywords and punctuation 
    int t = find_terminal_id_local(g, lexeme);
    if (t >= 0) return t;

    // Class-based fallbacks
    switch (lexer_token)
    {
    case TOK_IDENTIFIER:
        t = find_terminal_id_local(g, "IDENTIFIER");
        if (t < 0) t = find_terminal_id_local(g, "ID");
        if (t < 0) t = find_terminal_id_local(g, "id");
        return t;
    case TOK_INT_LITERAL:
        t = find_terminal_id_local(g, "INT_LITERAL");
        if (t < 0) t = find_terminal_id_local(g, "INT");
        if (t < 0) t = find_terminal_id_local(g, "num");
        return t;
    case TOK_FLOAT_LITERAL:
        t = find_terminal_id_local(g, "FLOAT_LITERAL");
        if (t < 0) t = find_terminal_id_local(g, "FLOAT");
        if (t < 0) t = find_terminal_id_local(g, "num");
        return t;
    case TOK_STRING_LITERAL:
        t = find_terminal_id_local(g, "STRING_LITERAL");
        if (t < 0) t = find_terminal_id_local(g, "STRING");
        if (t < 0) t = find_terminal_id_local(g, "str");
        return t;
    case TOK_CHAR_LITERAL:
        t = find_terminal_id_local(g, "CHAR_LITERAL");
        if (t < 0) t = find_terminal_id_local(g, "CHAR");
        if (t < 0) t = find_terminal_id_local(g, "char_lit");
        return t;
    default:
        break;
    }

    // single-character ASCII punctuation as char code
    if (lexer_token > 0 && lexer_token <= 127)
    {
        char one_char[2] = {(char)lexer_token, '\0'};
        t = find_terminal_id_local(g, one_char);
        if (t >= 0) return t;
    }

    return -1;
}

/**
 * @brief Retrieves the next token from the lexer and maps it to a terminal id.
 * @param g Parsed grammar.
 * @param out Output token structure.
 * @return true if the token is valid, false if it cannot be mapped.
 *
 * Encapsulates the interaction with the lexer and converts its output into the internal
 * token representation used by the traced parser.
 *
 * Simplifies the main parsing loop in trace mode and
 * to ensure consistent token handling.
 */
static bool next_tok(const grammar *g, ts_token *out)
{
    int         lt = yylex();
    const char *lx = yytext ? yytext : "";
    int        tid = map_lexer_token_to_terminal_id(g, lt, lx);

    out->lexer_token = lt;
    out->terminal_id = tid;
    out->lexeme      = lx;

    return tid >= 0;
}

/**
 * @brief Computes the number of stack elements to pop during a reduction.
 * @param g Parsed grammar.
 * @param p Production rule.
 * @return Number of symbols to remove from the stack.
 *
 * This function calculates how many symbols should be removed when
 * applying a reduce action, ignoring epsilon productions.
 *
 * Reduces the logic making explicit and easier
 * to trace, instead of putting this logic directly in the parser loop.
 */
static int pop_count_for(const grammar *g, production p)
{
    int eps = find_terminal_id_local(g, "epsilon");
    int n   = 0;
    for (int i = 0; i < p.production_length; i++)
        if (p.production_symbol_ids[i] != eps)
            n++;
    return n;
}

/**
 * @brief Returns the symbol name for a given encoded symbol id.
 * @param g Parsed grammar.
 * @param encoded Encoded symbol id (terminal or non-terminal).
 * @return Symbol name or "?" if invalid.
 *
 * Resolves whether the id corresponds to a terminal or
 * non-terminal and retrieves its string representation.
 *
 * Makes the trace output more human-readable.
 */
static const char *sym_name(const grammar *g, int encoded)
{
    if (!g) return "?";
    if (encoded >= 0 && encoded < g->num_terminals)
        return g->terminals[encoded].symbol;
    int nt = encoded - g->num_terminals;
    if (nt >= 0 && nt < g->num_non_terminals)
        return g->non_terminals[nt].symbol;
    return "?";
}

/**
 * @brief Returns the display name for a lookahead token.
 * @param g Parsed grammar.
 * @param tid Terminal id.
 * @return Symbol name or "$" if EOF.
 *
 * This function extends sym_name by handling the special case of EOF,
 * which is represented as "$" in parsing traces.
 *
 * It was introduced to improve clarity in trace outputs.
 */
static const char *la_name(const grammar *g, int tid)
{
    if (!g) return "?";
    if (tid == g->num_terminals) return "$";
    return sym_name(g, tid);
}

// Column widths
#define COL_STEP    5
#define COL_STACK   30
#define COL_TOKEN   12
#define COL_ACTION  20

/**
 * @brief Prints the current parser stack.
 * @param out Output stream.
 * @param s Pointer to the trace stack.
 */
static void print_stack(FILE *out, const trace_stack *s)
{
    fputc('[', out);
    for (int i = 0; i < s->size; i++)
    {
        if (i) fputc(' ', out);
        fprintf(out, "%d", s->states[i]);
    }
    fputc(']', out);
}

/**
 * @brief Computes the printed length of the stack representation.
 * @param s Pointer to the trace stack.
 * @return Length in characters of the formatted stack.
 *
 * This helper is used to align columns in the trace output,
 * ensuring consistent formatting across steps.
 */
static int stack_str_len(const trace_stack *s)
{
    int len = 2; /* [ ] */
    for (int i = 0; i < s->size; i++)
    {
        if (i) len++;
        int v = s->states[i];
        do { len++; v /= 10; } while (v);
    }
    return len;
}

/**
 * @brief Formats a production rule into a human-readable string.
 * @param g Parsed grammar.
 * @param prod_idx Production index.
 * @param buf Output buffer.
 * @param bufsz Buffer size.
 *
 * Builds a string representation such as "A -> B C D" or "A -> ε".
 *
 * This function was introduced to display reductions clearly
 * during traced parsing, improving readability and debugging.
 */
static void sprint_production(const grammar *g, int prod_idx, char *buf, int bufsz)
{
    if (prod_idx < 0 || prod_idx >= g->num_productions)
    {
        snprintf(buf, (size_t)bufsz, "(unknown rule %d)", prod_idx);
        return;
    }
    production p   = g->productions[prod_idx];
    const char *lhs = g->non_terminals[p.non_terminal_id].symbol;

    int written = snprintf(buf, (size_t)bufsz, "%s ->", lhs);
    if (p.production_length == 0)
    {
        snprintf(buf + written, (size_t)(bufsz - written), " ε");
        return;
    }
    for (int i = 0; i < p.production_length && written < bufsz - 1; i++)
    {
        written += snprintf(buf + written, (size_t)(bufsz - written),
                            " %s", sym_name(g, p.production_symbol_ids[i]));
    }
}

/**
 * @brief Executes the LALR(1) parsing algorithm with step-by-step tracing.
 * @param g Parsed grammar.
 * @param table Parser ACTION/GOTO table.
 * @param out Output stream for trace logging.
 * @return true if input is accepted, false otherwise.
 *
 * This function behaves like the original parser but prints each step:
 * - Current stack
 * - Lookahead token
 * - Selected action (shift/reduce/accept/error)
 */
bool parse_with_trace(const grammar *g, const parser_table *table, FILE *out)
{
    if (!g || !table || !out) return false;

    // print table header
    fprintf(out, "\n");
    fprintf(out, "%-*s | %-*s | %-*s | %-*s | %s\n",
            COL_STEP,   "Step",
            COL_STACK,  "Stack",
            COL_TOKEN,  "Token",
            COL_ACTION, "Action",
            "Rule");

    // separator line
    for (int i = 0; i < COL_STEP;   i++) fputc('-', out);
    fprintf(out, "-+-");
    for (int i = 0; i < COL_STACK;  i++) fputc('-', out);
    fprintf(out, "-+-");
    for (int i = 0; i < COL_TOKEN;  i++) fputc('-', out);
    fprintf(out, "-+-");
    for (int i = 0; i < COL_ACTION; i++) fputc('-', out);
    fprintf(out, "-+-%s\n", "-----------------------------");

    // initialise stack & first token
    trace_stack stk;
    if (!ts_init(&stk)) return false;

    ts_token la;
    if (!next_tok(g, &la))
    {
        fprintf(stderr, "[trace] lexer error: cannot map '%s' (tok=%d)\n",
                yytext ? yytext : "", la.lexer_token);
        ts_free(&stk);
        return false;
    }

    int   step   = 0;
    bool  result = false;
    char  rule_buf[128];

    // main loop
    while (true)
    {
        step++;
        int          state  = ts_top(&stk);
        parser_action act   = get_parser_action(table, state, la.terminal_id);

        // build column strings
        char action_buf[64];

        switch (act.type)
        {
        case PARSER_ACTION_SHIFT:
            snprintf(action_buf, sizeof(action_buf), "shift  -> s%d", act.value);
            rule_buf[0] = '\0';
            break;

        case PARSER_ACTION_REDUCE:
            snprintf(action_buf, sizeof(action_buf), "reduce -> r%d", act.value);
            sprint_production(g, act.value, rule_buf, (int)sizeof(rule_buf));
            break;

        case PARSER_ACTION_ACCEPT:
            snprintf(action_buf, sizeof(action_buf), "accept");
            rule_buf[0] = '\0';
            break;

        default:
            snprintf(action_buf, sizeof(action_buf), "ERROR");
            rule_buf[0] = '\0';
            break;
        }

        //print the step row
        fprintf(out, "%*d | ", COL_STEP, step);

        // Stack column (left-aligned, padded manually) 
        int slen = stack_str_len(&stk);
        print_stack(out, &stk);
        for (int i = slen; i < COL_STACK; i++) fputc(' ', out);

        // Token, Action, Rule columns
        fprintf(out, " | %-*s | %-*s | %s\n",
                COL_TOKEN, la_name(g, la.terminal_id),
                COL_ACTION, action_buf,
                rule_buf);

        //execute action
        if (act.type == PARSER_ACTION_SHIFT)
        {
            if (!ts_push(&stk, act.value))
            {
                fprintf(stderr, "[trace] stack overflow on shift.\n");
                ts_free(&stk);
                return false;
            }
            if (!next_tok(g, &la))
            {
                fprintf(stderr, "[trace] lexer error: cannot map '%s' (tok=%d)\n",
                        yytext ? yytext : "", la.lexer_token);
                ts_free(&stk);
                return false;
            }
        }
        else if (act.type == PARSER_ACTION_REDUCE)
        {
            if (act.value < 0 || act.value >= g->num_productions)
            {
                fprintf(stderr, "[trace] invalid production index %d.\n", act.value);
                ts_free(&stk);
                return false;
            }
            production p   = g->productions[act.value];
            int        pop = pop_count_for(g, p);

            if (!ts_pop(&stk, pop))
            {
                fprintf(stderr, "[trace] stack underflow on reduce p%d.\n", act.value);
                ts_free(&stk);
                return false;
            }

            int goto_from  = ts_top(&stk);
            int goto_state = get_parser_goto(table, goto_from, p.non_terminal_id);
            if (goto_state < 0)
            {
                fprintf(stderr, "[trace] missing GOTO[%d, %s].\n",
                        goto_from, g->non_terminals[p.non_terminal_id].symbol);
                ts_free(&stk);
                return false;
            }
            if (!ts_push(&stk, goto_state))
            {
                fprintf(stderr, "[trace] stack overflow after reduce.\n");
                ts_free(&stk);
                return false;
            }
        }
        else if (act.type == PARSER_ACTION_ACCEPT)
        {
            result = true;
            break;
        }
        else
        {
            fprintf(stderr,
                    "[trace] syntax error at '%s' (terminal=%d) in state %d.\n",
                    la.lexeme, la.terminal_id, state);
            result = false;
            break;
        }
    } 

    // footer
    fprintf(out, "\n%s\n", result ? "Input accepted." : "Input rejected.");

    ts_free(&stk);
    return result;
}