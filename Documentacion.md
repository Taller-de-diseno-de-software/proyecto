# Documentación del proyecto

## Alcance actual

El proyecto implementa dos etapas iniciales de un compilador:

- **Analizador léxico:** desarrollado con Flex. Reconoce palabras reservadas,
  identificadores, literales, operadores y delimitadores.
- **Analizador sintáctico:** desarrollado con Bison. Comprueba si la secuencia
  de tokens cumple la gramática definida para el lenguaje.

El lenguaje implementado es intencionalmente reducido. Por el momento, el
proyecto valida la estructura sintáctica, pero todavía no incluye análisis
semántico, tabla de símbolos, verificación de tipos ni generación real de un
AST.

## Día 1

### Trabajo realizado

- Se implementó el reconocimiento de palabras reservadas, identificadores,
  constantes enteras, reales y lógicas.
- Se agregaron operadores aritméticos, relacionales y lógicos.
- Se agregaron asignación, paréntesis, llaves, comas y punto y coma.
- Se definieron las primeras reglas del analizador sintáctico con Bison.

### Decisiones de diseño

#### Resta binaria y signo negativo

El símbolo `-` puede representar una resta binaria o el signo de un número
negativo. Se utiliza el token `OP_RESTA` junto con una precedencia específica
para su uso unario:

```yacc
| OP_RESTA EXPR %prec SIGNO_MENOS
```

#### Declaraciones globales

Las variables y los métodos comienzan con tokens similares. Por eso se usa
una lista general de declaraciones:

```yacc
DECLS
    : DECL DECLS
    |
    ;

DECL
    : VAR
    | METHOD
    ;
```

El token que aparece después del identificador, como `;` o `(`, permite
continuar con la alternativa correspondiente.

### Problemas detectados

- La primera versión no aceptaba métodos sin parámetros, por ejemplo
  `main()`.
- La definición inicial de declaraciones globales producía conflictos porque
  `VAR` y `METHOD` comparten el comienzo de sus producciones.

### Pendientes para el Día 2

- Incorporar el manejo de comentarios de una línea (`//`) y multilínea
  (`/* ... */`) en el analizador léxico.

### Comandos usados hasta este momento

En esta etapa se generaban el analizador léxico y el analizador sintáctico con:

```bash
flex analizadorLexico.l
bison -d analizadorSintactico.y
```

Todavía no había un ejecutable completo para correr los tests.

## Día 2

### Trabajo realizado

#### Paréntesis en `while`

Se modificó la regla para exigir una expresión entre paréntesis:

```yacc
| WHILE PAR_IZQ EXPR PAR_DER BLOQUE
```

Ahora se acepta, por ejemplo:

```c
while (x < 10) {
    x = x + 1;
}
```

#### Declaraciones locales

Los bloques permiten declarar cero o más variables antes de las sentencias:

```yacc
BLOQUE
    : LLAVE_IZQ VAR_DECL STATEMENTS LLAVE_DER
    ;

VAR_DECL
    : VAR VAR_DECL
    |
    ;
```

Se mantiene `DECLS` para las declaraciones globales, ya que reutilizarlo
dentro de un bloque también permitiría declarar métodos localmente.

#### Comentarios

En el lexer se incorporó el estado `COMENTARIO` de Flex:

- `//` ignora el resto de la línea.
- `/*` cambia al estado `COMENTARIO`.
- `*/` finaliza el comentario multilínea y vuelve al estado `INITIAL`.
- El contenido de un comentario multilínea se ignora.

### Problemas detectados:

No hubo problemas detectados durante el Día 2.

### Comandos usados hasta este momento

Se regeneraban los archivos del lexer y del parser con los mismos comandos:

```bash
flex analizadorLexico.l
bison -d analizadorSintactico.y
```

El ejecutable completo todavía no incluía el AST.

## Día 3
### Trabajo Realizado
  Primero completamos el analizador sintáctico, corregimos errores sintácticos.
  Implementamos crearNodo e imprimirArbol.
  Creamos el enum de los tipos de nodo y tipoNodoNombre para los nombres a imprimir.
  Se organizaron los test correctos y se crearon los test con errores para analizar gramatica: puntos y coma, alcance, bloques, etc-
  Se hicieron test especificamente para dejar listas las pruebas para la tabla de simbolos

### Comando para ejecutar todo lo realizado

```bash
flex analizadorLexico.l
bison -d analizadorSintactico.y
gcc analizadorSintactico.tab.c lex.yy.c ast.c -o parser
./parser tests/test_correctos/test_simple.txt
```

### Problemas Detectados
  Reemplacé el tipo del tipo de nodo en el perfil de crearNodo sin cambiar que fuera un puntero y no compilaba.
  Notamos que la gramática acepta BLOQUE sueltos sin nada más. EJ:
  {
    i = 2;
  }

### Pendientes
  Eliminar notas de los test de test_errores/tabla_simbolos test_correctos/tabla_simbolos y probar estos tests con el analizador semantico.

