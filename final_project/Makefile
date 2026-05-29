CC     = gcc
CFLAGS = -std=c99 -Wall -Wextra -g
BISON  = bison
FLEX   = flex

SRCS = src/ast.c src/semantic.c src/codegen.c src/main.c
GEN  = parser.tab.c parser.tab.h lexer.yy.c

# Build principal
all: simple

simple: parser.tab.c lexer.yy.c $(SRCS)
	$(CC) $(CFLAGS) -DBISON_NO_MAIN -o $@ parser.tab.c lexer.yy.c $(SRCS) -lm

# Generación de parser y lexer 
parser.tab.c parser.tab.h: src/parser.y
	$(BISON) -d -o parser.tab.c src/parser.y

lexer.yy.c: lexer.l parser.tab.h
	$(FLEX) -o lexer.yy.c src/lexer.l

# Prueba rápida
test: all
	@echo "Corriendo prueba con examples/correcto.sp"
	@if [ -f examples/correcto.sp ]; then \
		./simple < examples/correcto.sp; \
	else \
		echo "No se encontró examples/correcto.sp"; \
		echo "Crea un archivo .sp y ejecuta: ./simple < tu_archivo.sp"; \
	fi

# Visualizar AST
dot: asa.dot
	dot -Tpng asa.dot -o asa.png
	@echo "AST generado en asa.png"

# Limpieza
clean:
	rm -f $(GEN) simple

distclean: clean
	rm -f asa.dot asa.png codigo.ci

.PHONY: all test dot clean distclean