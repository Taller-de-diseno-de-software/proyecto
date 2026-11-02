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
./c-tds -opt dce programa.ctds             # eliminación de código muerto sobre el AST
./c-tds -opt all programa.ctds             # todas las optimizaciones
./c-tds -opt cf -opt dce programa.ctds     # se pueden combinar (siempre se pliega antes de eliminar)
```

`-opt cf` reemplaza en el AST las expresiones enteras y lógicas con todos los operandos constantes por su resultado (`2 * 3 + 1` pasa a `7`). No pliega floats, divisiones por cero ni resultados que se salgan de `int`.

`-opt dce` elimina el código que nunca se ejecuta: lo que sigue a un `return` o a un `while(true)`, los `if(false)` sin `else` y los `while(false)`, y en `if(true)` deja solo la rama que se ejecuta. Las condiciones se evalúan solo con constantes bien tipadas, nunca con variables ni llamadas. Conviene combinarla con `cf` para que las condiciones ya estén plegadas.

Ambas se pueden usar con cualquier `-target` (con `parse` el `.sint` ya muestra el árbol optimizado). Ejemplos en `tests/test_optimizaciones`.

`-debug` imprime información de debugging y conserva el `.ass`.

## Tests

Los tests de código objeto compilan programas, los ejecutan y comparan la salida con la esperada:

```sh
sh tests/test_codigo_objeto/correr_tests.sh
sh tests/test_optimizaciones/correr_tests.sh
```

Usan `../../c-tds` como compilador; con otro se puede indicar `COMPILADOR=/ruta/a/c-tds sh tests/test_codigo_objeto/correr_tests.sh`. Los casos están en `tests/`: `test_codigo_objeto` (entrada `.in` y salida `.esperado`), `test_codigo_intermedio` (código intermedio esperado `.ci`), `test_optimizaciones` (programas y salida esperada con `-opt all`), `test_correctos` y `test_errores` (sintaxis y tabla de símbolos).
