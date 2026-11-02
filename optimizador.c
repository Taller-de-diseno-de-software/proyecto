/*
 * Optimizaciones sobre el AST.
 *
 * Constant folding: recorre el arbol en postorden; cuando todos los operandos de una
 * operacion ya son literales, calcula el resultado y reescribe el nodo como un literal.
 * Solo pliega combinaciones bien tipadas (int con int, boolean con boolean), asi que el
 * analizador semantico sigue viendo los mismos errores de tipo que sin optimizar.
 *
 * Eliminacion de codigo muerto: saca las sentencias que nunca se ejecutan (despues de un
 * return o de un while(true)), los if/while cuya condicion constante decide que no se
 * ejecutan y la rama de un if que no se toma. Tambien usa solo constantes bien tipadas.
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

static int esLiteral(const nodoAST *nodo) {
    return nodo != NULL && nodo->tipo == NODO_LITERAL && nodo->valor != NULL;
}

// El valor del literal si es entero o booleano; los floats quedan como NO_CTE
static Cte leerConstante(const nodoAST *nodo) {
    if (!esLiteral(nodo)) return NO_CTE;

    const char *texto = nodo->valor;
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
    // NODO_TYPE guarda un literal del parser ("int", "float", "boolean"), no memoria reservada
    if (nodo->tipo != NODO_TYPE) free(nodo->valor);
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
    nodo->valor = copia;
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



/* ---- Eliminacion de codigo muerto ---- */

static void optimizarSentencias(nodoAST **enlace);
static void optimizarLlave(nodoAST *llave);
static int optimizarSentencia(nodoAST *sentencia);
static int cortaFlujo(const nodoAST *sentencia);
static int cortaFlujoBloque(const nodoAST *llave);

// Evalua una expresion formada solo por literales y operadores (sin variables ni llamadas).
// Devuelve 1 si se pudo calcular y deja el resultado en *resultado.
static int evaluarConstante(const nodoAST *nodo, Cte *resultado) {
    Cte a, b;

    if (nodo == NULL) return 0;

    switch (nodo->tipo) {
        case NODO_LITERAL:
            *resultado = leerConstante(nodo);
            return resultado->clase != CTE_NINGUNA;
        case NODO_SUMA: case NODO_RESTA: case NODO_PROD: case NODO_DIV: case NODO_DIVENT:
        case NODO_MENOR: case NODO_MAYOR: case NODO_EQ: case NODO_AND: case NODO_OR:
            if (!evaluarConstante(nodo->hijos[0], &a) || !evaluarConstante(nodo->hijos[1], &b)) return 0;
            *resultado = operarBinaria(nodo->tipo, a, b);
            return resultado->clase != CTE_NINGUNA;
        case NODO_SIGNO_MENOS: case NODO_NEG:
            if (!evaluarConstante(nodo->hijos[0], &a)) return 0;
            *resultado = operarUnaria(nodo->tipo, a);
            return resultado->clase != CTE_NINGUNA;
        default:
            return 0;
    }
}

// Condicion booleana constante: 1 si se puede decidir, con el valor en *valor.
// Solo booleanos, asi "while (1)" no se toca y el semantico sigue reportando el error de tipo.
static int condicionConstante(const nodoAST *condicion, int *valor) {
    Cte c;

    if (!evaluarConstante(condicion, &c) || c.clase != CTE_BOOL) return 0;
    *valor = c.valor != 0;
    return 1;
}

// 1 si la sentencia nunca cae a la siguiente: un return, un while(true) o un if
// cuyas dos ramas cortan el flujo. Se usa para tirar lo que sigue.
static int cortaFlujo(const nodoAST *sentencia) {
    switch (sentencia->tipo) {
        case NODO_RETURN:
            return 1;
        case NODO_WHILE: {
            int valor;
            return condicionConstante(sentencia->hijos[0], &valor) && valor;
        }
        case NODO_IF:
            if (sentencia->hijos[2] == NULL) return 0;
            return cortaFlujoBloque(sentencia->hijos[1]) &&
                   cortaFlujoBloque(sentencia->hijos[2]->hijos[0]);
        case NODO_BLOQUE:
            return cortaFlujoBloque(sentencia->hijos[0]);
        default:
            return 0;
    }
}

// 1 si la cadena de sentencias tiene alguna que corta el flujo (lo que sigue es inalcanzable)
static int cortaFlujoCadena(const nodoAST *cadena) {
    for (const nodoAST *nodo = cadena; nodo != NULL; nodo = nodo->hijos[1]) {
        if (nodo->hijos[0] != NULL && cortaFlujo(nodo->hijos[0])) return 1;
    }
    return 0;
}

// Un bloque (NODO_LLAVE) corta el flujo si lo hace su cadena de sentencias
static int cortaFlujoBloque(const nodoAST *llave) {
    if (llave == NULL) return 0;
    return cortaFlujoCadena(llave->hijos[1]);
}

// Convierte el nodo IF en un NODO_BLOQUE que ejecuta `llave` (la rama que sobrevive)
static void ifComoBloque(nodoAST *sentencia, nodoAST *llave) {
    sentencia->tipo = NODO_BLOQUE;
    sentencia->valor = NULL;
    sentencia->hijos[0] = llave;
    sentencia->hijos[1] = NULL;
    sentencia->hijos[2] = NULL;
    sentencia->hijos[3] = NULL;
}

// Optimiza una sentencia. Devuelve 1 si quedo muerta y hay que desengancharla de la cadena.
static int optimizarSentencia(nodoAST *sentencia) {
    switch (sentencia->tipo) {
        case NODO_IF: {
            nodoAST *sino = sentencia->hijos[2];
            int valor;

            // Primero el interior: las ramas pueden quedar vacias o cortar el flujo
            optimizarLlave(sentencia->hijos[1]);
            if (sino != NULL) optimizarLlave(sino->hijos[0]);

            if (!condicionConstante(sentencia->hijos[0], &valor)) return 0;

            if (valor) { // if (true): queda la rama then
                nodoAST *cuerpo = sentencia->hijos[1];
                liberarSubarbol(sentencia->hijos[0]);
                liberarSubarbol(sino);
                ifComoBloque(sentencia, cuerpo);
                return 0;
            }

            // if (false) sin else: no hace nada. Lo libera entero el llamador.
            if (sino == NULL) return 1;

            // if (false) con else: queda la rama else (el NODO_ELSE era solo un envoltorio)
            nodoAST *cuerpo = sino->hijos[0];
            liberarSubarbol(sentencia->hijos[0]);
            liberarSubarbol(sentencia->hijos[1]);
            free(sino);
            ifComoBloque(sentencia, cuerpo);
            return 0;
        }
        case NODO_WHILE:
            optimizarLlave(sentencia->hijos[1]);
            {
                int valor;
                // while (false) no itera nunca; while (true) se conserva (lo siguiente es muerto)
                if (condicionConstante(sentencia->hijos[0], &valor) && !valor) return 1;
            }
            return 0;
        case NODO_BLOQUE:
            optimizarLlave(sentencia->hijos[0]);
            return 0;
        default:
            return 0;
    }
}

// Recorre una cadena STATEMENT (hijos[0]=sentencia, hijos[1]=resto). Desengancha las
// sentencias muertas y tira todo lo que sigue a una que corta el flujo.
// Recibe el puntero al enlace del padre para poder sacar tambien la primera sentencia.
static void optimizarSentencias(nodoAST **enlace) {
    while (*enlace != NULL) {
        nodoAST *nodo = *enlace;
        nodoAST *sentencia = nodo->hijos[0];

        if (sentencia != NULL && optimizarSentencia(sentencia)) {
            nodoAST *resto = nodo->hijos[1];
            liberarSubarbol(sentencia);
            free(nodo);
            *enlace = resto;
            continue; // el nodo que quedo en su lugar todavia no se optimizo
        }

        if (sentencia != NULL && cortaFlujo(sentencia)) {
            liberarSubarbol(nodo->hijos[1]);
            nodo->hijos[1] = NULL;
            return;
        }
        enlace = &nodo->hijos[1];
    }
}

// BLOQUE (NODO_LLAVE): hijos[0]=declaraciones (no se tocan), hijos[1]=sentencias
static void optimizarLlave(nodoAST *llave) {
    if (llave == NULL || llave->tipo != NODO_LLAVE) return;
    optimizarSentencias(&llave->hijos[1]);
}

// Cadena DECLS(decl, resto): solo los metodos tienen cuerpo para optimizar
static void optimizarDeclaraciones(nodoAST *declaraciones) {
    for (nodoAST *nodo = declaraciones; nodo != NULL; nodo = nodo->hijos[1]) {
        nodoAST *declaracion = nodo->hijos[0];
        if (declaracion == NULL) continue;

        if (declaracion->tipo == NODO_METHOD) {
            nodoAST *params = declaracion->hijos[1];
            nodoAST *cuerpo = (params != NULL && params->tipo == NODO_PARAMS)
                            ? declaracion->hijos[2] : declaracion->hijos[1];
            optimizarLlave(cuerpo);
        } else if (declaracion->tipo == NODO_DECLS) {
            optimizarDeclaraciones(declaracion);
        }
    }
}

/*
 * Eliminacion de codigo muerto (-opt dce):
 *  - saca las sentencias que siguen a un return, a un while(true) o a un if cuyas dos
 *    ramas cortan el flujo (dentro del mismo bloque);
 *  - saca los if (false) sin else y los while (false), que no tienen efecto;
 *  - en if (true) ... [else ...] deja solo la rama que se ejecuta.
 * Las condiciones se evaluan con literales y operadores (no con variables ni llamadas),
 * asi que nunca se descarta codigo con efectos. Conviene correrla despues del plegado.
 */
void eliminarCodigoMuerto(nodoAST *raiz) {
    if (raiz == NULL) return;
    optimizarDeclaraciones(raiz);
}
