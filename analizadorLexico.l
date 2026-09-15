/*
 * Analizador léxico desarrollado con Flex.
 *
 * Reconoce palabras reservadas, identificadores, constantes,
 * operadores y delimitadores y agrupa estas cadenas en TOKENS
 *
 */

%option noyywrap
%option yylineno

%{
    #include <stdio.h>
%}

ALPHA_NUM     {ALPHA}|{DIGIT}|_
ALPHA         [a-zA-Z]
DIGIT         [0-9]
ID            {ALPHA}{ALPHA_NUM}*
INT_LITERAL   {DIGIT}{DIGIT}*
FLOAT_LITERAL {DIGIT}{DIGIT}*.{DIGIT}{DIGIT}*

%%

"boolean"       {
                    printf("BOOLEAN    %s\n", yytext);
                }

"else"          {
                    printf("ELSE    %s\n", yytext);
                }

"true"          {
                    printf("CTE_LOGICA"  %s\n", yytext); 
                }

"false"         {
                    printf("CTE_LOGICA"  %s\n", yytext); 
                }

"if"            {
                    printf("IF    %s\n", yytext);
                }

"int"           {
                    printf("INT    %s\n", yytext);
                }

"return"        {
                    printf("RETURN    %s\n", yytext);
                }

"void"          {
                    printf("VOID    %s\n", yytext);
                }

"while"         {
                    printf("WHILE    %s\n", yytext);
                }

"float"         {
                    printf("FLOAT    %s\n", yytext);
                }

{INT_LITERAL}   {
                    printf("CTE_ENTERA  %s\n", yytext); 
                }

{FLOAT_LITERAL} {
                    printf("CTE_FLOAT  %s\n", yytext); 
                }

{ID}            {
                    printf("ID  %s\n", yytext); 
                }

"+"             {
                    printf("OP_SUMA  %s\n", yytext); 
                }

"*"             {
                    printf("OP_PROD  %s\n", yytext); 
                }

"-"             {
                    printf("OP_RESTA  %s\n", yytext); 
                }

"/"             {
                   printf("OP_DIV  %s\n", yytext); 
                }

"%"             {
                   printf("OP_DIVENT  %s\n", yytext); 
                }

"<"             {
                   printf("OP_MENOR  %s\n", yytext); 
                }

">"             {
                   printf("OP_MAYOR  %s\n", yytext); 
                }

"=="            {
                   printf("OP_EQ  %s\n", yytext); 
                }

"&&"            {
                   printf("OP_AND  %s\n", yytext); 
                }

"||"            {
                   printf("OP_OR  %s\n", yytext); 
                }

"("             {
                   printf("PAR_IZQ  %s\n", yytext); 
                }

")"             {
                   printf("PAR_DER  %s\n", yytext); 
                }

"{"             {
                   printf("LLAVE_IZQ  %s\n", yytext); 
                }

"}"             {
                   printf("LLAVE_DER  %s\n", yytext); 
                }

"!"             {
                   printf("OP_NEG  %s\n", yytext); 
                }

";"             {
                   printf("PUNTO_COMA  %s\n", yytext); 
                }

","             {
                   printf("COMA  %s\n", yytext); 
                }

"="             {
                    printf("OP_ASIG  %s\n", yytext);
}
[ \t\r\n]+  { }

.           { printf("ERROR linea %d: caracter no reconocido '%s'\n", yylineno, yytext); }

%%