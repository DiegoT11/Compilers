entero n;
leer(n);
mostrar("Rutina de Kaprekar para:", n);

entero pasos = 0;

mientras (n != 6174) {
    // --- Extraer digitos de n ---
    entero tmp = n;

    entero d0 = tmp / 1000;
    tmp = tmp - d0 * 1000;

    entero d1 = tmp / 100;
    tmp = tmp - d1 * 100;

    entero d2 = tmp / 10;
    entero d3 = tmp - d2 * 10;

    // --- Ordenar d0..d3 de MAYOR a MENOR (burbuja desenrollada) ---
    // Burbuja pasada 1
    entero aux = 0;

    si (d0 < d1) { aux = d0; d0 = d1; d1 = aux; }
    si (d1 < d2) { aux = d1; d1 = d2; d2 = aux; }
    si (d2 < d3) { aux = d2; d2 = d3; d3 = aux; }

    // Burbuja pasada 2
    si (d0 < d1) { aux = d0; d0 = d1; d1 = aux; }
    si (d1 < d2) { aux = d1; d1 = d2; d2 = aux; }

    // Burbuja pasada 3
    si (d0 < d1) { aux = d0; d0 = d1; d1 = aux; }

    // Numero mayor (digitos desc): d0 d1 d2 d3
    entero mayor = d0 * 1000 + d1 * 100 + d2 * 10 + d3;

    // Numero menor (digitos asc):  d3 d2 d1 d0
    entero menor = d3 * 1000 + d2 * 100 + d1 * 10 + d0;

    mostrar("Mayor:", mayor, "  Menor:", menor);

    n = mayor - menor;
    pasos = pasos + 1;

    mostrar("Diferencia:", n);
}

mostrar("Constante de Kaprekar (6174) alcanzada en", pasos, "pasos.");
