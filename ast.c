#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

#define nHijos 4

const char *tipoNodoNombre(TipoNodo tipo){
    switch (tipo) {
        case NODO_DECLS: return "DECLS";
        case NODO_VAR: return "VAR";
        case NODO_IDS: return "IDS";
        case NODO_METHOD: return "METHOD";
        case NODO_PARAMS: return "PARAMS";
        case NODO_PARAM: return "PARAM";
        case NODO_LLAVE: return "BLOQUE";
        case NODO_VAR_DECL: return "VAR_DECL";
        case NODO_STATEMENT: return "STATEMENT";
        case NODO_TYPE: return "TYPE";
        case NODO_OP_ASIG: return "OP_ASIG";
        case NODO_METHOD_CALL: return "METHOD_CALL";
        case NODO_IF: return "IF";
        case NODO_WHILE: return "WHILE";
        case NODO_RETURN: return "RETURN";
        case NODO_BLOQUE: return "BLOQUE";
        case NODO_ELSE: return "ELSE";
        case NODO_EXPR: return "EXPR";
        case NODO_ARGUMENTS_CALL: return "ARGUMENTS_CALL";
        case NODO_NO_ARGUMENTS_CALL: return "NO_ARGUMENTS_CALL";
        case NODO_ARG: return "ARG";
        case NODO_ARGS: return "ARGS";
        case NODO_ID: return "ID";
        case NODO_LITERAL: return "LITERAL";
        case NODO_SUMA: return "SUMA";
        case NODO_RESTA: return "RESTA";
        case NODO_PROD: return "PROD";
        case NODO_DIV: return "DIV";
        case NODO_DIVENT: return "DIVENT";
        case NODO_MENOR: return "MENOR";
        case NODO_MAYOR: return "MAYOR";
        case NODO_EQ: return "EQ";
        case NODO_AND: return "AND";
        case NODO_OR: return "OR";
        case NODO_SIGNO_MENOS: return "SIGNO_MENOS";
        case NODO_NEG: return "NEG";
        default: return "DESCONOCIDO";
    }
}

nodoAST *crearNodo(TipoNodo tipo, char *valor, nodoAST *hijo1, nodoAST *hijo2, nodoAST *hijo3, nodoAST *hijo4){
    nodoAST *nuevoNodo = malloc(sizeof(nodoAST));
    nuevoNodo->tipo = tipo;
    nuevoNodo->valor = valor;
    
    nuevoNodo->hijos[0] = hijo1;
    nuevoNodo->hijos[1] = hijo2;
    nuevoNodo->hijos[2] = hijo3;
    nuevoNodo->hijos[3] = hijo4;

    return nuevoNodo;
}

void imprimirArbol(nodoAST *nodo, int nivel){
    if (!nodo) return;
    for (int i = 0; i < nivel; i++){
        printf("  ");
    } 
    
    printf("%s", tipoNodoNombre(nodo->tipo));
    if (nodo->valor) {
        printf(" : %s\n", nodo->valor);
    } else {
        printf("\n");
    }
    
    for (int i = 0; i < nHijos; i++) {
        imprimirArbol(nodo->hijos[i], nivel + 1);
    }
}
