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
} Simbolo;

// Convierte el texto del token de tipo ("int"/"bool"/"void"/"float") al enum correspondiente
TipoDato tipoDesdeTexto(const char *texto);

/* OPERACIONES DEL TAD*/

void inicializarTablaSimbolos(void);
void abrirNivel(void);
void cerrarNivel(void);
Simbolo* insertarSimbolo(FlagSimbolo flag, char *nombre, TipoDato tipo);
Simbolo* buscarSimbolo(char *nombre);

#endif // TABLA_SIMBOLOS_H