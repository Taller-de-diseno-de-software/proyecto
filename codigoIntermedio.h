#ifndef CODIGO_INTERMEDIO_H
#define CODIGO_INTERMEDIO_H

#include <stdio.h>
#include "tablaSimbolos.h"
#include "ast.h"

// Instrucciones para el codigo intermedio
typedef enum {
    //Arimeticas
    IR_ADD,
    IR_SUB,
    IR_MULT,
    IR_DIV,
    IR_MOD, 
    IR_NEG,


    //LÃ³gicas
    IR_AND,
    IR_OR,
    IR_NOT,



    //Relacionales
    IR_LT,
    IR_GT,
    IR_EQ,


    //Copia
    IR_ASSIGN,


    //Flujo
    IR_LABEL,
    IR_JUMP,
    IR_JMPF,


    //Funciones
    IR_ENTER,
    IR_PARAM,
    IR_CALL,
    IR_RET,
    IR_LEAVE
} OperadorIR;

typedef enum {
    OPD_NONE,
    OPD_TEMP,
    OPD_VAR,
    OPD_LABEL,
    OPD_CONST,
    OPD_FUNC
} KindOperando;

typedef struct {
    KindOperando kind;
    union {
        int valor;
        Simbolo *simbolo;
        int id;
    }u;
} Operando;

typedef struct Cuadrupla {
    OperadorIR op;
    Operando arg1, arg2, res;
    struct Cuadrupla *siguiente;
} Cuadrupla;

typedef struct {
    Cuadrupla *cabeza;
    Cuadrupla *cola;
    int cantidad;
} ListaIR;

// Constructores de operandos
Operando operandoNone(void);
Operando operandoConst(int valor);
Operando operandoVar(Simbolo *s);
Operando operandoTemp(int id);
Operando operandoLabel(int id);
Operando operandoFunc(Simbolo *f);

// Generadores de temporales y etiquetas
Operando nuevoTemp(void);
Operando nuevaEtiqueta(void);

// Lista de cuadruplas
ListaIR *crearLista(void);
void emitir(ListaIR *lista, OperadorIR op, Operando a, Operando b, Operando res);
void liberarLista(ListaIR *lista);

// Escribe el IR en `out`, una instruccion por linea
void imprimirIR(ListaIR *lista, FILE *out);

// Genera el IR de un programa ya analizado semanticamente (AST decorado).
// Devuelve NULL si el analisis semantico detecto errores. El llamador libera con liberarLista.
ListaIR *generarCodigoIntermedio(nodoAST *raiz);

#endif // CODIGO_INTERMEDIO_H