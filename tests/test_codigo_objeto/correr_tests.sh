#!/bin/sh
# Prueba el codigo objeto en Linux/WSL. Para cada coNN.ctds compila a assembly, lo enlaza con
# coNN.c (un main en C que llama a las funciones generadas y compara resultados) y lo ejecuta.
# Uso: COMPILADOR=/ruta/a/c-tds sh tests/test_codigo_objeto/correr_tests.sh   (por defecto, ../../c-tds)
# En el assembly se renombra main a tds_main y se exportan todas las funciones, solo para el test.
cd "$(dirname "$0")" || exit 1
COMPILADOR=${COMPILADOR:-../../c-tds}
fallos=0
for fuente in co_*.ctds; do
    base=${fuente%.ctds}
    $COMPILADOR -target assembly -o "$base.ass" "$fuente" || { echo "FALLO compilando $base"; fallos=1; continue; }
    sed -E 's/\bmain\b/tds_main/g; s/^([A-Za-z_][A-Za-z0-9_]*):$/\t.globl \1\n\1:/' "$base.ass" > "$base.s"
    if gcc -o "$base.out" "$base.c" "$base.s" && "./$base.out"; then echo "OK    $base"; else echo "FALLO $base"; fallos=1; fi
    rm -f "$base.ass" "$base.s" "$base.out"
done
exit $fallos
