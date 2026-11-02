#!/bin/sh
# Prueba las optimizaciones. Cada opNN_*.ctds se compila con todas las optimizaciones (-opt all):
#  - si hay opNN_*.esperado, se ejecuta el programa (con opNN_*.in si existe) y se compara la salida;
#  - si no, alcanza con que compile (por ejemplo, casos que solo miran el AST o el IR);
#  - los err_*.ctds tienen un error semantico en codigo que una optimizacion podria descartar (despues
#    de un return, en un if(false)...): tienen que seguir sin compilar, una optimizacion no debe volver
#    valido un programa invalido.
# Uso: COMPILADOR=/ruta/a/c-tds sh tests/test_optimizaciones/correr_tests.sh   (por defecto, ../../c-tds)
cd "$(dirname "$0")" || exit 1
COMPILADOR=${COMPILADOR:-../../c-tds}
fallos=0
for fuente in *.ctds; do
    base=${fuente%.ctds}
    case $base in
        err_*)
            if $COMPILADOR -opt all -target codinter -o "$base.ci" "$fuente" > /dev/null 2>&1; then
                echo "FALLO $base"; fallos=1
            else
                echo "OK    $base"
            fi
            rm -f "$base.ci"
            continue ;;
    esac
    if [ -f "$base.esperado" ]; then
        entrada=/dev/null; [ -f "$base.in" ] && entrada="$base.in"
        if $COMPILADOR -opt all -o "$base.out" "$fuente" && "./$base.out" < "$entrada" | diff -q - "$base.esperado" > /dev/null; then
            echo "OK    $base"
        else
            echo "FALLO $base"; fallos=1
        fi
        rm -f "$base.out"

        # slots_01_reuso: ademas de ejecutar, comprueba que -opt slots achica el frame.
        # Se comparan los dos assembly: sin optimizaciones y solo con reutilizacion de lugares.
        if [ "$base" = "slots_01_reuso" ]; then
            $COMPILADOR -target assembly -o "$base.sin.ass" "$fuente" || fallos=1
            $COMPILADOR -opt slots -target assembly -o "$base.con.ass" "$fuente" || fallos=1
            sin=$(sed -n '/^main:/,/^$/s/.*enter \$\([0-9]*\).*/\1/p' "$base.sin.ass" | head -1)
            con=$(sed -n '/^main:/,/^$/s/.*enter \$\([0-9]*\).*/\1/p' "$base.con.ass" | head -1)
            if [ -n "$sin" ] && [ -n "$con" ] && [ "$con" -lt "$sin" ]; then
                echo "OK    $base (frame $sin -> $con)"
            else
                echo "FALLO $base (frame $sin -> $con)"; fallos=1
            fi
            rm -f "$base.sin.ass" "$base.con.ass"
        fi
    else
        if $COMPILADOR -opt all -target codinter -o "$base.ci" "$fuente" > /dev/null; then
            echo "OK    $base"
        else
            echo "FALLO $base"; fallos=1
        fi
        rm -f "$base.ci"
    fi
done
exit $fallos
