%option noyywrap yylineno

%{ 
	#include "scanner.h" 
%}

%x COMMENT 

ID [a-zA-Z_][a-zA-Z_0-9]*
INT [0-9]+
FLOAT ( ({INT}\.{INT}) | ({INT}\.{INT})([eE][+-]?{INT}) | {INT}(e[+-]?{INT}) )
WS [ \t\r\n]+

%%

"/*" {BEGIN(COMMENT);}
<COMMENT>"*/"    {BEGIN(INITIAL);}
<COMMENT>\n {}
<COMMENT>. {}
<COMMENT><<EOF>> {return TOK_ERROR;}
"//"[^\n]* {}


/*
SECCIÓN 7: Espacios en blanco
- Regla usando {WS} para ignorar whitespace.
*/

/*
SECCIÓN 8: Palabras reservadas
- Agrega reglas exactas para:
	int, float, double, char, void,
	if, else, while, for, return, break, continue
- Cada una debe retornar su token TOK_KW_* correspondiente.
*/

/*
SECCIÓN 9: Operadores compuestos (siempre antes de los simples)
- ++, --, +=, -=, *=, /=, %=, ==, !=, <=, >=, &&, ||
- Devuelven los TOK_* correspondientes.
*/
%%
"++" { return TOK_INC; }
"--" { return TOK_DEC; }
"+=" { return TOK_PLUS_ASSIGN;_}
"-=" { return TOK_MINUS_ASSING; }
"*=" { return TOK_MUL_ASSIGN; }
"/=" { return TOK_DIV_ASSIGN; }
"%=" { return TOK_MOD_ASSIGN; }

"==" { return TOK_EQ; }
"!=" { return TOK_NEQ; }
"<=" { return TOK_LE; }
">=" { return TOK_GE;}

"&&" { return TOK_AND}
"||" { return TOK_OR}
%%

/*
SECCIÓN 10: Operadores simples
- =, <, >, !, +, -, *, /, %
- Devuelven los TOK_* correspondientes.
*/
%%
"=" { return TOK_EQ; }
"<" { return TOK_LT; }
">" { return TOK_GT; }
"!" { return TOK_NOT; }
"+" { return TOK_PLUS; }
"-" { return TOK_MINUS; }
"*" { return TOK_MUL; }
"/" { return TOK_DIV; }
"%" { return TOK_MOD; }
%%

/*
SECCIÓN 11: Delimitadores
- (, ), {, }, [, ], ,, ;
- Devuelven los TOK_* correspondientes.
*/

/*
SECCIÓN 12: Literales e identificadores (orden importante)
- Primero FLOAT -> TOK_FLOAT_LITERAL.
- Luego INT -> TOK_INT_LITERAL.
- Luego literal de char (admitiendo escapes) -> TOK_CHAR_LITERAL.
- Luego literal de string (admitiendo escapes) -> TOK_STRING_LITERAL.
- Finalmente ID -> TOK_IDENTIFIER.

Nota: si pones ID antes de keywords, las keywords se reconocerán como identificadores.
*/

/*
SECCIÓN 13: Fin de archivo y error genérico
- Regla <<EOF>> que retorna TOK_EOF.
- Regla final con . que retorna TOK_ERROR para cualquier carácter no reconocido.
*/


%%