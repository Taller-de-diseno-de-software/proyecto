/*
 * Biblioteca de ejecucion de C-TDS: las funciones que un programa declara con extern.
 * Se enlaza con el assembly generado.
 */
#include <stdio.h>

// Lee un entero de la entrada estandar (0 si no hay uno)
int get_int(void) {
    int valor;
    if (scanf("%d", &valor) != 1) return 0;
    return valor;
}

// Escribe un entero seguido de un salto de linea
void printInt(int valor) {
    printf("%d\n", valor);
}
