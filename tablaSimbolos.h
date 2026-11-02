/*
 * TAD: Tabla de Símbolos (archivo.h)

 * Se definen las estructuras y operaciones necesarias para gestionar
 * el ciclo de vida y alcance (scope) de los identificadores durante la
 * compilación.
 *
 * Arquitectura principal:
 * - Implementa una Pila de Niveles encadenada, donde cada nivel representa
 *   un contexto local (bloque) y contiene su propia lista de símbolos.
 *   NodoNivel y NodoSimbolo son detalles de implementación de esa pila y
 *   viven solo en tablaSimbolos.c; nadie fuera de este módulo los necesita.
 *
 * - Este módulo no sabe nada del AST. La decoración del árbol (guardar un
 *   puntero a Simbolo en cada nodo) es responsabilidad del analizador
 *   semántico, que sí conoce ambos módulos.
*/

#ifndef TABLA_SIMBOLOS_H
#define TABLA_SIMBOLOS_H

/*ESTRUCTURA DE LOS SIMBOLOS*/

#define MAX_PARAMS 16 // Maximo de parametros formales que admite una funcion
#define BYTES_SLOT 8  // Tamano de cada lugar del frame (variables, parametros y temporales)

//Enumerado de flags
typedef enum {
    FLAG_VARIABLE,
    FLAG_FUNCION,
    FLAG_PARAMETRO
} FlagSimbolo;

// Enumerado de los tipos de dato del lenguaje fuente
typedef enum {
    TIPO_INT,
    TIPO_BOOL,
    TIPO_VOID,
    TIPO_FLOAT,
    TIPO_INDEFINIDO // no se pudo determinar el tipo (error semantico)
} TipoDato;

// El registro individual para cada identificador (Datos puros)
typedef struct simbolo{
    FlagSimbolo flag;
    char *nombre;
    TipoDato tipo;
    int valor; //Solo lo voy a usar para crear constantes porque la tabla de simbolos no debe actualizar ni guardar valores, solo direcciones y nombres
    int inicializado; //1 si ya hubo una asignacion previa a la variable; lo usa el analisis semantico
    int numParams; //Solo FLAG_FUNCION: cantidad de parametros formales
    TipoDato tiposParams[MAX_PARAMS]; //Solo FLAG_FUNCION: tipo de cada parametro, en orden
    struct simbolo *params[MAX_PARAMS]; //Solo FLAG_FUNCION: simbolo de cada parametro formal, en orden (lo usa el generador de codigo objeto)
    int esExterna; //Solo FLAG_FUNCION: 1 si se declaro con extern (no tiene cuerpo, la define otra unidad)
    int esGlobal; //1 si se declaro en el nivel global (variables y funciones)
    int offset; //Solo variables y parametros locales: desplazamiento respecto de %rbp; lo asigna el generador de codigo objeto (o el analisis semantico con -opt slots)
    int slotsLocales; //Solo FLAG_FUNCION: cantidad maxima de slots de 8 bytes que usan las variables locales de la funcion (con -opt slots)
} Simbolo;

// Convierte el texto del token de tipo ("int"/"bool"/"void"/"float") al enum correspondiente
TipoDato tipoDesdeTexto(const char *texto);

/* OPERACIONES DEL TAD*/

void inicializarTablaSimbolos(void);
void abrirNivel(void);
void cerrarNivel(void);
Simbolo* insertarSimbolo(FlagSimbolo flag, char *nombre, TipoDato tipo);
Simbolo* buscarSimbolo(char *nombre);

/*
 * Reutilizacion de lugares del frame (-opt slots).
 * Mientras el analisis semantico recorre una funcion, insertarSimbolo le da un lugar
 * a cada variable local. Si hay lugares liberados por scopes ya cerrados, reutiliza
 * uno; si no, reserva el siguiente. Al cerrar un nivel se liberan los lugares de las
 * variables declaradas en ese nivel. Los parametros y los temporales no se reutilizan.
 */
void activarReusoLugares(int activo);
void reiniciarLugares(void); // Empieza una funcion nueva: vacia la lista de lugares libres
int lugaresReservados(void); // Maximo de slots usados por las variables locales de la funcion actual

#endif // TABLA_SIMBOLOS_H