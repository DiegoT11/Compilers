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

{WS} {}

"int" {return TOK_KW_INT;}
"float" {return TOK_KW_FLOAT;}
"double" {return TOK_KW_DOUBLE:}
"char" {return TOK_KW_CHAR;}
"void" {return TOK_KW_VOID;}
"if" {return TOK_KW_IF;}
"else" {return TOK_KW_ELSE;}
"while" {return TOK_KW_WHILE;}
"for" {return TOK_KW_FOR;}
"return" {return TOK_KW_RETURN;}
"break" {return TOK_KW_BREAK;}
"continue" {return TOK_KW_CONTINUE;}

/*
SECCIÓN 9: Operadores compuestos (siempre antes de los simples)
- ++, --, +=, -=, *=, /=, %=, ==, !=, <=, >=, &&, ||
- Devuelven los TOK_* correspondientes.
*/

/*
SECCIÓN 10: Operadores simples
- =, <, >, !, +, -, *, /, %
- Devuelven los TOK_* correspondientes.
*/

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

<<EOF>> {return TOK_EOF}
. {return TOK_EOF}

%%