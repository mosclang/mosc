//
// Created by Mahamadou DOUMBIA [OML DSI] on 11/01/2022.
//

#ifndef CPMSC_CONSTANTS_H
#define CPMSC_CONSTANTS_H

#define MAX_VARIABLE_NAME 64
#define MAX_FUNCTION_NAME 64
#define MAX_INTERPOLATION_NESTING 8
#define MAX_LOCALS 256
#define MAX_UPVALUES 256
#define MAX_PARAMETERS 16
#define MAX_JUMP 1 << 16
#define MAX_CONSTANTS 1 << 16
#define MAX_MODULE_VARS 1 << 16
#define ERROR_MESSAGE_SIZE 80 + MAX_VARIABLE_NAME + 15
#define MAX_FIELDS 1 << 16
#define MAP_LOAD_PERCENT 90
#define MAP_GROW_FACTOR 2
#define LIST_GROW_FACTOR 2
#define MAP_MIN_CAPACITY 16
// #define CLOCKS_PER_SEC 1000

// The maximum name of a method, not including the signature. This is an
// arbitrary but enforced maximum just so we know how long the method name
// strings need to be in the parser.
#define MAX_METHOD_NAME 64

#define MAX_METHOD_SIGNATURE (MAX_METHOD_NAME + (MAX_PARAMETERS * 2) + 6)


#endif //CPMSC_CONSTANTS_H
