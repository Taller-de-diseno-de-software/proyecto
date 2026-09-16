DIA 1: Analizador léxico en conjunto por ds, usando el del pre-proyecto cómo referencia.
    Problema que vamos a tener cuando hagamos el analizador sintáctico:
        Cuando es resta y cuando es nro negativo??
    Problema que tuvimos que desencapsular los operadores binarios para dar precedencia a OP_RESTA cuando representa un numero negativo sobre cuando representa la resta binaria

    NOTAS DIA 1
    -> PENDIENTE PARA DIA 2: Lexer no resuelve comentarios (//    /**/)
    -> NOTAMOS: La gramatica no acepta funciones con parametros vacios (ej: main())
    -> NOTAMOS: P → VAR_DECL METHOD_DECL no funciona. El problema es que VAR y METHOD
       arrancan con los mismos tokens, entonces bison entra en conflicto y elige mal.
       La solución es DECLS → DECL DECLS, DECL → VAR | METHOD. Así los dos caminos
       están abiertos al mismo tiempo y el ( o el ; después del ID decide cuál es.
       En criollo: sacás VAR_DECL y METHOD_DECL y ponés DECLS/DECL en su lugar.

DIA 2:
    Se cambió el while para que acepte expresiones dentro de parentesis
