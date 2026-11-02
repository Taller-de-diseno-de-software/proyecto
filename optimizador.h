#ifndef OPTIMIZADOR_H
#define OPTIMIZADOR_H

#include "ast.h"

/*
 * Constant folding sobre el AST (-opt cf): reemplaza las expresiones enteras y logicas cuyos
 * operandos son todos constantes por el literal con su resultado. Ejemplo: 2 * 3 + 1 -> 7.
 * Los floats no se pliegan (el codigo intermedio solo cubre enteros y logicos), y tampoco las
 * operaciones que fallarian o desbordarian en ejecucion (division por cero, resultado fuera de int).
 */
void plegarConstantes(nodoAST *raiz);

/*
 * Eliminacion de codigo muerto sobre el AST (-opt dce): quita las sentencias inalcanzables
 * (despues de return, de while(true) o de un if cuyas dos ramas cortan el flujo), los
 * if(false) sin else y los while(false), y deja solo la rama que se ejecuta en if(true).
 * Las condiciones se evaluan solo con constantes bien tipadas (nunca con variables o llamadas).
 */
void eliminarCodigoMuerto(nodoAST *raiz);

#endif // OPTIMIZADOR_H
