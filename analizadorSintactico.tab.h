/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_ANALIZADORSINTACTICO_TAB_H_INCLUDED
# define YY_YY_ANALIZADORSINTACTICO_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 9 "analizadorSintactico.y"

#include "ast.h"

#line 53 "analizadorSintactico.tab.h"

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    BOOLEAN = 258,                 /* BOOLEAN  */
    ELSE = 259,                    /* ELSE  */
    EXTERN = 260,                  /* EXTERN  */
    IF = 261,                      /* IF  */
    INT = 262,                     /* INT  */
    RETURN = 263,                  /* RETURN  */
    VOID = 264,                    /* VOID  */
    WHILE = 265,                   /* WHILE  */
    FLOAT = 266,                   /* FLOAT  */
    CTE_LOGICA = 267,              /* CTE_LOGICA  */
    CTE_ENTERA = 268,              /* CTE_ENTERA  */
    CTE_FLOAT = 269,               /* CTE_FLOAT  */
    ID = 270,                      /* ID  */
    OP_SUMA = 271,                 /* OP_SUMA  */
    OP_PROD = 272,                 /* OP_PROD  */
    OP_RESTA = 273,                /* OP_RESTA  */
    OP_DIV = 274,                  /* OP_DIV  */
    OP_DIVENT = 275,               /* OP_DIVENT  */
    OP_MENOR = 276,                /* OP_MENOR  */
    OP_MAYOR = 277,                /* OP_MAYOR  */
    OP_EQ = 278,                   /* OP_EQ  */
    OP_AND = 279,                  /* OP_AND  */
    OP_OR = 280,                   /* OP_OR  */
    OP_NEG = 281,                  /* OP_NEG  */
    OP_ASIG = 282,                 /* OP_ASIG  */
    PUNTO_COMA = 283,              /* PUNTO_COMA  */
    PAR_IZQ = 284,                 /* PAR_IZQ  */
    PAR_DER = 285,                 /* PAR_DER  */
    LLAVE_IZQ = 286,               /* LLAVE_IZQ  */
    LLAVE_DER = 287,               /* LLAVE_DER  */
    COMA = 288,                    /* COMA  */
    SIGNO_MENOS = 289              /* SIGNO_MENOS  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 26 "analizadorSintactico.y"

    nodoAST *nodo;
    char *str;
    

#line 110 "analizadorSintactico.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_ANALIZADORSINTACTICO_TAB_H_INCLUDED  */
