%{
#include "ast.h"
#include "parser.h"
#include <string.h>
#include <stdio.h>

static int bracket_depth = 0;

/* Include stack for m4_include / my_include */
#define MAX_INCLUDE_DEPTH 64
static YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
static FILE *include_file_stack[MAX_INCLUDE_DEPTH];
static int include_stack_ptr = 0;

void lexer_push_file(const char *filename);
%}

%option noyywrap

%%

"#"[^\n]*           { /* comment, skip */ }
[ \t\r\n]+          { /* whitespace, skip */ }

"AT_INIT"           { return AT_INIT; }
"AT_SETUP"          { return AT_SETUP; }
"AT_CHECK"          { return AT_CHECK; }
"AT_DATA"           { return AT_DATA; }
"AT_CLEANUP"        { return AT_CLEANUP; }
"m4_include"        { return M4_INCLUDE; }
"my_include"        { return M4_INCLUDE; }

"("                 { return LPAREN; }
")"                 { return RPAREN; }
","                 { return COMMA; }

"["                 {
                      bracket_depth = 1;
                      size_t cap = 256, len = 0;
                      char *buf = (char *)malloc(cap);
                      while (bracket_depth > 0) {
                          int c = input();
                          if (c == EOF || c == 0) break;
                          /* Check for @<:@ and @:>@ escape sequences */
                          if (c == '@') {
                              int c2 = input();
                              if (c2 == '<') {
                                  int c3 = input();
                                  if (c3 == ':') {
                                      int c4 = input();
                                      if (c4 == '@') {
                                          if (len + 1 >= cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
                                          buf[len++] = '[';
                                          continue;
                                      }
                                      if (c4) unput(c4);
                                  }
                                  if (c3) unput(c3);
                              } else if (c2 == ':') {
                                  int c3 = input();
                                  if (c3 == '>') {
                                      int c4 = input();
                                      if (c4 == '@') {
                                          if (len + 1 >= cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
                                          buf[len++] = ']';
                                          continue;
                                      }
                                      if (c4) unput(c4);
                                  }
                                  if (c3) unput(c3);
                              }
                              if (c2) unput(c2);
                          }
                          if (c == '[') bracket_depth++;
                          else if (c == ']') {
                              bracket_depth--;
                              if (bracket_depth == 0) break;
                          }
                          if (len + 1 >= cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
                          buf[len++] = (char)c;
                      }
                      buf[len] = '\0';
                      yylval.str = buf;
                      return STRING;
                    }

<<EOF>>             {
                      if (include_stack_ptr > 0) {
                          include_stack_ptr--;
                          fclose(yyin);
                          yy_delete_buffer(YY_CURRENT_BUFFER);
                          yy_switch_to_buffer(include_stack[include_stack_ptr]);
                          yyin = include_file_stack[include_stack_ptr];
                      } else {
                          yyterminate();
                      }
                    }

.                   { /* skip unknown characters */ }

%%

void lexer_push_file(const char *filename) {
    if (include_stack_ptr >= MAX_INCLUDE_DEPTH) {
        fprintf(stderr, "error: include depth exceeded\n");
        return;
    }

    /* Build path: <basedir>/<basename>.src/<filename> */
    char path[4096];
    if (input_basedir && input_basedir[0]) {
        snprintf(path, sizeof(path), "%s/%s.src/%s",
                 input_basedir, input_basename, filename);
    } else {
        snprintf(path, sizeof(path), "%s.src/%s",
                 input_basename, filename);
    }

    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "error: cannot open include file '%s'\n", path);
        return;
    }

    include_stack[include_stack_ptr] = YY_CURRENT_BUFFER;
    include_file_stack[include_stack_ptr] = yyin;
    include_stack_ptr++;

    yyin = f;
    yy_switch_to_buffer(yy_create_buffer(f, YY_BUF_SIZE));
}
