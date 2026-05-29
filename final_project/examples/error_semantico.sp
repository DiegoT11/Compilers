// Archivo de prueba — errores semánticos
// Cada bloque demuestra un tipo de error diferente.
// El compilador debe reportar TODOS sin abortar en el primero.

// 1. Variable no declarada
x = 10;

// 2. Redeclaración en el mismo bloque
entero contador = 0;
entero contador = 5;

// 3. Pérdida de precisión: flotante → entero
flotante promedio = 3.14;
entero resultado = promedio;

// 4. Tipo incompatible en operación aritmética
cadena nombre = "Simple";
entero calc = nombre + 1;

// 5. Condición no booleana en si
entero nivel = 3;
si (nivel) {
    mostrar("nunca");
}

// 6. Operador lógico con tipo no booleano
booleano bandera = verdadero && nivel;

// 7. Shadowing válido (esto NO es error, debe compilar bien)
{
    flotante contador = 9.9;
    mostrar(contador);
}
