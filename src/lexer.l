%{
#include "parser.h"
#include <string.h>

static int bracket_depth = 0;
%}

%option noyywrap nounput

%%

"#"[^\n]*           { /* comment, skip */ }
[ \t\r\n]+          { /* whitespace, skip */ }

"AT_INIT"           { return AT_INIT; }
"AT_SETUP"          { return AT_SETUP; }
"AT_CHECK"          { return AT_CHECK; }
"AT_DATA"           { return AT_DATA; }
"AT_CLEANUP"        { return AT_CLEANUP; }
"m4_include"        { return M4_INCLUDE; }

"("                 { return LPAREN; }
")"                 { return RPAREN; }
","                 { return COMMA; }

"["                 { bracket_depth = 1;
                      while (bracket_depth > 0) {
                          int c = input();
                          if (c == EOF) break;
                          if (c == '[') bracket_depth++;
                          else if (c == ']') bracket_depth--;
                      }
                      return STRING;
                    }

.                   { /* skip unknown characters */ }

%%
