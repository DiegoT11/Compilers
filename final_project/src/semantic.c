#include "semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

typedef enum { CAT_VARIABLE, CAT_ARREGLO } SymbolCategory;

typedef struct Symbol {
    char nombre[128];
    DataType tipo;
    SymbolCategory categoria;
    int profundidad;
    int linea_decl;
    int usado;
    struct Symbol* siguiente;
} Symbol;

typedef struct Scope {
    Symbol* simbolos;
    struct Scope* anterior;
    int profundidad;
} Scope;

typedef struct {
    Scope* tope;
} TablaSimbolos;

static TablaSimbolos* tabla_nueva(void) {
    TablaSimbolos* t = (TablaSimbolos*)calloc(1, sizeof(TablaSimbolos));
    Scope* global = (Scope*)calloc(1, sizeof(Scope));
    global->profundidad = 0;
    t->tope = global;
    return t;
}

static void tabla_entrar_scope(TablaSimbolos* t) {
    Scope* s = (Scope*)calloc(1, sizeof(Scope));
    s->profundidad = t->tope->profundidad + 1;
    s->anterior = t->tope;
    t->tope = s;
}

/**
 * @brief Cierra el scope actual y recoge los símbolos no usados.
 * @param t Tabla de símbolos.
 * @param no_usados Buffer de salida para símbolos no usados (heap-alocados).
 * @param max Capacidad máxima de no_usados[].
 * @return Cantidad de símbolos no usados copiados en no_usados[].
 */
static int tabla_salir_scope(TablaSimbolos* t, Symbol** no_usados, int max) {
    Scope* s = t->tope;
    int cnt = 0;
    Symbol* sym = s->simbolos;
    while (sym) {
        Symbol* sig = sym->siguiente;
        if (!sym->usado && cnt < max) {
            Symbol* copia = (Symbol*)malloc(sizeof(Symbol));
            *copia = *sym;
            copia->siguiente = NULL;
            no_usados[cnt++] = copia;
        }
        free(sym);
        sym = sig;
    }
    t->tope = s->anterior;
    free(s);
    return cnt;
}