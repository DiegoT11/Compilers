entero a;
entero b;

leer(a);
leer(b);

mostrar("Algoritmo de Euclides Extendido");
mostrar("a =", a, "  b =", b);

// Estado inicial
entero old_r = a;
entero r     = b;

entero old_s = 1;
entero s     = 0;

entero old_t = 0;
entero t     = 1;

// Iterar hasta que el resto sea 0
mientras (r != 0) {
    entero cociente = old_r / r;

    // Actualizar resto
    entero temp_r = r;
    r     = old_r - cociente * temp_r;
    old_r = temp_r;

    // Actualizar coeficiente s
    entero temp_s = s;
    s     = old_s - cociente * temp_s;
    old_s = temp_s;

    // Actualizar coeficiente t
    entero temp_t = t;
    t     = old_t - cociente * temp_t;
    old_t = temp_t;
}

// Al terminar: old_r = mcd, old_s = x, old_t = y
mostrar("MCD(a, b) =", old_r);
mostrar("Coeficiente x (para a):", old_s);
mostrar("Coeficiente y (para b):", old_t);
mostrar("Verificacion: a*x + b*y deberia ser igual al MCD.");

// Verificacion numerica: calculamos a*x + b*y
entero verificacion = a * old_s + b * old_t;
mostrar("a*x + b*y =", verificacion);
