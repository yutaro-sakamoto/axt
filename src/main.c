#include <stdio.h>
#include <stdlib.h>

extern FILE *yyin;
extern int yyparse(void);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: axt <testfile.at>\n");
        return 1;
    }

    yyin = fopen(argv[1], "r");
    if (!yyin) {
        fprintf(stderr, "error: cannot open '%s'\n", argv[1]);
        return 1;
    }

    int result = yyparse();
    fclose(yyin);
    return result;
}
