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

/**
 * @brief Inserta un símbolo en el scope actual.
 * @return 0 si OK, -1 si ya existe en este scope (redeclaración).
 */
static int tabla_definir(TablaSimbolos* t, const char* nombre,
                          DataType tipo, SymbolCategory cat, int linea) {
    for (Symbol* s = t->tope->simbolos; s; s = s->siguiente)
        if (strcmp(s->nombre, nombre) == 0) return -1;

    Symbol* nuevo = (Symbol*)calloc(1, sizeof(Symbol));
    strncpy(nuevo->nombre, nombre, sizeof(nuevo->nombre) - 1);
    nuevo->tipo = tipo;
    nuevo->categoria = cat;
    nuevo->profundidad = t->tope->profundidad;
    nuevo->linea_decl = linea;
    nuevo->usado = 0;
    nuevo->siguiente = t->tope->simbolos;
    t->tope->simbolos = nuevo;
    return 0;
}

/**
 * @brief Busca un símbolo desde el scope más interno hacia el global.
 *        Marca el símbolo encontrado como usado.
 */
static Symbol* tabla_buscar(TablaSimbolos* t, const char* nombre) {
    for (Scope* sc = t->tope; sc; sc = sc->anterior)
        for (Symbol* s = sc->simbolos; s; s = s->siguiente)
            if (strcmp(s->nombre, nombre) == 0) {
                s->usado = 1;
                return s;
            }
    return NULL;
}

static void tabla_liberar(TablaSimbolos* t) {
    for (Scope* sc = t->tope; sc; ) {
        for (Symbol* s = sc->simbolos; s; ) {
            Symbol* sig = s->siguiente;
            free(s);
            s = sig;
        }
        Scope* ant = sc->anterior;
        free(sc);
        sc = ant;
    }
    free(t);
}

struct AnalizadorSemantico {
    TablaSimbolos* tabla;
    char errores[SEM_MAX_ERRORES][SEM_MAX_MSG_LEN];
    int num_errores;
    char advertencias[SEM_MAX_ADVERTENCIAS][SEM_MAX_MSG_LEN];
    int num_advertencias;
};

AnalizadorSemantico* sem_nuevo(void) {
    AnalizadorSemantico* sem =
        (AnalizadorSemantico*)calloc(1, sizeof(AnalizadorSemantico));
    sem->tabla = tabla_nueva();
    return sem;
}

void sem_liberar(AnalizadorSemantico* sem) {
    if (!sem) return;
    tabla_liberar(sem->tabla);
    free(sem);
}

int sem_tiene_errores(const AnalizadorSemantico* sem) {
    return sem->num_errores > 0;
}

int sem_tiene_advertencias(const AnalizadorSemantico* sem) {
    return sem->num_advertencias > 0;
}

void sem_reportar_errores(const AnalizadorSemantico* sem) {
    for (int i = 0; i < sem->num_errores; i++)
        fprintf(stdout, "  [Error semantico %d] %s\n", i + 1, sem->errores[i]);
}

void sem_reportar_advertencias(const AnalizadorSemantico* sem) {
    for (int i = 0; i < sem->num_advertencias; i++)
        fprintf(stdout, "  [Advertencia %d] %s\n", i + 1, sem->advertencias[i]);
}

static void agregar_error(AnalizadorSemantico* sem, const char* fmt, ...) {
    if (sem->num_errores >= SEM_MAX_ERRORES) return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(sem->errores[sem->num_errores++], SEM_MAX_MSG_LEN, fmt, ap);
    va_end(ap);
}

static void agregar_advertencia(AnalizadorSemantico* sem, const char* fmt, ...) {
    if (sem->num_advertencias >= SEM_MAX_ADVERTENCIAS) return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(sem->advertencias[sem->num_advertencias++], SEM_MAX_MSG_LEN, fmt, ap);
    va_end(ap);
}