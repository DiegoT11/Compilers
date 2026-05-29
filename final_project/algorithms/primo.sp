entero n;
leer(n);
mostrar("Calculando el primo numero", n);

entero conteo    = 0;   // cuantos primos hemos encontrado
entero candidato = 1;   // numero que se esta probando
entero primo     = 0;   // ultimo primo encontrado (resultado)

mientras (conteo < n) {
    candidato = candidato + 1;

    // Probar si 'candidato' es primo
    entero divisor    = 2;
    booleano es_primo = verdadero;

    mientras (divisor < candidato) {
        // residuo = candidato mod divisor
        entero cociente = candidato / divisor;
        entero producto = cociente * divisor;
        entero residuo  = candidato - producto;

        si (residuo == 0) {
            es_primo = falso;
        }

        divisor = divisor + 1;
    }

    si (es_primo) {
        conteo = conteo + 1;
        primo  = candidato;
    }
}

mostrar("El primo numero", n, "es:", primo);
