//
// Created by Mahamadou DOUMBIA [OML DSI] on 11/01/2022.
//

#include "Parser.h"
#include "../runtime/MVM.h"
#include <errno.h>

static Keyword keywords[] =
        {
                {"dilan",   5, INIT_TOKEN},
                {"tii",     3, JOB_TOKEN},
                {"nin",     3, VAR_TOKEN},
                {"nii",     3, IF_TOKEN},
                {"note",    4, ELSE_TOKEN},
                {"seginka", 7, FOR_LOOP_TOKEN},
                {"foo",     3, WHILE_TILL_TOKEN},
                {"tumamin", 7, WHEN_TOKEN},
                {"gansan",  6, NULL_TOKEN},
                {"foyi",    4, VOID_TOKEN},
                {"tien",    4, TRUE_TOKEN},
                {"galon",   5, FALSE_TOKEN},
                {"segin",   5, RETURN_TOKEN},
                {"dialen",  6, STATIC_TOKEN},
                {"inafo",   5, AS_TOKEN},
                {"kono",    4, IN_TOKEN},
                {"niin",    4, WITH_TOKEN},
                {"kay",     3, UP_TOKEN},
                {"kaj",     3, DOWN_TOKEN},
                {"ye",      2, YE_TOKEN},
                {"nani",    4, IMPORT_TOKEN},
                {"kabo",    4, FROM_TOKEN},
                {"kulu",    4, CLASS_TOKEN},
                // {"kura",    4, NEW_TOKEN},
                {"ake",     3, DO_TOKEN},
                {"namason", 7, CATCH_TOKEN},
                {"bawo",    4, BECAUSE_TOKEN},
                {"ale",     3, THIS_TOKEN},
                {"faa",     3, SUPER_TOKEN},
                {"atike",   5, BREAK_TOKEN},
                {"ipan",    4, CONTINUE_TOKEN},
                {"afili",   5, THROW_TOKEN},
                {"dunan",   5, EXTERN_TOKEN},
                {NULL,      0, EOF_TOKEN} // Sentinel to mark the end of the array.

        };


void initParser(Parser *parser, MVM *vm, Module *module, const char *src) {

    parser->vm = vm;
    parser->module = module;
    parser->source = src;
    parser->tokenStart = src;
    parser->currentChar = src;
    parser->currentLine = 1;
    parser->numParens = 0;

    // Zero-init the current token. parser will get copied to previous when
    // nextToken() is called below.
    parser->next.type = ERROR_TOKEN;
    parser->next.start = src;
    parser->next.length = 0;
    parser->next.line = 0;
    parser->next.value = UNDEFINED_VAL;

    parser->interpolating = false;
}


void printError(Parser *parser, int line, const char *label, const char *format, va_list args) {
    parser->hasError = true;
    if (!parser->printErrors) return;

    // Only report errors if there is a WrenErrorFn to handle them.
    if (parser->vm->config.errorHandler == NULL) return;

    // Format the label and message.
    char message[ERROR_MESSAGE_SIZE];
    int length = sprintf(message, "%s: ", label);
    length += vsprintf(message + length, format, args);
    ASSERT(length < ERROR_MESSAGE_SIZE, "Error should not exceed buffer.");

    String *module = parser->module->name;
    const char *module_name = module ? module->value : "<unknown>";

    parser->vm->config.errorHandler(parser->vm, ERROR_COMPILE,
                                    module_name, line, message);
}

static void lexError(Parser *parser, const char *format, ...) {
    va_list args;
    va_start(args, format);
    printError(parser, parser->currentLine, "Error", format, args);
    va_end(args);
}


// Returns true if [c] is a valid (non-initial) identifier character.
static bool isName(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

// Returns true if [c] is a digit.
static bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

// Returns the current character the parser is sitting on.
char peekChar(Parser *parser) {
    return *parser->currentChar;
}

// Returns the character after the current character.
char peekNextChar(Parser *parser) {
    // If we're at the end of the source, don't read past it.
    if (peekChar(parser) == '\0') return '\0';
    return *(parser->currentChar + 1);
}


// Advances the parser forward one character.
char nextChar(Parser *parser) {
    char c = peekChar(parser);
    parser->currentChar++;
    if (c == '\n') parser->currentLine++;
    return c;
}

// If the current character is [c], consumes it and returns `true`.
bool matchChar(Parser *parser, char c) {
    if (peekChar(parser) != c) return false;
    nextChar(parser);
    return true;
}

// Sets the parser's current token to the given [type] and current character
// range.
static void makeToken(Parser *parser, TokenType type) {
    parser->next.type = type;
    parser->next.start = parser->tokenStart;
    parser->next.length = (uint32_t) (parser->currentChar - parser->tokenStart);
    parser->next.line = parser->currentLine;
    // Make line tokens appear on the line containing the "\n".
    if (type == EOL_TOKEN) parser->next.line--;
}

// If the current character is [c], then consumes it and makes a token of type
// [two]. Otherwise makes a token of type [one].
static void twoCharToken(Parser *parser, char c, TokenType two, TokenType one) {
    makeToken(parser, matchChar(parser, c) ? two : one);
}

// Skips the rest of the current line.
static void skipLineComment(Parser *parser) {
    while (peekChar(parser) != '\n' && peekChar(parser) != '\0') {
        nextChar(parser);
    }
}

// Skips the rest of a block comment.
static void skipBlockComment(Parser *parser) {
    int nesting = 1;
    while (nesting > 0) {
        if (peekChar(parser) == '\0') {
            lexError(parser, "Unterminated block comment.");
            return;
        }

        if (peekChar(parser) == '#' && peekNextChar(parser) == '*') {
            nextChar(parser);
            nextChar(parser);
            nesting++;
            continue;
        }

        if (peekChar(parser) == '*' && peekNextChar(parser) == '#') {
            nextChar(parser);
            nextChar(parser);
            nesting--;
            continue;
        }
        // Regular comment character.
        nextChar(parser);
    }
}

// Reads the next character, which should be a hex digit (0-9, a-f, or A-F) and
// returns its numeric value. If the character isn't a hex digit, returns -1.
static int readHexDigit(Parser *parser) {
    char c = nextChar(parser);
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;

    // Don't consume it if it isn't expected. Keeps us from reading past the end
    // of an unterminated string.
    parser->currentChar--;
    return -1;
}

// Reads the next character, which should be a binary digit (0-1) and
// returns its numeric value. If the character isn't a binary digit, returns -1.
static int readBinaryDigit(Parser *parser) {
    char c = nextChar(parser);
    if (c >= '0' && c <= '1') return c - '0';

    // Don't consume it if it isn't expected. Keeps us from reading past the end
    // of an unterminated string.
    parser->currentChar--;
    return -1;
}

// Reads the next character, which should be a binary digit (0-1) and
// returns its numeric value. If the character isn't a binary digit, returns -1.
static int readOctalDigit(Parser *parser) {
    char c = nextChar(parser);
    if (c >= '0' && c <= '7') return c - '0';

    // Don't consume it if it isn't expected. Keeps us from reading past the end
    // of an unterminated string.
    parser->currentChar--;
    return -1;
}

// Parses the numeric value of the current token.
static void makeNumber(Parser *parser, int base) {
    errno = 0;

    if (base != 10) {
        parser->next.value = NUM_VAL((double) strtoll(parser->tokenStart, NULL, base));
    } else {
        parser->next.value = NUM_VAL(strtod(parser->tokenStart, NULL));
    }

    if (errno == ERANGE) {
        lexError(parser, "Number literal was too large (%d).", sizeof(long int));
        parser->next.value = NUM_VAL(0);
    }
    // We don't check that the entire token is consumed after calling strtoll()
    // or strtod() because we've already scanned it ourselves and know it's valid.
    makeToken(parser, NUMBER_CONST_TOKEN);
}

// Finishes lexing a hexadecimal number literal.
static void readHexNumber(Parser *parser) {
    // Skip past the `x` used to denote a hexadecimal literal.
    nextChar(parser);

    // Iterate over all the valid hexadecimal digits found.
    while (readHexDigit(parser) != -1) continue;

    makeNumber(parser, 16);
}

// Finishes lexing a hexadecimal number literal.
static void readBinaryNumber(Parser *parser) {
    // Skip past the `b` used to denote a hexadecimal literal.
    nextChar(parser);
    parser->tokenStart = parser->tokenStart + 2;
    // Iterate over all the valid hexadecimal digits found.
    while (readBinaryDigit(parser) != -1) continue;

    makeNumber(parser, 2);
}

// Finishes lexing a hexadecimal number literal.
static void readOctalNumber(Parser *parser) {
    // Skip past the `b` used to denote a hexadecimal literal.
    nextChar(parser);
    parser->tokenStart = parser->tokenStart + 2;
    // Iterate over all the valid hexadecimal digits found.
    while (readOctalDigit(parser) != -1) continue;

    makeNumber(parser, 8);
}

// Finishes lexing a number literal.
static void readNumber(Parser *parser) {
    while (isDigit(peekChar(parser))) nextChar(parser);

    // See if it has a floating point. Make sure there is a digit after the "."
    // so we don't get confused by method calls on number literals.
    if (peekChar(parser) == '.' && isDigit(peekNextChar(parser))) {
        nextChar(parser);
        while (isDigit(peekChar(parser))) nextChar(parser);
    }

    // See if the number is in scientific notation.
    if (matchChar(parser, 'e') || matchChar(parser, 'E')) {
        // Allow a single positive/negative exponent symbol.
        if (!matchChar(parser, '+')) {
            matchChar(parser, '-');
        }

        if (!isDigit(peekChar(parser))) {
            lexError(parser, "Unterminated scientific notation.");
        }

        while (isDigit(peekChar(parser))) nextChar(parser);
    }
    makeNumber(parser, 10);
}

// Finishes lexing an identifier. Handles reserved words.
static void readName(Parser *parser, TokenType type, char firstChar) {
    ByteBuffer string;
    MSCInitByteBuffer(&string);
    MSCWriteByteBuffer(parser->vm, &string, (uint8_t) firstChar);

    while (isName(peekChar(parser)) || isDigit(peekChar(parser))) {
        char c = nextChar(parser);
        MSCWriteByteBuffer(parser->vm, &string, (uint8_t) c);
    }

    // Update the type if it's a keyword.
    size_t length = parser->currentChar - parser->tokenStart;
    for (int i = 0; keywords[i].identifier != NULL; i++) {
        if (length == keywords[i].length &&
            memcmp(parser->tokenStart, keywords[i].identifier, length) == 0) {
            type = keywords[i].tokenType;
            break;
        }
    }
    parser->next.value = MSCStringFromCharsWithLength(parser->vm,
                                                      (char *) string.data, (uint32_t) string.count);

    MSCFreeByteBuffer(parser->vm, &string);
    makeToken(parser, type);
}

// Reads [digits] hex digits in a string literal and returns their number value.
static int readHexEscape(Parser *parser, int digits, const char *description) {
    int value = 0;
    for (int i = 0; i < digits; i++) {
        if (peekChar(parser) == '"' || peekChar(parser) == '\0') {
            lexError(parser, "Incomplete %s escape sequence.", description);

            // Don't consume it if it isn't expected. Keeps us from reading past the
            // end of an unterminated string.
            parser->currentChar--;
            break;
        }

        int digit = readHexDigit(parser);
        if (digit == -1) {
            lexError(parser, "Invalid %s escape sequence.", description);
            break;
        }

        value = (value * 16) | digit;
    }

    return value;
}

// Reads a hex digit Unicode escape sequence in a string literal.
static void readUnicodeEscape(Parser *parser, ByteBuffer *string, int length) {
    int value = readHexEscape(parser, length, "Unicode");

    // Grow the buffer enough for the encoded result.
    int numBytes = MSCUtf8EncodeNumBytes(value);
    if (numBytes != 0) {
        MSCFillByteBuffer(parser->vm, string, 0, numBytes);
        MSCUtf8Encode(value, string->data + string->count - numBytes);
    }
}

static void readRawString(Parser *parser) {
    ByteBuffer string;
    MSCInitByteBuffer(&string);
    TokenType type = STRING_CONST_TOKEN;

    //consume the second and third "
    nextChar(parser);
    nextChar(parser);

    int skipStart = 0;
    int firstNewline = -1;

    int skipEnd = -1;
    int lastNewline = -1;

    for (;;) {
        char c = nextChar(parser);
        char c1 = peekChar(parser);
        char c2 = peekNextChar(parser);

        if (c == '\r') continue;

        if (c == '\n') {
            lastNewline = string.count;
            skipEnd = lastNewline;
            firstNewline = firstNewline == -1 ? string.count : firstNewline;
        }

        if (c == '"' && c1 == '"' && c2 == '"') break;

        bool isWhitespace = c == ' ' || c == '\t';
        skipEnd = c == '\n' || isWhitespace ? skipEnd : -1;

        // If we haven't seen a newline or other character yet,
        // and still seeing whitespace, count the characters
        // as skippable till we know otherwise
        bool skippable = skipStart != -1 && isWhitespace && firstNewline == -1;
        skipStart = skippable ? string.count + 1 : skipStart;

        // We've counted leading whitespace till we hit something else,
        // but it's not a newline, so we reset skipStart since we need these characters
        if (firstNewline == -1 && !isWhitespace && c != '\n') skipStart = -1;

        if (c == '\0' || c1 == '\0' || c2 == '\0') {
            lexError(parser, "Unterminated raw string.");

            // Don't consume it if it isn't expected. Keeps us from reading past the
            // end of an unterminated string.
            parser->currentChar--;
            break;
        }

        MSCWriteByteBuffer(parser->vm, &string, (uint8_t) c);
    }

    //consume the second and third "
    nextChar(parser);
    nextChar(parser);

    int offset = 0;
    int count = string.count;

    if (firstNewline != -1 && skipStart == firstNewline) offset = firstNewline + 1;
    if (lastNewline != -1 && skipEnd == lastNewline) count = lastNewline;

    count -= (offset > count) ? count : offset;

    parser->next.value = MSCStringFromCharsWithLength(parser->vm,
                                                      ((char *) string.data) + offset, (uint32_t) count);

    MSCFreeByteBuffer(parser->vm, &string);
    makeToken(parser, type);
}

// Finishes lexing a string literal.
static void readString(Parser *parser) {
    ByteBuffer string;
    TokenType type = STRING_CONST_TOKEN;
    MSCInitByteBuffer(&string);

    for (;;) {
        char c = nextChar(parser);
        if (c == '"') break;
        if (c == '\r') continue;

        if (c == '\0') {
            lexError(parser, "Unterminated string.");

            // Don't consume it if it isn't expected. Keeps us from reading past the
            // end of an unterminated string.
            parser->currentChar--;
            break;
        }

        if (c == '$') {
            if (isName(peekChar(parser))) {
                parser->interpolating = true;
                type = DOLLAR_INTERPOL_TOKEN;
                break;
            }
            if (peekChar(parser) == '{') {
                nextChar(parser);
                if (parser->numParens < MAX_INTERPOLATION_NESTING) {
                    // TODO: Allow format string.
                    //  lexError(parser,"Expect '{' or identifier after '$'.");
                    parser->parens[parser->numParens++] = 1;
                    type = DOLLAR_INTERPOL_TOKEN;
                    break;
                }
                lexError(parser, "Interpolation may only nest %d levels deep.",
                         MAX_INTERPOLATION_NESTING);
            }

        }

        if (c == '\\') {
            switch (nextChar(parser)) {
                case '"':
                    MSCWriteByteBuffer(parser->vm, &string, '"');
                    break;
                case '\\':
                    MSCWriteByteBuffer(parser->vm, &string, '\\');
                    break;
                case '{':
                    MSCWriteByteBuffer(parser->vm, &string, '{');
                    break;
                case '$':
                    MSCWriteByteBuffer(parser->vm, &string, '$');
                    break;
                case '0':
                    MSCWriteByteBuffer(parser->vm, &string, '\0');
                    break;
                case 'a':
                    MSCWriteByteBuffer(parser->vm, &string, '\a');
                    break;
                case 'b':
                    MSCWriteByteBuffer(parser->vm, &string, '\b');
                    break;
                case 'e':
                    MSCWriteByteBuffer(parser->vm, &string, '\33');
                    break;
                case 'f':
                    MSCWriteByteBuffer(parser->vm, &string, '\f');
                    break;
                case 'n':
                    MSCWriteByteBuffer(parser->vm, &string, '\n');
                    break;
                case 'r':
                    MSCWriteByteBuffer(parser->vm, &string, '\r');
                    break;
                case 't':
                    MSCWriteByteBuffer(parser->vm, &string, '\t');
                    break;
                case 'u':
                    readUnicodeEscape(parser, &string, 4);
                    break;
                case 'U':
                    readUnicodeEscape(parser, &string, 8);
                    break;
                case 'v':
                    MSCWriteByteBuffer(parser->vm, &string, '\v');
                    break;
                case 'x':
                    MSCWriteByteBuffer(parser->vm, &string,
                                       (uint8_t) readHexEscape(parser, 2, "byte"));
                    break;

                default:
                    lexError(parser, "Invalid escape character '%c'.",
                             *(parser->currentChar - 1));
                    break;
            }
        } else {
            MSCWriteByteBuffer(parser->vm, &string, (uint8_t) c);
        }
    }

    parser->next.value = MSCStringFromCharsWithLength(parser->vm,
                                                      (char *) string.data, (uint32_t) string.count);

    MSCFreeByteBuffer(parser->vm, &string);
    makeToken(parser, type);
}

// Lex the next token and store it in [parser.next].
void nextToken(Parser *parser) {
    parser->previous = parser->current;
    parser->current = parser->next;

    // If we are out of tokens, don't try to tokenize any more. We *do* still
    // copy the TOKEN_EOF to previous so that code that expects it to be consumed
    // will still work.
    if (parser->next.type == EOF_TOKEN) return;
    if (parser->current.type == EOF_TOKEN) return;

    while (peekChar(parser) != '\0') {
        parser->tokenStart = parser->currentChar;

        char c = nextChar(parser);
        if (parser->interpolating) {
            if (!isName(c) && !isDigit(c)) {
                parser->interpolating = false;
                parser->currentChar--;
                readString(parser);
                return;
            }
        }
        switch (c) {
            case '(':

                makeToken(parser, LPAREN_TOKEN);
                return;

            case ')':

                makeToken(parser, RPAREN_TOKEN);
                return;

            case '[':
                makeToken(parser, LBRACKET_TOKEN);
                return;
            case ']':
                makeToken(parser, RBRACKET_TOKEN);
                return;
            case '{':
                // If we are inside an interpolated expression, count the unmatched "(".
                if (parser->numParens > 0) parser->parens[parser->numParens - 1]++;
                makeToken(parser, LBRACE_TOKEN);
                return;
            case '}':
                // If we are inside an interpolated expression, count the ")".
                if (parser->numParens > 0 &&
                    --parser->parens[parser->numParens - 1] == 0) {
                    // parser is the final "}", so the interpolation expression has ended.
                    // parser "}" now begins the next section of the template string.
                    parser->numParens--;
                    readString(parser);
                    return;
                }

                makeToken(parser, RBRACE_TOKEN);
                return;
            case ':':
                makeToken(parser, COLON_TOKEN);
                return;
            case ',':
                makeToken(parser, COMMA_TOKEN);
                return;
            case ';':
                makeToken(parser, SEMI_TOKEN);
                return;
            case '*':
                makeToken(parser, MULT_TOKEN);
                return;
            case '%':
                makeToken(parser, MOD_TOKEN);
                return;
            case '#': {
                // Ignore shebang on the first line.
                if (peekChar(parser) == '*') {
                    skipBlockComment(parser);
                    break;
                }
                skipLineComment(parser);
                break;
            }
            case '^':
                makeToken(parser, BT_XOR_TOKEN);
                return;
            case '+':
                twoCharToken(parser, '+', INC_TOKEN, PLUS_TOKEN);
                return;
            case '-':
                twoCharToken(parser, '-', DEC_TOKEN, MINUS_TOKEN);
                return;
            case '~':
                makeToken(parser, BT_INV_TOKEN);
                return;

            case '|':
                twoCharToken(parser, '|', OR_TOKEN, BT_OR_TOKEN);
                return;
            case '&':
                twoCharToken(parser, '&', AND_TOKEN, BT_AND_TOKEN);
                return;
            case '=':
                if (matchChar(parser, '>')) {
                    makeToken(parser, ARROW_TOKEN);
                    return;
                }
                twoCharToken(parser, '=', EQUAL_TOKEN, ASSIGN_TOKEN);
                return;
            case '!':
                twoCharToken(parser, '=', NEQUAL_TOKEN, NOT_TOKEN);
                return;

            case '.':
                if (matchChar(parser, '.')) {
                    twoCharToken(parser, '.', SPREAD_OR_REST_TOKEN, RANGE_TOKEN);
                    return;
                }

                makeToken(parser, DOT_TOKEN);
                return;

            case '/':
                makeToken(parser, DIV_TOKEN);
                return;

            case '<':
                if (matchChar(parser, '<')) {
                    makeToken(parser, BT_SHIFT_L_TOKEN);
                } else {
                    twoCharToken(parser, '=', LESS_EQ_TOKEN, LESS_TOKEN);
                }
                return;

            case '>':
                if (matchChar(parser, '>')) {
                    makeToken(parser, BT_SHIFT_R_TOKEN);
                } else {
                    twoCharToken(parser, '=', GREAT_EQ_TOKEN, GREAT_TOKEN);
                }
                return;

            case '\n':
                makeToken(parser, EOL_TOKEN);
                return;

            case ' ':
            case '\r':
            case '\t':
                // Skip forward until we run out of whitespace.
                while (peekChar(parser) == ' ' ||
                       peekChar(parser) == '\r' ||
                       peekChar(parser) == '\t') {
                    nextChar(parser);
                }
                break;

            case '"': {
                if (peekChar(parser) == '"' && peekNextChar(parser) == '"') {
                    readRawString(parser);
                    return;
                }
                readString(parser);
                return;
            }

            case '0': {
                char nextChar = peekChar(parser);
                if (nextChar == 'x' || nextChar == 'X') {
                    readHexNumber(parser);
                    return;
                }
                if (nextChar == 'b' || nextChar == 'B') {
                    readBinaryNumber(parser);
                    return;
                }
                if (nextChar == 'o' || nextChar == 'O') {
                    readOctalNumber(parser);
                    return;
                }
                readNumber(parser);
                return;
            }
            default:
                if (isName(c)) {
                    readName(parser, ID_TOKEN, c);
                } else if (isDigit(c)) {
                    readNumber(parser);
                } else {
                    if (c >= 32 && c <= 126) {
                        lexError(parser, "Invalid character '%c'.", c);
                    } else {
                        // Don't show non-ASCII values since we didn't UTF-8 decode the
                        // bytes. Since there are no non-ASCII byte values that are
                        // meaningful code units in MSC, the lexer works on raw bytes,
                        // even though the source code and console output are UTF-8.
                        lexError(parser, "Invalid byte 0x%x.", (uint8_t) c);
                    }
                    parser->next.type = ERROR_TOKEN;
                    parser->next.length = 0;
                }
                return;
        }
    }

    // If we get here, we're out of source, so just make EOF tokens.
    parser->tokenStart = parser->currentChar;
    makeToken(parser, EOF_TOKEN);
}
