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

/**
 * @brief Retorna el tipo promovido entre a y b según la jerarquía BOOL < INT < FLOAT.
 *        CADENA no participa en aritmética. Propaga TIPO_ERROR si alguno lo es.
 */
static DataType tipo_promovido(DataType a, DataType b) {
    if (a == TIPO_ERROR || b == TIPO_ERROR) return TIPO_ERROR;
    if (a == TIPO_DESCONOCIDO || b == TIPO_DESCONOCIDO) return TIPO_ERROR;
    return (a > b) ? a : b;
}

/**
 * @brief Inyecta un nodo "Conversion" entre padre->hijos[idx] y el padre,
 *        preservando la trazabilidad de promociones implícitas en el ASA.
 */
static void inyectar_conversion(Node* padre, int idx, DataType tipo_dest) {
    Node* conv = nodo_nuevo("Conversion", tipo_a_str(tipo_dest), 0);
    conv->data_type = tipo_dest;
    nodo_agregar_hijo(conv, padre->hijos[idx]);
    padre->hijos[idx] = conv;
}

static DataType sem_analizar_nodo(AnalizadorSemantico* sem, Node* n);

static void sem_analizar_hijos(AnalizadorSemantico* sem, Node* n) {
    for (int i = 0; i < n->num_hijos; i++)
        sem_analizar_nodo(sem, n->hijos[i]);
}

static DataType sem_analizar_nodo(AnalizadorSemantico* sem, Node* n) {
    if (!n) return TIPO_DESCONOCIDO;

    if (strcmp(n->tipo, "LiteralEntero") == 0) {
        n->data_type = TIPO_INT;
        return TIPO_INT;
    }
    if (strcmp(n->tipo, "LiteralFlotante") == 0) {
        n->data_type = TIPO_FLOAT;
        return TIPO_FLOAT;
    }
    if (strcmp(n->tipo, "LiteralCadena") == 0) {
        n->data_type = TIPO_CADENA;
        return TIPO_CADENA;
    }
    if (strcmp(n->tipo, "LiteralBooleano") == 0) {
        n->data_type = TIPO_BOOL;
        return TIPO_BOOL;
    }

    if (strcmp(n->tipo, "Identificador") == 0) {
        Symbol* s = tabla_buscar(sem->tabla, n->valor);
        if (!s) {
            agregar_error(sem,
                "Linea %d: variable '%s' no declarada.", n->line, n->valor);
            n->data_type = TIPO_ERROR;
            return TIPO_ERROR;
        }
        n->data_type = s->tipo;
        return s->tipo;
    }

    if (strcmp(n->tipo, "Tipo") == 0) {
        n->data_type = str_a_tipo(n->valor);
        return n->data_type;
    }

    if (strcmp(n->tipo, "Declaracion") == 0) {
        DataType tipo_decl = sem_analizar_nodo(sem, n->hijos[0]);
        Node* lista = n->hijos[1];
        for (int i = 0; i < lista->num_hijos; i++) {
            Node* var = lista->hijos[i];

            if (strcmp(var->tipo, "Variable") == 0) {
                if (tabla_definir(sem->tabla, var->valor,
                                  tipo_decl, CAT_VARIABLE, var->line) < 0) {
                    agregar_error(sem,
                        "Linea %d: redeclaracion de '%s' en el mismo bloque.",
                        var->line, var->valor);
                }
                var->data_type = tipo_decl;

            } else if (strcmp(var->tipo, "VarConInicio") == 0) {
                DataType tipo_expr = sem_analizar_nodo(sem, var->hijos[0]);

                if (tipo_expr != TIPO_ERROR && tipo_decl != TIPO_ERROR) {
                    if (tipo_expr == tipo_decl) {
                        /* OK exacto */
                    } else if (tipo_decl == TIPO_FLOAT &&
                               (tipo_expr == TIPO_INT || tipo_expr == TIPO_BOOL)) {
                        inyectar_conversion(var, 0, TIPO_FLOAT);
                    } else if (tipo_decl == TIPO_INT && tipo_expr == TIPO_BOOL) {
                        inyectar_conversion(var, 0, TIPO_INT);
                    } else if (tipo_decl == TIPO_INT && tipo_expr == TIPO_FLOAT) {
                        agregar_error(sem,
                            "Linea %d: perdida de precision al asignar "
                            "flotante a entero '%s'.", var->line, var->valor);
                    } else {
                        agregar_error(sem,
                            "Linea %d: tipo incompatible en inicializacion "
                            "de '%s' (esperado %s, encontrado %s).",
                            var->line, var->valor,
                            tipo_a_str(tipo_decl), tipo_a_str(tipo_expr));
                    }
                }

                if (tabla_definir(sem->tabla, var->valor,
                                  tipo_decl, CAT_VARIABLE, var->line) < 0) {
                    agregar_error(sem,
                        "Linea %d: redeclaracion de '%s' en el mismo bloque.",
                        var->line, var->valor);
                }
                var->data_type = tipo_decl;
            }
        }
        n->data_type = tipo_decl;
        return tipo_decl;
    }

    if (strcmp(n->tipo, "DeclaracionArreglo") == 0) {
        DataType tipo_elem = sem_analizar_nodo(sem, n->hijos[0]);

        Node* tam_nodo = n->hijos[1];
        DataType tipo_tam = sem_analizar_nodo(sem, tam_nodo->hijos[0]);
        if (tipo_tam != TIPO_INT && tipo_tam != TIPO_ERROR) {
            agregar_error(sem,
                "Linea %d: el tamanio del arreglo '%s' debe ser entero.",
                n->line, n->valor);
        }

        if (tabla_definir(sem->tabla, n->valor,
                          tipo_elem, CAT_ARREGLO, n->line) < 0) {
            agregar_error(sem,
                "Linea %d: redeclaracion del arreglo '%s'.", n->line, n->valor);
        }
        n->data_type = tipo_elem;

        if (n->num_hijos >= 3) {
            Node* vals = n->hijos[2];
            Node* lista = vals->hijos[0];
            for (int i = 0; i < lista->num_hijos; i++) {
                DataType tv = sem_analizar_nodo(sem, lista->hijos[i]);
                if (tv == tipo_elem) continue;
                if (tipo_elem == TIPO_FLOAT &&
                    (tv == TIPO_INT || tv == TIPO_BOOL)) {
                    inyectar_conversion(lista, i, TIPO_FLOAT);
                } else if (tv != TIPO_ERROR) {
                    agregar_error(sem,
                        "Linea %d: valor inicial %d del arreglo '%s' "
                        "tiene tipo incompatible (%s vs %s).",
                        n->line, i, n->valor,
                        tipo_a_str(tipo_elem), tipo_a_str(tv));
                }
            }
        }
        return tipo_elem;
    }

    if (strcmp(n->tipo, "Asignacion") == 0) {
        Symbol* s = tabla_buscar(sem->tabla, n->valor);
        if (!s) {
            agregar_error(sem,
                "Linea %d: variable '%s' no declarada.", n->line, n->valor);
            sem_analizar_nodo(sem, n->hijos[0]);
            n->data_type = TIPO_ERROR;
            return TIPO_ERROR;
        }
        if (s->categoria == CAT_ARREGLO) {
            agregar_error(sem,
                "Linea %d: '%s' es un arreglo; usa '%s[i] = ...'.",
                n->line, n->valor, n->valor);
        }
        DataType tipo_expr = sem_analizar_nodo(sem, n->hijos[0]);
        DataType tipo_var = s->tipo;

        if (tipo_expr != TIPO_ERROR && tipo_var != TIPO_ERROR) {
            if (tipo_expr == tipo_var) {
                /* OK */
            } else if (tipo_var == TIPO_FLOAT &&
                       (tipo_expr == TIPO_INT || tipo_expr == TIPO_BOOL)) {
                inyectar_conversion(n, 0, TIPO_FLOAT);
            } else if (tipo_var == TIPO_INT && tipo_expr == TIPO_BOOL) {
                inyectar_conversion(n, 0, TIPO_INT);
            } else if (tipo_var == TIPO_INT && tipo_expr == TIPO_FLOAT) {
                agregar_error(sem,
                    "Linea %d: perdida de precision al asignar flotante "
                    "a entero '%s'.", n->line, n->valor);
            } else {
                agregar_error(sem,
                    "Linea %d: tipo incompatible en asignacion a '%s' "
                    "(esperado %s, encontrado %s).",
                    n->line, n->valor,
                    tipo_a_str(tipo_var), tipo_a_str(tipo_expr));
            }
        }
        n->data_type = tipo_var;
        return tipo_var;
    }

    if (strcmp(n->tipo, "AsignacionArreglo") == 0) {
        Symbol* s = tabla_buscar(sem->tabla, n->valor);
        if (!s) {
            agregar_error(sem,
                "Linea %d: arreglo '%s' no declarado.", n->line, n->valor);
            n->data_type = TIPO_ERROR;
            return TIPO_ERROR;
        }
        if (s->categoria != CAT_ARREGLO) {
            agregar_error(sem,
                "Linea %d: '%s' no es un arreglo.", n->line, n->valor);
        }
        DataType tipo_idx = sem_analizar_nodo(sem, n->hijos[0]->hijos[0]);
        if (tipo_idx != TIPO_INT && tipo_idx != TIPO_ERROR) {
            agregar_error(sem,
                "Linea %d: el indice del arreglo '%s' debe ser entero.",
                n->line, n->valor);
        }
        DataType tipo_val = sem_analizar_nodo(sem, n->hijos[1]);
        if (tipo_val != s->tipo && tipo_val != TIPO_ERROR) {
            if (s->tipo == TIPO_FLOAT &&
                (tipo_val == TIPO_INT || tipo_val == TIPO_BOOL)) {
                inyectar_conversion(n, 1, TIPO_FLOAT);
            } else {
                agregar_error(sem,
                    "Linea %d: tipo incompatible al asignar a '%s[i]' "
                    "(esperado %s, encontrado %s).",
                    n->line, n->valor,
                    tipo_a_str(s->tipo), tipo_a_str(tipo_val));
            }
        }
        n->data_type = s->tipo;
        return s->tipo;
    }

    if (strcmp(n->tipo, "AccesoArreglo") == 0) {
        Symbol* s = tabla_buscar(sem->tabla, n->valor);
        if (!s) {
            agregar_error(sem,
                "Linea %d: arreglo '%s' no declarado.", n->line, n->valor);
            n->data_type = TIPO_ERROR;
            return TIPO_ERROR;
        }
        if (s->categoria != CAT_ARREGLO) {
            agregar_error(sem,
                "Linea %d: '%s' no es un arreglo; no se puede indexar.",
                n->line, n->valor);
        }
        DataType tipo_idx = sem_analizar_nodo(sem, n->hijos[0]->hijos[0]);
        if (tipo_idx != TIPO_INT && tipo_idx != TIPO_ERROR) {
            agregar_error(sem,
                "Linea %d: el indice de '%s' debe ser entero.",
                n->line, n->valor);
        }
        n->data_type = s->tipo;
        return s->tipo;
    }

    if (strcmp(n->tipo, "Operacion") == 0) {
        const char* op = n->valor;

        if (strcmp(op, "&&") == 0 || strcmp(op, "||") == 0) {
            DataType t0 = sem_analizar_nodo(sem, n->hijos[0]);
            DataType t1 = sem_analizar_nodo(sem, n->hijos[1]);
            if (t0 != TIPO_BOOL && t0 != TIPO_ERROR)
                agregar_error(sem,
                    "Linea %d: operando izquierdo de '%s' debe ser booleano "
                    "(encontrado %s).", n->line, op, tipo_a_str(t0));
            if (t1 != TIPO_BOOL && t1 != TIPO_ERROR)
                agregar_error(sem,
                    "Linea %d: operando derecho de '%s' debe ser booleano "
                    "(encontrado %s).", n->line, op, tipo_a_str(t1));
            n->data_type = TIPO_BOOL;
            return TIPO_BOOL;
        }

        if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
            strcmp(op, "<")  == 0 || strcmp(op, ">")  == 0 ||
            strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0) {
            DataType t0 = sem_analizar_nodo(sem, n->hijos[0]);
            DataType t1 = sem_analizar_nodo(sem, n->hijos[1]);
            int ok = 0;
            if (t0 == TIPO_ERROR || t1 == TIPO_ERROR) ok = 1;
            else if (t0 == t1)                         ok = 1;
            else if (t0 <= TIPO_FLOAT && t1 <= TIPO_FLOAT) {
                ok = 1;
                if (t0 < t1) inyectar_conversion(n, 0, t1);
                else         inyectar_conversion(n, 1, t0);
            }
            if (!ok)
                agregar_error(sem,
                    "Linea %d: comparacion '%s' entre tipos incompatibles "
                    "(%s vs %s).", n->line, op,
                    tipo_a_str(t0), tipo_a_str(t1));
            n->data_type = TIPO_BOOL;
            return TIPO_BOOL;
        }

        if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
            strcmp(op, "*") == 0 || strcmp(op, "/") == 0) {
            DataType t0 = sem_analizar_nodo(sem, n->hijos[0]);
            DataType t1 = sem_analizar_nodo(sem, n->hijos[1]);

            if (strcmp(op, "+") == 0 && t0 == TIPO_CADENA && t1 == TIPO_CADENA) {
                n->data_type = TIPO_CADENA;
                return TIPO_CADENA;
            }

            int t0_num = (t0 == TIPO_INT || t0 == TIPO_FLOAT || t0 == TIPO_BOOL);
            int t1_num = (t1 == TIPO_INT || t1 == TIPO_FLOAT || t1 == TIPO_BOOL);

            if (!t0_num && t0 != TIPO_ERROR)
                agregar_error(sem,
                    "Linea %d: operando izquierdo de '%s' no es numerico "
                    "(%s).", n->line, op, tipo_a_str(t0));
            if (!t1_num && t1 != TIPO_ERROR)
                agregar_error(sem,
                    "Linea %d: operando derecho de '%s' no es numerico "
                    "(%s).", n->line, op, tipo_a_str(t1));

            DataType result = tipo_promovido(t0, t1);
            if (result != TIPO_ERROR) {
                if (t0 < result && t0 != TIPO_ERROR)
                    inyectar_conversion(n, 0, result);
                if (t1 < result && t1 != TIPO_ERROR)
                    inyectar_conversion(n, 1, result);
            }
            n->data_type = (result == TIPO_ERROR) ? TIPO_ERROR : result;
            return n->data_type;
        }

        n->data_type = TIPO_ERROR;
        return TIPO_ERROR;
    }

    if (strcmp(n->tipo, "Negativo") == 0) {
        DataType t = sem_analizar_nodo(sem, n->hijos[0]);
        if (t != TIPO_INT && t != TIPO_FLOAT && t != TIPO_ERROR) {
            agregar_error(sem,
                "Linea %d: operador unario '-' requiere tipo numerico "
                "(encontrado %s).", n->line, tipo_a_str(t));
            n->data_type = TIPO_ERROR;
            return TIPO_ERROR;
        }
        n->data_type = t;
        return t;
    }

    if (strcmp(n->tipo, "Negacion") == 0) {
        DataType t = sem_analizar_nodo(sem, n->hijos[0]);
        if (t != TIPO_BOOL && t != TIPO_ERROR) {
            agregar_error(sem,
                "Linea %d: operador '!' requiere tipo booleano "
                "(encontrado %s).", n->line, tipo_a_str(t));
            n->data_type = TIPO_ERROR;
            return TIPO_ERROR;
        }
        n->data_type = TIPO_BOOL;
        return TIPO_BOOL;
    }

    if (strcmp(n->tipo, "Bloque") == 0) {
        tabla_entrar_scope(sem->tabla);

        if (n->num_hijos > 0)
            sem_analizar_hijos(sem, n->hijos[0]);

        Symbol* no_usados[64];
        int cnt = tabla_salir_scope(sem->tabla, no_usados, 64);
        for (int i = 0; i < cnt; i++) {
            agregar_advertencia(sem,
                "Linea %d: variable '%s' declarada pero no usada.",
                no_usados[i]->linea_decl, no_usados[i]->nombre);
            free(no_usados[i]);
        }
        return TIPO_DESCONOCIDO;
    }

    if (strcmp(n->tipo, "Si") == 0 || strcmp(n->tipo, "Si-Sino") == 0) {
        DataType tipo_cond = sem_analizar_nodo(sem, n->hijos[0]);
        if (tipo_cond != TIPO_BOOL && tipo_cond != TIPO_ERROR) {
            agregar_error(sem,
                "Linea %d: la condicion del 'si' debe ser booleana "
                "(encontrado %s).", n->line, tipo_a_str(tipo_cond));
        }
        for (int i = 1; i < n->num_hijos; i++)
            sem_analizar_nodo(sem, n->hijos[i]);
        return TIPO_DESCONOCIDO;
    }

    if (strcmp(n->tipo, "Mientras") == 0) {
        DataType tipo_cond = sem_analizar_nodo(sem, n->hijos[0]);
        if (tipo_cond != TIPO_BOOL && tipo_cond != TIPO_ERROR) {
            agregar_error(sem,
                "Linea %d: la condicion del 'mientras' debe ser booleana "
                "(encontrado %s).", n->line, tipo_a_str(tipo_cond));
        }
        sem_analizar_nodo(sem, n->hijos[1]);
        return TIPO_DESCONOCIDO;
    }

    if (strcmp(n->tipo, "Para") == 0) {
        /* hijos: [0]=asign_init, [1]=condicion, [2]=asign_paso, [3]=bloque */
        sem_analizar_nodo(sem, n->hijos[0]);
        DataType tipo_cond = sem_analizar_nodo(sem, n->hijos[1]);
        if (tipo_cond != TIPO_BOOL && tipo_cond != TIPO_ERROR) {
            agregar_error(sem,
                "Linea %d: la condicion del 'para' debe ser booleana "
                "(encontrado %s).", n->line, tipo_a_str(tipo_cond));
        }
        sem_analizar_nodo(sem, n->hijos[2]);
        sem_analizar_nodo(sem, n->hijos[3]);
        return TIPO_DESCONOCIDO;
    }

    if (strcmp(n->tipo, "Mostrar") == 0) {
        Node* lista = n->hijos[0];
        for (int i = 0; i < lista->num_hijos; i++)
            sem_analizar_nodo(sem, lista->hijos[i]);
        return TIPO_DESCONOCIDO;
    }

    if (strcmp(n->tipo, "Leer") == 0) {
        Symbol* s = tabla_buscar(sem->tabla, n->valor);
        if (!s) {
            agregar_error(sem,
                "Linea %d: variable '%s' no declarada en leer().",
                n->line, n->valor);
            n->data_type = TIPO_ERROR;
            return TIPO_ERROR;
        }
        n->data_type = s->tipo;
        return s->tipo;
    }

    if (strcmp(n->tipo, "LeerArreglo") == 0) {
        DataType t = sem_analizar_nodo(sem, n->hijos[0]);
        n->data_type = t;
        return t;
    }

    sem_analizar_hijos(sem, n);
    return TIPO_DESCONOCIDO;
}

/**
 * @brief Punto de entrada del análisis semántico.
 * @param sem Analizador inicializado con sem_nuevo().
 * @param raiz Raíz del ASA a analizar.
 * @return 1 si no hay errores, 0 si hay al menos uno.
 */
int sem_analizar(AnalizadorSemantico* sem, Node* raiz) {
    if (!raiz) return 0;
    sem_analizar_nodo(sem, raiz);

    Symbol* no_usados[64];
    int cnt = tabla_salir_scope(sem->tabla, no_usados, 64);
    for (int i = 0; i < cnt; i++) {
        agregar_advertencia(sem,
            "Linea %d: variable '%s' declarada pero no usada (scope global).",
            no_usados[i]->linea_decl, no_usados[i]->nombre);
        free(no_usados[i]);
    }

    return (sem->num_errores == 0) ? 1 : 0;
}