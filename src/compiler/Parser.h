//
// Created by Mahamadou DOUMBIA [OML DSI] on 11/01/2022.
//

#ifndef CPMSC_PARSER_H
#define CPMSC_PARSER_H


#include <stdarg.h>
#include "../memory/Value.h"
#include "Token.h"

typedef struct {
    MVM *vm;
    // The module being parsed.
    Module *module;

    // The source code being parsed.
    const char *source;

    // The beginning of the currently-being-lexed token in [source].
    const char *tokenStart;

    // The current character being lexed in [source].
    const char *currentChar;

    // The 1-based line number of [currentChar].
    int currentLine;

    // The upcoming token.
    Token next;

    // The most recently lexed token.
    Token current;

    // The most recently consumed/advanced token.
    Token previous;

    // Tracks the lexing state when tokenizing interpolated strings.
    //
    // Interpolated strings make the lexer not strictly regular: we don't know
    // whether a ")" should be treated as a RIGHT_PAREN token or as ending an
    // interpolated expression unless we know whether we are inside a string
    // interpolation and how many unmatched "(" there are. parser is particularly
    // complex because interpolation can nest:
    //
    //     " ${ " ${ inner } " } "
    //
    // parser tracks that state. The parser maintains a stack of ints, one for each
    // level of current interpolation nesting. Each value is the number of
    // unmatched "(" that are waiting to be closed.
    int parens[MAX_INTERPOLATION_NESTING];
    int numParens;

    // Whether compile errors should be printed to stderr or discarded.
    bool printErrors;
    bool interpolating;



// If a syntax or compile error has occurred.
    bool hasError;

} Parser;
void initParser(Parser *parser, MVM *vm, Module *module, const char *src);
void nextToken(Parser* parser);
char peekChar(Parser *parser);

char peekNextChar(Parser *parser);


bool matchChar(Parser *parser, char c);

char nextChar(Parser *parser);
void printError(Parser *parser, int line, const char *label, const char *format, va_list args);

/*



void printError(int line, const char* label,
                const char* format, va_list args);

// Outputs a lexical error.
void lexError(const char* format, ...);

char peekChar();

char peekNextChar();

char nextChar();

bool matchChar(char c);

void makeToken(TokenType type);

void twoCharToken(char c, TokenType two, TokenType one);

void skipLineComment();

void skipBlockComment();

int readHexDigit();

void makeNumber(int base);

void readHexNumber();

void readNumber();

void readName(TokenType type, char firstChar);

int readHexEscape(int digits, const char *description);

void readUnicodeEscape(ByteBuffer *string, int length);

void readRawString();

void readString();

void nextToken();

void readBinaryNumber();

int readBinaryDigit();

int readOctalDigit();

void readOctalNumber();*/
typedef struct {
    const char *identifier;
    size_t length;
    TokenType tokenType;
} Keyword;

// The table of reserved words and their associated token types.



#endif //CPMSC_PARSER_H
