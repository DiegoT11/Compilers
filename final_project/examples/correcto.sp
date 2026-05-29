// Programa de prueba - sintaxis correcta

/* Declaraciones con y sin inicializacion */
entero contador = 0, limite = 10;
flotante promedio = 0.0;
booleano activo = verdadero;
cadena mensaje = "Hola mundo";

/* Asignacion simple */
contador = 5;

/* Sentencia si-sino encadenada */
si (contador > 0) {
    mensaje = "positivo";
} sino si (contador == 0) {
    mensaje = "cero";
} sino {
    mensaje = "negativo";
}

/* Mientras */
mientras (contador < limite) {
    contador = contador + 1;
    activo = verdadero && activo;
}

/* Para */
para (contador = 0; contador < 5; contador = contador + 1) {
    promedio = promedio + 1.5;
}

/* Expresiones complejas */
booleano resultado = (contador >= limite) || !activo;
entero calc = (contador + 2) * 3 - limite / 2;

/* Bloque anidado */
{
    entero local = 99;
    local = local - 1;
}
