#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

#define nHijos 4

nodoAST *crearNodo(const char *tipo, char *valor, nodoAST *hijo1, nodoAST *hijo2, nodoAST *hijo3, nodoAST *hijo4){
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
