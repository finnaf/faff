#ifndef LEXER_H
#define LEXER_H

#define LEX_SIZE 128
#define KEYWORD_COUNT 12
#define SYMBOL_COUNT 14

typedef enum {
    KEYWORD, // reserved words
    IDENTIFIER,
    INT,
    SYMBOL,
    STRING,
    EOFile,
    INDENT,
    DEDENT,
    ERR,
    NONE
} TokenType;

typedef enum {
    NoLexError,
    EofInComment,
    NewLineInStr,
    EofInStr,
    IllegalSym,
    TabError
} LexError;

typedef struct {
    TokenType tp;
    char lx[LEX_SIZE];
    LexError ec;
    int ln;
} Token;

int initLexer (char* file);
Token getNextToken ();
Token peekNextToken ();
int stopLexer ();

const char *tokenTypeString(TokenType tp);
const char *lexErrorString(LexError err);

#endif