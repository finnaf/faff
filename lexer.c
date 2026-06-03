#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

#define BUF_SIZE 1024
#define INDENT_STACK_SIZE 256
#define TAB_WIDTH 4

static char buf[BUF_SIZE];
static char* b_ptr;

static char filename[LEX_SIZE];
static unsigned long file_pos;
static size_t num_bytes_read;

static Token t;
static Token* t_ptr;
static Token* tokens;

static int indent_stack[INDENT_STACK_SIZE];
static int stack_top = 0;

// helpers

static Token getClearToken ()
{
    Token tok;

    tok.tp = NONE;
    strcpy(tok.lx, "");
    tok.ec = NoLexError;
    tok.ln = 0; // 1-based (source is obvious)

    return tok;
}

static void clearIdentStack()
{
    stack_top = 0;
    memset(indent_stack, 0, sizeof(indent_stack));
}

// buffer management

// returns 0 on success, else 1
static int readFileChunk()
{
    FILE* file = fopen(filename, "r");
    if (!file)
    {
        printf("Could not open file %s\n", filename);
        return 1;
    }

    memset(buf, 0, sizeof(buf));
    fseek(file, file_pos, SEEK_SET);
    num_bytes_read = fread(buf, 1, BUF_SIZE-1, file);
    fclose(file);

    buf[num_bytes_read] = '\0';
    file_pos += num_bytes_read;

    return 0;
}

// return 1 on new chunk read, else 0
// does not increment safely
static int incrementPtr ()
{
    if (b_ptr >= buf + BUF_SIZE - 2)
    {
        if (readFileChunk())
            return 0;
        
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

// character class helpers

static bool isKeyword (char* str)
{
    for (int i = 0; i < KEYWORD_COUNT; i++)
        if (!strcmp(KEYWORDS[i], str)) // are identical
            return true;
    return false;
}

static bool isSymbol (char str)
{
    for (int i = 0; i < SYMBOL_COUNT; i++)
        if (SYMBOLS[i] == str)
            return true;
    return false;
}

/*
Skip the body of a block comment (after opening /* is consumed)
Returns 2 on unexpected EOF, 1 on end, else 0 & increments
*/
static int findMultiCommentEnd ()
{
    while (*b_ptr != '\0')
    {
        if (*b_ptr == '\n')
            t.ln++;

        if (*b_ptr == '*')
        {
            incrementPtr();
            if (*b_ptr == '/')
            {
                incrementPtr();
                return 1;
            }

            continue;
        }

        incrementPtr();
    }

    t.ec = EofInComment;
    t.tp = ERR;
    strcpy(t.lx, "Error: unexpected EOF in block comment");
    return 2;
}

// skips horizontal whitespace
// newlines are not handeld
// returns first character after whitespace
static int eatWhitespace ()
{
    while (1)
    {
        while (*b_ptr == ' ' || *b_ptr == '\t')
            incrementPtr();

        if (*b_ptr != '/')
            return (unsigned char)*b_ptr;

        incrementPtr();

        switch (*b_ptr)
        {
            case '/': // single-line comment
                while (*b_ptr != '\0' && *b_ptr != '\n')
                    incrementPtr();
                return (unsigned char)*b_ptr;
            
            case '*': // block comment
                incrementPtr();
                {
                    int r;
                    do
                    {
                        r = findMultiCommentEnd();
                    }
                    while (r == 0);

                    if (r == 2) return 0;
                }
                continue;

            default: // is just a '/'
                decrementPtr();
                return (unsigned char)*b_ptr;
                
        }
    }
}

// token building

// return 1 on error, else 0
static int buildToken ()
{
    int c = eatWhitespace();
    if (t.tp == ERR)
        return 1;

    if (*b_ptr == '\0')
    {
        t.lx[0] = '\0';
        t.tp = EOFile;
        return 0;
    }

    if (*b_ptr == '\n')
    {        
        t.tp = NONE;
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
                strcpy(t.lx, "Error: newline in string literal");
                return 1;
            }
            if (*b_ptr == '\0')
            {
                t.tp = ERR;
                t.ec = EofInStr;
                strcpy(t.lx, "Error: unexpected EOF in string literal");
                return 1;
            }
            if (i < (LEX_SIZE-2))
                temp[i++] = *b_ptr;
            incrementPtr(); 
        }

        incrementPtr(); // eat closing "

        temp[i] = '\0';
        strcpy(t.lx, temp);
        t.tp = STRING;
        
        return 0;
    }


    // keywords and identifiers
    if (isalpha((unsigned char)*b_ptr) || *b_ptr == '_')
    {
        while (isalpha((unsigned char)*b_ptr) || 
               isdigit((unsigned char)*b_ptr) ||
               *b_ptr == '_')
        {
            if (i < (LEX_SIZE-2))
                temp[i++] = *b_ptr;
            incrementPtr();
        }

        temp[i] = '\0';
        strcpy(t.lx, temp);

        t.tp = isKeyword(temp) ? KEYWORD : IDENTIFIER;

        return 0;
    }

    // ints
    if (isdigit((unsigned char)*b_ptr))
    {
        while(isdigit((unsigned char)*b_ptr))
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
// returns 1 on allocation fail, else 0
static int addToken (int* count, int* capacity)
{
    if (*count >= *capacity)
    {
        *capacity *= 2;
        int new_capacity = *capacity * 2;
        Token* tmp = realloc(tokens, (size_t)new_capacity * sizeof(Token));
        if (!tmp)
            return 1;

        tokens = tmp;
        *capacity = new_capacity;
    }

    tokens[*count] = t;
    (*count)++;

    return 0;
}

// measures indentation of the newline and emits INDENT/DEDENT tokens
// after, t points at first non-whitespace character in the line
static int eatIndent(int* count, int* capacity)
{
    int spaces = 0, tabs = 0;

    while (*b_ptr == ' ' || *b_ptr == '\t')
    {
        if (*b_ptr == '\t') tabs++;
        else                spaces++;
        incrementPtr();
    }

    // blank line, EOF or comment only
    if (*b_ptr == '\n' || *b_ptr == '\0' || *b_ptr == '/')
        return 0;

    if (tabs > 0 && spaces > 0)
    {
        t.tp = ERR;
        t.ec = TabError;
        strcpy(t.lx, "Error: mixed indentation");
        return 1;
    }

    int indent_level = (tabs * TAB_WIDTH) + spaces;
    
    if (indent_level > indent_stack[stack_top])
    {
        if (stack_top >= INDENT_STACK_SIZE - 1)
        {
            t.tp = ERR;
            t.ec = TabError;
            strcpy(t.lx, "Error: max indentation level reached");
            return 1;
        }

        indent_stack[++stack_top] = indent_level;

        t.tp = INDENT;
        t.ec = NoLexError;
        t.lx[0] = '\0';
        if (addToken(count, capacity))
            return 1;
    }
    else if (indent_level < indent_stack[stack_top])
    {
        // pop stack until match
        while (stack_top > 0 && indent_level < indent_stack[stack_top])
        {
            stack_top--;

            t.tp = DEDENT;
            t.ec = NoLexError;
            t.lx[0] = '\0';
            if (addToken(count, capacity)) return 1;
        }

        if (indent_level != indent_stack[stack_top])
        {
            t.tp = ERR;
            t.ec = TabError;
            strcpy(t.lx, "Error: dedent does not match any outer indentation level");
            return 1;
        }
    }

    // same level

    return 0;
}

// returns 1 on error, else 0
int loadTokens ()
{
    t = getClearToken();
    t.ln = 1;
    int token_count = 0;
    int capacity = 64;

    tokens = malloc(capacity * sizeof(Token));
    if (!tokens)
        return 1;

    // for a file that starts indented
    if (eatIndent(&token_count, &capacity))
    {
        addToken(&token_count, &capacity);
        return 1;
    }

    while (t.tp != EOFile && t.tp != ERR)
    {
        if (*b_ptr == '\n')
        {
            t.ln++;
            incrementPtr();

            while (*b_ptr == '\n')
            {
                t.ln++;
                incrementPtr();
            }

            if (*b_ptr == '\0')
                break;

            if (eatIndent(&token_count, &capacity))
            {
                addToken(&token_count, &capacity);
                return 1;
            }
            continue;
        }

        if (buildToken())
        {
            addToken(&token_count, &capacity);
            return 1;
        }

        if (t.tp != NONE)
            if (addToken(&token_count, &capacity))
                return 1;
    }

    // pop indent_stack and emit a DEDENT
    while (stack_top > 0)
    {
        stack_top--;

        t.tp = DEDENT;
        t.ec = NoLexError;
        t.lx[0] = '\0';

        if (addToken(&token_count, &capacity))
            return 1;
    }

    return 0;
}


int initLexer (char* file_name)
{
    clearIdentStack();

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