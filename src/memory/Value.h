//
// Created by Mahamadou DOUMBIA [OML DSI] on 11/01/2022.
//

#ifndef CPMSC_VALUE_H
#define CPMSC_VALUE_H

#include <stdint.h>

#include "../common/constants.h"
#include "../helpers/Helper.h"
#include "../common/common.h"
#include "../api/msc.h"
#include <memory.h>
#include <stdbool.h>


#define AS_CLASS(value)       ((Class*)AS_OBJ(value))            // Class*
#define AS_CLOSURE(value)     ((Closure*)AS_OBJ(value))          // Closure*
#define AS_DJURU(v)           ((Djuru*)AS_OBJ(v))                // Djuru*
#define AS_FUNCTION(value)    ((Function*)AS_OBJ(value))               // Function*
#define AS_EXTERN(v)         ((Extern*)AS_OBJ(v))                // Extern*
#define AS_INSTANCE(value)    ((Instance*)AS_OBJ(value))         // Instance*
#define AS_LIST(value)        ((List*)AS_OBJ(value))             // List*
#define AS_MAP(value)         ((Map*)AS_OBJ(value))              // Map*
#define AS_MODULE(value)      ((Module*)AS_OBJ(value))           // ObjModule*
#define AS_NUM(value)         (MSCValueToNum(value))                // double
#define AS_RANGE(v)         ((Range*)AS_OBJ(v))              // Range*
#define AS_STRING(v)          ((String*)AS_OBJ(v))               // String*
#define AS_CSTRING(v)         (AS_STRING(v)->value)              // const char*

#define BOOL_VAL(boolean) ((boolean) ? TRUE_VAL : FALSE_VAL)     // boolean
#define NUM_VAL(num) (MSCNumToValue(num))                           // double
#define OBJ_VAL(obj) (MSCObjectToValue((Object*)(obj)))       // Any Obj___*

// These perform type tests on a Value, returning `true` if the Value is of the
// given type.
#define IS_BOOL(value) (MSCIsBool(value))                      // Bool
#define IS_CLASS(value) (MSCIsObjType(value, OBJ_CLASS))       // Class
#define IS_CLOSURE(value) (MSCIsObjType(value, OBJ_CLOSURE))   // Closure
#define IS_FIBER(value) (MSCIsObjType(value, OBJ_THREAD))      // Djuru
#define IS_FN(value) (MSCIsObjType(value, OBJ_FN))             // Function
#define IS_FOREIGN(value) (MSCIsObjType(value, OBJ_EXTERN))    // Extern
#define IS_INSTANCE(value) (MSCIsObjType(value, OBJ_INSTANCE)) // Instance
#define IS_LIST(value) (MSCIsObjType(value, OBJ_LIST))         // List
#define IS_MAP(value) (MSCIsObjType(value, OBJ_MAP))           // Map
#define IS_MODULE(value) (MSCIsObjType(value, OBJ_MODULE))     // Module
#define IS_RANGE(value) (MSCIsObjType(value, OBJ_RANGE))       // Range
#define IS_STRING(value) (MSCIsObjType(value, OBJ_STRING))     // String

// Creates a new string object from [text], which should be a bare C string
// literal. This determines the length of the string automatically at compile
// time based on the size of the character array (-1 for the terminating '\0').
#define CONST_STRING(vm, text) MSCStringFromCharsWithLength((vm), (text), sizeof(text) - 1)

// forward declaration
typedef struct sClass Class;

// typedef struct sString String;


typedef enum {
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_THREAD,
    OBJ_FN,
    OBJ_EXTERN,
    OBJ_INSTANCE,
    OBJ_LIST,
    OBJ_MAP,
    OBJ_MODULE,
    OBJ_STRING,
    OBJ_UPVALUE,
    OBJ_RANGE
} ObjType;

typedef struct sObject Object;

struct sObject {
    bool isDark;
    ObjType type;
    Class *classObj;
    // The next object in the linked list of all currently allocated objects.
    struct sObject *next;
};

void MSCBlackenObject(Object *obj, MVM *vm);

void MSCGrayObject(Object *obj, MVM *vm);

void MSCFreeObject(Object *object, MVM *vm);

#if MSC_NAN_TAGGING

typedef uint64_t Value;

#else
typedef enum {
    VAL_FALSE,
    VAL_NULL,
    VAL_NUM,
    VAL_TRUE,
    VAL_UNDEFINED,
    VAL_OBJ
} ValueType;

typedef struct {

    /*Value();

    explicit Value(ValueType type);
    Value(ValueType type, double value);
    Value(ValueType type, Object* value);*/
    ValueType type;
    union {
        double num;
        Object *obj;
    } as;

} Value;

#endif


struct sString {
    Object obj;

    // Number of bytes in the string, not including the null terminator.
    uint32_t length;

    // The hash value of the string's contents.
    uint32_t hash;

    // Inline array of the string's bytes followed by a null terminator.
    char value[FLEXIBLE_ARRAY];
};


void MSCBlackenString(String *str, MVM *vm);


Value MSCStringFromCodePointAt(String *string, MVM *vm, uint32_t code);

Value MSCStringFromCharsWithLength(MVM *vm, const char *cstr, uint32_t length);

Value MSCStringFromRange(String *thisString, MVM *vm, int start, uint32_t count, int step);

uint32_t MSCStringFind(String *string, String *needle, uint32_t code);
// static members

Value MSCStringFromConstChars(MVM *vm, const char *cstr);


Value MSCStringFromNum(MVM *vm, double value);

Value MSCStringFromCodePoint(MVM *vm, int point);

Value MSCStringFromByte(MVM *vm, uint8_t byte);

Value MSCStringFormatted(MVM *vm, const char *format, ...);

String *MSCStringNew(MVM *vm, const char *text, uint32_t length);

String *MSCStringAllocate(MVM *vm, uint32_t length);

DECLARE_BUFFER(Value, Value);


/** End string related functions **/

typedef struct {
    Object obj;
    String *name;

    ValueBuffer variables;
    SymbolTable variableNames;
} Module;

Module *MSCModuleFrom(MVM *vm, String *name);

void MSCBlackenModule(Module *module, MVM *vm);

/** End Module related functions **/


typedef bool (*Primitive)(Djuru *vm, Value *args);

typedef struct {
    // The name of the function. Heap allocated and owned by the FnDebug.
    char *name;

    // An array of line numbers. There is one element in this array for each
    // bytecode in the function's bytecode array. The value of that element is
    // the line in the source code that generated that instruction.
    IntBuffer sourceLines;
} FnDebug;

typedef struct {
    Object obj;
    // The maximum number of stack slots this function may use.
    int maxSlots;
    // The number of upvalues this function closes over.
    int numUpvalues;
    int arity;

    Class* boundToClass;

    Module *module;
    FnDebug *debug;

    ByteBuffer code;
    ValueBuffer constants;
} Function;

Function *MSCFunctionFrom(MVM *vm, Module *module, int maxSlots);

void MSCFunctionBindName(Function *fn, MVM *vm, const char *debugName, int debugLength);

void MSCBlackenFunction(Function *fn, MVM *vm);

/** End of Function related functions **/

typedef struct sUpvalue Upvalue;

struct sUpvalue {
    Object obj;
    Value closed;

    Value *value;
    Upvalue *next;
};

Upvalue *MSCUpvalueFrom(MVM *vm, Value *value);

void MSCBlackenUpvalue(Upvalue *upvalue, MVM *vm);

/** End of Upvalue related functions **/

typedef struct {
    Object obj;
    Function *fn;
    Upvalue *upvalues[FLEXIBLE_ARRAY];


} Closure;

Closure *MSCClosureFrom(MVM *vm, Function *fn);

void MSCBlackenClosure(Closure *closure, MVM *vm);

/** End of Closure related functions **/

typedef struct {
    Object obj;
    bool isInclusive;
    double from;
    double to;
} Range;

Value MSCRangeFrom(MVM *vm, double start, double end, bool isInclusive);

void MSCBlackenRange(Range *range, MVM *vm);

/** End of Closure related functions **/

typedef enum {
    // A primitive method implemented in C in the VM. Unlike foreign methods,
    // this can directly manipulate the fiber's stack.
            METHOD_PRIMITIVE,

    // A primitive that handles .call on Fn.
            METHOD_FUNCTION_CALL,

    // A externally-defined C method.
            METHOD_EXTERN,

    // A normal user-defined method.
            METHOD_BLOCK,

    // No method for the given symbol.
            METHOD_NONE
} MethodType;

typedef struct {
    MethodType type;
    // The method function itself. The [type] determines which field of the union
    // is used.
    union {
        Primitive primitive;
        MSCExternMethodFn foreign;
        Closure *closure;
    } as;
} Method;

typedef struct {
    bool isStatic;
    Value defaultValue;
} Field;

DECLARE_BUFFER(Method, Method);

DECLARE_BUFFER(Field, Field);


struct sClass {
    Object obj;

    FieldBuffer fields;
    MethodBuffer methods;

    int numFields;
    Class *superclass;
    String *name;
    Value attributes;
};

Class *MSCSingleClass(MVM *vm, int numFields, String *name);

void MSCBindSuperclass(Class *classObj, MVM *vm, Class *superClass);

void MSCBindMethod(Class *classObj, MVM *vm, int symbol, Method method);

void MSCBindField(Class *classObj, MVM *vm, int symbol, Field method);

void MSCInitClass(Class *classObj, MVM *vm, String *name, int numOfFields);

Class *MSCClassFrom(MVM *vm, Class *parent, int numOfFields, String *name);

void MSCBlackenClass(Class *classObj, MVM *vm);

/** End of Class related functions **/

typedef struct {
    Object obj;

    uint8_t data[FLEXIBLE_ARRAY];

} Extern;

Extern *MSCExternFrom(MVM *vm, Class *objClass, size_t size);

void MSCBlackenExtern(Extern *externObj, MVM *vm);

/** End of Closure related functions **/

typedef struct {
    Object obj;
    Value fields[FLEXIBLE_ARRAY];
} Instance;

Instance *MSCInstanceFrom(MVM *vm, Class *iClass);

void MSCBlackenInstance(Instance *instance, MVM *vm);

/** End of Instance related functions **/

typedef struct {
    Object obj;

    ValueBuffer elements;
} List;

void MSCListInsert(List *list, MVM *vm, Value value, int index);

Value MSCListRemoveAt(List *list, MVM *vm, uint32_t index);

int MSCListIndexOf(List *list, MVM *vm, Value value);

void MSCBlackenList(List *list, MVM *vm);

List *MSCListFrom(MVM *vm, int numElements);

/** End of List related functions **/
typedef struct {
    // The entry's key, or UNDEFINED_VAL if the entry is not in use.
    Value key;

    // The value associated with the key. If the key is UNDEFINED_VAL, this will
    // be false to indicate an open available entry or true to indicate a
    // tombstone -- an entry that was previously in use but was then deleted.
    Value value;
} MapEntry;

typedef struct {
    Object obj;

    uint32_t capacity;
    uint32_t count;

    MapEntry *entries;

} Map;

Map *MSCMapFrom(MVM *vm);

void MSCBlackenMap(Map *map, MVM *vm);


void MSCMapResize(Map *map, MVM *vm, uint32_t capacity);

Value MSCMapGet(Map *map, Value key);

void MSCMapSet(Map *map, MVM *vm, Value key, Value value);

void MSCMapAddAll(Map *map, MVM *vm, Map *other);

void MSCMapClear(Map *map, MVM *vm);

Value MSCMapRemove(Map *map, MVM *vm, Value key);

/** End of Map related functions **/

typedef struct {
    uint8_t *ip;
    Closure *closure;
    Value *stackStart;

} CallFrame;


typedef enum {
    DJURU_TRY,
    DJURU_ROOT,
    DJURU_OTHER,
} DjuruState;

struct sDjuru {
    Object obj;

    DjuruState state;
    DjuruState entryState;
    Value error;

    // The stack of value slots. This is used for holding local variables and
    // temporaries while the thread is executing. It is heap-allocated and grown
    // as needed.
    Value *stack;

    // A pointer to one past the top-most value on the stack.
    Value *stackTop;
    Value *stackStart;
    // The number of allocated slots in the stack array.
    int stackCapacity;
    int numOfFrames;
    int frameCapacity;
    CallFrame *frames;
    Upvalue *openUpvalues;

    struct sDjuru *caller;

    MVM *vm;
};

Djuru *MSCDjuruFrom(MVM *vm, Closure *closure);

void MSCBlackenDjuru(Djuru *djuru, MVM *vm);

void MSCEnsureStack(Djuru *djuru, int needed);
void MSCEnsureFrames(Djuru* djuru, int needed);

void inline static MSCPushCallFrame(Djuru *djuru, Closure *closure, Value *stackStart) {
    MSCEnsureFrames(djuru, djuru->numOfFrames + 1);
    // The caller should have ensured we already have enough capacity.
    ASSERT(djuru->frameCapacity > djuru->numOfFrames, "No memory for call frame.");

    CallFrame *frame = &djuru->frames[djuru->numOfFrames++];
    frame->stackStart = djuru->stackStart = stackStart;
    frame->closure = closure;
    frame->ip = closure != NULL ? closure->fn->code.data: 0;

}
static inline void MSCPopCallFrame(Djuru *djuru) {
    ASSERT(djuru->numOfFrames > 0, "Frame stack underflow.");

    djuru->stackStart = djuru->numOfFrames > 1 ?
                        djuru->frames[djuru->numOfFrames - 1].stackStart :
                        djuru->stack;
    djuru->numOfFrames--;
}


/** End of Djuru related functions **/

// An IEEE 754 double-precision float is a 64-bit value with bits laid out like:
//
// 1 Sign bit
// | 11 Exponent bits
// | |          52 Mantissa (i.e. fraction) bits
// | |          |
// S[Exponent-][Mantissa------------------------------------------]
//
// The details of how these are used to represent numbers aren't really
// relevant here as long we don't interfere with them. The important bit is NaN.
//
// An IEEE double can represent a few magical values like NaN ("not a number"),
// Infinity, and -Infinity. A NaN is any value where all exponent bits are set:
//
//  v--NaN bits
// -11111111111----------------------------------------------------
//
// Here, "-" means "doesn't matter". Any bit sequence that matches the above is
// a NaN. With all of those "-", it obvious there are a *lot* of different
// bit patterns that all mean the same thing. NaN tagging takes advantage of
// this. We'll use those available bit patterns to represent things other than
// numbers without giving up any valid numeric values.
//
// NaN values come in two flavors: "signalling" and "quiet". The former are
// intended to halt execution, while the latter just flow through arithmetic
// operations silently. We want the latter. Quiet NaNs are indicated by setting
// the highest mantissa bit:
//
//             v--Highest mantissa bit
// -[NaN      ]1---------------------------------------------------
//
// If all of the NaN bits are set, it's not a number. Otherwise, it is.
// That leaves all of the remaining bits as available for us to play with. We
// stuff a few different kinds of things here: special singleton values like
// "true", "false", and "null", and pointers to objects allocated on the heap.
// We'll use the sign bit to distinguish singleton values from pointers. If
// it's set, it's a pointer.
//
// v--Pointer or singleton?
// S[NaN      ]1---------------------------------------------------
//
// For singleton values, we just enumerate the different values. We'll use the
// low bits of the mantissa for that, and only need a few:
//
//                                                 3 Type bits--v
// 0[NaN      ]1------------------------------------------------[T]
//
// For pointers, we are left with 51 bits of mantissa to store an address.
// That's more than enough room for a 32-bit address. Even 64-bit machines
// only actually use 48 bits for addresses, so we've got plenty. We just stuff
// the address right into the mantissa.
//
// Ta-da, double precision numbers, pointers, and a bunch of singleton values,
// all stuffed into a single 64-bit sequence. Even better, we don't have to
// do any masking or work to extract number values: they are unmodified. This
// means math on numbers is fast.
#if MSC_NAN_TAGGING

// A mask that selects the sign bit.
#define SIGN_BIT ((uint64_t)1 << 63)

// The bits that must be set to indicate a quiet NaN.
#define QNAN ((uint64_t)0x7ffc000000000000)

// If the NaN bits are set, it's not a number.
#define IS_NUM(value) (((value) & QNAN) != QNAN)

// An object pointer is a NaN with a set sign bit.
#define IS_OBJ(value) (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define IS_FALSE(value)     ((value) == FALSE_VAL)
#define IS_NULL(value)      ((value) == NULL_VAL)
#define IS_UNDEFINED(value) ((value) == UNDEFINED_VAL)

// Masks out the tag bits used to identify the singleton value.
#define MASK_TAG (7)

// Tag values for the different singleton values.
#define TAG_NAN       (0)
#define TAG_NULL      (1)
#define TAG_FALSE     (2)
#define TAG_TRUE      (3)
#define TAG_UNDEFINED (4)
#define TAG_UNUSED2   (5)
#define TAG_UNUSED3   (6)
#define TAG_UNUSED4   (7)

// Value -> 0 or 1.
#define AS_BOOL(value) ((value) == TRUE_VAL)

// Value -> Object*.
#define AS_OBJ(value) ((Object*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

// Singleton values.
#define NULL_VAL      ((Value)(uint64_t)(QNAN | TAG_NULL))
#define FALSE_VAL     ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL      ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define UNDEFINED_VAL ((Value)(uint64_t)(QNAN | TAG_UNDEFINED))

// Gets the singleton type tag for a Value (which must be a singleton).
#define GET_TAG(value) ((int)((value) & MASK_TAG))

#else

// Value -> 0 or 1.
#define AS_BOOL(value) ((value).type == VAL_TRUE)

// Value -> Object*.
#define AS_OBJ(v) ((v).as.obj)

// Determines if [value] is a garbage-collected object or not.
#define IS_OBJ(value) ((value).type == VAL_OBJ)

#define IS_FALSE(value)     ((value).type == VAL_FALSE)
#define IS_NULL(value)      ((value).type == VAL_NULL)
#define IS_NUM(value)       ((value).type == VAL_NUM)
#define IS_UNDEFINED(value) ((value).type == VAL_UNDEFINED)

// Singleton values.
#define FALSE_VAL     ((Value){ VAL_FALSE, { 0 } })
#define NULL_VAL      ((Value){ VAL_NULL, { 0 } })
#define TRUE_VAL      ((Value){ VAL_TRUE, { 0 } })
#define UNDEFINED_VAL ((Value){ VAL_UNDEFINED, { 0 } })

#endif


static inline bool MSCHasError(Djuru *djuru) {
    return !IS_NULL(djuru->error);
}


// Converts the raw object pointer [obj] to a [Value].
static inline Value MSCObjectToValue(Object *obj) {
#if MSC_NAN_TAGGING
    // The triple casting is necessary here to satisfy some compilers:
    // 1. (uintptr_t) Convert the pointer to a number of the right size.
    // 2. (uint64_t)  Pad it up to 64 bits in 32-bit builds.
    // 3. Or in the bits to make a tagged Nan.
    // 4. Cast to a typedef'd value.
    return (Value) (SIGN_BIT | QNAN | (uint64_t) (uintptr_t) (obj));
#else
    Value value;
    value.type = VAL_OBJ;
    value.as.obj = obj;
    return value;
#endif

}

// Interprets [value] as a [double].
static inline double MSCValueToNum(Value value) {
#if MSC_NAN_TAGGING
    return MSCDoubleFromBits(value);
#else
    return value.as.num;
#endif
}

// Converts [num] to a [Value].
static inline Value MSCNumToValue(double num) {
#if MSC_NAN_TAGGING
    return MSCDoubleToBits(num);
#else
    Value value;
    value.type = VAL_NUM;
    value.as.num = num;
    return value;
#endif
}

// Returns true if [value] is an object of type [type]. Do not call this
// directly, instead use the [IS___] macro for the type in question.
static inline bool MSCIsObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

// Returns true if [value] is a bool. Do not call this directly, instead use
// [IS_BOOL].
static inline bool MSCIsBool(Value value) {
#if MSC_NAN_TAGGING
    return value == TRUE_VAL || value == FALSE_VAL;
#else
    return value.type == VAL_FALSE || value.type == VAL_TRUE;
#endif
}

static inline bool MSCMapIsValidKey(Value arg) {
    return IS_BOOL(arg)
           || IS_CLASS(arg)
           || IS_NULL(arg)
           || IS_NUM(arg)
           || IS_STRING(arg);
}

// Returns true if [a] and [b] represent the same string.
static inline bool MSCStringEqualsCString(const String *a,
                                          const char *b, size_t length) {
    return a->length == length && memcmp(a->value, b, length) == 0;
}

static inline Method *MSCClassGetMethod(MVM* vm, const Class* classObj,
                                         int symbol)
{
    Method* method;
    if (symbol >= 0 && symbol < classObj->methods.count &&
        (method = &classObj->methods.data[symbol])->type != METHOD_NONE)
    {
        return method;
    }
    return NULL;
}

void MSCGrayValue(MVM *vm, Value value);

void MSCGrayBuffer(MVM *vm, ValueBuffer *buffer);

Class *MSCGetClass(MVM *vm, Value buffer);

static inline bool MSCValuesSame(Value a, Value b) {

#if MSC_NAN_TAGGING
    // Value types have unique bit representations and we compare object types
// by identity (i.e. pointer), so all we need to do is compare the bits.
    return a == b;
#else
    if (a.type != b.type) return false;
    if (a.type == VAL_NUM) return a.as.num == b.as.num;
    return a.as.obj == b.as.obj;
#endif

}

bool MSCValuesEqual(Value a, Value b);


#endif //CPMSC_VALUE_H
