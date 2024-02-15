//
// Created by Mahamadou DOUMBIA [OML DSI] on 13/01/2022.
//

#ifndef CPMSC_PRIMIVITE_H
#define CPMSC_PRIMIVITE_H

#include "../memory/Value.h"


    // Binds a primitive method named [name] (in Msc) implemented using C function
// [fn] to `Class` [cls].
#define PRIMITIVE(cls, name, function)                                         \
    do                                                                         \
    {                                                                          \
      int symbol = MSCSymbolTableEnsure(vm,                                   \
          &vm->methodNames, name, strlen(name));                               \
      Method method;                                                           \
      method.type = METHOD_PRIMITIVE;                                          \
      method.as.primitive = prim_##function;                                   \
      MSCBindMethod(cls, vm, symbol, method);                                 \
    } while (false)

// Binds a primitive method named [name] (in Msc) implemented using C function
// [fn] to `Class` [cls], but as a FN call.
#define FUNCTION_CALL(cls, name, function)                                     \
    do                                                                         \
    {                                                                          \
      int symbol = MSCSymbolTableEnsure(vm,                                   \
          &vm->methodNames, name, strlen(name));                               \
      Method method;                                                           \
      method.type = METHOD_FUNCTION_CALL;                                      \
      method.as.primitive = prim_##function;                                   \
      MSCBindMethod(cls, vm, symbol, method);                                 \
    } while (false)

// Defines a primitive method whose C function name is [name]. This abstracts
// the actual type signature of a primitive function and makes it clear which C
// functions are invoked as primitives.
#define DEF_PRIMITIVE(name)                                                    \
    static bool prim_##name(Djuru* djuru, Value* args)

#define RETURN_VAL(value)                                                      \
    do                                                                         \
    {                                                                          \
      args[0] = value;                                                         \
      return true;                                                             \
    } while (false)

#define RETURN_OBJ(obj)     RETURN_VAL(OBJ_VAL(obj))
#define RETURN_BOOL(value)  RETURN_VAL(BOOL_VAL(value))
#define RETURN_FALSE        RETURN_VAL(FALSE_VAL)
#define RETURN_NULL         RETURN_VAL(NULL_VAL)
#define RETURN_NUM(value)   RETURN_VAL(NUM_VAL(value))
#define RETURN_TRUE         RETURN_VAL(TRUE_VAL)

#define RETURN_ERROR(msg)                                                      \
    do                                                                         \
    {                                                                          \
      djuru->error = MSCStringFromCharsWithLength(djuru->vm, msg, sizeof(msg) - 1);        \
      return false;                                                            \
    } while (false)

#define RETURN_ERROR_FMT(...)                                                  \
    do                                                                         \
    {                                                                          \
      djuru->error = MSCStringFormatted(djuru->vm, __VA_ARGS__);                    \
      return false;                                                            \
    } while (false)
// Validates that [arg] is a valid object for use as a map key. Returns true if
// it is. If not, reports an error and returns false.

    bool validateKey(Djuru *djuru, Value arg);
    // Validates that the given [arg] is a function. Returns true if it is. If not,
// reports an error and returns false.
    bool validateFn(Djuru* djuru, Value arg, const char* argName);

// Validates that the given [arg] is a Num. Returns true if it is. If not,
// reports an error and returns false.
    bool validateNum(Djuru* djuru, Value arg, const char* argName);

// Validates that [value] is an integer. Returns true if it is. If not, reports
// an error and returns false.
    bool validateIntValue(Djuru* djuru, double value, const char* argName);

// Validates that the given [arg] is an integer. Returns true if it is. If not,
// reports an error and returns false.
    bool validateInt(Djuru* djuru, Value arg, const char* argName);



// Validates that the argument at [argIndex] is an integer within `[0, count)`.
// Also allows negative indices which map backwards from the end. Returns the
// valid positive index value. If invalid, reports an error and returns
// `UINT32_MAX`.
    uint32_t validateIndex(Djuru* djuru, Value arg, uint32_t count, const char* argName);

// Validates that the given [arg] is a String. Returns true if it is. If not,
// reports an error and returns false.
    bool validateString(Djuru *djuru, Value arg, const char *argName);

    uint32_t calculateRange(Djuru* djuru, Range* range, uint32_t* length,
                           int* step);


#endif //CPMSC_PRIMIVITE_H
