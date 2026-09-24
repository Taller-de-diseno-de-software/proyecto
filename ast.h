#ifndef AST_H
#define AST_H

struct nodoAST { 
    char *tipo;
    char *valor;
    struct nodoAST *hijos[4];
};

typedef struct nodoAST nodoAST;

nodoAST *crearNodo(const char *tipo, char *valor, nodoAST *hijo1, nodoAST *hijo2, nodoAST *hijo3, nodoAST *hijo4);
void imprimirArbol(nodoAST *nodo, int nivel);



#endif // AST_H