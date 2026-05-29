#ifndef AST_H
#define AST_H

#include <stdio.h>

#define MAX_HIJOS 64

/**
 * @brief Tipos de datos soportados por el lenguaje.
 */
typedef enum {
    TIPO_DESCONOCIDO = -1,
    TIPO_ERROR = 0,
    TIPO_BOOL = 1,
    TIPO_INT = 2,
    TIPO_FLOAT = 3,
    TIPO_CADENA = 4
} DataType;

/**
 * @brief Nodo del Árbol de Sintaxis Abstracta.
 */
typedef struct Node {
    char tipo[64];
    char valor[256];
    struct Node* hijos[MAX_HIJOS];
    int num_hijos;
    int id;
    DataType data_type;
    int line;
} Node;

/**
 * @brief Convierte un tipo interno a texto.
 * @param t Tipo de dato.
 * @return Representación textual del tipo.
 */
const char* tipo_a_str(DataType t);

/**
 * @brief Convierte una cadena al tipo interno correspondiente.
 * @param s Nombre textual del tipo.
 * @return Tipo asociado.
 */
DataType str_a_tipo(const char* s);

/**
 * @brief Crea un nuevo nodo del AST.
 * @param tipo Tipo de nodo.
 * @param valor Valor asociado.
 * @param line Línea de origen.
 * @return Nodo inicializado.
 */
Node* nodo_nuevo(const char* tipo, const char* valor, int line);

/**
 * @brief Agrega un hijo a un nodo padre.
 * @param padre Nodo padre.
 * @param hijo Nodo hijo.
 */
void nodo_agregar_hijo(Node* padre, Node* hijo);

/**
 * @brief Libera recursivamente un árbol AST.
 * @param n Nodo raíz.
 */
void nodo_liberar(Node* n);

/**
 * @brief Imprime el AST en formato textual.
 * @param n Nodo raíz.
 * @param sangria Nivel de indentación.
 */
void nodo_imprimir(const Node* n, int sangria);

/**
 * @brief Genera un archivo DOT del AST.
 * @param n Nodo raíz.
 * @param archivo Archivo de salida.
 */
void nodo_generar_dot(const Node* n, const char* archivo);

#endif