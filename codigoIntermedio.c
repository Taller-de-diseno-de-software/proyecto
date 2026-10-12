#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "analizadorSemantico.h"
#include "codigoIntermedio.h"

static int contadorTemp = 0;
static int contadorEtiqueta = 0;

// Lista donde emite la generacion en curso (la fija generarCodigoIntermedio)
static ListaIR *listaActual = NULL;

Operando operandoNone(void) {
    Operando o = { OPD_NONE };
    return o;
}

Operando operandoConst(int valor) {
    Operando o = { OPD_CONST };
    o.u.valor = valor;
    return o;
}

Operando operandoVar(Simbolo *s) {
    Operando o = { OPD_VAR };
    o.u.simbolo = s;
    return o;
}

Operando operandoTemp(int id) {
    Operando o = { OPD_TEMP };
    o.u.id = id;
    return o;
}

Operando operandoLabel(int id) {
    Operando o = { OPD_LABEL };
    o.u.id = id;
    return o;
}

Operando operandoFunc(Simbolo *f) {
    Operando o = { OPD_FUNC };
    o.u.simbolo = f;
    return o;
}



Operando nuevoTemp(void) {
    return operandoTemp(++contadorTemp);
}

Operando nuevaEtiqueta(void) {
    return operandoLabel(++contadorEtiqueta);
}



ListaIR *crearLista(void) {
    ListaIR *l = malloc(sizeof(ListaIR));
    if (l == NULL) {
        fprintf(stderr, "Error: sin memoria para la lista de codigo intermedio\n");
        exit(EXIT_FAILURE);
    }
    l->cabeza = NULL;
    l->cola = NULL;
    l->cantidad = 0;
    return l;
}

void emitir(ListaIR *lista, OperadorIR op, Operando a, Operando b, Operando res) {
    Cuadrupla *c = malloc(sizeof(Cuadrupla));
    if (c == NULL) {
        fprintf(stderr, "Error: sin memoria para una cuadrupla\n");
        exit(EXIT_FAILURE);
    }
    c->op = op;
    c->arg1 = a;
    c->arg2 = b;
    c->res = res;
    c->siguiente = NULL;

    if (lista->cola == NULL) {
        lista->cabeza = c;
    } else {
        lista->cola->siguiente = c;
    }
    lista->cola = c;
    lista->cantidad++;
}

void liberarLista(ListaIR *lista) {
    if (lista == NULL) return;
    Cuadrupla *c = lista->cabeza;
    while (c != NULL) {
        Cuadrupla *sig = c->siguiente;
        free(c);
        c = sig;
    }
    free(lista);
}



/* ---- Impresion ---- */

static void imprimirOperando(Operando o, FILE *out) {
    switch (o.kind) {
        case OPD_CONST: fprintf(out, "%d", o.u.valor); break;
        case OPD_VAR:
        case OPD_FUNC:  fprintf(out, "%s", o.u.simbolo->nombre); break;
        case OPD_TEMP:  fprintf(out, "t%d", o.u.id); break;
        case OPD_LABEL: fprintf(out, "L%d", o.u.id); break;
        case OPD_NONE:  break;
    }
}

// Simbolo infijo de las instrucciones binarias; NULL si la instruccion no es binaria
static const char *simboloBinario(OperadorIR op) {
    switch (op) {
        case IR_ADD:  return "+";
        case IR_SUB:  return "-";
        case IR_MULT: return "*";
        case IR_DIV:  return "/";
        case IR_MOD:  return "%";
        case IR_AND:  return "&&";
        case IR_OR:   return "||";
        case IR_LT:   return "<";
        case IR_GT:   return ">";
        case IR_EQ:   return "==";
        default:      return NULL;
    }
}

static void imprimirCuadrupla(const Cuadrupla *c, FILE *out) {
    const char *simbolo = simboloBinario(c->op);
    if (simbolo != NULL) {
        imprimirOperando(c->res, out);
        fprintf(out, " = ");
        imprimirOperando(c->arg1, out);
        fprintf(out, " %s ", simbolo);
        imprimirOperando(c->arg2, out);
        fprintf(out, "\n");
        return;
    }

    switch (c->op) {
        case IR_NEG:
        case IR_NOT:
            imprimirOperando(c->res, out);
            fprintf(out, " = %s", c->op == IR_NEG ? "-" : "!");
            imprimirOperando(c->arg1, out);
            break;
        case IR_ASSIGN:
            imprimirOperando(c->res, out);
            fprintf(out, " = ");
            imprimirOperando(c->arg1, out);
            break;
        case IR_LABEL:
            imprimirOperando(c->res, out);
            fprintf(out, ":");
            break;
        case IR_JUMP:
            fprintf(out, "goto ");
            imprimirOperando(c->res, out);
            break;
        case IR_JMPF:
            fprintf(out, "ifFalse ");
            imprimirOperando(c->arg1, out);
            fprintf(out, " goto ");
            imprimirOperando(c->res, out);
            break;
        case IR_ENTER:
            fprintf(out, "enter ");
            imprimirOperando(c->arg1, out);
            break;
        case IR_PARAM:
            fprintf(out, "param ");
            imprimirOperando(c->arg1, out);
            break;
        case IR_CALL:
            if (c->res.kind != OPD_NONE) {
                imprimirOperando(c->res, out);
                fprintf(out, " = ");
            }
            fprintf(out, "call ");
            imprimirOperando(c->arg1, out);
            fprintf(out, ", ");
            imprimirOperando(c->arg2, out);
            break;
        case IR_RET:
            fprintf(out, "return");
            if (c->arg1.kind != OPD_NONE) {
                fprintf(out, " ");
                imprimirOperando(c->arg1, out);
            }
            break;
        case IR_LEAVE:
            fprintf(out, "leave");
            break;
        default:
            break;
    }
    fprintf(out, "\n");
}

void imprimirIR(ListaIR *lista, FILE *out) {
    for (const Cuadrupla *c = lista->cabeza; c != NULL; c = c->siguiente) {
        imprimirCuadrupla(c, out);
    }
}



/* ---- Generacion de expresiones ---- */

static Operando genExpr(nodoAST *e);

// Evalua las expresiones de la cadena de argumentos de izquierda a derecha y deja cada
// operando en `valores`. La cadena es ARG(EXPR, ARGS(ARG(EXPR, ...))): se recorre en preorden.
static void evaluarArgumentos(nodoAST *args, Operando *valores, int *cantidad) {
    if (args == NULL) return;

    if (args->tipo == NODO_EXPR) {
        if (*cantidad >= MAX_PARAMS) {
            fprintf(stderr, "Codigo intermedio: demasiados argumentos (maximo %d)\n", MAX_PARAMS);
            return;
        }
        valores[(*cantidad)++] = genExpr(args->hijos[0]);
        return;
    }
    evaluarArgumentos(args->hijos[0], valores, cantidad);
    evaluarArgumentos(args->hijos[1], valores, cantidad);
}

// Llamada (ARGUMENTS_CALL o NO_ARGUMENTS_CALL). Primero se evaluan TODOS los argumentos y
// recien despues se emiten los PARAM: un argumento puede ser otra llamada, y sus PARAM
// no deben intercalarse con los de la externa. Devuelve el temporal con el resultado, o
// operandoNone() si la funcion es void.
static Operando genLlamada(nodoAST *llamada) {
    Simbolo *funcion = llamada->simbolo;
    Operando valores[MAX_PARAMS];
    int cantidad = 0;

    evaluarArgumentos(llamada->hijos[0], valores, &cantidad);
    for (int i = 0; i < cantidad; i++) {
        emitir(listaActual, IR_PARAM, valores[i], operandoNone(), operandoNone());
    }

    Operando resultado = funcion->tipo == TIPO_VOID ? operandoNone() : nuevoTemp();
    emitir(listaActual, IR_CALL, operandoFunc(funcion), operandoConst(cantidad), resultado);
    return resultado;
}

// El literal viene doblemente envuelto: LITERAL(LITERAL "texto"); el texto esta en el hijo
static Operando genLiteral(const nodoAST *e) {
    const nodoAST *literal = e->hijos[0] ? e->hijos[0] : e;
    const char *texto = literal->valor;

    if (strcmp(texto, "true") == 0) return operandoConst(1);
    if (strcmp(texto, "false") == 0) return operandoConst(0);

    if (strpbrk(texto, ".eE") != NULL) {
        // Limitacion documentada: el IR solo cubre enteros y logicos, el float se trunca
        return operandoConst((int)strtod(texto, NULL));
    }
    return operandoConst((int)strtol(texto, NULL, 10));
}

static Operando genBinaria(OperadorIR op, nodoAST *e) {
    Operando a = genExpr(e->hijos[0]);
    Operando b = genExpr(e->hijos[1]);
    Operando t = nuevoTemp(); // despues de los operandos: el subresultado toma el temporal menor
    emitir(listaActual, op, a, b, t);
    return t;
}

static Operando genUnaria(OperadorIR op, nodoAST *e) {
    Operando a = genExpr(e->hijos[0]);
    Operando t = nuevoTemp();
    emitir(listaActual, op, a, operandoNone(), t);
    return t;
}

// Devuelve el operando donde queda el valor de la expresion. Las hojas (constantes y
// variables) no emiten codigo: se usan directamente como operando.
static Operando genExpr(nodoAST *e) {
    switch (e->tipo) {
        case NODO_EXPR:        return genExpr(e->hijos[0]); // envoltorio de argumentos y return
        case NODO_LITERAL:     return genLiteral(e);
        case NODO_ID:          return operandoVar(e->simbolo);
        case NODO_METHOD_CALL: return genLlamada(e->hijos[0]);
        case NODO_SUMA:        return genBinaria(IR_ADD, e);
        case NODO_RESTA:       return genBinaria(IR_SUB, e);
        case NODO_PROD:        return genBinaria(IR_MULT, e);
        case NODO_DIV:         return genBinaria(IR_DIV, e);
        case NODO_DIVENT:      return genBinaria(IR_MOD, e);
        case NODO_MENOR:       return genBinaria(IR_LT, e);
        case NODO_MAYOR:       return genBinaria(IR_GT, e);
        case NODO_EQ:          return genBinaria(IR_EQ, e);
        case NODO_AND:         return genBinaria(IR_AND, e);
        case NODO_OR:          return genBinaria(IR_OR, e);
        case NODO_SIGNO_MENOS: return genUnaria(IR_NEG, e);
        case NODO_NEG:         return genUnaria(IR_NOT, e);
        default:
            fprintf(stderr, "Codigo intermedio: nodo de expresion inesperado (%s)\n",
                    tipoNodoNombre(e->tipo));
            return operandoNone();
    }
}



/* ---- Generacion de sentencias y control de flujo ---- */

static void genStmt(nodoAST *s);
static void genBloque(nodoAST *bloque);

// Recorre la cadena STATEMENT (hijos[0]=sentencia, hijos[1]=resto). Una sentencia NULL es un ';'
static void genSentencias(nodoAST *cadena) {
    for (nodoAST *n = cadena; n != NULL; n = n->hijos[1]) {
        genStmt(n->hijos[0]);
    }
}

// NODO_LLAVE: hijos[0]=declaraciones (no generan codigo), hijos[1]=sentencias
static void genBloque(nodoAST *bloque) {
    if (bloque == NULL) return;
    genSentencias(bloque->hijos[1]);
}

static void emitirSalto(OperadorIR op, Operando condicion, Operando destino) {
    emitir(listaActual, op, condicion, operandoNone(), destino);
}

static void emitirEtiqueta(Operando etiqueta) {
    emitir(listaActual, IR_LABEL, operandoNone(), operandoNone(), etiqueta);
}

// IF: hijos[0]=condicion, hijos[1]=bloque then, hijos[2]=NODO_ELSE (hijos[0]=bloque) o NULL
static void genIf(nodoAST *s) {
    Operando condicion = genExpr(s->hijos[0]);
    nodoAST *sino = s->hijos[2];

    if (sino == NULL) {
        Operando fin = nuevaEtiqueta();
        emitirSalto(IR_JMPF, condicion, fin);
        genBloque(s->hijos[1]);
        emitirEtiqueta(fin);
        return;
    }

    Operando laOtra = nuevaEtiqueta();
    Operando fin = nuevaEtiqueta();
    emitirSalto(IR_JMPF, condicion, laOtra);
    genBloque(s->hijos[1]);
    emitirSalto(IR_JUMP, operandoNone(), fin);
    emitirEtiqueta(laOtra);
    genBloque(sino->hijos[0]);
    emitirEtiqueta(fin);
}

// WHILE: hijos[0]=condicion, hijos[1]=cuerpo. La condicion se reevalua en cada vuelta.
static void genWhile(nodoAST *s) {
    Operando inicio = nuevaEtiqueta();
    Operando fin = nuevaEtiqueta();

    emitirEtiqueta(inicio);
    Operando condicion = genExpr(s->hijos[0]);
    emitirSalto(IR_JMPF, condicion, fin);
    genBloque(s->hijos[1]);
    emitirSalto(IR_JUMP, operandoNone(), inicio);
    emitirEtiqueta(fin);
}

// RETURN: hijos[0]=NODO_EXPR (envoltorio de la expresion) o NULL si es un return sin valor
static void genReturn(nodoAST *s) {
    Operando valor = s->hijos[0] != NULL ? genExpr(s->hijos[0]) : operandoNone();
    emitir(listaActual, IR_RET, valor, operandoNone(), operandoNone());
}

static void genStmt(nodoAST *s) {
    if (s == NULL) return; // ';' vacio

    switch (s->tipo) {
        case NODO_OP_ASIG: {
            Operando valor = genExpr(s->hijos[0]);
            emitir(listaActual, IR_ASSIGN, valor, operandoNone(), operandoVar(s->simbolo));
            break;
        }
        case NODO_METHOD_CALL: genLlamada(s->hijos[0]); break; // resultado descartado
        case NODO_IF:          genIf(s); break;
        case NODO_WHILE:       genWhile(s); break;
        case NODO_RETURN:      genReturn(s); break;
        case NODO_BLOQUE:      genBloque(s->hijos[0]); break;
        default:
            fprintf(stderr, "Codigo intermedio: sentencia inesperada (%s)\n",
                    tipoNodoNombre(s->tipo));
            break;
    }
}



/* ---- Metodos y punto de entrada ---- */

// METHOD: hijos[0]=tipo o NULL (void). Con parametros: hijos[1]=PARAMS, hijos[2]=bloque;
// sin parametros: hijos[1]=bloque. Los parametros no generan codigo.
static void genMetodo(nodoAST *m) {
    nodoAST *bloque = m->hijos[1]->tipo == NODO_PARAMS ? m->hijos[2] : m->hijos[1];

    emitir(listaActual, IR_ENTER, operandoFunc(m->simbolo), operandoNone(), operandoNone());
    genBloque(bloque);

    // Un metodo void puede terminar sin return: se cierra con un RET implicito
    if (m->simbolo->tipo == TIPO_VOID && listaActual->cola->op != IR_RET) {
        emitir(listaActual, IR_RET, operandoNone(), operandoNone(), operandoNone());
    }
    emitir(listaActual, IR_LEAVE, operandoNone(), operandoNone(), operandoNone());
}

// Cadena DECLS(decl, resto): decl es NODO_VAR (global, sin codigo), NODO_METHOD u otro DECLS
static void genDeclaraciones(nodoAST *decls) {
    for (nodoAST *n = decls; n != NULL; n = n->hijos[1]) {
        nodoAST *decl = n->hijos[0];
        if (decl == NULL) continue;

        if (decl->tipo == NODO_METHOD) {
            genMetodo(decl);
        } else if (decl->tipo == NODO_DECLS) {
            genDeclaraciones(decl);
        }
    }
}

ListaIR *generarCodigoIntermedio(nodoAST *raiz) {
    if (huboErrorSemantico()) {
        return NULL; // el IR solo se genera sobre un programa semanticamente correcto
    }

    contadorTemp = 0;
    contadorEtiqueta = 0;
    listaActual = crearLista();
    genDeclaraciones(raiz);

    ListaIR *resultado = listaActual;
    listaActual = NULL;
    return resultado;
}