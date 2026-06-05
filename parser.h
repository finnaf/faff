#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef enum
{
    PAR_OK,
    PAR_LEX_ERR,
    PAR_ID_EXPECTED,
    PAR_ILLEGAL_TYPE,
    PAR_EQUAL_EXPECTED,
    PAR_OPEN_PAREN_EXPECTED,
    PAR_CLOSE_PAREN_EXPECTED,
    PAR_COLON_EXPECTED,
    PAR_INDENT_EXPECTED,
    PAR_DEDENT_EXPECTED,
    PAR_RETURN_TYPE_EXPECTED,
    PAR_ELSE_WITHOUT_IF,
    PAR_SYNTAX_ERROR
} ParError;

typedef struct
{
    ParError er;
    Token tk;
} ParserInfo;

ParError initParser (char* path);
ParserInfo parse ();
int stopParser ();
char* errorString (ParError e);


#endif