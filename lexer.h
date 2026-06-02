#ifndef LEXER_H
#define LEXER_H

#define LEX_SIZE 128
#define KEYWORD_COUNT 12
#define SYMBOL_COUNT 14

static const char* KEYWORDS[KEYWORD_COUNT] = {
    "character", "integer", "floating-point", "double", "boolean",
    "void", "if", "else", "while", "return", "true", "false"
};

static const char SYMBOLS[SYMBOL_COUNT] = {
    '(', ')', '[', ']', ',', 
    '=', '.', '+', '-', '*', '/', '<', '>'
};

typedef enum {
    KEYWORD, // reserved words
    IDENTIFIER,
    INT,
    SYMBOL,
    STRING,
    EOFile,
    ERR,
    NONE
} TokenType;

typedef enum {
    EofInComment,
    NewLineInStr,
    EofInStr,
    IllegalSym,
    NoLexError
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

#endif