#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int nodo_contador = 0;

Node* nodo_nuevo(const char* tipo, const char* valor, int line) {
    Node* n = calloc(1, sizeof(Node));

    if (!n) {
        fprintf(stderr, "Error: sin memoria para nodo AST\n");
        exit(1);
    }

    strncpy(n->tipo, tipo, sizeof(n->tipo) - 1);
    strncpy(n->valor, valor ? valor : "", sizeof(n->valor) - 1);

    n->id = nodo_contador++;
    n->data_type = TIPO_DESCONOCIDO;
    n->line = line;

    return n;
}

void nodo_agregar_hijo(Node* padre, Node* hijo) {
    if (!padre || !hijo) {
        return;
    }

    if (padre->num_hijos < MAX_HIJOS) {
        padre->hijos[padre->num_hijos++] = hijo;
    }
}

void nodo_liberar(Node* n) {
    if (!n) {
        return;
    }

    for (int i = 0; i < n->num_hijos; i++) {
        nodo_liberar(n->hijos[i]);
    }

    free(n);
}