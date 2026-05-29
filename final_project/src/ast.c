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

static void nodo_a_dot(const Node* n, FILE* out) {
    if (!n) {
        return;
    }

    char etiqueta[512];

    snprintf(etiqueta, sizeof(etiqueta), "%s", n->tipo);

    if (n->valor[0]) {
        char val_esc[300];
        int j = 0;

        for (int i = 0; n->valor[i] && j < (int)sizeof(val_esc) - 3; i++) {
            if (n->valor[i] == '"') {
                val_esc[j++] = '\\';
                val_esc[j++] = '"';
            } else {
                val_esc[j++] = n->valor[i];
            }
        }

        val_esc[j] = '\0';

        strncat(etiqueta, "\\n(", sizeof(etiqueta) - strlen(etiqueta) - 1);
        strncat(etiqueta, val_esc, sizeof(etiqueta) - strlen(etiqueta) - 1);
        strncat(etiqueta, ")", sizeof(etiqueta) - strlen(etiqueta) - 1);
    }

    if (n->data_type != TIPO_DESCONOCIDO &&
        n->data_type != TIPO_ERROR) {
        strncat(etiqueta, "\\n[", sizeof(etiqueta) - strlen(etiqueta) - 1);
        strncat(
            etiqueta,
            tipo_a_str(n->data_type),
            sizeof(etiqueta) - strlen(etiqueta) - 1
        );
        strncat(etiqueta, "]", sizeof(etiqueta) - strlen(etiqueta) - 1);
    }

    fprintf(out, "  nodo%d [label=\"%s\"];\n", n->id, etiqueta);

    for (int i = 0; i < n->num_hijos; i++) {
        if (!n->hijos[i]) {
            continue;
        }

        fprintf(out, "  nodo%d -> nodo%d;\n", n->id, n->hijos[i]->id);

        nodo_a_dot(n->hijos[i], out);
    }
}

void nodo_generar_dot(const Node* n, const char* archivo) {
    FILE* out = fopen(archivo, "w");

    if (!out) {
        fprintf(stderr, "Error: no se pudo crear '%s'\n", archivo);
        return;
    }

    fprintf(out, "digraph ASA {\n");
    fprintf(
        out,
        "  node [shape=box, style=rounded, fontname=\"Arial\", fontsize=10];\n"
    );
    fprintf(out, "  edge [arrowsize=0.7];\n");
    fprintf(out, "  rankdir=TB;\n");

    nodo_a_dot(n, out);

    fprintf(out, "}\n");

    fclose(out);

    printf("--- ASA decorado generado en: %s ---\n", archivo);
}