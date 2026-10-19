#!/bin/sh
# Prueba el codigo objeto en Linux/WSL. Cada coNN.ctds se compila a assembly y se ejecuta de una de dos formas:
#  - con coNN.esperado: se enlaza con libtds.c, se le da coNN.in como entrada (si existe) y se compara la salida
#  - con coNN.c: un main en C llama a las funciones generadas y compara resultados (en el assembly se renombra
#    main a tds_main y se exportan todas las funciones, solo para el test)
# Uso: COMPILADOR=/ruta/a/c-tds sh tests/test_codigo_objeto/correr_tests.sh   (por defecto, ../../c-tds)
cd "$(dirname "$0")" || exit 1
COMPILADOR=${COMPILADOR:-../../c-tds}
LIBTDS=../../libtds.c
fallos=0
for fuente in co_*.ctds; do
    base=${fuente%.ctds}
    $COMPILADOR -target assembly -o "$base.ass" "$fuente" || { echo "FALLO compilando $base"; fallos=1; continue; }
    cp "$base.ass" "$base.s"
    if [ -f "$base.esperado" ]; then
        entrada=/dev/null; [ -f "$base.in" ] && entrada="$base.in"
        if gcc -o "$base.out" "$base.s" "$LIBTDS" && "./$base.out" < "$entrada" | diff -q - "$base.esperado" > /dev/null; then
            echo "OK    $base"
        else
            echo "FALLO $base"; fallos=1
        fi
    else
        sed -E 's/\bmain\b/tds_main/g; s/^([A-Za-z_][A-Za-z0-9_]*):$/\t.globl \1\n\1:/' "$base.ass" > "$base.s"
        if gcc -o "$base.out" "$base.c" "$base.s" "$LIBTDS" && "./$base.out"; then echo "OK    $base"; else echo "FALLO $base"; fallos=1; fi
    fi
    rm -f "$base.ass" "$base.s" "$base.out"
done
exit $fallos
