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
 */
#include "ast.h"
#include "tablaSimbolos.h"
#include "analizadorSemantico.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int errorSemantico = 0;
static TipoDato tipoRetornoActual = TIPO_VOID; // Tipo de retorno del método que se está visitando

int huboErrorSemantico(void){
    return errorSemantico;
}

static void visitarDeclaracionesGlobales(nodoAST *declaraciones);
static void visitarMetodos(nodoAST *declaraciones);
static void visitarMetodo(nodoAST *metodo);
static void visitarBloque(nodoAST *bloque);
static void visitarContenidoBloque(nodoAST *bloque);
static void visitarDeclaraciones(nodoAST *declaraciones);
static void visitarSentencias(nodoAST *sentencias);
static void visitarDeclaracion(nodoAST *declaracion);
static void visitarSentencia(nodoAST *sentencia);
static TipoDato visitarExpresion(nodoAST *expresion);
static TipoDato visitarLlamada(nodoAST *llamada);

// NODO_TYPE -> TipoDato. NULL es void (así lo genera el parser en METHOD).
static TipoDato tipoDeNodo(nodoAST *tipo){
    if(!tipo){
        return TIPO_VOID;
    }
    // El parser genera "boolean", pero tipoDesdeTexto espera "bool".
    if(strcmp(tipo->valor, "boolean") == 0){
        return TIPO_BOOL;
    }
    return tipoDesdeTexto(tipo->valor);
}

// La cadena PARAMS anida wrappers: PARAMS(PARAM, PARAMS(PARAMS(PARAM, ...))).
// Un recorrido en preorden por hijos[0], hijos[1] visita los PARAM en orden.
static void recogerTiposParametros(nodoAST *params, TipoDato *tipos, int *cantidad){
    if(!params){
        return;
    }
    if(params->tipo == NODO_PARAM){
        if(*cantidad >= MAX_PARAMS){
            fprintf(stderr,"Error semántico: una función admite como máximo %d parámetros\n", MAX_PARAMS);
            errorSemantico = 1;
            return;
        }
        tipos[(*cantidad)++] = tipoDeNodo(params->hijos[0]);
        return;
    }
    recogerTiposParametros(params->hijos[0], tipos, cantidad);
    recogerTiposParametros(params->hijos[1], tipos, cantidad);
}

// Guarda en el Simbolo de la función la firma (cantidad y tipos de parámetros)
static void guardarParametros(Simbolo *funcion, nodoAST *params){
    int cantidad = 0;
    recogerTiposParametros(params, funcion->tiposParams, &cantidad);
    funcion->numParams = cantidad;
}

/* ---------- Primera pasada: declaraciones globales ---------- */

// Registra variables globales y métodos antes de visitar cuerpos,
// así un método puede llamar a otro declarado más abajo.
static void visitarDeclaracionesGlobales(nodoAST *declaraciones){
    if(!declaraciones){
        return;
    }

    // El parser envuelve la lista en un DECLS extra: se recorre igual.
    nodoAST *declaracion = declaraciones->hijos[0];
    nodoAST *declaracionesAux = declaraciones->hijos[1];

    if(declaracion){
        if(declaracion->tipo == NODO_VAR){
            visitarDeclaracion(declaracion);
        }else if(declaracion->tipo == NODO_METHOD){
            Simbolo *simbolo = insertarSimbolo(FLAG_FUNCION, declaracion->valor, tipoDeNodo(declaracion->hijos[0]));
            if(!simbolo){
                errorSemantico = 1;
            }else if(declaracion->hijos[1] && declaracion->hijos[1]->tipo == NODO_PARAMS){
                guardarParametros(simbolo, declaracion->hijos[1]);
            }
            declaracion->simbolo = simbolo;
        }else{
            visitarDeclaracionesGlobales(declaracion); // DECLS anidado
        }
    }
    visitarDeclaracionesGlobales(declaracionesAux);
}

/* ---------- Segunda pasada: cuerpos de los métodos ---------- */

static void visitarMetodos(nodoAST *declaraciones){
    if(!declaraciones){
        return;
    }

    nodoAST *declaracion = declaraciones->hijos[0];
    nodoAST *declaracionesAux = declaraciones->hijos[1];

    if(declaracion){
        if(declaracion->tipo == NODO_METHOD){
            visitarMetodo(declaracion);
        }else if(declaracion->tipo == NODO_DECLS){
            visitarMetodos(declaracion);
        }
    }
    visitarMetodos(declaracionesAux);
}

static void visitarMetodo(nodoAST *metodo){
    // Con parámetros: hijos[1]=PARAMS, hijos[2]=BLOQUE. Sin parámetros: hijos[1]=BLOQUE.
    nodoAST *params = metodo->hijos[1]->tipo == NODO_PARAMS ? metodo->hijos[1] : NULL;
    nodoAST *bloque = params ? metodo->hijos[2] : metodo->hijos[1];

    tipoRetornoActual = tipoDeNodo(metodo->hijos[0]);

    abrirNivel(); // Los parámetros y las variables de primer nivel comparten nivel
    // TODO: insertar parámetros (NODO_PARAM: valor=id, hijos[0]=TYPE; FLAG_PARAMETRO),
    //       recorriendo la cadena PARAMS (hijos[1] es otro PARAMS o NULL)
    (void)params;
    visitarContenidoBloque(bloque);
    cerrarNivel();
}

static void visitarBloque(nodoAST *bloque){
    if(!bloque){
        return;
    }

    abrirNivel();
    visitarContenidoBloque(bloque);
    cerrarNivel();
}

// BLOQUE (NODO_LLAVE): hijos[0]=cadena VAR_DECL, hijos[1]=cadena STATEMENT.
// No abre nivel: lo decide quien lo llama.
static void visitarContenidoBloque(nodoAST *bloque){
    visitarDeclaraciones(bloque->hijos[0]);
    visitarSentencias(bloque->hijos[1]);
}

static void visitarDeclaraciones(nodoAST *declaraciones){
    if(!declaraciones){
        return;
    }

    nodoAST *declaracion = declaraciones->hijos[0];
    nodoAST *declaracionesAux = declaraciones->hijos[1];

    visitarDeclaracion(declaracion);
    visitarDeclaraciones(declaracionesAux);
}

static void visitarSentencias(nodoAST *sentencias){
    if(!sentencias){
        return;
    }

    nodoAST *sentencia = sentencias->hijos[0];
    nodoAST *sentenciasAux = sentencias->hijos[1];

    visitarSentencia(sentencia);
    visitarSentencias(sentenciasAux);
}

// VAR: hijos[0]=TYPE, hijos[1]=IDS. IDS encadena: valor=id, hijos[0]=NODO_IDS (valor NULL) -> IDS ...
static void visitarDeclaracion(nodoAST *declaracion){
    if(!declaracion){
        return;
    }

    TipoDato tipo = tipoDeNodo(declaracion->hijos[0]);

    for(nodoAST *id = declaracion->hijos[1]; id; id = id->hijos[0]){
        if(!id->valor){
            continue; // Nodo intermedio de ID_PRIMA
        }

        // insertarSimbolo ya imprime "Identificador redeclarado!!!"
        Simbolo *simbolo = insertarSimbolo(FLAG_VARIABLE, id->valor, tipo);
        if(!simbolo){
            errorSemantico = 1;
        }

        //Guardamos en el arbol el resultado
        id->simbolo = simbolo;
    }
}

static void visitarSentencia(nodoAST *sentencia){
    if(!sentencia){
        return; // ';' vacío
    }

    if(sentencia->tipo == NODO_OP_ASIG){
        nodoAST *id = sentencia->hijos[0];
        if(id->tipo == NODO_ID && id->simbolo){
            TipoDato tipoId = id->simbolo->tipo;
            TipoDato tipoExpr = visitarExpresion(sentencia->hijos[1]);
            if(tipoId != tipoExpr && tipoExpr != TIPO_INDEFINIDO){
                fprintf(stderr,"Error semántico: asignación de tipos incompatibles\n");
                errorSemantico = 1;
            } else {
                id->simbolo->inicializado = 1;
            }
        } else {
            fprintf(stderr,"Error semántico: asignación a identificador no declarado o no válido\n");
            errorSemantico = 1;
        }
    }else if(sentencia->tipo == NODO_METHOD_CALL){
        visitarLlamada(sentencia->hijos[0]); // hijos[0]=ARGUMENTS_CALL o NO_ARGUMENTS_CALL
    }else if(sentencia->tipo == NODO_IF){
        // TODO: hijos[0]=EXPR debe ser bool; hijos[1]=BLOQUE; hijos[2]=ELSE (opcional)
    }else if(sentencia->tipo == NODO_WHILE){
        // TODO: hijos[0]=EXPR debe ser bool; hijos[1]=BLOQUE
    }else if(sentencia->tipo == NODO_RETURN){
        nodoAST *envoltorioExpr = sentencia->hijos[0];

        if(!envoltorioExpr){
            if(tipoRetornoActual != TIPO_VOID){
                fprintf(stderr,"Error semántico: el método debe retornar un valor\n");
                errorSemantico = 1;
            }
        }else if(tipoRetornoActual == TIPO_VOID){
            fprintf(stderr,"Error semántico: un método void no puede retornar un valor\n");
            errorSemantico = 1;
        }else{
            TipoDato tipoExpr = visitarExpresion(envoltorioExpr->hijos[0]);

            if(tipoExpr != TIPO_INDEFINIDO && tipoExpr != tipoRetornoActual){
                fprintf(stderr,"Error semántico: el tipo retornado no coincide con el del método\n");
                errorSemantico = 1;
            }
        }
    }else if(sentencia->tipo == NODO_BLOQUE){
        visitarBloque(sentencia->hijos[0]); // Bloque anidado: hijos[0]=NODO_LLAVE
    }else{
        fprintf(stderr,"Sentencia desconocida\n");
        errorSemantico = 1;
    }
}

// Recorre la cadena de argumentos (ARG(EXPR, ARGS(ARG(EXPR, ...)))) en orden y compara
// cada uno con el parámetro formal de la misma posición. *indice termina con la cantidad pasada.
static void visitarArgumentos(nodoAST *args, Simbolo *funcion, int *indice){
    if(!args){
        return;
    }
    if(args->tipo == NODO_EXPR){
        TipoDato tipoArg = visitarExpresion(args->hijos[0]);
        if(*indice < funcion->numParams && tipoArg != TIPO_INDEFINIDO
           && tipoArg != funcion->tiposParams[*indice]){
            fprintf(stderr,"Error semántico: el argumento %d de '%s' tiene un tipo incompatible\n", *indice + 1, funcion->nombre);
            errorSemantico = 1;
        }
        (*indice)++;
        return;
    }
    visitarArgumentos(args->hijos[0], funcion, indice);
    visitarArgumentos(args->hijos[1], funcion, indice);
}

// Valida una llamada (existe, es función, cantidad y tipos de argumentos).
// Devuelve el tipo de retorno de la función, o TIPO_INDEFINIDO si la llamada es inválida.
static TipoDato visitarLlamada(nodoAST *llamada){
    Simbolo *funcion = buscarSimbolo(llamada->valor);
    if(!funcion){
        fprintf(stderr,"Error semántico: llamada a '%s', que no está declarada\n", llamada->valor);
        errorSemantico = 1;
        return TIPO_INDEFINIDO;
    }
    if(funcion->flag != FLAG_FUNCION){
        fprintf(stderr,"Error semántico: '%s' no es una función\n", llamada->valor);
        errorSemantico = 1;
        return TIPO_INDEFINIDO;
    }
    llamada->simbolo = funcion;

    int pasados = 0;
    visitarArgumentos(llamada->hijos[0], funcion, &pasados); // NULL si no hay argumentos
    if(pasados != funcion->numParams){
        fprintf(stderr,"Error semántico: '%s' espera %d argumento(s) pero recibió %d\n", funcion->nombre, funcion->numParams, pasados);
        errorSemantico = 1;
    }
    return funcion->tipo;
}

static TipoDato visitarExpresion(nodoAST *expresion){
    if(!expresion){
        return TIPO_INDEFINIDO;
    }

    // TODO: devolver el tipo de la expresión; TIPO_INDEFINIDO si hay error (evita errores en cascada)
    //   NODO_ID / NODO_LITERAL / NODO_METHOD_CALL / aritméticos / relacionales / AND, OR / SIGNO_MENOS / NEG
    // OJO: el parser crea NODO_LITERAL igual para int, float y bool; con eso no se puede tipar
    //      una constante. Hace falta distinguirlas en el AST (como NODO_CTE_ENTERA del pre-proyecto).

    if(expresion->tipo == NODO_ID){
        if(expresion->simbolo){
            return expresion->simbolo->tipo;
        }
    }else if(expresion->tipo == NODO_METHOD_CALL){
        return visitarLlamada(expresion->hijos[0]);
    }

    return TIPO_INDEFINIDO;
}

static void exigirMain(){
    Simbolo *main = buscarSimbolo("main");
    if(!main){
        fprintf(stderr,"Error semántico: no se encontró la función main\n");
        errorSemantico = 1;
    }
}

//Punto de entrada del analisis semantico: recibe la raiz del AST
void analizarSemantica(nodoAST *raiz){
    if(!raiz){
        return;
    }

    errorSemantico = 0;
    inicializarTablaSimbolos();
    abrirNivel(); // Nivel global

    visitarDeclaracionesGlobales(raiz);
    // TODO: reglas globales (p. ej. exigir un método main, según la especificación del lenguaje)
    exigirMain();
    visitarMetodos(raiz);

    cerrarNivel();
}
