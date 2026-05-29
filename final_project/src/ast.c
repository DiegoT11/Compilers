#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int nodo_contador = 0;

const char* tipo_a_str(DataType t) {
    switch (t) {
        case TIPO_BOOL:
            return "booleano";

        case TIPO_INT:
            return "entero";

        case TIPO_FLOAT:
            return "flotante";

        case TIPO_CADENA:
            return "cadena";

        case TIPO_DESCONOCIDO:
            return "desconocido";

        case TIPO_ERROR:
            return "error";
    }

    return "desconocido";
}

DataType str_a_tipo(const char* s) {
    if (!s) {
        return TIPO_DESCONOCIDO;
    }

    if (strcmp(s, "entero") == 0) {
        return TIPO_INT;
    }

    if (strcmp(s, "flotante") == 0) {
        return TIPO_FLOAT;
    }

    if (strcmp(s, "booleano") == 0) {
        return TIPO_BOOL;
    }

    if (strcmp(s, "cadena") == 0) {
        return TIPO_CADENA;
    }

    return TIPO_DESCONOCIDO;
}

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

void nodo_imprimir(const Node* n, int sangria) {
    if (!n) {
        return;
    }

    for (int i = 0; i < sangria; i++) {
        printf("  ");
    }

    printf("|-- %s", n->tipo);

    if (n->valor[0]) {
        printf(": %s", n->valor);
    }

    if (n->data_type != TIPO_DESCONOCIDO &&
        n->data_type != TIPO_ERROR) {
        printf(" [%s]", tipo_a_str(n->data_type));
    }

    printf("\n");

    for (int i = 0; i < n->num_hijos; i++) {
        nodo_imprimir(n->hijos[i], sangria + 1);
    }
}