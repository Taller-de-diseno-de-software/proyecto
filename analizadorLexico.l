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
    #include "analizadorSintactico.tab.h"
%}

ALPHA_NUM     {ALPHA}|{DIGIT}|_
ALPHA         [a-zA-Z]
DIGIT         [0-9]
ID            {ALPHA}{ALPHA_NUM}*
INT_LITERAL   {DIGIT}{DIGIT}*
FLOAT_LITERAL {DIGIT}{DIGIT}*.{DIGIT}{DIGIT}*

%x COMENTARIO

%%

"//".*          {
                    printf("COMENTARIO_LINEA  %s\n", yytext);
                }

"/*"            {
                    printf("COMENTARIO_ABRE  %s\n", yytext);
                    BEGIN(COMENTARIO);
                }

<COMENTARIO>"*/"            {
                    printf("COMENTARIO_CIERRA  %s\n", yytext);
                    BEGIN(INITIAL);
                }

<COMENTARIO>\n  {
                    // Saltar de línea siguiendo en modo comentario.
                }

<COMENTARIO>.   {
                    // Ignorar cualquier otro carácter dentro del comentario.
                }

"boolean"       {
                    printf("BOOLEAN    %s\n", yytext);
                    return BOOLEAN;
                }

"else"          {
                    printf("ELSE    %s\n", yytext);
                    return ELSE;
                }

"true"          {
                    printf("CTE_LOGICA  %s\n", yytext);
                    return CTE_LOGICA;
                }

"false"         {
                    printf("CTE_LOGICA  %s\n", yytext);
                    return CTE_LOGICA;
                }

"if"            {
                    printf("IF    %s\n", yytext);
                    return IF;
                }

"int"           {
                    printf("INT    %s\n", yytext);
                    return INT;
                }

"return"        {
                    printf("RETURN    %s\n", yytext);
                    return RETURN;
                }

"void"          {
                    printf("VOID    %s\n", yytext);
                    return VOID;
                }

"while"         {
                    printf("WHILE    %s\n", yytext);
                    return WHILE;
                }

"float"         {
                    printf("FLOAT    %s\n", yytext);
                    return FLOAT;
                }

{INT_LITERAL}   {
                    printf("CTE_ENTERA  %s\n", yytext); 
                    return CTE_ENTERA;
                }

{FLOAT_LITERAL} {
                    printf("CTE_FLOAT  %s\n", yytext); 
                    return CTE_FLOAT;
                }

{ID}            {
                    printf("ID  %s\n", yytext); 
                    return ID;
                }

"+"             {
                    printf("OP_SUMA  %s\n", yytext); 
                    return OP_SUMA;
                }

"*"             {
                    printf("OP_PROD  %s\n", yytext); 
                    return OP_PROD;
                }

"-"             {
                    printf("OP_RESTA  %s\n", yytext);
                    return OP_RESTA; 
                }

"/"             {
                   printf("OP_DIV  %s\n", yytext); 
                   return OP_DIV;
                }

"%"             {
                   printf("OP_DIVENT  %s\n", yytext); 
                   return OP_DIVENT;
                }

"<"             {
                   printf("OP_MENOR  %s\n", yytext); 
                   return OP_MENOR;
                }

">"             {
                   printf("OP_MAYOR  %s\n", yytext); 
                   return OP_MAYOR;
                }

"=="            {
                   printf("OP_EQ  %s\n", yytext);
                   return OP_EQ; 
                }

"&&"            {
                   printf("OP_AND  %s\n", yytext); 
                   return OP_AND;
                }

"||"            {
                   printf("OP_OR  %s\n", yytext); 
                   return OP_OR;
                }

"("             {
                   printf("PAR_IZQ  %s\n", yytext); 
                   return PAR_IZQ;
                }

")"             {
                   printf("PAR_DER  %s\n", yytext); 
                   return PAR_DER;
                }

"{"             {
                   printf("LLAVE_IZQ  %s\n", yytext); 
                   return LLAVE_IZQ;
                }

"}"             {
                   printf("LLAVE_DER  %s\n", yytext); 
                   return LLAVE_DER;
                }

"!"             {
                   printf("OP_NEG  %s\n", yytext);
                   return OP_NEG;
                }

";"             {
                   printf("PUNTO_COMA  %s\n", yytext); 
                   return PUNTO_COMA;
                }

","             {
                   printf("COMA  %s\n", yytext); 
                   return COMA;
                }

"="             {
                    printf("OP_ASIG  %s\n", yytext);
                    return OP_ASIG;
                }
                
[ \t\r\n]+  { }

.           { printf("ERROR linea %d: caracter no reconocido '%s'\n", yylineno, yytext); }

%%