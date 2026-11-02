#!/bin/sh
# Prueba las optimizaciones. Cada opNN_*.ctds se compila con todas las optimizaciones (-opt all):
#  - si hay opNN_*.esperado, se ejecuta el programa (con opNN_*.in si existe) y se compara la salida;
#  - si no, alcanza con que compile (por ejemplo, casos que solo miran el AST o el IR).
# Uso: COMPILADOR=/ruta/a/c-tds sh tests/test_optimizaciones/correr_tests.sh   (por defecto, ../../c-tds)
cd "$(dirname "$0")" || exit 1
COMPILADOR=${COMPILADOR:-../../c-tds}
fallos=0
for fuente in *.ctds; do
    base=${fuente%.ctds}
    if [ -f "$base.esperado" ]; then
        entrada=/dev/null; [ -f "$base.in" ] && entrada="$base.in"
        if $COMPILADOR -opt all -o "$base.out" "$fuente" && "./$base.out" < "$entrada" | diff -q - "$base.esperado" > /dev/null; then
            echo "OK    $base"
        else
            echo "FALLO $base"; fallos=1
        fi
        rm -f "$base.out"
    else
        if $COMPILADOR -opt all -target parse -o "$base.sint" "$fuente" > /dev/null; then
            echo "OK    $base"
        else
            echo "FALLO $base"; fallos=1
        fi
        rm -f "$base.sint"
    fi
done
exit $fallos
