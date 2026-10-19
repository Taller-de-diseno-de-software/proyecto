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
    #include <string.h>
    #include "analizadorSintactico.tab.h"

    /* Definidos en main.c. archivoTokens es el destino del listado de tokens (el .lex con
       -target scan, stdout con -debug); si es NULL el lexer no imprime nada. */
    extern FILE *archivoTokens;
    extern int huboErrorLexico;

    #define LOG_TOKEN(...) \
        do { if (archivoTokens) fprintf(archivoTokens, __VA_ARGS__); } while (0)
%}

ALPHA_NUM     {ALPHA}|{DIGIT}|_
ALPHA         [a-zA-Z]
DIGIT         [0-9]
ID            {ALPHA}{ALPHA_NUM}*
INT_LITERAL   {DIGIT}{DIGIT}*
FLOAT_LITERAL {DIGIT}+\.{DIGIT}+

%x COMENTARIO

%%

"//".*          {
                    LOG_TOKEN("COMENTARIO_LINEA  %s\n", yytext);
                }

"/*"            {
                    LOG_TOKEN("COMENTARIO_ABRE  %s\n", yytext);
                    BEGIN(COMENTARIO);
                }

<COMENTARIO>"*/"            {
                    LOG_TOKEN("COMENTARIO_CIERRA  %s\n", yytext);
                    BEGIN(INITIAL);
                }

<COMENTARIO>\n  {
                    // Saltar de línea siguiendo en modo comentario.
                }

<COMENTARIO>.   {
                    // Ignorar cualquier otro carácter dentro del comentario.
                }

"boolean"       {
                    LOG_TOKEN("BOOLEAN    %s\n", yytext);
                    yylval.str = strdup(yytext);
                    return BOOLEAN;
                }

"extern"        {
                    LOG_TOKEN("EXTERN    %s\n", yytext);
                    yylval.str = strdup(yytext);
                    return EXTERN;
                }

"else"          {
                    LOG_TOKEN("ELSE    %s\n", yytext);
                    yylval.str = strdup(yytext);
                    return ELSE;
                }

"true"          {
                    LOG_TOKEN("CTE_LOGICA  %s\n", yytext);
                    yylval.str = strdup(yytext);
                    return CTE_LOGICA;
                }

"false"         {
                    LOG_TOKEN("CTE_LOGICA  %s\n", yytext);
                    yylval.str = strdup(yytext);
                    return CTE_LOGICA;
                }

"if"            {
                    LOG_TOKEN("IF    %s\n", yytext);
                    yylval.str = strdup(yytext);
                    return IF;
                }

"int"           {
                    LOG_TOKEN("INT    %s\n", yytext);
                    yylval.str = strdup(yytext);
                    return INT;
                }

"return"        {
                    LOG_TOKEN("RETURN    %s\n", yytext);
                    yylval.str = strdup(yytext);
                    return RETURN;
                }

"void"          {
                    LOG_TOKEN("VOID    %s\n", yytext);
                    yylval.str = strdup(yytext);
                    return VOID;
                }

"while"         {
                    LOG_TOKEN("WHILE    %s\n", yytext);
                    yylval.str = strdup(yytext);
                    return WHILE;
                }

"float"         {
                    LOG_TOKEN("FLOAT    %s\n", yytext);
                    yylval.str = strdup(yytext);
                    return FLOAT;
                }

{INT_LITERAL}   {
                    LOG_TOKEN("CTE_ENTERA  %s\n", yytext);
                    yylval.str = strdup(yytext); 
                    return CTE_ENTERA;
                }

{FLOAT_LITERAL} {
                    LOG_TOKEN("CTE_FLOAT  %s\n", yytext);
                    yylval.str = strdup(yytext);
                    return CTE_FLOAT;
                }

{ID}            {
                    LOG_TOKEN("ID  %s\n", yytext);
                    yylval.str = strdup(yytext);
                    return ID;
                }

"+"             {
                    LOG_TOKEN("OP_SUMA  %s\n", yytext); 
                    yylval.str = strdup(yytext);
                    return OP_SUMA;
                }

"*"             {
                    LOG_TOKEN("OP_PROD  %s\n", yytext); 
                    yylval.str = strdup(yytext);
                    return OP_PROD;
                }

"-"             {
                    LOG_TOKEN("OP_RESTA  %s\n", yytext);
                    yylval.str = strdup(yytext);
                    return OP_RESTA; 
                }

"/"             {
                   LOG_TOKEN("OP_DIV  %s\n", yytext); 
                   yylval.str = strdup(yytext);
                   return OP_DIV;
                }

"%"             {
                   LOG_TOKEN("OP_DIVENT  %s\n", yytext); 
                   yylval.str = strdup(yytext);
                   return OP_DIVENT;
                }

"<"             {
                   LOG_TOKEN("OP_MENOR  %s\n", yytext); 
                   yylval.str = strdup(yytext);
                   return OP_MENOR;
                }

">"             {
                   LOG_TOKEN("OP_MAYOR  %s\n", yytext); 
                   yylval.str = strdup(yytext);
                   return OP_MAYOR;
                }

"=="            {
                   LOG_TOKEN("OP_EQ  %s\n", yytext);
                   yylval.str = strdup(yytext);
                   return OP_EQ; 
                }

"&&"            {
                   LOG_TOKEN("OP_AND  %s\n", yytext); 
                   yylval.str = strdup(yytext);
                   return OP_AND;
                }

"||"            {
                   LOG_TOKEN("OP_OR  %s\n", yytext); 
                   yylval.str = strdup(yytext);
                   return OP_OR;
                }

"("             {
                   LOG_TOKEN("PAR_IZQ  %s\n", yytext); 
                   yylval.str = strdup(yytext);
                   return PAR_IZQ;
                }

")"             {
                   LOG_TOKEN("PAR_DER  %s\n", yytext); 
                   yylval.str = strdup(yytext);
                   return PAR_DER;
                }

"{"             {
                   LOG_TOKEN("LLAVE_IZQ  %s\n", yytext); 
                   yylval.str = strdup(yytext);
                   return LLAVE_IZQ;
                }

"}"             {
                   LOG_TOKEN("LLAVE_DER  %s\n", yytext); 
                   yylval.str = strdup(yytext);
                   return LLAVE_DER;
                }

"!"             {
                   LOG_TOKEN("OP_NEG  %s\n", yytext);
                   yylval.str = strdup(yytext);
                   return OP_NEG;
                }

";"             {
                   LOG_TOKEN("PUNTO_COMA  %s\n", yytext); 
                   yylval.str = strdup(yytext);
                   return PUNTO_COMA;
                }

","             {
                   LOG_TOKEN("COMA  %s\n", yytext); 
                   yylval.str = strdup(yytext);
                   return COMA;
                }

"="             {
                    LOG_TOKEN("OP_ASIG  %s\n", yytext);
                    yylval.str = strdup(yytext);
                    return OP_ASIG;
                }
                
[ \t\r\n]+  { }

.           {
                fprintf(stderr, "Error léxico en la línea %d: caracter no reconocido '%s'\n", yylineno, yytext);
                huboErrorLexico = 1;
            }

%%