#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tablaSimbolos.h"

/*
 * Estructuras internas de la pila de niveles. Son detalle de implementación
 * del TAD: no aparecen en tablaSimbolos.h porque nada fuera de este archivo
 * debe conocerlas ni manipularlas directamente.
 */

// Nodo para armar la lista enlazada de símbolos dentro del mismo nivel
typedef struct entradaSimbolo {
    Simbolo *simbolo;
    struct entradaSimbolo *siguiente;
} EntradaSimbolo;

typedef struct nivel {
    EntradaSimbolo *primer_nodo; // Cada nivel contiene una lista de entradas de símbolos
    struct nivel *anterior; // Puntero al nivel anterior
} Nivel;

static Nivel *topeTablaSimbolos = NULL;

// Convierte el texto del token de tipo ("int"/"bool"/"void") al enum correspondiente
TipoDato tipoDesdeTexto(const char *texto) {
    if (!texto) return TIPO_INDEFINIDO;
    if (strcmp(texto, "int") == 0) return TIPO_INT;
    if (strcmp(texto, "bool") == 0) return TIPO_BOOL;
    if (strcmp(texto, "void") == 0) return TIPO_VOID;
    if (strcmp(texto, "float") == 0) return TIPO_FLOAT;
    return TIPO_INDEFINIDO;
}


//Primer nivel de la tabla de simbolos
void inicializarTablaSimbolos(void) {
    topeTablaSimbolos = NULL;
}

//Creo nuevo nivel y lo pongo en la cima de la pila
void abrirNivel(void) {
    Nivel *nuevoNivel = (Nivel *)malloc(sizeof(Nivel));
    if (!nuevoNivel) {
        fprintf(stderr, "Error: Memoria insuficiente para abrir un nuevo nivel.\n");
        exit(1);
    }
    
    nuevoNivel->primer_nodo = NULL;
    nuevoNivel->anterior = topeTablaSimbolos;
    
    topeTablaSimbolos = nuevoNivel;
}

//Cerramos/eliminamos el ultimo nivel
void cerrarNivel(void) {
    if (topeTablaSimbolos == NULL) return;

    //Guardo el nivel a cerrar
    Nivel *nivelACerrar = topeTablaSimbolos;
    EntradaSimbolo *actual = nivelACerrar->primer_nodo;

    /*
     * Liberamos solo el andamiaje del nivel (las EntradaSimbolo de la lista).
     * Los Simbolo en si NO se liberan aca: durante el analisis semantico el
     * AST quedo decorado con punteros nodo->simbolo, y el generador de codigo
     * que corre despues los necesita. La propiedad de cada Simbolo pasa al
     * AST, que vive hasta el final del programa.
     */
    while (actual != NULL) {
        EntradaSimbolo *siguiente = actual->siguiente;
        free(actual);          // Libero solo el nodo de la lista
        actual = siguiente;
    }

    //El tope ahora es el anterior al tope anterior
    topeTablaSimbolos = nivelACerrar->anterior;
    free(nivelACerrar);
}

//Inserto un simbolo en el nivel actual de la tabla de simbolos (el tope de la pila)
Simbolo* insertarSimbolo(FlagSimbolo flag, char *nombre, TipoDato tipo) {
    if (topeTablaSimbolos == NULL) {
        fprintf(stderr, "Error: No hay un nivel abierto en la Tabla de Símbolos.\n");
        return NULL;
    }
    
    // Verificamos si ya existe en el MISMO nivel para evitar redeclaraciones
    EntradaSimbolo *actual = topeTablaSimbolos->primer_nodo;
    while (actual != NULL) {
        //Verifico si el nombre del simbolo ya existe en el nivel actual
        if (strcmp(actual->simbolo->nombre, nombre) == 0) {
            printf("Identificador redeclarado!!!\n"); // Mensaje exacto de tus diapositivas
            return NULL; 
        }
        actual = actual->siguiente;
    }
    
    // Si no existe en el nivel actual, creamos los datos del símbolo
    Simbolo *nuevoSimbolo = (Simbolo *)malloc(sizeof(Simbolo));
    nuevoSimbolo->flag = flag;
    nuevoSimbolo->nombre = strdup(nombre);
    nuevoSimbolo->valor = 0; //Nota amadeo: Es 0 porque a la hora de insertar un simbolo no sabemos su valor...
    nuevoSimbolo->inicializado = 0; //recien declarada, todavia sin asignar
    nuevoSimbolo->tipo = tipo;
    nuevoSimbolo->numParams = 0;

    // Creamos el nodo para insertarlo en la lista enlazada del nivel
    EntradaSimbolo *nuevoNodo = (EntradaSimbolo *)malloc(sizeof(EntradaSimbolo));
    nuevoNodo->simbolo = nuevoSimbolo;
    nuevoNodo->siguiente = topeTablaSimbolos->primer_nodo;
    
    topeTablaSimbolos->primer_nodo = nuevoNodo;
    
    return nuevoSimbolo;
}

//Busca un simbolo en la tabla de simbolos, empezando desde el nivel actual y subiendo hasta el nivel global
Simbolo* buscarSimbolo(char *nombre) {
    // En este punto buscamos en la TS los identificadores[cite: 1]
    Nivel *nivelActual = topeTablaSimbolos;

    // Recorremos desde el nivel más profundo (actual) hacia el global
    while (nivelActual != NULL) {
        EntradaSimbolo *nodoActual = nivelActual->primer_nodo;
        
        // Buscamos en la lista enlazada del nivel actual
        while (nodoActual != NULL) {
            if (strcmp(nodoActual->simbolo->nombre, nombre) == 0) {
                return nodoActual->simbolo; // Lo encontramos y devolvemos la info semántica pura
            }
            nodoActual = nodoActual->siguiente;
        }
        
        // Si no está en este bloque, bajamos al bloque circundante[cite: 2]
        nivelActual = nivelActual->anterior;
    }
    
    return NULL; // No está definido en ningún alcance visible
}