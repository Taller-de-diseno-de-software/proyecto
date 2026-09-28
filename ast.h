#ifndef AST_H
#define AST_H

typedef enum {
    NODO_DECLS, NODO_VAR, NODO_IDS, NODO_METHOD, NODO_PARAMS, NODO_PARAM,
    NODO_LLAVE, NODO_VAR_DECL, NODO_STATEMENT, NODO_TYPE, NODO_OP_ASIG,
    NODO_METHOD_CALL, NODO_IF, NODO_WHILE, NODO_RETURN, NODO_BLOQUE, NODO_ELSE,
    NODO_EXPR, NODO_ARGUMENTS_CALL, NODO_NO_ARGUMENTS_CALL, NODO_ARG, NODO_ARGS,
    NODO_ID, NODO_LITERAL, NODO_SUMA, NODO_RESTA, NODO_PROD, NODO_DIV, NODO_DIVENT,
    NODO_MENOR, NODO_MAYOR, NODO_EQ, NODO_AND, NODO_OR, NODO_SIGNO_MENOS, NODO_NEG
} TipoNodo;


struct nodoAST {
    TipoNodo tipo;
    char *valor;
    struct nodoAST *hijos[4];
};

typedef struct nodoAST nodoAST;

nodoAST *crearNodo(TipoNodo tipo, char *valor, nodoAST *hijo1, nodoAST *hijo2, nodoAST *hijo3, nodoAST *hijo4);
void imprimirArbol(nodoAST *nodo, int nivel);
const char *tipoNodoNombre(TipoNodo tipo);



#endif // AST_H