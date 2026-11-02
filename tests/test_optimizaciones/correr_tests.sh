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
