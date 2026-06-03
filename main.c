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

    while ((tok = getNextToken()).tp != EOFile && tok.tp != ERR)
    {
        printf("%-3d  lexeme: %-20s  line: %-4d  error: %-12s  type: %s\n",
            i,
            tok.lx,
            tok.ln,
            lexErrorString(tok.ec),
            tokenTypeString(tok.tp)
        );        
        i++;
    }

    printf("Done\n");
    return EXIT_SUCCESS;
}