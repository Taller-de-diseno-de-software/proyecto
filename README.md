# Proyecto
Repositorio para la entrega del proyecto para la materia Taller de diseño de software  Resources


## Compilar

Se necesita `flex`, `gcc` y, solo si se modifica la gramática, `bison`. Para probar el código generado hace falta Linux (o WSL en Windows), porque el assembly es x86-64 de Linux.

```sh
bison -d -v analizadorSintactico.y     # solo si cambió la gramática
flex analizadorLexico.l
gcc -o c-tds main.c ast.c optimizador.c tablaSimbolos.c analizadorSemantico.c codigoIntermedio.c codigoObjeto.c analizadorSintactico.tab.c lex.yy.c
```

`libtds.c` (define `get_int` y `printInt`, las funciones `extern`) tiene que estar en el mismo directorio que `c-tds`.

## Usar

```sh
./c-tds programa.ctds                      # genera programa.out
./c-tds -o mi_programa programa.ctds       # el ejecutable se llama mi_programa
./c-tds -target assembly programa.ctds     # solo el assembly: programa.ass
./c-tds -target codinter programa.ctds     # solo el codigo intermedio: programa.ci
./c-tds -target parse programa.ctds        # solo el AST: programa.sint
./c-tds -target scan programa.ctds         # solo los tokens: programa.lex
./c-tds -opt cf programa.ctds              # constant folding sobre el AST
./c-tds -opt as programa.ctds              # simplificación algebraica (x + 0, x * 8, ...)
./c-tds -opt dce programa.ctds             # eliminación de código muerto sobre el AST
./c-tds -opt slots programa.ctds           # reutilización de lugares del frame entre scopes
./c-tds -opt all programa.ctds             # las cuatro: cf, as, dce y slots
./c-tds -opt cf -opt dce -opt slots programa.ctds   # se pueden combinar y repetir
```

`-opt cf` reemplaza en el AST las expresiones enteras y lógicas con todos los operandos constantes por su resultado (`2 * 3 + 1` pasa a `7`). No pliega floats, divisiones por cero ni resultados que se salgan de `int`.

`-opt dce` elimina el código que nunca se ejecuta: lo que sigue a un `return` o a un `while(true)`, los `if(false)` sin `else` y los `while(false)`, y en `if(true)` deja solo la rama que se ejecuta. Las condiciones se evalúan solo con constantes bien tipadas, nunca con variables ni llamadas. Conviene combinarla con `cf` para que las condiciones ya estén plegadas. Actúa después del análisis semántico y solo si no hubo errores (como `as`, desde `-target codinter`; con `parse` no hace nada): así no puede volver válido un programa con un error en el código que descarta.

`-opt cf` actúa justo después del parseo, así que con `-target parse` el `.sint` ya muestra el árbol plegado. Ejemplo en `tests/test_optimizaciones`.

`-opt as` simplifica operaciones con un operando neutro, absorbente o potencia de 2: `x + 0`, `x * 1` y `x / 1` pasan a `x`; `x * 0` y `x % 1` a `0`; `x * 2^k` a `x << k`; `x / 2^k` a `x >> k` (con corrección de signo, da lo mismo que `/`); `x && true`, `x || false`, `!!x` y `--x` a `x`; los absorbentes de `&&` y `||`; y elimina las sentencias `x = x`. Solo opera sobre enteros salvo las identidades que conservan el tipo, y no descarta nunca una expresión con llamadas o divisiones. Necesita el análisis semántico, así que actúa desde `-target codinter` (con `parse` no hace nada). Ejemplo en `tests/test_optimizaciones/as_01_simplificaciones.ctds`; cuando se combinan, el orden es `as`, `cf` y por último `dce`; el test `co_05` de código objeto verifica que el resultado no cambia.

`-opt slots` reutiliza lugares del frame entre variables locales de scopes disjuntos: al declarar una variable se usa un lugar liberado por un scope ya cerrado si hay, y al cerrar el scope se liberan sus lugares. No reutiliza parámetros ni temporales. Se decide durante el análisis semántico (antes de `as`, `cf` y `dce`), así que actúa desde `-target codinter` (con `parse` no hace nada). Test en `tests/test_optimizaciones/slots_01_reuso.ctds`: además de ejecutar el programa, el runner compara el `enter` del assembly con y sin `-opt slots` y exige que el frame sea más chico.

`-debug` imprime información de debugging y conserva el `.ass`.

## Tests

Los tests de código objeto compilan programas, los ejecutan y comparan la salida con la esperada:

```sh
sh tests/test_codigo_objeto/correr_tests.sh
sh tests/test_optimizaciones/correr_tests.sh
```

Usan `../../c-tds` como compilador; con otro se puede indicar `COMPILADOR=/ruta/a/c-tds sh tests/test_codigo_objeto/correr_tests.sh`. Los casos están en `tests/`: `test_codigo_objeto` (entrada `.in` y salida `.esperado`), `test_codigo_intermedio` (código intermedio esperado `.ci`), `test_optimizaciones` (programas y salida esperada con `-opt all`), `test_correctos` y `test_errores` (sintaxis y tabla de símbolos).
