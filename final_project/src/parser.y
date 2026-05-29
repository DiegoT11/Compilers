%{
#include "ast.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Node* raiz;
%}

%locations

%union {
    char*  sval;
    Node*  nptr;
}

%token <sval> TOK_IDENTIFICADOR
%token <sval> TOK_LITERAL_ENTERO
%token <sval> TOK_LITERAL_FLOTANTE
%token <sval> TOK_LITERAL_CADENA

%token TOK_KW_ENTERO TOK_KW_FLOTANTE TOK_KW_BOOLEANO TOK_KW_CADENA
%token TOK_KW_SI TOK_KW_SINO TOK_KW_MIENTRAS TOK_KW_PARA
%token TOK_KW_MOSTRAR TOK_KW_LEER
%token TOK_VERDADERO TOK_FALSO

%token TOK_ASIGNAR
%token TOK_IGUAL TOK_DIFERENTE_DE
%token TOK_MENOR_QUE TOK_MAYOR_QUE TOK_MENOR_IGUAL TOK_MAYOR_IGUAL
%token TOK_AND TOK_OR TOK_NOT
%token TOK_MAS TOK_MENOS TOK_MULTIPLICACION TOK_DIVISION

%token TOK_PARENTESIS_IZQUIERDO TOK_PARENTESIS_DERECHO
%token TOK_LLAVE_IZQUIERDA TOK_LLAVE_DERECHA
%token TOK_CORCHETE_IZQUIERDO TOK_CORCHETE_DERECHO
%token TOK_COMA TOK_PUNTO_COMA

%type <nptr> programa lista_sentencias sentencia bloque cuerpo_si
%type <nptr> declaracion tipo lista_var var_o_init asignacion
%type <nptr> expresion expr_or expr_and expr_comp expr_arit termino factor
%type <nptr> sent_si mientras_decl para_decl
%type <nptr> mostrar_decl leer_decl lista_expr
%type <nptr> decl_arreglo asign_arreglo acceso_arreglo lista_init

%%

declaracion:
    tipo lista_var {
        $$ = nodo_nuevo("Declaracion", "", 0);
        nodo_agregar_hijo($$, $1);
        nodo_agregar_hijo($$, $2);
    }
;

tipo:
    TOK_KW_ENTERO   { $$ = nodo_nuevo("Tipo", "entero",   0); }
  | TOK_KW_FLOTANTE { $$ = nodo_nuevo("Tipo", "flotante", 0); }
  | TOK_KW_BOOLEANO { $$ = nodo_nuevo("Tipo", "booleano", 0); }
  | TOK_KW_CADENA   { $$ = nodo_nuevo("Tipo", "cadena",   0); }
;

lista_var:
    var_o_init {
        $$ = nodo_nuevo("ListaVariables", "", 0);
        nodo_agregar_hijo($$, $1);
    }
  | lista_var TOK_COMA var_o_init {
        nodo_agregar_hijo($1, $3);
        $$ = $1;
    }
;

var_o_init:
    TOK_IDENTIFICADOR {
        $$ = nodo_nuevo("Variable", $1, yylineno);
    }
  | TOK_IDENTIFICADOR TOK_ASIGNAR expresion {
        $$ = nodo_nuevo("VarConInicio", $1, yylineno);
        nodo_agregar_hijo($$, $3);
    }
;

decl_arreglo:
    tipo TOK_IDENTIFICADOR
         TOK_CORCHETE_IZQUIERDO expresion TOK_CORCHETE_DERECHO {
        Node* tam = nodo_nuevo("Tamano", "", 0);
        nodo_agregar_hijo(tam, $4);
        $$ = nodo_nuevo("DeclaracionArreglo", $2, yylineno);
        nodo_agregar_hijo($$, $1);
        nodo_agregar_hijo($$, tam);
    }
  | tipo TOK_IDENTIFICADOR
         TOK_CORCHETE_IZQUIERDO expresion TOK_CORCHETE_DERECHO
         TOK_ASIGNAR
         TOK_LLAVE_IZQUIERDA lista_init TOK_LLAVE_DERECHA {
        Node* tam = nodo_nuevo("Tamano", "", 0);
        nodo_agregar_hijo(tam, $4);
        Node* ini = nodo_nuevo("ValoresIniciales", "", 0);
        nodo_agregar_hijo(ini, $8);
        $$ = nodo_nuevo("DeclaracionArreglo", $2, yylineno);
        nodo_agregar_hijo($$, $1);
        nodo_agregar_hijo($$, tam);
        nodo_agregar_hijo($$, ini);
    }
;

lista_init:
    expresion {
        $$ = nodo_nuevo("ListaValores", "", 0);
        nodo_agregar_hijo($$, $1);
    }
  | lista_init TOK_COMA expresion {
        nodo_agregar_hijo($1, $3);
        $$ = $1;
    }
;

%%