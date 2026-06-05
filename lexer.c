#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

const char* KEYWORDS[KEYWORD_COUNT] = {
    "character", "integer", "floating", "double", "boolean",
    "void", "if", "else", "while", "return", "true", "false"
};

const char SYMBOLS[SYMBOL_COUNT] = {
    '(', ')', '[', ']', ',', ':',
    '=', '.', '+', '-', '*', '/', '<', '>'
};

#define INDENT_STACK_SIZE 256
#define TAB_WIDTH 4

char* buf;
char* b_ptr;

static char filename[LEX_SIZE];

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
    tok.ec = LEX_OK;
    tok.ln = 0; // 1-based (so source is obvious)

    return tok;
}

static void clearIdentStack()
{
    stack_top = 0;
    memset(indent_stack, 0, sizeof(indent_stack));
}

// buffer management

LexStatus readFileToBuf(char *path)
{
    FILE* f = fopen(path, "rb");
    if (!f) return LEX_ERR_FILE_NOT_FOUND;

    int status;
    status = fseek(f, 0, SEEK_END);
    if (status != 0) 
    {
        fclose(f);
        return LEX_ERR_FILE_READ;
    }
    
    long b_length = ftell(f);
    if (b_length < 0)
    {
        fclose(f);
        return LEX_ERR_FILE_READ;
    }

    status = fseek(f, 0, SEEK_SET);
    if (status != 0)
    {
        fclose(f);
        return LEX_ERR_FILE_READ;
    }

    buf = malloc(b_length+1);
    if (buf == NULL)
    {
        fclose(f);
        return LEX_ERR_MALLOC;
    }

    long count = fread(buf, 1, b_length, f);
    fclose(f);
    
    if (count != b_length) return LEX_ERR_FILE_READ;
    
    buf[b_length] = '\0';

    return LEX_OK;
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
returns LEX_ERR_COMMENT_EOF if eof found
*/
static LexStatus findMultiCommentEnd ()
{
    while (*b_ptr != '\0')
    {
        if (*b_ptr == '\n')
            t.ln++;

        if (*b_ptr == '*')
        {
            b_ptr++;
            if (*b_ptr == '/')
            {
                b_ptr++;
                return LEX_OK;
            }

            continue;
        }

        b_ptr++;
    }

    t.ec = LEX_ERR_COMMENT_EOF;
    t.tp = ERR;
    strcpy(t.lx, "Error: unexpected EOF in block comment");
    return LEX_ERR_COMMENT_EOF;
}

// skips horizontal whitespace
// newlines are not handeld
static LexStatus eatWhitespace ()
{
    while (1)
    {
        while (*b_ptr == ' ' || *b_ptr == '\t' || *b_ptr == '\r')
            b_ptr++;

        if (*b_ptr != '/')
            return LEX_OK;
        
        // is safe to look ahead
        char* next = b_ptr + 1;
        switch (*next)
        {
            case '/': // single-line comment
                while (*b_ptr != '\0' && *b_ptr != '\n')
                    b_ptr++;
                return LEX_OK;
            
            case '*': // block comment
                b_ptr++;
                b_ptr++;
                return findMultiCommentEnd();

            default: // is just a '/'
                return LEX_OK;
        }
    }
}

// token building

// return 0 on error, else 1
static LexStatus buildToken ()
{
    eatWhitespace();
    if (t.tp == ERR)
        return LEX_ERR;

    if (*b_ptr == '\0')
    {
        t.lx[0] = '\0';
        t.tp = EOFile;
        return LEX_ERR;
    }

    if (*b_ptr == '\n' || *b_ptr == '\r')
    {        
        t.tp = NONE;
        return LEX_ERR;
    }

    int i = 0;
    char temp[LEX_SIZE] = {0};

    // handle string literal
    if (*b_ptr == '"')
    {
        b_ptr++;
        while (*b_ptr != '"')
        {
            if (*b_ptr == '\n')
            {
                t.tp = ERR;
                t.ec = LEX_ERR_STR_NEWLINE;
                strcpy(t.lx, "Error: newline in string literal");
                return LEX_ERR_STR_NEWLINE;
            }
            if (*b_ptr == '\0')
            {
                t.tp = ERR;
                t.ec = LEX_ERR_STR_EOF;
                strcpy(t.lx, "Error: unexpected EOF in string literal");
                return LEX_ERR_STR_EOF;
            }
            if (i < (LEX_SIZE-2))
                temp[i++] = *b_ptr;
            b_ptr++; 
        }

        b_ptr++; // eat closing "

        temp[i] = '\0';
        strcpy(t.lx, temp);
        t.tp = STRING;
        
        return LEX_OK;
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
            b_ptr++;
        }

        temp[i] = '\0';
        strcpy(t.lx, temp);

        t.tp = isKeyword(temp) ? KEYWORD : IDENTIFIER;

        return LEX_OK;
    }

    // ints (TODO needs spacing)
    if (isdigit((unsigned char)*b_ptr))
    {
        while(isdigit((unsigned char)*b_ptr))
        {
            if (i < (LEX_SIZE-2))
                temp[i++] = *b_ptr;
            b_ptr++;
        }

        temp[i] = '\0';
        strcpy(t.lx, temp);
        t.tp = INT;
        return LEX_OK;
    }

    // symbols
    else
    {
        if (!isSymbol(*b_ptr))
        {
            t.lx[0] = *b_ptr;
            t.lx[1] = '\0';
            t.tp = ERR;
            t.ec = LEX_ERR_ILLEGAL_SYM;
            return LEX_ERR_ILLEGAL_SYM;
        }

        t.tp = SYMBOL;
        t.lx[0] = *b_ptr;
        t.lx[1] = '\0';
        b_ptr++;
        return LEX_OK;
    }
}

// adds global token to tokens array
// doubles memory if required
// returns 0 on allocation fail, else !
LexStatus addToken (int* count, int* capacity)
{
    if (*count >= *capacity)
    {
        int new_capacity = *capacity * 2;
        Token* tmp = realloc(tokens, (size_t)new_capacity * sizeof(Token));
        if (!tmp) return LEX_ERR_MALLOC;

        tokens = tmp;
        *capacity = new_capacity;
    }

    tokens[*count] = t;
    (*count)++;

    return LEX_OK;
}

// measures indentation of the newline and emits INDENT/DEDENT tokens
// after, t points at first non-whitespace character in the line
LexStatus eatIndent(int* count, int* capacity)
{
    int spaces = 0, tabs = 0;

    while (*b_ptr == ' ' || *b_ptr == '\t')
    {
        if (*b_ptr == '\t') tabs++;
        else                spaces++;
        b_ptr++;
    }

    // blank line, EOF or comment only
    if (*b_ptr == '\n' || *b_ptr == '\0' || *b_ptr == '/')
        return LEX_OK;

    if (tabs > 0 && spaces > 0)
    {
        t.tp = ERR;
        t.ec = LEX_ERR_TAB;
        strcpy(t.lx, "Error: mixed indentation");
        return LEX_ERR_TAB;
    }

    int indent_level = (tabs * TAB_WIDTH) + spaces;
    
    if (indent_level > indent_stack[stack_top])
    {
        if (stack_top >= INDENT_STACK_SIZE - 1)
        {
            t.tp = ERR;
            t.ec = LEX_ERR_TAB;
            strcpy(t.lx, "Error: max indentation level reached");
            return LEX_ERR_TAB;
        }

        indent_stack[++stack_top] = indent_level;

        t.tp = INDENT;
        t.ec = LEX_OK;
        t.lx[0] = '\0';
        if (addToken(count, capacity) != LEX_OK)
            return LEX_ERR_MALLOC;
    }
    else if (indent_level < indent_stack[stack_top])
    {
        // pop stack until match
        while (stack_top > 0 && indent_level < indent_stack[stack_top])
        {
            stack_top--;

            t.tp = DEDENT;
            t.ec = LEX_OK;
            t.lx[0] = '\0';
            if (addToken(count, capacity) != LEX_OK)
                return LEX_ERR_MALLOC;
        }

        if (indent_level != indent_stack[stack_top])
        {
            t.tp = ERR;
            t.ec = LEX_ERR_TAB;
            strcpy(t.lx, "Error: dedent does not match any outer indentation level");
            return LEX_ERR_TAB;
        }
    }

    // same level
    return LEX_OK;
}

LexStatus loadTokens ()
{
    t = getClearToken();
    t.ln = 1;
    int token_count = 0;
    int capacity = 64;

    tokens = malloc(capacity * sizeof(Token));
    if (!tokens) return LEX_ERR_MALLOC;

    // for a file that starts indented
    {
        int status = eatIndent(&token_count, &capacity);
        if (status != LEX_OK)
        {
            if (addToken(&token_count, &capacity) != LEX_OK)
                return LEX_ERR_MALLOC;
            return status;
        }
    }
    

    while (t.tp != EOFile && t.tp != ERR)
    {
        if (*b_ptr == '\n')
        {
            t.ln++;
            b_ptr++;

            while (*b_ptr == '\n')
            {
                t.ln++;
                b_ptr++;
            }

            if (*b_ptr == '\0')
                break;

            int status = eatIndent(&token_count, &capacity);
            if (status != LEX_OK)
            {
                if (addToken(&token_count, &capacity) != LEX_OK)
                    return LEX_ERR_MALLOC;
                return status;
            }
            continue;
        }
        // TODO

        int status = buildToken();
        if (status == LEX_OK)
        {
            if (t.tp != NONE)
            {
                if (addToken(&token_count, &capacity) != LEX_OK)
                    return LEX_ERR_MALLOC;
            }
        }
        else if (t.tp == EOFile)
        {
            if (addToken(&token_count, &capacity) != LEX_OK)
                return LEX_ERR_MALLOC;
            break;
        }
        else if (t.tp != NONE)
        {
            if (addToken(&token_count, &capacity) != LEX_OK)
                return LEX_ERR_MALLOC;
            return status;
        }
    }

    // pop indent_stack and emit a DEDENT
    while (stack_top > 0)
    {
        stack_top--;

        t.tp = DEDENT;
        t.ec = LEX_OK;
        t.lx[0] = '\0';

        int status = addToken(&token_count, &capacity);
        if (status != LEX_OK) return status;
    }

    return LEX_OK;
}


int initLexer (char* file_name)
{
    clearIdentStack();

    strncpy(filename, file_name, sizeof(filename)-1);
    filename[sizeof(filename)-1] = '\0';

    int status = readFileToBuf(file_name);
    if (status != LEX_OK) return status;

    b_ptr = buf;

    status = loadTokens();
    if (status != LEX_OK)
    {
        printf("Could not load tokens\n");
        t_ptr = tokens;
        return status;
    }

    t_ptr = tokens;
    return LEX_OK;
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

    if (buf)
        free(buf);
    
    tokens = NULL;
    t_ptr = NULL;
    return 0;
}

const char *tokenTypeString(TokenType tp)
{
    switch (tp)
    {
        case KEYWORD:    return "KEYWORD";
        case IDENTIFIER: return "IDENTIFIER";
        case INT:        return "INT";
        case SYMBOL:     return "SYMBOL";
        case STRING:     return "STRING";
        case INDENT:     return "INDENT";
        case DEDENT:     return "DEDENT";
        case EOFile:     return "EOFile";
        case ERR:        return "ERR";
        case NONE:       return "NONE";
        default:         return "UNKNOWN";
    }
}

const char *lexStatusString(LexStatus err)
{
    switch (err)
    {
        case LEX_OK:                return "None";
        case LEX_ERR_COMMENT_EOF:   return "EOF in Comment";
        case LEX_ERR_STR_NEWLINE:   return "Newline in String";
        case LEX_ERR_STR_EOF:       return "EOF in String";
        case LEX_ERR_ILLEGAL_SYM:   return "Illegal Symbol";
        case LEX_ERR_TAB:           return "Tab Error";
        case LEX_ERR_FILE_NOT_FOUND:return "File not found";
        case LEX_ERR_FILE_READ:     return "File read err";
        case LEX_ERR_MALLOC:        return "Malloc err";
        case LEX_ERR:               return "generic";
        default:                    return "OTHER";
    }
}