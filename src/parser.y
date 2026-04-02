%{
#include <stdio.h>
#include <stdlib.h>

int yylex(void);
void yyerror(const char *s);
extern FILE *yyin;
%}

%token AT_INIT AT_SETUP AT_CHECK AT_DATA AT_CLEANUP M4_INCLUDE
%token STRING
%token LPAREN RPAREN COMMA

%%

input
    : /* empty */
    | input statement
    ;

statement
    : AT_INIT LPAREN STRING RPAREN
    | AT_SETUP LPAREN STRING RPAREN
    | AT_CHECK LPAREN arglist RPAREN
    | AT_DATA LPAREN STRING COMMA STRING RPAREN
    | M4_INCLUDE LPAREN STRING RPAREN
    | AT_CLEANUP
    ;

arglist
    : STRING
    | arglist COMMA STRING
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "parse error: %s\n", s);
}
