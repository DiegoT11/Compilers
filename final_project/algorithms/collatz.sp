entero n;
leer(n);
mostrar("Secuencia de Collatz para n =", n);

entero pasos = 0;

mientras (n != 1) {
    mostrar(n);

    // Calcular residuo de n mod 2 usando division entera
    entero mitad    = n / 2;
    entero doble    = mitad * 2;
    entero residuo  = n - doble;

    si (residuo == 0) {
        n = n / 2;
    } sino {
        n = n * 3 + 1;
    }

    pasos = pasos + 1;
}

mostrar(1);
pasos = pasos + 1;

mostrar("Total de pasos hasta llegar a 1:", pasos);
