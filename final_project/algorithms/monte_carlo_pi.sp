entero total;
leer(total);
mostrar("Muestras a usar:", total);

// Generador congruencial lineal (LCG)
// semilla inicial (puede cambiarse)
entero semilla = 12345;

// Constantes del LCG
entero A = 1103515245;
entero C = 12345;
// Modulo M = 2147483647 (no cabe en multiplicacion directa
// con A, pero Simple solo tiene enteros — usaremos M reducido)
// Usamos M = 32767 para mantener el producto A*semilla manejable
// con numeros de Simple.
//
// LCG ligero: semilla = (214013 * semilla + 2531011) mod 32768
// Rango de salida: [0, 32767]   =>  ESCALA = 32768
entero M = 32768;
entero ESCALA = 32768;

entero dentro  = 0;
entero i       = 0;

mientras (i < total) {
    // Generar x en [0, ESCALA)
    semilla = semilla * 214013 + 2531011;
    // semilla mod M  usando division entera
    entero q = semilla / M;
    entero r = semilla - q * M;
    // Aseguramos positivo (Simple no tiene abs, usamos truco)
    si (r < 0) {
        r = r + M;
    }
    entero x = r;

    // Generar y en [0, ESCALA)
    semilla = semilla * 214013 + 2531011;
    q = semilla / M;
    r = semilla - q * M;
    si (r < 0) {
        r = r + M;
    }
    entero y = r;

    // Comprobar si (x, y) cae dentro del cuarto de circulo
    // x^2 + y^2 < ESCALA^2
    entero dist2  = x * x + y * y;
    entero limite = ESCALA * ESCALA;

    si (dist2 < limite) {
        dentro = dentro + 1;
    }

    i = i + 1;
}

// Pi ≈ 4 * dentro / total  (aritmetica entera escalada)
// Para dar mas decimales: calculamos 10000 * Pi / 1
entero pi_num   = 4 * dentro;         // numerador
entero pi_mil   = pi_num * 1000;      // escalar x1000 para decimales
entero pi_ent   = pi_mil / total;     // parte entera * 1000

// Separar parte entera y decimal
entero parte_entera   = pi_ent / 1000;
entero parte_decimal  = pi_ent - parte_entera * 1000;

mostrar("Puntos dentro del circulo:", dentro);
mostrar("Total de puntos:", total);
mostrar("Pi aproximado (parte entera):", parte_entera);
mostrar("Pi aproximado (milésimas):", parte_decimal);
mostrar("(Interpretar como parte_entera.parte_decimal)");
mostrar("Referencia: Pi = 3.14159...");
