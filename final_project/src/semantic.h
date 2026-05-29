#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

#define SEM_MAX_ERRORES 256
#define SEM_MAX_ADVERTENCIAS 256
#define SEM_MAX_MSG_LEN 512

/**
 * @brief Analizador semántico del lenguaje.
 *
 * La implementación interna y la tabla de símbolos permanecen ocultas
 * dentro de semantic.c.
 */
typedef struct AnalizadorSemantico AnalizadorSemantico;

/**
 * @brief Crea una nueva instancia del analizador semántico.
 * @return Analizador inicializado.
 */
AnalizadorSemantico* sem_nuevo(void);

/**
 * @brief Libera los recursos asociados al analizador.
 * @param sem Analizador a liberar.
 */
void sem_liberar(AnalizadorSemantico* sem);

/**
 * @brief Ejecuta el análisis semántico sobre el AST.
 * @param sem Analizador semántico.
 * @param raiz Nodo raíz del AST.
 * @return 1 si no hubo errores semánticos, 0 en caso contrario.
 */
int sem_analizar(AnalizadorSemantico* sem, Node* raiz);

/**
 * @brief Indica si el análisis produjo errores semánticos.
 * @param sem Analizador semántico.
 * @return 1 si existen errores, 0 en caso contrario.
 */
int sem_tiene_errores(const AnalizadorSemantico* sem);

/**
 * @brief Indica si el análisis produjo advertencias.
 * @param sem Analizador semántico.
 * @return 1 si existen advertencias, 0 en caso contrario.
 */
int sem_tiene_advertencias(const AnalizadorSemantico* sem);

/**
 * @brief Imprime los errores semánticos acumulados.
 * @param sem Analizador semántico.
 */
void sem_reportar_errores(const AnalizadorSemantico* sem);

/**
 * @brief Imprime las advertencias acumuladas.
 * @param sem Analizador semántico.
 */
void sem_reportar_advertencias(const AnalizadorSemantico* sem);

#endif