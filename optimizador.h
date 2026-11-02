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
 * Requiere el AST ya analizado semanticamente y sin errores: si corriera antes, un programa con un
 * error en el codigo descartado (una variable no declarada, un tipo incompatible) pasaria a compilar.
 */
void eliminarCodigoMuerto(nodoAST *raiz);

/*
 * Simplificacion algebraica sobre el AST (-opt as). Requiere el AST ya analizado semanticamente,
 * porque necesita el tipo de las variables y las llamadas. Reemplaza:
 *   x + 0, 0 + x, x - 0, x * 1, 1 * x, x / 1   ->  x
 *   0 - x                                      ->  -x
 *   x * 0, 0 * x, x % 1, x - x                 ->  0     (solo enteros, y x sin llamadas ni divisiones)
 *   x * 2^k, 2^k * x                           ->  x << k  (NODO_SHL, solo enteros)
 *   x / 2^k                                    ->  x >> k  (NODO_SHR, solo enteros; redondea hacia cero como /)
 *   x && true, x || false (y simetricos)       ->  x
 *   false && x, true || x                      ->  la constante (x no se evalua por el cortocircuito)
 *   x && false, x || true                      ->  la constante (solo si x no tiene llamadas ni divisiones)
 *   !!x, --x                                   ->  x
 *   la sentencia x = x (tambien la que dejan las anteriores, como x = x + 0)  ->  se elimina
 * Los floats solo admiten las identidades que conservan el tipo (x + 0, x * 1, ...).
 */
void simplificarAlgebraicamente(nodoAST *raiz);

#endif // OPTIMIZADOR_H
