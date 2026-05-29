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

// Sentencias

static void gen_nodo(
    Generador* gen,
    const Node* n,
    char* resultado,
    int resbuf
) {
    if (!n) {
        return;
    }
    
    // Programa / nodo raiz
    if (strcmp(n->tipo, "Programa") == 0) {
        for (int i = 0; i < n->num_hijos; i++) {
            gen_nodo(gen, n->hijos[i], NULL, 0);
        }

        return;
    }

    if (strcmp(n->tipo, "Bloque") == 0) {
        if (n->num_hijos > 0) {
            gen_nodo(gen, n->hijos[0], NULL, 0);
        }

        return;
    }

    if (strcmp(n->tipo, "Declaracion") == 0) {
        Node* lista = n->hijos[1];

        for (int i = 0; i < lista->num_hijos; i++) {
            Node* var = lista->hijos[i];

            if (strcmp(var->tipo, "VarConInicio") != 0) {
                continue;
            }

            char src[CODEGEN_MAX_OPERAND];

            gen_expr(gen, var->hijos[0], src, sizeof(src));

            emit(gen, OP_ASIGNAR, var->valor, src, NULL, NULL);
        }

        return;
    }

    if (strcmp(n->tipo, "DeclaracionArreglo") == 0) {
        if (n->num_hijos >= 3) {
            Node* vals = n->hijos[2];
            Node* lista = vals->hijos[0];

            for (int i = 0; i < lista->num_hijos; i++) {
                char src[CODEGEN_MAX_OPERAND];
                char idx[CODEGEN_MAX_OPERAND];

                gen_expr(gen, lista->hijos[i], src, sizeof(src));

                snprintf(idx, sizeof(idx), "%d", i);

                emit(gen, OP_ARRAY_WRITE, n->valor, idx, src, NULL);
            }
        }

        return;
    }

    if (strcmp(n->tipo, "Asignacion") == 0) {
        char src[CODEGEN_MAX_OPERAND];

        gen_expr(gen, n->hijos[0], src, sizeof(src));

        emit(gen, OP_ASIGNAR, n->valor, src, NULL, NULL);

        return;
    }

    if (strcmp(n->tipo, "AsignacionArreglo") == 0) {
        char idx[CODEGEN_MAX_OPERAND];
        char src[CODEGEN_MAX_OPERAND];

        gen_expr(gen, n->hijos[0]->hijos[0], idx, sizeof(idx));
        gen_expr(gen, n->hijos[1], src, sizeof(src));

        emit(gen, OP_ARRAY_WRITE, n->valor, idx, src, NULL);

        return;
    }

    if (strcmp(n->tipo, "Mostrar") == 0) {
        Node* lista = n->hijos[0];
        int nargs = lista->num_hijos;

        for (int i = 0; i < nargs; i++) {
            char src[CODEGEN_MAX_OPERAND];

            gen_expr(gen, lista->hijos[i], src, sizeof(src));

            emit(gen, OP_PARAM, NULL, src, NULL, NULL);
        }

        char nstr[16];

        snprintf(nstr, sizeof(nstr), "%d", nargs);

        emit(gen, OP_CALL, NULL, "mostrar", NULL, nstr);

        return;
    }

    if (strcmp(n->tipo, "Leer") == 0) {
        char ref[258];

        snprintf(ref, sizeof(ref), "&%s", n->valor);

        emit(gen, OP_CALL, NULL, "leer", NULL, ref);

        return;
    }

    if (strcmp(n->tipo, "LeerArreglo") == 0) {
        char idx[CODEGEN_MAX_OPERAND];
        char ref[256 + CODEGEN_MAX_OPERAND + 4];

        gen_expr(
            gen,
            n->hijos[0]->hijos[0]->hijos[0],
            idx,
            sizeof(idx)
        );

        snprintf(ref, sizeof(ref), "&%s[%s]", n->hijos[0]->valor, idx);

        emit(gen, OP_CALL, NULL, "leer", NULL, ref);

        return;
    }

    if (strcmp(n->tipo, "Si") == 0) {
        char cond[CODEGEN_MAX_OPERAND];
        char l_fin[CODEGEN_MAX_OPERAND];

        gen_expr(gen, n->hijos[0], cond, sizeof(cond));

        nueva_etiqueta(gen, l_fin, sizeof(l_fin));

        emit(gen, OP_IF_FALSE, NULL, cond, NULL, l_fin);

        gen_nodo(gen, n->hijos[1], NULL, 0);

        emit(gen, OP_LABEL, l_fin, NULL, NULL, NULL);

        return;
    }

    if (strcmp(n->tipo, "Si-Sino") == 0) {
        char cond[CODEGEN_MAX_OPERAND];
        char l_sino[CODEGEN_MAX_OPERAND];
        char l_fin[CODEGEN_MAX_OPERAND];

        gen_expr(gen, n->hijos[0], cond, sizeof(cond));

        nueva_etiqueta(gen, l_sino, sizeof(l_sino));
        nueva_etiqueta(gen, l_fin, sizeof(l_fin));

        emit(gen, OP_IF_FALSE, NULL, cond, NULL, l_sino);

        gen_nodo(gen, n->hijos[1], NULL, 0);

        emit(gen, OP_GOTO, NULL, NULL, NULL, l_fin);
        emit(gen, OP_LABEL, l_sino, NULL, NULL, NULL);

        gen_nodo(gen, n->hijos[2], NULL, 0);

        emit(gen, OP_LABEL, l_fin, NULL, NULL, NULL);

        return;
    }

    if (strcmp(n->tipo, "Mientras") == 0) {
        char l_ini[CODEGEN_MAX_OPERAND];
        char l_fin[CODEGEN_MAX_OPERAND];
        char cond[CODEGEN_MAX_OPERAND];

        nueva_etiqueta(gen, l_ini, sizeof(l_ini));
        nueva_etiqueta(gen, l_fin, sizeof(l_fin));

        emit(gen, OP_LABEL, l_ini, NULL, NULL, NULL);

        gen_expr(gen, n->hijos[0], cond, sizeof(cond));

        emit(gen, OP_IF_FALSE, NULL, cond, NULL, l_fin);

        gen_nodo(gen, n->hijos[1], NULL, 0);

        emit(gen, OP_GOTO, NULL, NULL, NULL, l_ini);
        emit(gen, OP_LABEL, l_fin, NULL, NULL, NULL);

        return;
    }

    if (strcmp(n->tipo, "Para") == 0) {
        char l_ini[CODEGEN_MAX_OPERAND];
        char l_fin[CODEGEN_MAX_OPERAND];
        char cond[CODEGEN_MAX_OPERAND];

        nueva_etiqueta(gen, l_ini, sizeof(l_ini));
        nueva_etiqueta(gen, l_fin, sizeof(l_fin));

        gen_nodo(gen, n->hijos[0], NULL, 0);

        emit(gen, OP_LABEL, l_ini, NULL, NULL, NULL);

        gen_expr(gen, n->hijos[1], cond, sizeof(cond));

        emit(gen, OP_IF_FALSE, NULL, cond, NULL, l_fin);

        gen_nodo(gen, n->hijos[3], NULL, 0);
        gen_nodo(gen, n->hijos[2], NULL, 0);

        emit(gen, OP_GOTO, NULL, NULL, NULL, l_ini);
        emit(gen, OP_LABEL, l_fin, NULL, NULL, NULL);

        return;
    }

    // en los nodos estructurales no se retorna nada, solo se generan sus hijos
    for (int i = 0; i < n->num_hijos; i++) {
        gen_nodo(gen, n->hijos[i], resultado, resbuf);
    }
}

/**
 * @brief Genera código de tres direcciones a partir del AST.
 * @param gen Generador de código.
 * @param raiz Nodo raíz del AST.
 */
void gen_generar(Generador* gen, const Node* raiz) {
    gen_nodo(gen, (Node*)raiz, NULL, 0);
}

static void imprimir_instruccion(const Instruccion* ins, FILE* out) {
    switch (ins->op) {
        case OP_ASIGNAR:
            fprintf(out, "\t%s = %s\n", ins->dst, ins->src1);
            break;

        case OP_BINARIA:
            fprintf(
                out,
                "\t%s = %s %s %s\n",
                ins->dst,
                ins->src1,
                ins->extra,
                ins->src2
            );
            break;

        case OP_UNARIA:
            fprintf(
                out,
                "\t%s = %s%s\n",
                ins->dst,
                ins->extra,
                ins->src1
            );
            break;

        case OP_ARRAY_READ:
            fprintf(
                out,
                "\t%s = %s[%s]\n",
                ins->dst,
                ins->src1,
                ins->src2
            );
            break;

        case OP_ARRAY_WRITE:
            fprintf(
                out,
                "\t%s[%s] = %s\n",
                ins->dst,
                ins->src1,
                ins->src2
            );
            break;

        case OP_LABEL:
            fprintf(out, "%s:\n", ins->dst);
            break;

        case OP_GOTO:
            fprintf(out, "\tgoto %s\n", ins->extra);
            break;

        case OP_IF_TRUE:
            fprintf(out, "\tif %s goto %s\n", ins->src1, ins->extra);
            break;

        case OP_IF_FALSE:
            fprintf(out, "\tifFalse %s goto %s\n", ins->src1, ins->extra);
            break;

        case OP_PARAM:
            fprintf(out, "\tparam %s\n", ins->src1);
            break;

        case OP_CALL:
            if (ins->dst[0]) {
                fprintf(
                    out,
                    "\t%s = call %s, %s\n",
                    ins->dst,
                    ins->src1,
                    ins->extra
                );
            } else {
                fprintf(
                    out,
                    "\tcall %s, %s\n",
                    ins->src1,
                    ins->extra
                );
            }

            break;

        case OP_CAST:
            fprintf(
                out,
                "\t%s = (%s) %s\n",
                ins->dst,
                ins->extra,
                ins->src1
            );
            break;
    }
}

void gen_imprimir(const Generador* gen, FILE* out) {
    fprintf(out, "; === Codigo de Tres Direcciones ===\n");

    for (const Instruccion* ins = gen->cabeza;
         ins;
         ins = ins->siguiente) {
        imprimir_instruccion(ins, out);
    }

    fprintf(out, "; === Fin del codigo ===\n");
}

/**
 * @brief Guarda el código intermedio generado en un archivo.
 * @param gen Generador con las instrucciones.
 * @param archivo Ruta del archivo de salida.
 */
void gen_guardar(const Generador* gen, const char* archivo) {
    FILE* out = fopen(archivo, "w");

    if (!out) {
        fprintf(stderr, "Error: no se pudo crear '%s'\n", archivo);
        return;
    }

    gen_imprimir(gen, out);

    fclose(out);

    printf("--- Codigo intermedio generado en: %s ---\n", archivo);
}