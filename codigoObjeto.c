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

/* ---- Traduccion de cuadruplas ---- */

static int offsetTemporal(const Frame *f, int id) {
    for (int i = 0; i < f->cantTemps; i++) {
        if (f->temps[i].id == id) return f->temps[i].offset;
    }
    fprintf(stderr, "Codigo objeto: temporal t%d sin lugar en el frame\n", id);
    return 0;
}

// Operando de memoria (variable o temporal) en sintaxis AT&T
static void escribirMemoria(FILE *out, const Frame *f, Operando o) {
    if (o.kind == OPD_TEMP) {
        fprintf(out, "%d(%%rbp)", offsetTemporal(f, o.u.id));
    } else if (o.u.simbolo->esGlobal) {
        fprintf(out, "%s(%%rip)", o.u.simbolo->nombre);
    } else {
        fprintf(out, "%d(%%rbp)", o.u.simbolo->offset);
    }
}

// registro = valor del operando
static void cargar(FILE *out, const Frame *f, Operando o, const char *registro) {
    if (o.kind == OPD_CONST) {
        fprintf(out, "\tmov $%d, %s\n", o.u.valor, registro);
        return;
    }
    fprintf(out, "\tmov ");
    escribirMemoria(out, f, o);
    fprintf(out, ", %s\n", registro);
}

// operando = registro
static void guardar(FILE *out, const Frame *f, Operando o, const char *registro) {
    fprintf(out, "\tmov %s, ", registro);
    escribirMemoria(out, f, o);
    fprintf(out, "\n");
}

// res = arg1 <op> arg2, con op una instruccion de dos operandos (add, sub, imul)
static void traducirAritmetica(FILE *out, const Frame *f, const Cuadrupla *c, const char *instruccion) {
    cargar(out, f, c->arg1, "%r10");
    cargar(out, f, c->arg2, "%r11");
    fprintf(out, "\t%s %%r11, %%r10\n", instruccion);
    guardar(out, f, c->res, "%r10");
}

// Division entera con signo: idiv divide rdx:rax, asi que antes hay que extender el signo con cqto.
// El cociente queda en %rax y el resto en %rdx.
static void traducirDivision(FILE *out, const Frame *f, const Cuadrupla *c, const char *resultado) {
    cargar(out, f, c->arg1, "%rax");
    cargar(out, f, c->arg2, "%r10");
    fprintf(out, "\tcqto\n");
    fprintf(out, "\tidiv %%r10\n");
    guardar(out, f, c->res, resultado);
}

// res = (arg1 <cmp> arg2) como 0 o 1. Los mov no modifican los flags de cmp, asi que
// se precargan 0 y 1 y un cmov elige cual queda.
static void traducirComparacion(FILE *out, const Frame *f, const Cuadrupla *c, const char *cmov) {
    cargar(out, f, c->arg1, "%r10");
    cargar(out, f, c->arg2, "%r11");
    fprintf(out, "\tcmp %%r11, %%r10\n"); // flags de arg1 - arg2
    fprintf(out, "\tmov $0, %%r11\n");
    fprintf(out, "\tmov $1, %%r10\n");
    fprintf(out, "\t%s %%r10, %%r11\n", cmov);
    guardar(out, f, c->res, "%r11");
}

// Argumentos acumulados por los IR_PARAM que preceden a un IR_CALL
typedef struct {
    Operando args[MAX_PARAMS];
    int cantidad;
} Pendientes;

// Los primeros 6 argumentos van en registros y el resto en la pila, en orden inverso
// (el septimo queda en 16(%rbp) del callee). Si la cantidad de argumentos de pila es
// impar se reserva un slot de relleno para mantener %rsp alineado a 16 en el call.
static void traducirLlamada(FILE *out, const Frame *f, const Cuadrupla *c, Pendientes *p) {
    int enPila = p->cantidad > PARAMS_EN_REGISTROS ? p->cantidad - PARAMS_EN_REGISTROS : 0;
    int relleno = enPila % 2;

    if (relleno) {
        fprintf(out, "\tsub $%d, %%rsp\n", BYTES_SLOT);
    }
    for (int i = p->cantidad - 1; i >= PARAMS_EN_REGISTROS; i--) {
        cargar(out, f, p->args[i], "%r10");
        fprintf(out, "\tpush %%r10\n");
    }
    for (int i = 0; i < p->cantidad && i < PARAMS_EN_REGISTROS; i++) {
        cargar(out, f, p->args[i], REGISTROS_PARAMS[i]);
    }

    // Las externas son funciones de C, asi que hay que respetar dos cosas:
    //  - antes del call, %rax en 0: printInt acepta cantidad variable de argumentos y C
    //    mira ese registro para saber cuantos registros especiales se usaron (nosotros, ninguno)
    //  - despues del call, cltq: C devuelve el int en 32 bits (%eax) pero nosotros guardamos
    //    enteros de 64, entonces hay que extender el signo o un -5 quedaria como un numero enorme
    int externa = c->arg1.u.simbolo->esExterna;
    if (externa) {
        fprintf(out, "\tmov $0, %%rax\n");
    }
    fprintf(out, "\tcall %s\n", c->arg1.u.simbolo->nombre);
    if (externa && c->res.kind != OPD_NONE) {
        fprintf(out, "\tcltq\n");
    }

    if (enPila > 0) {
        fprintf(out, "\tadd $%d, %%rsp\n", (enPila + relleno) * BYTES_SLOT);
    }
    if (c->res.kind != OPD_NONE) {
        guardar(out, f, c->res, "%rax");
    }
    p->cantidad = 0;
}

static void traducirCuadrupla(FILE *out, const Frame *f, const Cuadrupla *c, Pendientes *p) {
    switch (c->op) {
        case IR_ADD:  traducirAritmetica(out, f, c, "add"); break;
        case IR_SUB:  traducirAritmetica(out, f, c, "sub"); break;
        case IR_MULT: traducirAritmetica(out, f, c, "imul"); break;
        case IR_DIV:  traducirDivision(out, f, c, "%rax"); break;
        case IR_MOD:  traducirDivision(out, f, c, "%rdx"); break;
        case IR_NEG:
            cargar(out, f, c->arg1, "%r10");
            fprintf(out, "\tneg %%r10\n");
            guardar(out, f, c->res, "%r10");
            break;
        case IR_NOT: // los booleanos son 0 o 1
            cargar(out, f, c->arg1, "%r10");
            fprintf(out, "\txor $1, %%r10\n");
            guardar(out, f, c->res, "%r10");
            break;
        case IR_LT: traducirComparacion(out, f, c, "cmovl"); break;
        case IR_GT: traducirComparacion(out, f, c, "cmovg"); break;
        case IR_EQ: traducirComparacion(out, f, c, "cmove"); break;
        case IR_ASSIGN:
            cargar(out, f, c->arg1, "%r10");
            guardar(out, f, c->res, "%r10");
            break;
        case IR_LABEL:
            fprintf(out, ".L%d:\n", c->res.u.id);
            break;
        case IR_JUMP:
            fprintf(out, "\tjmp .L%d\n", c->res.u.id);
            break;
        case IR_JMPF:
            cargar(out, f, c->arg1, "%r10");
            fprintf(out, "\tcmp $0, %%r10\n");
            fprintf(out, "\tje .L%d\n", c->res.u.id);
            break;
        case IR_PARAM:
            if (p->cantidad < MAX_PARAMS) p->args[p->cantidad++] = c->arg1;
            break;
        case IR_CALL:
            traducirLlamada(out, f, c, p);
            break;
        case IR_RET:
            if (c->arg1.kind != OPD_NONE) {
                cargar(out, f, c->arg1, "%rax");
            } else {
                fprintf(out, "\tmov $0, %%rax\n"); // asi main termina con estado 0
            }
            fprintf(out, "\tleave\n\tret\n");
            break;
        case IR_LEAVE:
            fprintf(out, "\tleave\n\tret\n");
            break;
        case IR_ENTER: // lo emite generarFuncion
            break;
    }
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

    Pendientes pendientes = { .cantidad = 0 };
    const Cuadrupla *c = enter->siguiente;
    while (c != NULL && c->op != IR_LEAVE) {
        traducirCuadrupla(out, &frame, c, &pendientes);
        c = c->siguiente;
    }
    if (c != NULL) {
        traducirCuadrupla(out, &frame, c, &pendientes); // el IR_LEAVE cierra la funcion
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