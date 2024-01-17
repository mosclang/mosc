//
// Created by Mahamadou DOUMBIA [OML DSI] on 12/01/2022.
//

#ifndef CPMSC_TOKEN_H
#define CPMSC_TOKEN_H

#include "../memory/Value.h"


typedef enum {
    NUMBER_CONST_TOKEN = 0,
    STRING_CONST_TOKEN = 1,
    PLUS_TOKEN = 2,
    MINUS_TOKEN = 3,
    MULT_TOKEN = 4,
    DIV_TOKEN = 5,
    MOD_TOKEN = 6,
    DOLLAR_INTERPOL_TOKEN = 7,
    LPAREN_TOKEN = 8,
    RPAREN_TOKEN = 9,
    LBRACE_TOKEN = 10,
    RBRACE_TOKEN = 11,
    LBRACKET_TOKEN = 12,
    RBRACKET_TOKEN = 13,
    JOB_TOKEN = 14,
    ARROW_TOKEN = 15,
    GREAT_TOKEN = 16,
    GREAT_EQ_TOKEN = 17,
    LESS_TOKEN = 18,
    LESS_EQ_TOKEN = 19,
    EQUAL_TOKEN = 20,
    NEQUAL_TOKEN = 21,
    IF_TOKEN = 22,
    ELSE_TOKEN = 23,
    AND_TOKEN = 24,
    OR_TOKEN = 25,
    BT_XOR_TOKEN = 26,
    BT_AND_TOKEN = 27,
    BT_OR_TOKEN = 28,
    BT_SHIFT_R_TOKEN = 29,
    BT_SHIFT_L_TOKEN = 30,
    INC_TOKEN = 31,
    DEC_TOKEN = 32,
    NOT_TOKEN = 33,
    BT_INV_TOKEN = 34,
    ASSIGN_TOKEN = 35,
    SEMI_TOKEN = 36,
    COMMA_TOKEN = 37,
    COLON_TOKEN = 38,
    DOT_TOKEN = 39,
    SPREAD_OR_REST_TOKEN = 40,
    RANGE_TOKEN = 41,
    VAR_TOKEN = 42,
    TRUE_TOKEN = 43,
    NULL_TOKEN = 44,
    VOID_TOKEN = 45,
    FALSE_TOKEN = 46,
    RETURN_TOKEN = 47,
    WHILE_TILL_TOKEN = 48,
    WITH_TOKEN = 49,
    AS_TOKEN = 50,
    FROM_TOKEN = 51,
    UP_TOKEN = 52,
    DOWN_TOKEN = 53,
    FOR_LOOP_TOKEN = 54,
    IN_TOKEN = 55,
    YE_TOKEN = 56,
    DO_TOKEN = 57,
    CATCH_TOKEN = 58,
    THROW_TOKEN = 59,
    BECAUSE_TOKEN = 60,
    IMPORT_TOKEN = 61,
    CLASS_TOKEN = 62,
    STATIC_TOKEN = 63,
    INIT_TOKEN = 64,
    WHEN_TOKEN = 65,
    BREAK_TOKEN = 66,
    CONTINUE_TOKEN = 67,
    ID_TOKEN = 68,
    EOF_TOKEN = 69,
    EOL_TOKEN = 70,
    ERROR_TOKEN = 71,
    THIS_TOKEN = 72,
    SUPER_TOKEN = 73,
    EXTERN_TOKEN = 74,
    PLUS_EQ_TOKEN = 75,
    MINUS_EQ_TOKEN = 76,
    MULT_EQ_TOKEN = 77,
    DIV_EQ_TOKEN = 78,
    MOD_EQ_TOKEN = 79,
    INVALID_TOKEN = 80,
    NULLISH_TOKEN = 81,
    NULLCHECK_TOKEN = 82,
    AT_TOKEN = 83,
} TokenType;

typedef struct {

    TokenType type;
    // The beginning of the token, pointing directly into the source.
    const char *start;

    // The length of the token in characters.
    uint32_t length;

    // The 1-based line where the token appears.
    int line;

    // The parsed value if the token is a literal.
    Value value;

} Token;

Token newToken(TokenType type, const char *start, int length, int line, Value value);
void initToken(Token* token, TokenType type, const char *start, int length, int line, Value value);


#endif //CPMSC_TOKEN_H
