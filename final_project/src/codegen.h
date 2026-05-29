#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"

#include <stdio.h>

#define CODEGEN_MAX_OPERAND 320

/**
 * @brief Tipos de instrucciones soportadas por el generador 3AC.
 */
typedef enum {
    OP_ASIGNAR,
    OP_BINARIA,
    OP_UNARIA,
    OP_ARRAY_READ,
    OP_ARRAY_WRITE,
    OP_LABEL,
    OP_GOTO,
    OP_IF_TRUE,
    OP_IF_FALSE,
    OP_PARAM,
    OP_CALL,
    OP_CAST
} OpCode;

/**
 * @brief Representa una instrucción de código de tres direcciones.
 */
typedef struct Instruccion {
    OpCode op;
    char dst[CODEGEN_MAX_OPERAND];
    char src1[CODEGEN_MAX_OPERAND];
    char src2[CODEGEN_MAX_OPERAND];
    char extra[CODEGEN_MAX_OPERAND];
    struct Instruccion* siguiente;
} Instruccion;

/**
 * @brief Estado interno del generador de código intermedio.
 */
typedef struct {
    Instruccion* cabeza;
    Instruccion* cola;
    int temp_count;
    int label_count;
} Generador;

/**
 * @brief Crea un nuevo generador de código.
 * @return Instancia inicializada del generador.
 */
Generador* gen_nuevo(void);

/**
 * @brief Libera los recursos asociados al generador.
 * @param gen Generador a liberar.
 */
void gen_liberar(Generador* gen);

/**
 * @brief Genera código de tres direcciones a partir del AST.
 * @param gen Generador destino.
 * @param raiz Nodo raíz del AST.
 */
void gen_generar(Generador* gen, const Node* raiz);

/**
 * @brief Imprime el código generado.
 * @param gen Generador con las instrucciones.
 * @param out Flujo de salida.
 */
void gen_imprimir(const Generador* gen, FILE* out);

/**
 * @brief Guarda el código generado en un archivo.
 * @param gen Generador con las instrucciones.
 * @param archivo Ruta del archivo de salida.
 */
void gen_guardar(const Generador* gen, const char* archivo);

#endif