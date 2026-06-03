#ifndef LEXER_H
#define LEXER_H

#define LEX_SIZE 128
#define KEYWORD_COUNT 12
#define SYMBOL_COUNT 15

static const char* KEYWORDS[KEYWORD_COUNT] = {
    "character", "integer", "floating-point", "double", "boolean",
    "void", "if", "else", "while", "return", "true", "false"
};

static const char SYMBOLS[SYMBOL_COUNT] = {
    '(', ')', '[', ']', ',', ':',
    '=', '.', '+', '-', '*', '/', '<', '>'
};

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

#endif