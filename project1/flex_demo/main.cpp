#include <iostream>
#include <cstdio>
#include "lexer.h"
using namespace std;

int yylex();
extern "C" FILE *yyin;
extern "C" char *yytext;

int main(int argc, char **argv)
{
    if (argc > 1){
        yyin = fopen(argv[1], "r");
    } else {
        yyin = stdin;
    }

    while (true){
        int n = yylex();
        if (n == T_EOF){
            break;
        }
        cout << yytext << endl;
    }
    
    return 0;
}
