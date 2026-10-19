/*
 * Programa principal del compilador (c-tds).
 *
 * Interfaz:  c-tds [opcion] nombreArchivo.ctds
 *   -target <scan|parse|codinter|assembly> la compilacion procede hasta la etapa dada
 *   -o <salida>                     nombre del archivo de salida
 *   -opt [optimizacion]             se acepta; todavia no hace nada
 *   -debug                          imprime informacion de debugging
 *
 * Etapas y archivo de salida (nombreArchivo + extension):
 *   scan     -> .lex    listado de tokens
 *   parse    -> .sint   AST
 *   codinter -> .ci     codigo intermedio (etapa por defecto)
 *   assembly -> .ass    assembly x86-64
 *
 * Sin -debug una compilacion exitosa no imprime nada; los errores van a stderr.
 * El codigo de retorno es distinto de 0 si alguna etapa falla.
 * 
 * Compilacion:
 * bison -d -v analizadorSintactico.y
 * flex analizadorLexico.l
 * gcc -Wall -g -o c-tds main.c ast.c tablaSimbolos.c analizadorSemantico.c codigoIntermedio.c codigoObjeto.c analizadorSintactico.tab.c lex.yy.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "analizadorSemantico.h"
#include "codigoIntermedio.h"
#include "codigoObjeto.h"

#define EXTENSION_ENTRADA ".ctds"

typedef enum {
    ETAPA_SCAN,
    ETAPA_PARSE,
    ETAPA_CODINTER,
    ETAPA_ASSEMBLY
} Etapa;

typedef struct {
    Etapa target;
    int debug;
    const char *entrada;
    const char *salida; // NULL: se deriva del nombre de entrada
} Opciones;

// Generados por flex y bison
extern FILE *yyin;
extern nodoAST *raiz;
int yyparse(void);
int yylex(void);

// Usados por el lexer: destino del listado de tokens y marca de error lexico
FILE *archivoTokens = NULL;
int huboErrorLexico = 0;

static void mostrarUso(void) {
    fprintf(stderr, "Uso: c-tds [-target scan|parse|codinter|assembly] [-o salida] [-opt [optimizacion]] [-debug] archivo.ctds\n");
}

static int terminaEn(const char *texto, const char *sufijo) {
    size_t n = strlen(texto);
    size_t m = strlen(sufijo);
    return n >= m && strcmp(texto + n - m, sufijo) == 0;
}

static int leerTarget(const char *texto, Etapa *etapa) {
    if (strcmp(texto, "scan") == 0)     { *etapa = ETAPA_SCAN; return 1; }
    if (strcmp(texto, "parse") == 0)    { *etapa = ETAPA_PARSE; return 1; }
    if (strcmp(texto, "codinter") == 0) { *etapa = ETAPA_CODINTER; return 1; }
    if (strcmp(texto, "assembly") == 0) { *etapa = ETAPA_ASSEMBLY; return 1; }
    return 0;
}

// Devuelve 1 si los argumentos son validos y deja el resultado en `op`
static int leerArgumentos(int argc, char **argv, Opciones *op) {
    op->target = ETAPA_CODINTER; // la etapa corriente
    op->debug = 0;
    op->entrada = NULL;
    op->salida = NULL;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "-debug") == 0) {
            op->debug = 1;
        } else if (strcmp(arg, "-o") == 0) {
            if (++i >= argc) { fprintf(stderr, "Error: -o requiere un nombre de salida\n"); return 0; }
            op->salida = argv[i];
        } else if (strcmp(arg, "-target") == 0) {
            if (++i >= argc || !leerTarget(argv[i], &op->target)) {
                fprintf(stderr, "Error: -target requiere scan, parse, codinter o assembly\n");
                return 0;
            }
        } else if (strcmp(arg, "-opt") == 0) {
            // La optimizacion es opcional: se consume solo si no es otra opcion ni el archivo
            if (i + 1 < argc && argv[i + 1][0] != '-' && !terminaEn(argv[i + 1], EXTENSION_ENTRADA)) {
                i++;
            }
        } else if (arg[0] == '-') {
            fprintf(stderr, "Error: opcion desconocida '%s'\n", arg);
            return 0;
        } else if (op->entrada != NULL) {
            fprintf(stderr, "Error: se indico mas de un archivo de entrada\n");
            return 0;
        } else {
            op->entrada = arg;
        }
    }

    if (op->entrada == NULL) {
        fprintf(stderr, "Error: falta el archivo de entrada\n");
        return 0;
    }
    if (!terminaEn(op->entrada, EXTENSION_ENTRADA)) {
        fprintf(stderr, "Error: el archivo de entrada debe tener extension %s\n", EXTENSION_ENTRADA);
        return 0;
    }
    return 1;
}

// Nombre del archivo de salida: -o si se dio; si no, la entrada con la extension de la etapa
static char *nombreSalida(const Opciones *op, const char *extension) {
    const char *origen = op->salida;
    size_t base = origen ? strlen(origen) : strlen(op->entrada) - strlen(EXTENSION_ENTRADA);
    size_t resto = origen ? 0 : strlen(extension);
    char *nombre = malloc(base + resto + 1);

    if (nombre == NULL) {
        fprintf(stderr, "Error: sin memoria\n");
        exit(EXIT_FAILURE);
    }
    if (origen) {
        strcpy(nombre, origen);
    } else {
        memcpy(nombre, op->entrada, base);
        strcpy(nombre + base, extension);
    }
    return nombre;
}

static FILE *abrirSalida(const Opciones *op, const char *extension) {
    char *nombre = nombreSalida(op, extension);
    FILE *archivo = fopen(nombre, "w");

    if (archivo == NULL) {
        fprintf(stderr, "Error: no se pudo crear %s\n", nombre);
    }
    free(nombre);
    return archivo;
}

static int etapaScan(const Opciones *op) {
    FILE *salida = abrirSalida(op, ".lex");
    if (salida == NULL) return 1;

    archivoTokens = salida;
    while (yylex() != 0) {
        // el lexer escribe cada token en archivoTokens
    }
    archivoTokens = NULL;
    fclose(salida);
    return huboErrorLexico;
}

static int etapaParse(const Opciones *op) {
    if (yyparse() != 0 || huboErrorLexico) return 1;

    if (op->debug) {
        imprimirArbol(raiz, 0);
    }
    if (op->target != ETAPA_PARSE) return 0;

    FILE *salida = abrirSalida(op, ".sint");
    if (salida == NULL) return 1;
    imprimirArbolEn(salida, raiz, 0);
    fclose(salida);
    return 0;
}

// Genera el IR; con -target codinter lo escribe en el .ci
static int etapaCodigoIntermedio(const Opciones *op, ListaIR **ir) {
    analizarSemantica(raiz);
    *ir = generarCodigoIntermedio(raiz);
    if (*ir == NULL) return 1; // errores semanticos, ya informados por stderr

    if (op->debug) {
        imprimirIR(*ir, stdout);
    }
    if (op->target != ETAPA_CODINTER) return 0;

    FILE *salida = abrirSalida(op, ".ci");
    if (salida == NULL) return 1;
    imprimirIR(*ir, salida);
    fclose(salida);
    return 0;
}

static int etapaAssembly(const Opciones *op, ListaIR *ir) {
    FILE *salida = abrirSalida(op, ".ass");
    if (salida == NULL) return 1;
    generarAssembly(ir, salida);
    fclose(salida);
    return 0;
}

int main(int argc, char **argv) {
    Opciones op;

    if (!leerArgumentos(argc, argv, &op)) {
        mostrarUso();
        return 1;
    }

    yyin = fopen(op.entrada, "r");
    if (yyin == NULL) {
        fprintf(stderr, "Error: no se pudo abrir %s\n", op.entrada);
        return 1;
    }

    int fallo;
    if (op.target == ETAPA_SCAN) {
        fallo = etapaScan(&op);
    } else {
        archivoTokens = op.debug ? stdout : NULL;
        fallo = etapaParse(&op);
        if (!fallo && op.target >= ETAPA_CODINTER) {
            ListaIR *ir = NULL;
            fallo = etapaCodigoIntermedio(&op, &ir);
            if (!fallo && op.target == ETAPA_ASSEMBLY) {
                fallo = etapaAssembly(&op, ir);
            }
            liberarLista(ir);
        }
    }

    fclose(yyin);
    return fallo ? 1 : 0;
}