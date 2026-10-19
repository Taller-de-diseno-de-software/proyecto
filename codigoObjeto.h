#ifndef CODIGO_OBJETO_H
#define CODIGO_OBJETO_H

#include <stdio.h>
#include "codigoIntermedio.h"

// Traduce el codigo intermedio a assembly x86-64 (sintaxis AT&T, ABI System V) y lo escribe en `out`.
// Sin optimizaciones: cada variable y temporal vive en un lugar del frame y las
// operaciones usan %r10 y %r11 como registros de trabajo.
void generarAssembly(ListaIR *lista, FILE *out);

#endif // CODIGO_OBJETO_H
