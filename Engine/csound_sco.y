/*
    csound_sco.y:

    Copyright (C) 2013
    John ffitch

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/
%pure_parser
 //%parse-param {PARSE_PARM *parm}
 //%parse-param {void *scanner}
 //%lex-param { CSOUND * csound }
 //%lex-param {yyscan_t *scanner}

%token NEWLINE

%token T_ERROR

%token STRING_TOKEN
%token T_IDENT
%token NUMBER_TOKEN

%start scolines
%left S_AND S_OR
%left '|'
%left '&'
%left S_LT S_GT S_LEQ S_GEQ S_EQ S_NEQ
%left S_BITSHIFT_LEFT S_BITSHIFT_RIGHT
%left '+' '-'
%left '*' '/' '%'
%left '^'
%left '#'
%right '~'
%right S_UNOT
%right S_UMINUS
%right S_ATAT
%right S_AT
%token T_HIGHEST
%pure_parser
%error-verbose
 //%parse-param { CSOUND * csound }

/* NOTE: Perhaps should use %union feature of bison */

%{
    //#define YYSTYPE SCORE*

#ifndef NULL
#define NULL 0L
#endif
    //#include "csoundCore.h"
#include <ctype.h>
#include <string.h>

#include "csound_sco.tab.h"
    void yyerror(const char *s);
    //extern int csound_scolex(TREE**, CSOUND *, void *);
#define LINE csound_scoget_lineno()
#define LOCN csound_scoget_locn()
extern int csound_orcget_locn(void *);
extern int csound_orcget_lineno(void *);
#define csound 0
%}
%token AT
%token NP
%token PP
%token NUMBER
%token STRING
%%

scoline           : statement 
                        {

                        }
                  | '{' scolines '}'
                  {
                  }
                  ;

scolines          : scoline scolines
                        {

                        }
                  |  {  }
                  ;

statement         : op arglist NEWLINE
                  {

                  }
                  ;

op                : 'i'    { $$ = $1; }
                  | 'f'    { $$ = $1; }
                  | 'a'    { $$ = $1; }
                  | 'e'    { $$ = $1; }
                  | 's'    { $$ = $1; }
                  | 't'    { $$ = $1; }
                  ;

arglist           : arg arglist {}
                  |             {}
                  ;

arg       : NUMBER {}
          | STRING {}
          | '[' exp ']' {}
          | NP
          | PP

exp       : exp '+' exp   {  }
          | exp '+' error
          | exp '-' exp  {  }
          | exp '-' error
          | '-' exp %prec S_UMINUS
            {
            }
          | '-' error           {  }
          | '+' exp %prec S_UMINUS
            {
                $$ = $2;
            }
          | '+' error           {  }
          | term                {  }
          ;

term      : exp '*' exp    {  }
          | exp '*' error
          | exp '/' exp    {  }
          | exp '/' error
          | exp '^' exp    {  }
          | exp '^' error
          | exp '%' exp    {  }
          | exp '%' error
          | fac            {  }
          ;

fac       : constant           { $$ = $1; }
          | exp '|' exp        {  }
          | exp '|' error
          | exp '&' exp        {  }
          | exp '&' error
          | exp '#' exp        {  }
          | exp '#' error
          | exp S_BITSHIFT_LEFT exp   
                 {  }
          | exp S_BITSHIFT_LEFT error
          | exp S_BITSHIFT_RIGHT exp
                 {  }
          | exp S_BITSHIFT_RIGHT error
          | '~' exp %prec S_UMINUS
            { }
          | '~' error         { $$ = 0; }
          | '(' exp ')'      { $$ = $2; }
          | '(' exp error    { $$ = 0; }
          | '(' error         { $$ = 0; }
          ;

constant  : NUMBER_TOKEN  {  }
          | AT NUMBER_TOKEN  {  }
          | AT AT NUMBER_TOKEN  {  }
          ;

%%

#ifdef SOME_FINE_DAY
void
yyerror(char *s, ...)
{
  va_list ap;
  va_start(ap, s);

  if(yylloc.first_line)
    fprintf(stderr, "%d.%d-%d.%d: error: ",
            yylloc.first_line, yylloc.first_column,
	    yylloc.last_line, yylloc.last_column);
  vfprintf(stderr, s, ap);
  fprintf(stderr, "\n");

}

void
lyyerror(YYLTYPE t, char *s, ...)
{
  va_list ap;
  va_start(ap, s);

  if(t.first_line)
    fprintf(stderr, "%d.%d-%d.%d: error: ", t.first_line, t.first_column,
	    t.last_line, t.last_column);
  vfprintf(stderr, s, ap);
  fprintf(stderr, "\n");
}

#endif

void
yyerror(const char *s)
{
    fprintf(stderr, s);
}

int yylex(void)
{
    int c;
    while ((c=getchar())==' ');
    if (isdigit(c)) {
      int n = c-'0';
      while (isdigit(c=getchar())) {
        n = 10*n+c-'0';
      }
      ungetc(c, stdin);
      return NUMBER;
    }
    return c=='\n' ? NEWLINE : c;
}

int main(void)
{
    yydebug = 1;
    yyparse();
}
