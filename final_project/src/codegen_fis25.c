#include "codegen_fis25.h"
#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FIS25_MAX_VARS   1024
#define FIS25_MAX_NAME   CODEGEN_MAX_OPERAND
#define FIS25_MAX_PARAMS 64

typedef struct {
    char nombres[FIS25_MAX_VARS][FIS25_MAX_NAME];
    int count;
} TablaVars;

typedef struct {
    char valores[FIS25_MAX_PARAMS][FIS25_MAX_NAME];
    int count;
} ColaParams;

static void tv_init(TablaVars* tv) {
    tv->count = 0;
}

static void cp_init(ColaParams* cp) {
    cp->count = 0;
}

static int es_literal(const char* tok) {
    if (!tok || tok[0] == '\0')
        return 1;

    if (tok[0] == '"')
        return 1;

    if (strcmp(tok, "verdadero") == 0 ||
        strcmp(tok, "falso") == 0)
        return 1;

    char* end;
    strtod(tok, &end);

    if (end != tok && *end == '\0')
        return 1;

    return 0;
}

static const char* valor_fis25(const char* tok) {
    if (!tok)
        return "";

    if (strcmp(tok, "verdadero") == 0)
        return "1";

    if (strcmp(tok, "falso") == 0)
        return "0";

    return tok;
}

static void tv_declarar(TablaVars* tv, const char* tok, FILE* out) {
    if (!tok || tok[0] == '\0')
        return;

    if (es_literal(tok))
        return;

    if (tok[0] == '&')
        return;

    if (strchr(tok, '['))
        return;

    for (int i = 0; i < tv->count; i++) {
        if (strcmp(tv->nombres[i], tok) == 0)
            return;
    }

    if (tv->count < FIS25_MAX_VARS) {
        strncpy(tv->nombres[tv->count], tok, FIS25_MAX_NAME - 1);
        tv->nombres[tv->count][FIS25_MAX_NAME - 1] = '\0';
        tv->count++;
    }

    fprintf(out, "VAR %s\n", tok);
}

static const char* op_a_fis25(const char* op) {
    if (strcmp(op, "+") == 0)  return "ADD";
    if (strcmp(op, "-") == 0)  return "SUB";
    if (strcmp(op, "*") == 0)  return "MUL";
    if (strcmp(op, "/") == 0)  return "DIV";
    if (strcmp(op, "==") == 0) return "EQ";
    if (strcmp(op, "!=") == 0) return "NEQ";
    if (strcmp(op, ">") == 0)  return "GT";
    if (strcmp(op, ">=") == 0) return "GTE";
    if (strcmp(op, "<") == 0)  return "LT";
    if (strcmp(op, "<=") == 0) return "LTE";
    if (strcmp(op, "&&") == 0) return "AND";
    if (strcmp(op, "||") == 0) return "OR";

    return NULL;
}

static void cp_push(ColaParams* cp, const char* val) {
    if (cp->count < FIS25_MAX_PARAMS) {
        strncpy(cp->valores[cp->count], valor_fis25(val), FIS25_MAX_NAME - 1);
        cp->valores[cp->count][FIS25_MAX_NAME - 1] = '\0';
        cp->count++;
    }
}

static void cp_flush_print(ColaParams* cp, FILE* out) {
    for (int i = 0; i < cp->count; i++)
        fprintf(out, "PRINT %s\n", cp->valores[i]);

    cp->count = 0;
}

static void cp_flush_param(ColaParams* cp, FILE* out) {
    for (int i = 0; i < cp->count; i++)
        fprintf(out, "PARAM %s\n", cp->valores[i]);

    cp->count = 0;
}

static void imprimir_fis25(
    const Instruccion* ins,
    TablaVars* tv,
    ColaParams* cp,
    FILE* out
) {
    switch (ins->op) {

        case OP_ASIGNAR:
            tv_declarar(tv, ins->dst, out);
            tv_declarar(tv, ins->src1, out);

            fprintf(out,
                    "ASSIGN %s %s\n",
                    valor_fis25(ins->src1),
                    ins->dst);
            break;

        case OP_BINARIA: {
            const char* fis_op = op_a_fis25(ins->extra);

            tv_declarar(tv, ins->dst, out);
            tv_declarar(tv, ins->src1, out);
            tv_declarar(tv, ins->src2, out);

            if (fis_op) {
                fprintf(out,
                        "%s %s %s %s\n",
                        fis_op,
                        valor_fis25(ins->src1),
                        valor_fis25(ins->src2),
                        ins->dst);
            } else {
                fprintf(out,
                        "// [op desconocido '%s'] %s = %s %s %s\n",
                        ins->extra,
                        ins->dst,
                        valor_fis25(ins->src1),
                        ins->extra,
                        valor_fis25(ins->src2));
            }

            break;
        }

        case OP_UNARIA:
            tv_declarar(tv, ins->dst, out);
            tv_declarar(tv, ins->src1, out);

            if (strcmp(ins->extra, "-") == 0) {
                fprintf(out,
                        "NEG %s %s\n",
                        valor_fis25(ins->src1),
                        ins->dst);
            } else if (strcmp(ins->extra, "!") == 0) {
                fprintf(out,
                        "NOT %s %s\n",
                        valor_fis25(ins->src1),
                        ins->dst);
            } else {
                fprintf(out,
                        "// [unaria desconocida '%s'] %s = %s%s\n",
                        ins->extra,
                        ins->dst,
                        ins->extra,
                        valor_fis25(ins->src1));
            }

            break;

        case OP_ARRAY_READ: {
            char ref[CODEGEN_MAX_OPERAND * 2 + 4];

            snprintf(ref,
                     sizeof(ref),
                     "%s[%s]",
                     ins->src1,
                     valor_fis25(ins->src2));

            tv_declarar(tv, ins->dst, out);
            tv_declarar(tv, ins->src1, out);
            tv_declarar(tv, ins->src2, out);

            fprintf(out,
                    "ASSIGN %s %s\n",
                    ref,
                    ins->dst);

            break;
        }

        case OP_ARRAY_WRITE: {
            char ref[CODEGEN_MAX_OPERAND * 2 + 4];

            snprintf(ref,
                     sizeof(ref),
                     "%s[%s]",
                     ins->dst,
                     valor_fis25(ins->src1));

            tv_declarar(tv, ins->dst, out);
            tv_declarar(tv, ins->src1, out);
            tv_declarar(tv, ins->src2, out);

            fprintf(out,
                    "ASSIGN %s %s\n",
                    valor_fis25(ins->src2),
                    ref);

            break;
        }

        case OP_LABEL:
            fprintf(out, "LABEL %s\n", ins->dst);
            break;

        case OP_GOTO:
            fprintf(out, "GOTO %s\n", ins->extra);
            break;

        case OP_IF_TRUE:
            tv_declarar(tv, ins->src1, out);

            fprintf(out,
                    "IF %s GOTO %s\n",
                    valor_fis25(ins->src1),
                    ins->extra);
            break;

        case OP_IF_FALSE:
            tv_declarar(tv, ins->src1, out);

            fprintf(out,
                    "IFFALSE %s GOTO %s\n",
                    valor_fis25(ins->src1),
                    ins->extra);
            break;

        case OP_PARAM:
            tv_declarar(tv, ins->src1, out);
            cp_push(cp, ins->src1);
            break;

        case OP_CALL:
            if (strcmp(ins->src1, "mostrar") == 0) {

                cp_flush_print(cp, out);

            } else if (strcmp(ins->src1, "leer") == 0) {

                const char* ref = ins->extra;

                if (ref[0] == '&')
                    ref++;

                char base[CODEGEN_MAX_OPERAND];

                strncpy(base, ref, sizeof(base) - 1);
                base[sizeof(base) - 1] = '\0';

                char* bracket = strchr(base, '[');

                if (bracket)
                    *bracket = '\0';

                tv_declarar(tv, base, out);

                fprintf(out, "INPUT %s\n", ref);

            } else {

                cp_flush_param(cp, out);

                if (ins->dst[0]) {
                    tv_declarar(tv, ins->dst, out);

                    fprintf(out,
                            "GOSUB %s\nASSIGN retval %s\n",
                            ins->src1,
                            ins->dst);
                } else {
                    fprintf(out,
                            "GOSUB %s\n",
                            ins->src1);
                }
            }

            break;

        case OP_CAST:
            tv_declarar(tv, ins->dst, out);
            tv_declarar(tv, ins->src1, out);

            fprintf(out,
                    "CAST %s %s %s\n",
                    ins->extra,
                    valor_fis25(ins->src1),
                    ins->dst);

            break;
    }
}

void gen_imprimir_fis25(const Generador* gen, FILE* out) {
    TablaVars tv;
    ColaParams cp;

    tv_init(&tv);
    cp_init(&cp);

    for (const Instruccion* ins = gen->cabeza;
         ins;
         ins = ins->siguiente) {
        imprimir_fis25(ins, &tv, &cp, out);
    }
}

void gen_guardar_fis25(const Generador* gen, const char* archivo) {
    FILE* out = fopen(archivo, "w");

    if (!out) {
        fprintf(stderr,
                "Error: no se pudo crear '%s'\n",
                archivo);
        return;
    }

    gen_imprimir_fis25(gen, out);

    fclose(out);

    printf("--- Codigo FIS-25 generado en: %s ---\n",
           archivo);
}