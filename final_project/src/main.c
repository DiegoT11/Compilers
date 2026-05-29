#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "codegen.h"
#include "semantic.h"
#include "parser.h"

extern int yyparse(void);
extern int yylineno;
extern Node* raiz;

/**
 * @brief Configuración de ejecución del compilador.
 */
typedef struct {
    int imprimir_ast;
    int generar_dot;
    int generar_ci;
    const char* archivo_dot;
    const char* archivo_ci;
} Opciones;

static void mostrar_ayuda(const char* prog) {
    fprintf(
        stderr,
        "Uso: %s [opciones]   (fuente leido desde stdin)\n"
        "\n"
        "  --no-ast       no imprimir el ASA decorado\n"
        "  --no-dot       no generar el archivo .dot (Graphviz)\n"
        "  --no-ci        no generar codigo intermedio\n"
        "  --dot=FILE     nombre del .dot  (por defecto: asa.dot)\n"
        "  --ci=FILE      nombre del .ci   (por defecto: codigo.ci)\n",
        prog
    );
}

static Opciones parsear_opciones(int argc, char* argv[]) {
    Opciones opt = {
        .imprimir_ast = 1,
        .generar_dot = 1,
        .generar_ci = 1,
        .archivo_dot = "asa.dot",
        .archivo_ci = "codigo.ci"
    };

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-ast") == 0) {
            opt.imprimir_ast = 0;
        } else if (strcmp(argv[i], "--no-dot") == 0) {
            opt.generar_dot = 0;
        } else if (strcmp(argv[i], "--no-ci") == 0) {
            opt.generar_ci = 0;
        } else if (strncmp(argv[i], "--dot=", 6) == 0) {
            opt.archivo_dot = argv[i] + 6;
        } else if (strncmp(argv[i], "--ci=", 5) == 0) {
            opt.archivo_ci = argv[i] + 5;
        } else {
            fprintf(stderr, "Opcion desconocida: %s\n", argv[i]);
            mostrar_ayuda(argv[0]);
            exit(1);
        }
    }

    return opt;
}

int main(int argc, char* argv[]) {
    Opciones opt = parsear_opciones(argc, argv);

    fprintf(stderr, "--- Iniciando compilador Simple ---\n");

    fprintf(stderr, "\n[Fase 1] Analisis lexico y sintactico...\n");

    if (yyparse() != 0 || !raiz) {
        fprintf(stderr, "Compilacion abortada: errores en fase 1.\n");
        return 1;
    }

    fprintf(stderr, "[Fase 1] OK — arbol construido.\n");

    fprintf(stderr, "\n[Fase 2] Analisis semantico...\n");

    AnalizadorSemantico* sem = sem_nuevo();

    int sem_ok = sem_analizar(sem, raiz);

    if (sem_tiene_advertencias(sem)) {
        printf("\n[Advertencias]:\n");
        sem_reportar_advertencias(sem);
    }

    if (!sem_ok) {
        printf("\n[Fase 2] Errores semanticos encontrados:\n");

        sem_reportar_errores(sem);

        sem_liberar(sem);
        nodo_liberar(raiz);

        fprintf(stderr, "Compilacion abortada: errores en fase 2.\n");

        return 1;
    }

    fprintf(stderr, "[Fase 2] OK — ASA decorado.\n");

    if (opt.imprimir_ast) {
        printf("\n[Fase 3] Arbol de Sintaxis Abstracta decorado:\n");
        printf("----------------------------------------------\n");

        nodo_imprimir(raiz, 0);
    }

    if (opt.generar_dot) {
        nodo_generar_dot(raiz, opt.archivo_dot);
    }

    if (opt.generar_ci) {
        fprintf(stderr, "\n[Fase 4] Generando codigo intermedio...\n");

        Generador* gen = gen_nuevo();

        gen_generar(gen, raiz);

        printf("\n[Fase 4] Codigo intermedio:\n");
        printf("----------------------------\n");

        gen_imprimir(gen, stdout);

        gen_guardar(gen, opt.archivo_ci);

        gen_liberar(gen);

        fprintf(stderr, "[Fase 4] OK.\n");
    }

    sem_liberar(sem);
    nodo_liberar(raiz);

    printf("\nAnalisis Exitoso\n");

    return 0;
}