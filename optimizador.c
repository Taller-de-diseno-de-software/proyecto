/*
 * Optimizaciones sobre el AST.
 *
 * Constant folding: recorre el arbol en postorden; cuando todos los operandos de una
 * operacion ya son literales, calcula el resultado y reescribe el nodo como un literal.
 * Solo pliega combinaciones bien tipadas (int con int, boolean con boolean), asi que el
 * analizador semantico sigue viendo los mismos errores de tipo que sin optimizar.
 */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "optimizador.h"

typedef enum { CTE_NINGUNA, CTE_INT, CTE_BOOL } ClaseCte;

typedef struct {
    ClaseCte clase;
    long long valor; // el entero, o 0/1 para un booleano
} Cte;

static const Cte NO_CTE = { CTE_NINGUNA, 0 };

// Un literal de expresion viene doblemente envuelto: LITERAL(LITERAL "texto")
static int esLiteral(const nodoAST *nodo) {
    return nodo != NULL && nodo->tipo == NODO_LITERAL
        && nodo->hijos[0] != NULL && nodo->hijos[0]->valor != NULL;
}

// El valor del literal si es entero o booleano; los floats quedan como NO_CTE
static Cte leerConstante(const nodoAST *nodo) {
    if (!esLiteral(nodo)) return NO_CTE;

    const char *texto = nodo->hijos[0]->valor;
    if (strcmp(texto, "true") == 0)  return (Cte){ CTE_BOOL, 1 };
    if (strcmp(texto, "false") == 0) return (Cte){ CTE_BOOL, 0 };
    if (strpbrk(texto, ".eE") != NULL) return NO_CTE;

    char *fin;
    errno = 0;
    long long valor = strtoll(texto, &fin, 10);
    if (errno != 0 || *fin != '\0' || valor < INT_MIN || valor > INT_MAX) return NO_CTE;
    return (Cte){ CTE_INT, valor };
}

static void liberarSubarbol(nodoAST *nodo) {
    if (nodo == NULL) return;
    for (int i = 0; i < 4; i++) {
        liberarSubarbol(nodo->hijos[i]);
    }
    free(nodo->valor);
    free(nodo);
}

// Reescribe `nodo` en el lugar (el padre sigue apuntando a el) como el literal `resultado`
static void reemplazarPorLiteral(nodoAST *nodo, Cte resultado) {
    char texto[32];

    if (resultado.clase == CTE_BOOL) {
        snprintf(texto, sizeof(texto), "%s", resultado.valor ? "true" : "false");
    } else {
        snprintf(texto, sizeof(texto), "%lld", resultado.valor);
    }

    char *copia = malloc(strlen(texto) + 1);
    if (copia == NULL) {
        fprintf(stderr, "Error: sin memoria\n");
        exit(EXIT_FAILURE);
    }
    strcpy(copia, texto);

    for (int i = 0; i < 4; i++) {
        liberarSubarbol(nodo->hijos[i]);
        nodo->hijos[i] = NULL;
    }
    nodo->tipo = NODO_LITERAL;
    nodo->valor = NULL;
    nodo->hijos[0] = crearNodo(NODO_LITERAL, copia, NULL, NULL, NULL, NULL);
}

static Cte operarBinaria(TipoNodo op, Cte a, Cte b) {
    if (a.clase == CTE_INT && b.clase == CTE_INT) {
        long long r;
        switch (op) {
            case NODO_SUMA:   r = a.valor + b.valor; break; // long long es de 64 bits: no desborda con operandos int
            case NODO_RESTA:  r = a.valor - b.valor; break;
            case NODO_PROD:   r = a.valor * b.valor; break;
            case NODO_DIV:    if (b.valor == 0) return NO_CTE; r = a.valor / b.valor; break;
            case NODO_DIVENT: if (b.valor == 0) return NO_CTE; r = a.valor % b.valor; break;
            case NODO_MENOR:  return (Cte){ CTE_BOOL, a.valor < b.valor };
            case NODO_MAYOR:  return (Cte){ CTE_BOOL, a.valor > b.valor };
            case NODO_EQ:     return (Cte){ CTE_BOOL, a.valor == b.valor };
            default:          return NO_CTE;
        }
        if (r < INT_MIN || r > INT_MAX) return NO_CTE; // dejar que desborde en ejecucion, como sin optimizar
        return (Cte){ CTE_INT, r };
    }
    if (a.clase == CTE_BOOL && b.clase == CTE_BOOL) {
        switch (op) {
            case NODO_AND: return (Cte){ CTE_BOOL, a.valor && b.valor };
            case NODO_OR:  return (Cte){ CTE_BOOL, a.valor || b.valor };
            case NODO_EQ:  return (Cte){ CTE_BOOL, a.valor == b.valor };
            default:       return NO_CTE;
        }
    }
    return NO_CTE;
}

static Cte operarUnaria(TipoNodo op, Cte a) {
    if (op == NODO_SIGNO_MENOS && a.clase == CTE_INT && -a.valor <= INT_MAX) {
        return (Cte){ CTE_INT, -a.valor };
    }
    if (op == NODO_NEG && a.clase == CTE_BOOL) {
        return (Cte){ CTE_BOOL, !a.valor };
    }
    return NO_CTE;
}

static void plegarNodo(nodoAST *nodo) {
    if (nodo == NULL) return;

    // Postorden: primero los hijos, para que los operandos ya esten plegados
    for (int i = 0; i < 4; i++) {
        plegarNodo(nodo->hijos[i]);
    }

    Cte resultado = NO_CTE;
    switch (nodo->tipo) {
        case NODO_SUMA: case NODO_RESTA: case NODO_PROD: case NODO_DIV: case NODO_DIVENT:
        case NODO_MENOR: case NODO_MAYOR: case NODO_EQ: case NODO_AND: case NODO_OR:
            resultado = operarBinaria(nodo->tipo, leerConstante(nodo->hijos[0]), leerConstante(nodo->hijos[1]));
            break;
        case NODO_SIGNO_MENOS: case NODO_NEG:
            resultado = operarUnaria(nodo->tipo, leerConstante(nodo->hijos[0]));
            break;
        default:
            return;
    }

    if (resultado.clase != CTE_NINGUNA) {
        reemplazarPorLiteral(nodo, resultado);
    }
}

void plegarConstantes(nodoAST *raiz) {
    plegarNodo(raiz);
}
