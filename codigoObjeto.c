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

/* ---- Frame de una funcion ---- */

/*
 * Layout del frame (la pila crece hacia abajo):
 *   16+8*(i-6)(%rbp)  parametro i, para i >= 6 (lo dejo el llamador en la pila)
 *   8(%rbp)           direccion de retorno
 *   0(%rbp)           %rbp del llamador
 *   -8(%rbp)...       parametros 0..5 (copiados desde registros), locales y temporales
 */
#define PARAMS_EN_REGISTROS 6
#define BASE_PARAMS_PILA 16 // 0(%rbp) = %rbp guardado, 8(%rbp) = direccion de retorno

static const char *REGISTROS_PARAMS[PARAMS_EN_REGISTROS] = { "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9" };

typedef struct {
    int id;
    int offset;
} SlotTemp;

typedef struct {
    SlotTemp *temps;
    int cantTemps;
    int capTemps;
    int slots; // slots de 8 bytes ya asignados debajo de %rbp
} Frame;

static int nuevoSlot(Frame *f) {
    return -BYTES_SLOT * ++f->slots;
}

// Las locales y parametros guardan su offset en el Simbolo. Un offset 0 significa "sin asignar":
// nunca es valido porque 0(%rbp) es el %rbp guardado.
static void asignarVariable(Frame *f, Simbolo *s) {
    if (!s->esGlobal && s->offset == 0) {
        s->offset = nuevoSlot(f);
    }
}

// Los ids de temporales son unicos en todo el programa, pero cada funcion tiene su propio frame
static void asignarTemporal(Frame *f, int id) {
    for (int i = 0; i < f->cantTemps; i++) {
        if (f->temps[i].id == id) return;
    }
    if (f->cantTemps == f->capTemps) {
        f->capTemps = f->capTemps ? f->capTemps * 2 : 16;
        f->temps = realloc(f->temps, f->capTemps * sizeof(SlotTemp));
        if (f->temps == NULL) {
            fprintf(stderr, "Error: sin memoria para los temporales\n");
            exit(EXIT_FAILURE);
        }
    }
    f->temps[f->cantTemps].id = id;
    f->temps[f->cantTemps].offset = nuevoSlot(f);
    f->cantTemps++;
}

static void asignarOperando(Frame *f, Operando o) {
    if (o.kind == OPD_VAR) {
        asignarVariable(f, o.u.simbolo);
    } else if (o.kind == OPD_TEMP) {
        asignarTemporal(f, o.u.id);
    }
}

// Asigna un lugar a los parametros, las locales y los temporales de la funcion que empieza en
// `enter`. Los primeros 6 parametros llegan en registros y se copian al frame; el resto ya
// esta en la pila del llamador, asi que conserva esa direccion.
static void armarFrame(const Cuadrupla *enter, Frame *f) {
    const Simbolo *funcion = enter->arg1.u.simbolo;

    f->temps = NULL;
    f->cantTemps = 0;
    f->capTemps = 0;
    f->slots = 0;

    for (int i = 0; i < funcion->numParams; i++) {
        Simbolo *p = funcion->params[i];
        p->offset = i < PARAMS_EN_REGISTROS ? nuevoSlot(f)
                                            : BASE_PARAMS_PILA + BYTES_SLOT * (i - PARAMS_EN_REGISTROS);
    }

    for (const Cuadrupla *c = enter->siguiente; c != NULL && c->op != IR_LEAVE; c = c->siguiente) {
        asignarOperando(f, c->arg1);
        asignarOperando(f, c->arg2);
        asignarOperando(f, c->res);
    }
}

// Bytes que reserva `enter`, redondeados a multiplo de 16 para que la pila quede alineada
// en cada call (la ABI lo exige): al entrar %rsp es 8 mod 16 y el push de %rbp lo deja en 0.
static int tamanioFrame(const Frame *f) {
    int bytes = f->slots * BYTES_SLOT;
    return (bytes + 15) / 16 * 16;
}

/* ---- Funciones ---- */

// Genera la funcion que empieza en `enter` (una cuadrupla IR_ENTER) y devuelve la
// cuadrupla siguiente a su IR_LEAVE.
static const Cuadrupla *generarFuncion(const Cuadrupla *enter, FILE *out) {
    const Simbolo *funcion = enter->arg1.u.simbolo;
    Frame frame;

    armarFrame(enter, &frame);

    fprintf(out, "\n");
    if (strcmp(funcion->nombre, "main") == 0) {
        fprintf(out, "\t.globl main\n");
    }
    fprintf(out, "%s:\n", funcion->nombre);
    fprintf(out, "\tenter $%d, $0\n", tamanioFrame(&frame));
    for (int i = 0; i < funcion->numParams && i < PARAMS_EN_REGISTROS; i++) {
        fprintf(out, "\tmov %s, %d(%%rbp)\n", REGISTROS_PARAMS[i], funcion->params[i]->offset);
    }

    const Cuadrupla *c = enter->siguiente;
    while (c != NULL && c->op != IR_LEAVE) {
        c = c->siguiente; // TODO: traducir el cuerpo (paso siguiente)
    }
    free(frame.temps);
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
