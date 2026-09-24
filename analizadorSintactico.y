/*
 * Analizador sintáctico generado con Bison. (Parser)
 *
 * Su objetivo principal es recibir la secuencia abstracta de tokens producidos por el analizadorLexico 
 * y verificar que dicha secuencia pueda ser generada por la gramática libre de contexto del lenguaje fuente.
 * Producto resultante: El AST
 *
 */
%{
#include <stdio.h>
#include <stdlib.h>

int yylex(void);
void yyerror(const char *s);
extern int yylineno;
%}

%union {
    nodoAST *nodo;
    char *str;
    }


%token BOOLEAN ELSE IF INT RETURN VOID WHILE FLOAT  //Palabras reservadas
%token <str> CTE_LOGICA CTE_ENTERA CTE_FLOAT ID   //Constantes e ID
%token OP_SUMA OP_PROD OP_RESTA OP_DIV OP_DIVENT OP_MENOR OP_MAYOR OP_EQ OP_AND OP_OR OP_NEG OP_ASIG   //Operaciones
%token PUNTO_COMA PAR_IZQ PAR_DER LLAVE_IZQ LLAVE_DER COMA  //Caracteres 

%type <nodo> P DECLS DECL VAR IDS ID_PRIMA METHOD PARAMS PARAMS_PRIMA PARAM BLOQUE VAR_DECL STATEMENTS TYPE STATEMENT OPTIONAL_ELSE OPTIONAL_EXPR METHOD_CALL ARGS ARGS_PRIMA ARG EXPR LITERAL

%left OP_OR
%left OP_AND
%left OP_EQ
%nonassoc OP_MENOR OP_MAYOR OP_EQ
%left OP_SUMA OP_RESTA
%left OP_PROD OP_DIV OP_DIVENT
%right OP_NEG
%right SIGNO_MENOS



%%

P 
    : DECLS
    ;

DECLS 
    : DECL DECLS
    |
    ;

DECL
    : VAR
    | METHOD
    ;

VAR
    : TYPE IDS PUNTO_COMA
    ;

IDS
    : ID ID_PRIMA   
    ;

ID_PRIMA
    : COMA IDS  //Multiples id
    | 
    ;

METHOD
    : TYPE ID PAR_IZQ PARAMS PAR_DER BLOQUE
    | VOID ID PAR_IZQ PARAMS PAR_DER BLOQUE
    | TYPE ID PAR_IZQ PAR_DER BLOQUE
    | VOID ID PAR_IZQ PAR_DER BLOQUE
    ;

PARAMS
    : PARAM PARAMS_PRIMA
    ;

PARAMS_PRIMA
    : COMA PARAMS
    | 
    ;

PARAM 
    : TYPE ID
    ;

BLOQUE
    : LLAVE_IZQ VAR_DECL STATEMENTS LLAVE_DER {$$ = crearNodo(NODO_LLAVE, NULL, $2, $3, NULL, NULL)}
    ;

VAR_DECL
    : VAR VAR_DECL {$$ = crearNodo(NODO_VAR, NULL, $1, $2, NULL, NULL);}
    | {$$ = NULL;}
    ;

STATEMENTS
    : STATEMENT STATEMENTS {$$ = crearNodo(NODO_STATEMENT, NULL, $1, $2, NULL, NULL);}
    | {$$ = NULL;} 
    ;

TYPE
    : INT {$$ = crearNodo(NODO_TYPE, "int", NULL, NULL, NULL, NULL);}
    | FLOAT {$$ = crearNodo(NODO_TYPE, "float", NULL, NULL, NULL, NULL);}
    | BOOLEAN {$$ = crearNodo(NODO_TYPE, "boolean", NULL, NULL, NULL, NULL);}
    ;

STATEMENT 
    : ID OP_ASIG EXPR PUNTO_COMA {$$ = crearNodo(NODO_OP_ASIG, $1, $3, NULL, NULL);}
    | METHOD_CALL PUNTO_COMA {$$ = crearNodo(NODO_METHOD_CALL, NULL, $1, NULL, NULL);}
    | IF PAR_IZQ EXPR PAR_DER BLOQUE OPTIONAL_ELSE {$$ = crearNodo(NODO_IF, NULL, $3, $5, $6);}
    | WHILE PAR_IZQ EXPR PAR_DER BLOQUE {$$ = crearNodo(NODO_WHILE, NULL, $3, $5, NULL);}
    | RETURN OPTIONAL_EXPR PUNTO_COMA {$$ = crearNodo(NODO_RETURN, NULL, $2, NULL, NULL);}
    | PUNTO_COMA {$$ = NULL}
    | BLOQUE {$$ = crearNodo(NODO_BLOQUE, NULL, $1, NULL, NULL, NULL);}
    ;

OPTIONAL_ELSE
    : ELSE BLOQUE {$$ = crearNodo(NODO_ELSE, NULL, $2, NULL, NULL, NULL);}
    | {$$ = NULL;}
    ;

OPTIONAL_EXPR
    : EXPR {$$ = crearNodo(NODO_EXPR, NULL, $1, NULL, NULL, NULL);}
    | {$$ = NULL;}
    ;

METHOD_CALL 
    : ID PAR_IZQ ARGS PAR_DER {$$ = crearNodo(NODO_ARGUMENTS_CALL, $1, $3, NULL, NULL, NULL);}
    | ID PAR_IZQ PAR_DER {$$ = crearNodo(NODO_NO_ARGUMENTS_CALL, $1, NULL, NULL, NULL, NULL);}
    ;

ARGS
    : ARG ARGS_PRIMA {$$ = crearNodo(NODO_ARG, NULL, $1, $2, NULL, NULL);}
    ;

ARGS_PRIMA
    : COMA ARGS {$$ = crearNodo(NODO_ARGS, NULL, $2, NULL, NULL, NULL);}
    | {$$ = NULL;}
    ;

ARG
    : EXPR {$$ = crearNodo(NODO_EXPR, NULL, $1, NULL, NULL, NULL);}
    ;

EXPR
    : ID {$$ = crearNodo(NODO_ID, NULL, $1, NULL, NULL, NULL);}
    | METHOD_CALL {$$ = crearNodo(NODO_METHOD_CALL, NULL, $1, NULL, NULL, NULL);}
    | LITERAL {$$ = crearNodo(NODO_LITERAL, NULL, $1, NULL, NULL, NULL);} 
    | EXPR OP_SUMA EXPR {$$ = crearNodo(NODO_SUMA, NULL, $1, $3, NULL, NULL);}//ARITH_OP
    | EXPR OP_RESTA EXPR {$$ = crearNodo(NODO_RESTA, NULL, $1, $3, NULL, NULL);}//ARITH_OP
    | EXPR OP_PROD EXPR {$$ = crearNodo(NODO_PROD, NULL, $1, $3, NULL, NULL);}//ARITH_OP
    | EXPR OP_DIV EXPR {$$ = crearNodo(NODO_DIV, NULL, $1, $3, NULL, NULL);}//ARITH_OP
    | EXPR OP_DIVENT EXPR {$$ = crearNodo(NODO_DIVENT, NULL, $1, $3, NULL, NULL);}//ARITH_OP
    | EXPR OP_MENOR EXPR {$$ = crearNodo(NODO_MENOR, NULL, $1, $3, NULL, NULL);}//REL_OP
    | EXPR OP_MAYOR EXPR {$$ = crearNodo(NODO_MAYOR, NULL, $1, $3, NULL, NULL);}//REL_OP
    | EXPR OP_EQ EXPR {$$ = crearNodo(NODO_EQ, NULL, $1, $3, NULL, NULL);}//REL_OP
    | EXPR OP_AND EXPR {$$ = crearNodo(NODO_AND, NULL, $1, $3, NULL, NULL);}//COND_OP
    | EXPR OP_OR EXPR {$$ = crearNodo(NODO_OR, NULL, $1, $3, NULL, NULL);}//COND_OP
    | OP_RESTA EXPR %prec SIGNO_MENOS { $$ = crearNodo(NODO_SIGNO_MENOS, NULL, $2, NULL, NULL, NULL);}  
    | OP_NEG EXPR {$$ = crearNodo(NODO_NEG, NULL, $2, NULL, NULL, NULL);} 
    | PAR_IZQ EXPR PAR_DER {$$ = $2;} 
    ;


LITERAL
    : CTE_ENTERA {$$ = crearNodo(NODO_LITERAL, $1, NULL, NULL, NULL, NULL);} 
    | CTE_LOGICA {$$ = crearNodo(NODO_LITERAL, $1, NULL, NULL, NULL, NULL);} 
    | CTE_FLOAT {$$ = crearNodo(NODO_LITERAL, $1, NULL, NULL, NULL, NULL);} 
    ;

%%

void yyerror(const char *s) {
    printf("Error sintáctico en la línea %d: %s\n", yylineno, s);
}

int main(int argc, char **argv) {
    extern FILE *yyin;
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            printf("No se pudo abrir %s\n", argv[1]);
            return 1;
        }
    }
    if (yyparse() == 0) {
        printf("Programa aceptado\n");
    }
    return 0;
}