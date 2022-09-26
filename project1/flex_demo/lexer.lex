%{
#include "lexer.h"
%}
%option     nounput
%option     noyywrap

DIGIT       [0-9]
INTEGER     {DIGIT}+
REAL        {DIGIT}+"."{DIGIT}*
WS          [ \t]+

%%
{WS}        /* skip blanks and tabs */
<<EOF>>     return T_EOF;
"+"         return ADD;
"-"         return SUB;
{INTEGER}|{REAL}   return NUMBER;
%%
