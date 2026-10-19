/*
 * Generador de codigo objeto: codigo intermedio -> assembly x86-64.
 *
 * Decisiones de diseno:
 *  - Todo valor (variable, parametro o temporal) ocupa un slot de 8 bytes, y las
 *    operaciones son de 64 bits. Los booleanos se guardan como 0 o 1.
 *  - Las variables globales van en la seccion .bss, inicializadas en cero, y se
 *    acceden como nombre(%rip). Las locales y los temporales van en el frame.
 *  - Cada funcion del IR es la secuencia de cuadruplas entre ENTER y LEAVE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tablaSimbolos.h"
#include "codigoObjeto.h"

#define BYTES_SLOT 8

/* ---- Variables globales ---- */

typedef struct {
    Simbolo **simbolos;
    int cantidad;
    int capacidad;
} ListaGlobales;

static void agregarGlobal(ListaGlobales *globales, Simbolo *s) {
    for (int i = 0; i < globales->cantidad; i++) {
        if (globales->simbolos[i] == s) return; // ya registrada
    }
    if (globales->cantidad == globales->capacidad) {
        globales->capacidad = globales->capacidad ? globales->capacidad * 2 : 8;
        globales->simbolos = realloc(globales->simbolos, globales->capacidad * sizeof(Simbolo *));
        if (globales->simbolos == NULL) {
            fprintf(stderr, "Error: sin memoria para las variables globales\n");
            exit(EXIT_FAILURE);
        }
    }
    globales->simbolos[globales->cantidad++] = s;
}

static void registrarSiGlobal(ListaGlobales *globales, Operando o) {
    if (o.kind == OPD_VAR && o.u.simbolo->esGlobal) {
        agregarGlobal(globales, o.u.simbolo);
    }
}

// Reserva en .bss un slot por cada global que el IR usa. Las que nadie usa no ocupan lugar.
static void emitirGlobales(const ListaIR *lista, FILE *out) {
    ListaGlobales globales = { NULL, 0, 0 };

    for (const Cuadrupla *c = lista->cabeza; c != NULL; c = c->siguiente) {
        registrarSiGlobal(&globales, c->arg1);
        registrarSiGlobal(&globales, c->arg2);
        registrarSiGlobal(&globales, c->res);
    }

    if (globales.cantidad > 0) {
        fprintf(out, "\t.bss\n");
        fprintf(out, "\t.align %d\n", BYTES_SLOT);
        for (int i = 0; i < globales.cantidad; i++) {
            fprintf(out, "%s:\n\t.zero %d\n", globales.simbolos[i]->nombre, BYTES_SLOT);
        }
    }
    free(globales.simbolos);
}

/* ---- Funciones ---- */

// Genera la funcion que empieza en `enter` (una cuadrupla IR_ENTER) y devuelve la
// cuadrupla siguiente a su IR_LEAVE.
static const Cuadrupla *generarFuncion(const Cuadrupla *enter, FILE *out) {
    const char *nombre = enter->arg1.u.simbolo->nombre;

    fprintf(out, "\n");
    if (strcmp(nombre, "main") == 0) {
        fprintf(out, "\t.globl main\n");
    }
    fprintf(out, "%s:\n", nombre);

    const Cuadrupla *c = enter->siguiente;
    while (c != NULL && c->op != IR_LEAVE) {
        c = c->siguiente; // TODO: traducir el cuerpo (pasos siguientes)
    }
    return c != NULL ? c->siguiente : NULL;
}

void generarAssembly(ListaIR *lista, FILE *out) {
    emitirGlobales(lista, out);

    fprintf(out, "\t.text\n");
    const Cuadrupla *c = lista->cabeza;
    while (c != NULL) {
        c = c->op == IR_ENTER ? generarFuncion(c, out) : c->siguiente;
    }

    // Evita que el linker marque el stack como ejecutable
    fprintf(out, "\n\t.section .note.GNU-stack,\"\",@progbits\n");
}
