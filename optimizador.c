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
 * Corre despues del analisis semantico: asi no puede ocultar errores que haya en el codigo que descarta.
 *
 * Simplificacion algebraica: recorre el arbol en postorden y reemplaza las operaciones que tienen un
 * operando neutro, absorbente o potencia de 2 por una forma mas barata (x + 0 -> x, x * 8 -> x << 3).
 * Corre despues del analisis semantico: necesita los tipos de las variables y las llamadas.
 */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "tablaSimbolos.h"
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
    free(nodo->valor); // si el nodo ya era un literal, su texto anterior
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
 * Requiere el AST ya analizado semanticamente (ver el encabezado de este archivo).
 */
void eliminarCodigoMuerto(nodoAST *raiz) {
    if (raiz == NULL) return;
    optimizarDeclaraciones(raiz);
}

/* ---- Simplificacion algebraica ---- */

// Tipo de una expresion ya analizada: los ID y las llamadas traen su Simbolo del analisis semantico
static TipoDato tipoDeExpresion(const nodoAST *e) {
    if (e == NULL) return TIPO_INDEFINIDO;

    switch (e->tipo) {
        case NODO_EXPR:
            return tipoDeExpresion(e->hijos[0]);
        case NODO_LITERAL:
            if (e->valor == NULL) return TIPO_INDEFINIDO;
            if (strcmp(e->valor, "true") == 0 || strcmp(e->valor, "false") == 0) return TIPO_BOOL;
            return strpbrk(e->valor, ".eE") != NULL ? TIPO_FLOAT : TIPO_INT;
        case NODO_ID:
            return e->simbolo != NULL ? e->simbolo->tipo : TIPO_INDEFINIDO;
        case NODO_METHOD_CALL:
            return e->hijos[0] != NULL && e->hijos[0]->simbolo != NULL ? e->hijos[0]->simbolo->tipo : TIPO_INDEFINIDO;
        case NODO_SUMA: case NODO_RESTA: case NODO_PROD: case NODO_DIV: case NODO_DIVENT: {
            TipoDato izq = tipoDeExpresion(e->hijos[0]);
            TipoDato der = tipoDeExpresion(e->hijos[1]);
            if (izq == TIPO_INDEFINIDO || der == TIPO_INDEFINIDO) return TIPO_INDEFINIDO;
            return izq == TIPO_FLOAT || der == TIPO_FLOAT ? TIPO_FLOAT : TIPO_INT;
        }
        case NODO_SIGNO_MENOS:
            return tipoDeExpresion(e->hijos[0]);
        case NODO_SHL: case NODO_SHR:
            return TIPO_INT;
        case NODO_MENOR: case NODO_MAYOR: case NODO_EQ: case NODO_AND: case NODO_OR: case NODO_NEG:
            return TIPO_BOOL;
        default:
            return TIPO_INDEFINIDO;
    }
}

// Una expresion se puede descartar sin cambiar el programa si no tiene llamadas (efectos) ni
// divisiones (que fallarian al dividir por cero)
static int esPura(const nodoAST *e) {
    if (e == NULL) return 1;
    if (e->tipo == NODO_METHOD_CALL || e->tipo == NODO_DIV || e->tipo == NODO_DIVENT) return 0;
    for (int i = 0; i < 4; i++) {
        if (!esPura(e->hijos[i])) return 0;
    }
    return 1;
}

// k >= 1 tal que valor == 2^k; 0 si no es una potencia de 2 mayor que 1
static int logaritmoDos(long long valor) {
    int k = 0;

    if (valor < 2) return 0;
    while ((valor & 1) == 0) {
        valor >>= 1;
        k++;
    }
    return valor == 1 ? k : 0;
}

static int esCte(Cte c, ClaseCte clase, long long valor) {
    return c.clase == clase && c.valor == valor;
}

// Reescribe `nodo` en el lugar como uno de sus hijos: el hijo elegido sube y el resto se libera
static void reemplazarPorHijo(nodoAST *nodo, int indice) {
    nodoAST *conservado = nodo->hijos[indice];

    nodo->hijos[indice] = NULL;
    for (int i = 0; i < 4; i++) {
        liberarSubarbol(nodo->hijos[i]);
    }
    *nodo = *conservado;
    free(conservado); // solo el nodo: sus hijos ahora cuelgan de `nodo`
}

// nodo = operando <op> bits, con op NODO_SHL o NODO_SHR. `indiceCte` es el hijo que era la constante.
static void reemplazarPorDesplazamiento(nodoAST *nodo, TipoNodo op, int indiceCte, int bits) {
    nodoAST *constante = nodo->hijos[indiceCte];
    nodoAST *operando = nodo->hijos[1 - indiceCte];

    nodo->tipo = op;
    nodo->hijos[0] = operando;
    nodo->hijos[1] = constante;
    reemplazarPorLiteral(constante, (Cte){ CTE_INT, bits });
}

// Suma, resta, producto, division y modulo con una constante entera de un lado
static void simplificarAritmetica(nodoAST *n, Cte izq, Cte der) {
    nodoAST *a = n->hijos[0];
    nodoAST *b = n->hijos[1];
    int enteroA = tipoDeExpresion(a) == TIPO_INT;
    int enteroB = tipoDeExpresion(b) == TIPO_INT;

    switch (n->tipo) {
        case NODO_SUMA: // x + 0 -> x ; 0 + x -> x
            if (esCte(der, CTE_INT, 0)) reemplazarPorHijo(n, 0);
            else if (esCte(izq, CTE_INT, 0)) reemplazarPorHijo(n, 1);
            break;
        case NODO_RESTA:
            if (esCte(der, CTE_INT, 0)) { // x - 0 -> x
                reemplazarPorHijo(n, 0);
            } else if (esCte(izq, CTE_INT, 0)) { // 0 - x -> -x
                n->tipo = NODO_SIGNO_MENOS;
                n->hijos[0] = b;
                n->hijos[1] = NULL;
                liberarSubarbol(a);
            } else if (enteroA && a->tipo == NODO_ID && b->tipo == NODO_ID && a->simbolo != NULL && a->simbolo == b->simbolo) {
                reemplazarPorLiteral(n, (Cte){ CTE_INT, 0 }); // x - x -> 0
            }
            break;
        case NODO_PROD:
            if (esCte(der, CTE_INT, 1)) reemplazarPorHijo(n, 0); // x * 1 -> x
            else if (esCte(izq, CTE_INT, 1)) reemplazarPorHijo(n, 1);
            else if (esCte(der, CTE_INT, 0) && enteroA && esPura(a)) reemplazarPorLiteral(n, (Cte){ CTE_INT, 0 }); // x * 0 -> 0
            else if (esCte(izq, CTE_INT, 0) && enteroB && esPura(b)) reemplazarPorLiteral(n, (Cte){ CTE_INT, 0 });
            else if (der.clase == CTE_INT && enteroA && logaritmoDos(der.valor)) reemplazarPorDesplazamiento(n, NODO_SHL, 1, logaritmoDos(der.valor)); // x * 2^k -> x << k
            else if (izq.clase == CTE_INT && enteroB && logaritmoDos(izq.valor)) reemplazarPorDesplazamiento(n, NODO_SHL, 0, logaritmoDos(izq.valor));
            break;
        case NODO_DIV:
            if (esCte(der, CTE_INT, 1)) reemplazarPorHijo(n, 0); // x / 1 -> x
            else if (der.clase == CTE_INT && enteroA && logaritmoDos(der.valor)) reemplazarPorDesplazamiento(n, NODO_SHR, 1, logaritmoDos(der.valor)); // x / 2^k -> x >> k
            break;
        case NODO_DIVENT:
            if (esCte(der, CTE_INT, 1) && enteroA && esPura(a)) reemplazarPorLiteral(n, (Cte){ CTE_INT, 0 }); // x % 1 -> 0
            break;
        default:
            break;
    }
}

// && y ||: el valor neutro deja pasar al otro operando; el absorbente decide el resultado.
// Si el absorbente esta a la izquierda el derecho nunca se evalua (cortocircuito), asi que se descarta
// sin condiciones; a la derecha el izquierdo ya se evaluo, y solo se descarta si no tiene efectos.
static void simplificarLogica(nodoAST *n, Cte izq, Cte der) {
    int esAnd = n->tipo == NODO_AND;
    long long neutro = esAnd ? 1 : 0;
    long long absorbente = esAnd ? 0 : 1;

    if (esCte(der, CTE_BOOL, neutro)) reemplazarPorHijo(n, 0); // x && true -> x ; x || false -> x
    else if (esCte(izq, CTE_BOOL, neutro)) reemplazarPorHijo(n, 1);
    else if (esCte(izq, CTE_BOOL, absorbente)) reemplazarPorLiteral(n, (Cte){ CTE_BOOL, absorbente }); // false && x -> false ; true || x -> true
    else if (esCte(der, CTE_BOOL, absorbente) && esPura(n->hijos[0])) reemplazarPorLiteral(n, (Cte){ CTE_BOOL, absorbente });
}

// x = x: el destino y el valor son la misma variable (ya resuelta por el semantico)
static int esAutoasignacion(const nodoAST *sentencia) {
    const nodoAST *valor;

    if (sentencia == NULL || sentencia->tipo != NODO_OP_ASIG) return 0;
    valor = sentencia->hijos[0];
    return valor != NULL && valor->tipo == NODO_ID
        && sentencia->simbolo != NULL && sentencia->simbolo == valor->simbolo;
}

static void simplificarNodo(nodoAST *n) {
    if (n == NULL) return;

    // Postorden: los operandos ya simplificados pueden dejar al nodo en una forma reconocible
    for (int i = 0; i < 4; i++) {
        simplificarNodo(n->hijos[i]);
    }

    switch (n->tipo) {
        case NODO_SUMA: case NODO_RESTA: case NODO_PROD: case NODO_DIV: case NODO_DIVENT:
            simplificarAritmetica(n, leerConstante(n->hijos[0]), leerConstante(n->hijos[1]));
            break;
        case NODO_AND: case NODO_OR:
            simplificarLogica(n, leerConstante(n->hijos[0]), leerConstante(n->hijos[1]));
            break;
        case NODO_STATEMENT: // x = x se elimina; una sentencia NULL es un ';' vacio, que no genera codigo
            if (esAutoasignacion(n->hijos[0])) {
                liberarSubarbol(n->hijos[0]);
                n->hijos[0] = NULL;
            }
            break;
        case NODO_NEG: case NODO_SIGNO_MENOS: // !!x -> x ; --x -> x
            if (n->hijos[0] != NULL && n->hijos[0]->tipo == n->tipo) {
                reemplazarPorHijo(n->hijos[0], 0);
                reemplazarPorHijo(n, 0);
            }
            break;
        default:
            break;
    }
}

void simplificarAlgebraicamente(nodoAST *raiz) {
    simplificarNodo(raiz);
}
