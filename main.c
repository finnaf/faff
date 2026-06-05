// Header file for input output functions
#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

int main ()
{
    printf("Initing\n");

    int r = initLexer("examples/a.faff");
    if (r != LEX_OK)
    {
        printf("Error during lexer initialisation: %s\n", lexStatusString(r));

        Token tok = getNextToken();
        printf("lexeme: %-20s  line: %-4d  error: %-12s  type: %s\n",
            tok.lx,
            tok.ln,
            lexStatusString(tok.ec),
            tokenTypeString(tok.tp)
        );
    }

    printf("Extracting tokens.\n");
    Token tok = getNextToken();

    int i=0;

    while (tok.tp != EOFile && tok.tp != ERR && tok.tp != NONE)
    {
        printf("%-3d  lexeme: %-20s  line: %-4d  error: %-12s  type: %s\n",
            i,
            tok.lx,
            tok.ln,
            lexStatusString(tok.ec),
            tokenTypeString(tok.tp)
        );        
        i++;
    }

    printf("Done\n");
    return EXIT_SUCCESS;
}