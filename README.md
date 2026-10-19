# Proyecto
Repositorio para la entrega del proyecto para la materia Taller de diseño de software  Resources


## Compilar

Se necesita `flex`, `gcc` y, solo si se modifica la gramática, `bison`. Para probar el código generado hace falta Linux (o WSL en Windows), porque el assembly es x86-64 de Linux.

```sh
bison -d -v analizadorSintactico.y     # solo si cambió la gramática
flex analizadorLexico.l
gcc -o c-tds main.c ast.c tablaSimbolos.c analizadorSemantico.c codigoIntermedio.c codigoObjeto.c analizadorSintactico.tab.c lex.yy.c
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
```

`-debug` imprime información de debugging y conserva el `.ass`.

## Tests

Los tests de código objeto compilan programas, los ejecutan y comparan la salida con la esperada:

```sh
sh tests/test_codigo_objeto/correr_tests.sh
```

Usa `../../c-tds` como compilador; con otro se puede indicar `COMPILADOR=/ruta/a/c-tds sh tests/test_codigo_objeto/correr_tests.sh`. Los casos están en `tests/`: `test_codigo_objeto` (entrada `.in` y salida `.esperado`), `test_codigo_intermedio` (código intermedio esperado `.ci`), `test_correctos` y `test_errores` (sintaxis y tabla de símbolos).
