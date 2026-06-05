#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "lexer.h"
#include "parser.h"

ParserInfo parseFile ();
ParserInfo parseType ();
ParserInfo parseFunction ();
ParserInfo parseParameterList ();
ParserInfo parseFunctionBody ();


ParserInfo getClearPI ()
{
    ParserInfo pi;
    Token t = getClearToken();

    pi.er = PAR_OK;
    pi.tk = t;

    return pi;
}

bool isLexErr (Token t, ParserInfo* pi)
{
    if (t.ec != LEX_OK)
    {
        pi->er = PAR_LEX_ERR;
        pi->tk = t;
        return true;
    }

    return false;
}


ParError initParser (char* path)
{
    if (initLexer(path) != LEX_OK)
        return PAR_LEX_ERR;

    return PAR_OK;
}

ParserInfo parse ()
{
    ParserInfo pi = parseFile();
    return pi;
}

ParserInfo parseFile ()
{
    // any number of function
    ParserInfo pi = getClearPI();
    Token t;

    t = peekNextToken();
    if (isLexErr(t, &pi)) return pi;

    while (t.tp != EOFile)
    {
        pi = parseFunction();
        if (pi.er != PAR_OK) return pi;

        t = peekNextToken();
        if (isLexErr(t, &pi)) return pi;
    }

    return pi;
}

ParserInfo parseType ()
{
    // integer | character | floating | double | boolean
    ParserInfo pi = getClearPI();
    Token t;

    t = getNextToken();
    if (isLexErr(t, &pi)) return pi;

    if (!strcmp(t.lx, "integer"))
		;
	else if (!strcmp(t.lx, "character"))
		;
    else if (!strcmp(t.lx, "floating"))
		;
    else if (!strcmp(t.lx, "double"))
		;
	else if (!strcmp(t.lx, "boolean"))
		;
	else if (t.tp == ID)
	{
        // is some identifier (struct)
        // TODO symbol table
    }
    else
    {
        pi.er = PAR_ILLEGAL_TYPE;
        pi.tk = t;
        return pi;
    }

    return pi;
}

ParserInfo parseFunction()
{
    // (type | void) identifier ( parameterList ) :
    //      functionBody
    //
    ParserInfo pi = getClearPI();
    Token t;

    t = peekNextToken();
    if (isLexErr(t, &pi)) return pi;

    if ((!strcmp(t.lx, "integer") || 
		 !strcmp(t.lx, "character")) || 
		(!strcmp(t.lx, "floating")) ||
        (!strcmp(t.lx, "double")) ||
        (!strcmp(t.lx, "boolean")) || 
		(t.tp == ID))
	{
		pi = parseType(); // consumes token
		if (pi.er != none) { return pi; }
	}
	else if (!strcmp(t.lx, "void"))
	{
		getNextToken(); // consume token
	}
    else
    {
        pi.er = PAR_ILLEGAL_TYPE;
        pi.tk = t;
        return pi;
    }

    // identifier
    t = getNextToken();
    if (isLexErr(t, &pi)) return pi;

    if (t.tp == IDENTIFIER)
    {
        // check symboltable
    }
    else
    {
        pi.er = PAR_ID_EXPECTED;
        pi.tk = t;
        return pi;
    }

    // is valid (can create a new scope) TODO

    // TODO code generation


    // ( parameterList )
    t = getNextToken();
    if (isLexErr(t, &pi)) return pi;

    if (!strcmp(t.lx, "("))
        ;
    else
    {
        pi.er = PAR_OPEN_PAREN_EXPECTED;
        pi.tk = t;
        return pi;
    }

    pi = parseParameterList();
    if (pi.er != PAR_OK) return pi;

    t = getNextToken();
    if (isLexErr(t, &pi)) return pi

    if (!strcmp(t.lx, ")"))
        ;
    else
    {
        pi.er = PAR_CLOSE_PAREN_EXPECTED;
        pi.tk = t;
        return pi;
    }

    if (!strcmp(t.lx, ":"))
        ;
    else
    {
        pi.er = PAR_COLON_EXPECTED;
        pi.tk = t;
        return pi;
    }

    t = getNextToken();
    if (isLexErr(t, &pi)) return pi

    if (t.tp == INDENT)
        ;
    else
    {
        pi.er = PAR_INDENT_EXPECTED;
        pi.tk = t;
        return pi;
    }

    pi = parseFunctionBody();
    if (pi.er != PAR_OK) return pi;

    t = getNextToken();
    if (isLexErr(t, &pi)) return pi

    if (t.tp == DEDENT)
        ;
    else
    {
        pi.er = PAR_DEDENT_EXPECTED;
        pi.tk = t;
        return pi;
    }

    return pi;
}

ParserInfo parseParameterList ()
{
    // type identifer {, type identifer} | epsilon
    ParserInfo pi = getClearPI();
	Token t;

	Token typ = peekNextToken();
    if (isLexErr(t, &pi)) return pi

    if ((!strcmp(t.lx, "integer") || 
		 !strcmp(t.lx, "character")) || 
		(!strcmp(t.lx, "floating")) ||
        (!strcmp(t.lx, "double")) ||
        (!strcmp(t.lx, "boolean")) || 
		(t.tp == ID))
	{
		pi = parseType(); // consumes token
		if (pi.er != none) { return pi; }
	}
    else
    {
        // epsilon
        return pi;
    }

    t = getNextToken();
    if (isLexErr(t, &pi)) return pi;

    if (t.tp == IDENTIFIER)
    {
        // set symbol
    }
    else
    {
        pi.er = PAR_ID_EXPECTED;
        pi.tk = t;
        return pi;
    }

    // zero or more ", type identifiers"
    t = peekNextToken();
    if (isLexErr(t, &pi)) return pi;

    while (t.tp != EOFile && !strcmp(t.lx, ","))
    {
        getNextToken(); // consume

        pi = parseType();
        if (pi.er != PAR_OK) return pi;

        t = getNextToken();
        if (isLexErr(t, &pi)) return pi;

        if (t.tp == IDENTIFIER)
        {
            // find symbol TODO
        }
        else
        {
            pi.er = PAR_ID_EXPECTED;
            pi.tk = t;
            return pi;
        }

        t = peekNextToken();
        if (isLexErr(t, &pi)) return pi;
    }

    return pi;
}

ParserInfo parseFunctionBody ()
{
    ParserInfo pi = getClearPI();
    Token t;

    t = peekNextToken();
    if (isLexErr(t, &pi)) return pi;

    while (t.tp != EOFile && strcmp(t.lx, ""))

    while
}