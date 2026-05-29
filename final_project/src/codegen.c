#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Genera un nombre de temporal t0, t1, t2, ...
static void nuevo_temp(Generador* gen, char* buf, int bufsz) {
    snprintf(buf, bufsz, "t%d", gen->temp_count++);
}

// Genera una etiqueta: L0, L1, L2, …
static void nueva_etiqueta(Generador* gen, char* buf, int bufsz) {
    snprintf(buf, bufsz, "L%d", gen->label_count++);
}

// Añade una instrucción al final de la lista
static Instruccion* emit(
    Generador* gen,
    OpCode op,
    const char* dst,
    const char* src1,
    const char* src2,
    const char* extra
) {
    Instruccion* ins = calloc(1, sizeof(Instruccion));

    ins->op = op;

    if (dst) {
        strncpy(ins->dst, dst, CODEGEN_MAX_OPERAND - 1);
    }

    if (src1) {
        strncpy(ins->src1, src1, CODEGEN_MAX_OPERAND - 1);
    }

    if (src2) {
        strncpy(ins->src2, src2, CODEGEN_MAX_OPERAND - 1);
    }

    if (extra) {
        strncpy(ins->extra, extra, CODEGEN_MAX_OPERAND - 1);
    }

    if (!gen->cabeza) {
        gen->cabeza = ins;
        gen->cola = ins;
    } else {
        gen->cola->siguiente = ins;
        gen->cola = ins;
    }

    return ins;
}

/**
 * @brief Crea un nuevo generador de código.
 * @return Generador inicializado.
 */
Generador* gen_nuevo(void) {
    return calloc(1, sizeof(Generador));
}

/**
 * @brief Libera todas las instrucciones generadas.
 * @param gen Generador a liberar.
 */
void gen_liberar(Generador* gen) {
    if (!gen) {
        return;
    }

    Instruccion* ins = gen->cabeza;

    while (ins) {
        Instruccion* sig = ins->siguiente;

        free(ins);

        ins = sig;
    }

    free(gen);
}

static void gen_nodo(
    Generador* gen,
    const Node* n,
    char* resultado,
    int resbuf
);

static void gen_expr(
    Generador* gen,
    const Node* n,
    char* res,
    int ressz
) {
    if (!n) {
        snprintf(res, ressz, "?");
        return;
    }

    // Literales e identificadores se retornan tal cual
    if (strcmp(n->tipo, "LiteralEntero") == 0 ||
        strcmp(n->tipo, "LiteralFlotante") == 0 ||
        strcmp(n->tipo, "LiteralBooleano") == 0 ||
        strcmp(n->tipo, "LiteralCadena") == 0 ||
        strcmp(n->tipo, "Identificador") == 0) {
        strncpy(res, n->valor, ressz - 1);
        return;
    }

    // Conversión de tipos. gen_expr hijo, luego cast
    if (strcmp(n->tipo, "Conversion") == 0) {
        char src[CODEGEN_MAX_OPERAND];

        gen_expr(gen, n->hijos[0], src, sizeof(src));

        nuevo_temp(gen, res, ressz);

        emit(
            gen,
            OP_CAST,
            res,
            src,
            NULL,
            tipo_a_str(n->data_type)
        );

        return;
    }

    if (strcmp(n->tipo, "Operacion") == 0 &&
        n->num_hijos == 2) {
        char l[CODEGEN_MAX_OPERAND];
        char r[CODEGEN_MAX_OPERAND];

        gen_expr(gen, n->hijos[0], l, sizeof(l));
        gen_expr(gen, n->hijos[1], r, sizeof(r));

        nuevo_temp(gen, res, ressz);

        emit(gen, OP_BINARIA, res, l, r, n->valor);

        return;
    }

    // Operaciones unarias: gen_expr hijo, luego unaria
    if (strcmp(n->tipo, "Negativo") == 0 ||
        strcmp(n->tipo, "Negacion") == 0) {
        char src[CODEGEN_MAX_OPERAND];

        gen_expr(gen, n->hijos[0], src, sizeof(src));

        nuevo_temp(gen, res, ressz);

        emit(
            gen,
            OP_UNARIA,
            res,
            src,
            NULL,
            strcmp(n->tipo, "Negativo") == 0 ? "-" : "!"
        );

        return;
    }

    if (strcmp(n->tipo, "AccesoArreglo") == 0) {
        char idx[CODEGEN_MAX_OPERAND];

        gen_expr(gen, n->hijos[0]->hijos[0], idx, sizeof(idx));

        nuevo_temp(gen, res, ressz);

        emit(gen, OP_ARRAY_READ, res, n->valor, idx, NULL);

        return;
    }

    // Fallback: se genera el nodo recursivamente y se retorna un temporal con su resultado
    if (n->valor[0]) 
        strncpy(res, n->valor, ressz - 1);
    else 
        snprintf(res, ressz, "t?");
    
}
