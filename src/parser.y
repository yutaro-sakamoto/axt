%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

int yylex(void);
void yyerror(const char *s);
extern FILE *yyin;
extern void lexer_push_file(const char *filename);

/* Temporary arglist storage */
static char *arglist_args[4];
static int arglist_count;
%}

%union {
    char *str;
}

%token AT_INIT AT_SETUP AT_CHECK AT_DATA AT_CLEANUP AT_KEYWORDS M4_INCLUDE
%token <str> STRING
%token LPAREN RPAREN COMMA

%%

input
    : /* empty */
    | input statement
    ;

statement
    : AT_INIT LPAREN STRING RPAREN
        {
          if (parsed_suite) {
            free(parsed_suite->title);
            parsed_suite->title = $3;
          } else {
            parsed_suite = suite_create($3);
          }
        }
    | AT_SETUP LPAREN STRING RPAREN
        { current_case = testcase_create($3); }
    | AT_CHECK LPAREN arglist RPAREN
        {
          if (current_case) {
            TestStep *step = step_check_create(arglist_args, arglist_count);
            testcase_add_step(current_case, step);
          } else {
            for (int i = 0; i < arglist_count; i++)
              free(arglist_args[i]);
          }
        }
    | AT_DATA LPAREN STRING COMMA STRING RPAREN
        {
          if (current_case) {
            testcase_add_step(current_case, step_data_create($3, $5));
          } else {
            free($3);
            free($5);
          }
        }
    | M4_INCLUDE LPAREN STRING RPAREN
        {
          lexer_push_file($3);
          free($3);
        }
    | AT_KEYWORDS LPAREN STRING RPAREN
        { free($3); }
    | error AT_CLEANUP
        {
          /* Error recovery: skip until AT_CLEANUP and process it */
          if (parsed_suite && current_case) {
            suite_add_case(parsed_suite, current_case);
            current_case = NULL;
          }
        }
    | AT_CLEANUP
        {
          if (parsed_suite && current_case) {
            suite_add_case(parsed_suite, current_case);
            current_case = NULL;
          }
        }
    ;

arglist
    : STRING
        {
          arglist_count = 1;
          arglist_args[0] = $1;
        }
    | arglist COMMA STRING
        {
          if (arglist_count < 4) {
            arglist_args[arglist_count++] = $3;
          } else {
            free($3);
          }
        }
    | arglist COMMA
        {
          /* Empty argument (e.g., AT_CHECK([cmd], [1], , [stderr])) */
          if (arglist_count < 4) {
            arglist_args[arglist_count++] = strdup("");
          }
        }
    ;

%%

extern int yylineno;

void yyerror(const char *s) {
    fprintf(stderr, "parse error at line %d: %s\n", yylineno, s);
}
