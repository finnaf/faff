#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

#define BUF_SIZE 1024

char buf[BUF_SIZE];
char* b_ptr;

char filename[LEX_SIZE];
unsigned long file_pos;
size_t num_bytes_read;

Token t;
Token* t_ptr;
Token* tokens;


Token getClearToken ()
{
    Token tok;

    tok.tp = NONE;
    strcpy(tok.lx, "");
    tok.ec = NoLexError;
    tok.ln = -1;

    return tok;
}

// returns 0 on success
int readFileChunk()
{
    FILE* file = fopen(filename, "r");
    if (!file)
    {
        printf("Could not open file %s\n", filename);
        return 0;
    }

    memset(buf, 0, sizeof(buf));
    fseek(file, file_pos, SEEK_SET);

    num_bytes_read = fread(buf, 1, BUF_SIZE-1, file);
    buf[num_bytes_read] = '\0';
    file_pos += num_bytes_read;

    return 0;
}

// return 1 on new chunk read, else 0
// does not increment safely
int incrementPtr ()
{
    if (b_ptr >= buf + BUF_SIZE - 2)
    {
        readFileChunk();
        b_ptr = buf;
        return 1;
    }

    b_ptr++;
    return 0;
}

// only called when can to go back to a previous full buffer, or doesnt need to
// return 1 on / needing return, 2 on read error, else 0
int decrementPtr ()
{
    if (b_ptr > buf)
    {
        --b_ptr;
        return 0;
    }

    // return to previous buffer
    if (file_pos < BUF_SIZE)
    {
        printf("INTERNAL LOGIC ERROR\n"); // should never get here
        file_pos = 0;
        return 2;
    }
    else
        file_pos -= (BUF_SIZE + num_bytes_read-1);

    if (!readFileChunk())
        return 2;

    b_ptr = buf+(BUF_SIZE-2); // go to 1023rd char in buf
    return 1;
}

bool isKeyword (char* str)
{
    for (int i = 0; i < KEYWORD_COUNT; i++)
        if (!strcmp(KEYWORDS[i], str)) // are identical
            return true;
    return false;
}

bool isSymbol (char str)
{
    for (int i = 0; i < SYMBOL_COUNT; i++)
        if (SYMBOLS[i] == str)
            return true;
    return false;
}

int isEOF ()
{
    if (*b_ptr == '\0')
    {
        t.ec = EofInComment;
        strcpy(t.lx, "Error: unexpected EOF");
        t.tp = ERR;
        return 1;
    }

    return 0;
}

/*
Find the end of *multiline comments* /like these/
Returns 2 on EOF, 1 on end, else 0 & increments
*/
int findMultiCommentEnd ()
{
    if (isEOF())
        return 2;
    if (*b_ptr == '\n')
        t.ln++;

    // is a candidate for end
    if (*b_ptr == '*')
    {
        while (*b_ptr == '*')
        {
            incrementPtr();
            if (*b_ptr == '/')
                return 1;
        }
    }
    else
        incrementPtr(); // regular comment
    
    return 0;
}

// skips all comments and newlines
int skipComments ()
{
    while (*b_ptr != '\0' && *b_ptr == '\n')
    {
        t.ln++;
        incrementPtr();
    }

    if (*b_ptr == '/')
    {
        incrementPtr();

        switch (*b_ptr)
        {
            // handle basic comment
            case '/':
                while (*b_ptr != '\0' && *b_ptr != '\n')
                {
                    incrementPtr();
                    if (*b_ptr == '\n')
                        t.ln++;
                }
                break;

            /* handle */ /***** and this */
            case '*':
            {
                int flag = 0;
                while (flag == 0)
                {
                    flag = findMultiCommentEnd();

                    if (flag) // positive flag: error or end
                        return *b_ptr;
                }
                break;
            }

            // is just an operator /
            default:
                decrementPtr();
                return *b_ptr;

        }

        incrementPtr();

        if (*b_ptr == '\n' || *b_ptr == '/')
            return skipComments();
    }

    return *b_ptr;
}

// return 1 on error, else 0
int buildToken ()
{
    int c = skipComments();

    if (*b_ptr == '\0')
    {
        if (t.ec == EofInComment)
            return 1;
        
        t.lx[0] = '\0';
        t.tp = EOFile;
        return 0;
    }

    int i = 0;
    char temp[LEX_SIZE] = {0};

    // handle string literal
    if (*b_ptr == '"')
    {
        incrementPtr();
        while (*b_ptr != '"')
        {
            if (*b_ptr == '\n')
            {
                t.tp = ERR;
                t.ec = NewLineInStr;
                strcpy(t.lx, "Error: newline in string");
                return 1;
            }
            if (*b_ptr == '\0')
            {
                t.tp = ERR;
                t.ec = EofInStr;
                strcpy(t.lx, "Error: unexpected eof in string");
                return 1;
            }
            if (i < (LEX_SIZE-2))
                temp[i++] = *b_ptr;
            incrementPtr(); 
        }

        incrementPtr(); // skip final "

        t.tp = STRING;
        temp[i] = '\0';
        strcpy(t.lx, temp);
        return 0;
    }


    // keywords and identifiers
    if (isalpha(*b_ptr) || *b_ptr == '_')
    {
        while (isalpha(*b_ptr) || isdigit(*b_ptr) || *b_ptr == '_')
        {
            if (i < (LEX_SIZE-2))
                temp[i++] = *b_ptr;
            incrementPtr();
        }

        temp[i] = '\0';
        strcpy(t.lx, temp);

        if (isKeyword(temp))
            t.tp = KEYWORD;
        else
            t.tp = IDENTIFIER;
        return 0;
    }

    // ints
    else if (isdigit(*b_ptr))
    {
        while(isdigit(*b_ptr))
        {
            if (i < (LEX_SIZE-2))
                temp[i++] = *b_ptr;
            incrementPtr();
        }

        temp[i] = '\0';
        strcpy(t.lx, temp);
        t.tp = INT;
        return 0;
    }

    // symbols
    else
    {
        if (!isSymbol(*b_ptr))
        {
            strcpy(t.lx, "Error: illegal symbol in source file");
            t.tp = ERR;
            t.ec = IllegalSym;
            return 1;
        }

        t.tp = SYMBOL;
        t.lx[0] = *b_ptr;
        t.lx[1] = '\0';
        incrementPtr();
        return 0;
    }
}

// adds global token to tokens array
// doubles memory if required
int addToken (int* count, int* capacity)
{
    if (*count >= *capacity)
    {
        *capacity *= 2;
        Token* tmp = realloc(tokens, *capacity * sizeof(Token));
        if (tmp == NULL)
            return 1;
        
        tokens = tmp;
    }

    tokens[*count] = t;
    (*count)++;

    return 0;
}

// returns 1 on error, else 0
int loadTokens ()
{
    t = getClearToken();
    int token_count = 0;
    int capacity = 64;

    tokens = malloc(capacity * sizeof(Token));
    if (!tokens)
        return 1;

    while (t.tp != EOFile && t.tp != ERR)
    {
        if (buildToken())
        {
            printf("(%c) %s\n", *b_ptr, t.lx);
            addToken(&token_count, &capacity);
            return 1;
        }
        if (addToken(&token_count, &capacity))
            return 1;
    }

    return 0;
}


int initLexer (char* file_name)
{
    file_pos = 0;
    b_ptr = buf;

    strncpy(filename, file_name, sizeof(filename)-1);
    filename[sizeof(filename)-1] = '\0';

    if (readFileChunk())
    {
        printf("Could not read initial file chunk\n");
        return 0;
    }

    if (loadTokens())
    {
        printf("Could not load tokens\n");
        t_ptr = tokens;
        return 0;
    }

    t_ptr = tokens;
    return 1;
}

// delete if not needed
Token getEndToken ()
{
    Token tok = getClearToken();
    tok.tp = EOFile;

    return tok;
}

Token getNextToken ()
{
    if (tokens == NULL)
        return getClearToken();
    if (t_ptr->tp == EOFile || t_ptr->tp == ERR)
        return *t_ptr;
    return *t_ptr++;
}

Token peekNextToken ()
{
    if (tokens == NULL)
        return getClearToken();
    return *t_ptr;
}

int stopLexer ()
{
    if (tokens)
        free(tokens);
    
    tokens = NULL;
    t_ptr = NULL;
    return 0;
}

