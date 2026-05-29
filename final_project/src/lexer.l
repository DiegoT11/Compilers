%option noyywrap yylineno noinput nounput

%top{
    #define _POSIX_C_SOURCE 200809L
}

%{
#include "ast.h"
#include "parser.tab.h"
#include <string.h>

#define YY_USER_ACTION                                  \
    yylloc.first_line   = yylineno;                     \
    yylloc.last_line    = yylineno;                     \
    yylloc.first_column = yycolumn;                     \
    yylloc.last_column  = yycolumn + (int)yyleng - 1;  \
    yycolumn           += (int)yyleng;

int yycolumn = 1;
%}

%x COMMENT

ID      [a-zA-Z_][a-zA-Z0-9_]*
INT     [0-9]+
FLOAT   ({INT}\.[0-9]*)
WS      [ \t\r]+

%%

"/*"                { BEGIN(COMMENT); }
<COMMENT>"*/"       { BEGIN(INITIAL); }
<COMMENT>\n         { yycolumn = 1; }
<COMMENT>.          { }
<COMMENT><<EOF>>    {
    fprintf(stderr,
        "Error lexico [linea %d]: comentario de bloque sin cerrar\n",
        yylineno);
    return -1;
}

"//"[^\n]*  { }

\n      { yycolumn = 1; }
{WS}    { }

"entero"    { return TOK_KW_ENTERO;   }
"flotante"  { return TOK_KW_FLOTANTE; }
"booleano"  { return TOK_KW_BOOLEANO; }
"cadena"    { return TOK_KW_CADENA;   }

"si"        { return TOK_KW_SI;       }
"sino"      { return TOK_KW_SINO;     }
"mientras"  { return TOK_KW_MIENTRAS; }
"para"      { return TOK_KW_PARA;     }



"verdadero" { return TOK_VERDADERO; }
"falso"     { return TOK_FALSO;     }

"mostrar"   { return TOK_KW_MOSTRAR;  }
"leer"      { return TOK_KW_LEER;     }

"=="  { return TOK_IGUAL;        }
"!="  { return TOK_DIFERENTE_DE; }
"<="  { return TOK_MENOR_IGUAL;  }
">="  { return TOK_MAYOR_IGUAL;  }
"&&"  { return TOK_AND;          }
"||"  { return TOK_OR;           }

"="   { return TOK_ASIGNAR;        }
"<"   { return TOK_MENOR_QUE;      }
">"   { return TOK_MAYOR_QUE;      }
"!"   { return TOK_NOT;            }
"+"   { return TOK_MAS;            }
"-"   { return TOK_MENOS;          }
"*"   { return TOK_MULTIPLICACION; }
"/"   { return TOK_DIVISION;       }

"("   { return TOK_PARENTESIS_IZQUIERDO; }
")"   { return TOK_PARENTESIS_DERECHO;   }
"{"   { return TOK_LLAVE_IZQUIERDA;      }
"}"   { return TOK_LLAVE_DERECHA;        }
"["   { return TOK_CORCHETE_IZQUIERDO;   }
"]"   { return TOK_CORCHETE_DERECHO;     }
","   { return TOK_COMA;                 }
";"   { return TOK_PUNTO_COMA;           }

{FLOAT} {
    yylval.sval = strdup(yytext);
    return TOK_LITERAL_FLOTANTE;
}

{INT} {
    yylval.sval = strdup(yytext);
    return TOK_LITERAL_ENTERO;
}

\"[^\"]*\" {
    yylval.sval = strdup(yytext);
    return TOK_LITERAL_CADENA;
}

\"[^\"]*\n {
    fprintf(stderr,
        "Error lexico [linea %d]: cadena de texto sin cerrar\n",
        yylineno);
    return -1;
}

{ID} {
    yylval.sval = strdup(yytext);
    return TOK_IDENTIFICADOR;
}

<<EOF>> { return 0; }

. {
    fprintf(stderr,
        "Error lexico [linea %d, col %d]: caracter desconocido '%s'\n",
        yylineno, yycolumn - 1, yytext);
    return -1;
}

%%