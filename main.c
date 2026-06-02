// Header file for input output functions
#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

int main ()
{
    printf("Initing\n");
    initLexer("examples/a.faff");

    printf("Extracting tokens.\n");
    Token tok = getNextToken();

    int i=0;

    while (tok.tp != EOFile && tok.tp != ERR)
    {
        tok = getNextToken();
        printf("%d:lexeme(%s), linenum(%d), error(%d), type(%d)\n", i, tok.lx, tok.ln, tok.ec, tok.tp);
        i++;
    }

    printf("Done\n");
        return EXIT_SUCCESS;
}