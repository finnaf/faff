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
    LEX_OK,
    //
    LEX_ERR_COMMENT_EOF,
    LEX_ERR_STR_NEWLINE,
    LEX_ERR_STR_EOF,
    LEX_ERR_ILLEGAL_SYM,
    LEX_ERR_TAB,
    //
    LEX_ERR_FILE_NOT_FOUND,
    LEX_ERR_FILE_READ,
    LEX_ERR_MALLOC,
    // todo 
    LEX_ERR
} LexStatus;

typedef struct {
    TokenType tp;
    char lx[LEX_SIZE];
    LexStatus ec;
    int ln;
} Token;

int initLexer (char* file);
Token getNextToken ();
Token peekNextToken ();
int stopLexer ();

const char *tokenTypeString(TokenType tp);
const char *lexStatusString(LexStatus err);

#endif