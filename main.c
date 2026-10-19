/*
 * Programa principal del compilador (c-tds).
 *
 * Interfaz:  c-tds [opcion] nombreArchivo.ctds
 *   -target <scan|parse|codinter|assembly> la compilacion procede hasta la etapa dada
 *   -o <salida>                     nombre del archivo de salida (sin -target, del ejecutable)
 *   -opt [optimizacion]             se acepta; todavia no hace nada
 *   -debug                          imprime informacion de debugging
 *
 * Etapas y archivo de salida (nombreArchivo + extension):
 *   scan     -> .lex    listado de tokens
 *   parse    -> .sint   AST
 *   codinter -> .ci     codigo intermedio
 *   assembly -> .ass    assembly x86-64
 *   (sin -target) -> .out   ejecutable: se ensambla y enlaza el .ass con gcc y libtds.c, que debe estar
 *                           en el mismo directorio que c-tds. El .ass intermedio se borra salvo con -debug.
 *
 * Sin -debug una compilacion exitosa no imprime nada; los errores van a stderr.
 * El codigo de retorno es distinto de 0 si alguna etapa falla.
 * 
 * Compilacion:
 * bison -d -v analizadorSintactico.y
 * flex analizadorLexico.l
 * gcc -o c-tds main.c ast.c tablaSimbolos.c analizadorSemantico.c codigoIntermedio.c codigoObjeto.c analizadorSintactico.tab.c lex.yy.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <unistd.h>
#endif
#include "ast.h"
#include "analizadorSemantico.h"
#include "codigoIntermedio.h"
#include "codigoObjeto.h"

#define EXTENSION_ENTRADA ".ctds"
#define BIBLIOTECA_EJECUCION "libtds.c" // define get_int y printInt, las funciones extern

typedef enum {
    ETAPA_SCAN,
    ETAPA_PARSE,
    ETAPA_CODINTER,
    ETAPA_ASSEMBLY,
    ETAPA_EJECUTABLE // la etapa por defecto: llega hasta el ejecutable
} Etapa;

typedef struct {
    Etapa target;
    int debug;
    const char *entrada;
    const char *salida; // NULL: se deriva del nombre de entrada
    const char *programa; // argv[0], para ubicar libtds.c junto al compilador
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
    op->target = ETAPA_EJECUTABLE;
    op->programa = argv[0];
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

static void *reservar(size_t bytes) {
    void *p = malloc(bytes);
    if (p == NULL) {
        fprintf(stderr, "Error: sin memoria\n");
        exit(EXIT_FAILURE);
    }
    return p;
}

// La entrada sin su extension .ctds mas `extension`
static char *nombreDerivado(const Opciones *op, const char *extension) {
    size_t base = strlen(op->entrada) - strlen(EXTENSION_ENTRADA);
    char *nombre = reservar(base + strlen(extension) + 1);

    memcpy(nombre, op->entrada, base);
    strcpy(nombre + base, extension);
    return nombre;
}

// Nombre del archivo de salida: -o si se dio; si no, la entrada con la extension de la etapa
static char *nombreSalida(const Opciones *op, const char *extension) {
    if (op->salida == NULL) return nombreDerivado(op, extension);

    char *nombre = reservar(strlen(op->salida) + 1);
    strcpy(nombre, op->salida);
    return nombre;
}

static FILE *abrirArchivo(const char *nombre) {
    FILE *archivo = fopen(nombre, "w");

    if (archivo == NULL) {
        fprintf(stderr, "Error: no se pudo crear %s\n", nombre);
    }
    return archivo;
}

static FILE *abrirSalida(const Opciones *op, const char *extension) {
    char *nombre = nombreSalida(op, extension);
    FILE *archivo = abrirArchivo(nombre);
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

// Directorio donde esta el compilador (ahi se busca libtds.c), con la barra final; "" si no se sabe
static char *directorioDelCompilador(const char *programa) {
    char ruta[4096] = "";

#ifdef __linux__
    ssize_t n = readlink("/proc/self/exe", ruta, sizeof(ruta) - 1);
    ruta[n > 0 ? n : 0] = '\0';
#endif
    if (ruta[0] == '\0' && strlen(programa) < sizeof(ruta)) {
        strcpy(ruta, programa);
    }

    char *corte = strrchr(ruta, '/');
    char *corteWindows = strrchr(ruta, '\\');
    if (corteWindows != NULL && (corte == NULL || corteWindows > corte)) corte = corteWindows;
    if (corte != NULL) {
        corte[1] = '\0';
    } else {
        ruta[0] = '\0';
    }

    char *directorio = reservar(strlen(ruta) + 1);
    strcpy(directorio, ruta);
    return directorio;
}

// Con -target assembly deja el .ass; sin target lo ensambla y enlaza hasta el ejecutable
static int etapaAssembly(const Opciones *op, ListaIR *ir) {
    int ejecutable = op->target == ETAPA_EJECUTABLE;
    char *rutaAss = ejecutable ? nombreDerivado(op, ".ass") : nombreSalida(op, ".ass");
    int fallo = 1;

    FILE *salida = abrirArchivo(rutaAss);
    if (salida != NULL) {
        generarAssembly(ir, salida);
        fclose(salida);
        fallo = 0;
    }

    if (ejecutable && !fallo) {
        char *rutaExe = nombreSalida(op, ".out");
        char *directorio = directorioDelCompilador(op->programa);
        size_t largo = strlen(rutaAss) + strlen(rutaExe) + strlen(directorio) + 128;
        char *comando = reservar(largo);

        // -x indica el lenguaje de cada archivo, asi no importa la extension .ass
        snprintf(comando, largo, "gcc -x assembler \"%s\" -x c \"%s%s\" -o \"%s\"",
                 rutaAss, directorio, BIBLIOTECA_EJECUCION, rutaExe);
        fallo = system(comando) != 0;
        if (fallo) {
            fprintf(stderr, "Error: fallo el ensamblado o el enlace (%s)\n", comando);
        }
        if (!op->debug) {
            remove(rutaAss);
        }
        free(comando);
        free(directorio);
        free(rutaExe);
    }
    free(rutaAss);
    return fallo;
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
            if (!fallo && op.target >= ETAPA_ASSEMBLY) {
                fallo = etapaAssembly(&op, ir);
            }
            liberarLista(ir);
        }
    }

    fclose(yyin);
    return fallo ? 1 : 0;
}