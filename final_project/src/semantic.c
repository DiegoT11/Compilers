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
