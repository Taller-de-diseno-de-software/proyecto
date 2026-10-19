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

## Día 3
### Trabajo Realizado
  Primero completamos el analizador sintáctico, corregimos errores sintácticos.
  Implementamos crearNodo e imprimirArbol.
  Creamos el enum de los tipos de nodo y tipoNodoNombre para los nombres a imprimir.

  Quedó implementada la tabla de símbolos, aislada del AST por ahora, para luego conectarlos con el analizador semántico.
  Abre nivel, cierra nivel, inserta símbolo y busca símbolo.

### Problemas Detectados
  Reemplacé el tipo del tipo de nodo en el perfil de crearNodo sin cambiar que fuera un puntero y no compilaba.
  Notamos que la gramática acepta BLOQUE sueltos sin nada más. EJ:
  {
    i = 2;
  }

  Podríamos usar la flag para diferenciar variables locales de globales.
  En el símbolo se podría agregar el campo parámetros para simplificar el uso de la tabla de símbolos.

## Día 4
### Trabajo Realizado
  Implementamos el analizador semántico y lo conectamos con el parser: el main llama a analizarSemantica y rechaza el programa si huboErrorSemantico.
  Se guardan en el símbolo de cada función la cantidad y los tipos de sus parámetros (numParams y tiposParams, con un máximo de MAX_PARAMS).
  Los parámetros se insertan en el mismo nivel que las variables del primer bloque del método, así que se detecta que se redeclaren.
  Se validan las llamadas: que la función exista, que sea una función, la cantidad de argumentos y el tipo de cada uno.
  visitarExpresion ahora devuelve el tipo de identificadores, literales, llamadas y operadores (aritméticos, relacionales, lógicos, signo menos y negación).
  Se validan las asignaciones, las condiciones de if y while y el return contra el tipo del método.
  Armamos la carpeta tests con casos correctos y con errores sintácticos y de tabla de símbolos.

### Problemas Detectados
  Los operadores no se visitaban, así que en x = z + 1; nunca se llegaba a z y no se avisaba que no estaba declarada.
  El bloque del else se pasaba a visitarBloque como NODO_ELSE y no como su hijo, y todo if con else terminaba en segmentation fault.
  Al principio el main ignoraba el resultado del análisis y aceptaba programas con errores semánticos.
  Los literales se tipan por su texto (true/false, con punto, entero) porque el parser crea el mismo nodo para los tres.
  Los tipos se comparan de forma exacta: float f; f = 5; da error porque 5 es int. Hay que decidir si se permite esa conversión.


## Día 5
### Trabajo Realizado
  Implementamos el generador de código intermedio de tres direcciones (codigoIntermedio.c y codigoIntermedio.h). Recorre el AST ya decorado por el analizador semántico y produce una lista enlazada de cuádruplas (op, arg1, arg2, res).
  Las instrucciones (OperadorIR) son: aritméticas (ADD, SUB, MULT, DIV, MOD, NEG), lógicas (AND, OR, NOT), relacionales (LT, GT, EQ), ASSIGN, de flujo (LABEL, JUMP, JMPF) y de funciones (ENTER, PARAM, CALL, RET, LEAVE).
  Los operandos (KindOperando) pueden ser constante, variable (apunta al Simbolo), temporal, etiqueta o función. Los constructores son operandoConst, operandoVar, operandoTemp, operandoLabel y operandoFunc, y nuevoTemp / nuevaEtiqueta llevan los contadores (t1, t2... y L1, L2...).
  genExpr devuelve el operando donde queda el valor de cada expresión. Las constantes y variables no emiten código, se usan directamente como operando. Los operadores binarios y unarios emiten una cuádrupla con un temporal nuevo.
  Las llamadas (genLlamada) evalúan primero todos los argumentos y recién después emiten los PARAM, para que los de una llamada anidada no se intercalen con los de la externa. El CALL guarda la función y la cantidad de argumentos, y devuelve un temporal con el resultado (nada si la función es void).
  El if usa JMPF y una etiqueta al final (con else, una más y un JUMP al final del then). El while pone una etiqueta al inicio, reevalúa la condición en cada vuelta y cierra con JUMP al inicio.
  Cada método se encierra entre ENTER y LEAVE. Los métodos void que no terminan en return reciben un RET implícito. Las declaraciones y los parámetros no generan código.
  generarCodigoIntermedio devuelve NULL si huboErrorSemantico, así que solo se genera código de programas correctos. imprimirIR lo escribe en formato legible (t1 = a + b, ifFalse t1 goto L1, param x, t2 = call f, 1).

  Armamos un main.c nuevo que organiza las etapas con la interfaz c-tds [opcion] archivo.ctds:
  - -target scan|parse|codinter: la compilación llega hasta esa etapa (por defecto codinter).
  - -o: nombre del archivo de salida.
  - -opt: se acepta pero todavía no hace nada.
  - -debug: imprime información de debugging por pantalla.
  Cada etapa escribe su archivo a partir del nombre de entrada: .lex (tokens), .sint (AST) y .ci (código intermedio). Sin -debug una compilación exitosa no imprime nada, los errores van a stderr y el código de retorno es distinto de 0 si falla alguna etapa.
  Para esto se sacó el main del analizador sintáctico, el lexer escribe los tokens en archivoTokens en lugar de usar printf, se agregó imprimirArbolEn (imprimirArbol a un archivo) y los mensajes de error pasaron a stderr.
  En el analizador semántico se terminó la tipificación de expresiones (tipoAritmetico, tipoRelacional, tipoLogico, tipoDeLiteral), que es lo que el generador necesita para decidir el IR.
  Armamos la carpeta tests/test_codigo_intermedio con cuatro casos: aritmética y lógica, control de flujo, llamadas y return, y recursión con variables globales. Incluye las salidas .lex, .sint y .ci del primer caso.

### Problemas Detectados
  El literal llega doblemente envuelto (LITERAL(LITERAL "texto")), así que hay que leer el texto del hijo.
  El IR solo cubre enteros y lógicos: los float se truncan a int (limitación documentada en genLiteral).
  Los operadores && y || evalúan siempre los dos operandos, no hay cortocircuito.
  Los temporales se piden después de generar los operandos para que el subresultado tome el número menor (t1 = i < 10, no al revés).
  Los argumentos están limitados a MAX_PARAMS por llamada; si se supera se avisa por stderr y se ignoran los sobrantes.
  Las variables globales no tienen código propio: solo aparecen como operando de las instrucciones que las usan.


## Día 6
### Trabajo Realizado
  Empezamos la preparación para el generador de código objeto corrigiendo el código intermedio y la tabla de símbolos.
  Cortocircuito: la especificación exige que && y || no evalúen el segundo operando si el primero ya decide el resultado. Se eliminaron las instrucciones AND y OR y genCortocircuito las traduce con saltos: el resultado queda en un temporal, y el operando derecho se evalúa solo si hace falta (a && b: t = a; ifFalse t goto L; t = b; L:). Para || se niega el temporal antes del ifFalse, así no hace falta una instrucción de salto si verdadero.
  El Simbolo ahora guarda esGlobal (se calcula al insertar, según si está en el primer nivel de la pila), offset (desplazamiento respecto de %rbp, lo asigna el generador de código objeto) y params (los símbolos de los parámetros formales de una función, en orden). El analizador semántico completa params en insertarParametros. Se necesitan para copiar %rdi, %rsi... a sus lugares en el prólogo de cada función, y para distinguir globales (van en .data/.bss) de locales (van en el stack).
  Se agregó el caso ci_05_cortocircuito con llamadas con efecto sobre una global en el lado derecho, y se regeneró ci_01 porque usa && y ||.

### Problemas Detectados
  El cortocircuito del Día 5 figuraba como problema conocido y queda resuelto.
  Los .ci están en el .gitignore, así que solo se versionan los que ya estaban agregados.
