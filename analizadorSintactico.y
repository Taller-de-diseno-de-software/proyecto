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

%token BOOLEAN ELSE IF INT RETURN VOID WHILE FLOAT  //Palabras reservadas
%token CTE_LOGICA CTE_ENTERA CTE_FLOAT ID   //Constantes e ID
%token OP_SUMA OP_PROD OP_RESTA OP_DIV OP_DIVENT OP_MENOR OP_MAYOR OP_EQ OP_AND OP_OR OP_NEG OP_ASIG   //Operaciones
%token PUNTO_COMA PAR_IZQ PAR_DER LLAVE_IZQ LLAVE_DER COMA  //Caracteres 

%left OP_OR
%left OP_AND
%left OP_EQ
%left OP_MENOR OP_MAYOR
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

DECL_PRIMA 
    : ID_PRIMA PUNTO_COMA
    | PAR_IZQ PARAMS PAR_DER BLOQUE
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
    : LLAVE_IZQ VAR_DECL STATEMENTS LLAVE_DER
    ;

VAR_DECL
    : VAR VAR_DECL
    | 
    ;

STATEMENTS
    : STATEMENT STATEMENTS
    | 
    ;

TYPE
    : INT
    | FLOAT
    | BOOLEAN
    ;

STATEMENT 
    : ID OP_ASIG EXPR PUNTO_COMA
    | METHOD_CALL PUNTO_COMA
    | IF PAR_IZQ EXPR PAR_DER BLOQUE OPTIONAL_ELSE
    | WHILE PAR_IZQ EXPR PAR_DER BLOQUE
    | RETURN OPTIONAL_EXPR PUNTO_COMA
    | PUNTO_COMA
    | BLOQUE
    ;

OPTIONAL_ELSE
    : ELSE BLOQUE
    |
    ;

OPTIONAL_EXPR
    : EXPR
    |
    ;

METHOD_CALL 
    : ID PAR_IZQ ARGS PAR_DER
    ;

ARGS
    : ARG ARGS_PRIMA
    ;

ARGS_PRIMA
    : COMA ARGS
    | 
    ;

ARG
    : EXPR
    ;

EXPR
    : ID
    | METHOD_CALL
    | LITERAL
    | EXPR OP_SUMA EXPR //ARITH_OP
    | EXPR OP_RESTA EXPR //ARITH_OP
    | EXPR OP_PROD EXPR //ARITH_OP
    | EXPR OP_DIV EXPR //ARITH_OP
    | EXPR OP_DIVENT EXPR //ARITH_OP
    | EXPR OP_MENOR EXPR //REL_OP
    | EXPR OP_MAYOR EXPR //REL_OP
    | EXPR OP_EQ EXPR //REL_OP
    | EXPR OP_AND EXPR //COND_OP
    | EXPR OP_OR EXPR //COND_OP
    | OP_RESTA EXPR %prec SIGNO_MENOS  
    | OP_NEG EXPR
    | PAR_IZQ EXPR PAR_DER
    ;


LITERAL
    : CTE_ENTERA
    | CTE_LOGICA
    | CTE_FLOAT
    ;

%%

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