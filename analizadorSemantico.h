/*
 * Analizador semántico.
 *
 * Recorre el AST que produjo el parser y verifica las reglas que la
 * gramática libre de contexto no puede expresar:
 *  - todo identificador usado fue declarado (y una sola vez por nivel)
 *  - los tipos son coherentes (asignaciones, operadores, condiciones, return)
 *  - las llamadas a métodos son válidas
 *
 * De paso decora el AST: engancha en cada nodo que declara o usa un
 * identificador el puntero al Simbolo correspondiente de la tabla de símbolos.
 *
 * Es el único módulo que conoce a la vez el AST y la tabla de símbolos.
 */

#ifndef ANALIZADOR_SEMANTICO_H
#define ANALIZADOR_SEMANTICO_H

#include "ast.h"

// Punto de entrada. Recibe la raíz del AST (nodo DECLS que devuelve el parser).
void analizarSemantica(nodoAST *raiz);

// Devuelve 1 si analizarSemantica() detectó algún error, 0 si no.
int huboErrorSemantico(void);

#endif
