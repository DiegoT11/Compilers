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