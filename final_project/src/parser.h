#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

extern Node* raiz;

/* Contadores de posición */
extern int yylineno;
extern int yycolumn;

/* Punto de entrada del parser generado por Bison */
extern int yyparse(void);

/* Manejador de errores sintácticos */
void yyerror(const char* s);

#endif 