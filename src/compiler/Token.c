//
// Created by Mahamadou DOUMBIA [OML DSI] on 12/01/2022.
//

#include "Token.h"


Token newToken(TokenType type, const char *start, int length, int line, Value value) {
    Token token;
    token.length = (uint32_t) length;
    token.start = start;
    token.type = type;
    token.value = value;
    token.line = line;
    return token;
}
void initToken(Token* token, TokenType type, const char *start, int length, int line, Value value) {
    token->length = (uint32_t) length;
    token->start = start;
    token->type = type;
    token->value = value;
    token->line = line;
}

