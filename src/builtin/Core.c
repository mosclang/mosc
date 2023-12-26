//
// Created by Mahamadou DOUMBIA [OML DSI] on 13/01/2022.
//

#include "Core.h"
#include "Primitive.h"
#include "../runtime/MVM.h"
#include "../runtime/debuger.h"
#include "core/core.msc.inc"
#include "../memory/Value.h"


#include <float.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <time.h>


DEF_PRIMITIVE(bool_not) {
    RETURN_BOOL(!AS_BOOL(args[0]));
}

DEF_PRIMITIVE(bool_toString) {
    if (AS_BOOL(args[0])) {
        RETURN_VAL(CONST_STRING(vm, "tien"));
    } else {
        RETURN_VAL(CONST_STRING(vm, "galon"));
    }
}

DEF_PRIMITIVE(class_name) {
    RETURN_OBJ(AS_CLASS(args[0])->name);
}

DEF_PRIMITIVE(class_supertype) {
    Class *classObj = AS_CLASS(args[0]);

    // Object has no superclass.
    if (classObj->superclass == NULL) RETURN_NULL;

    RETURN_OBJ(classObj->superclass);
}

DEF_PRIMITIVE(class_toString) {
    RETURN_OBJ(AS_CLASS(args[0])->name);
}


DEF_PRIMITIVE(djuru_new) {
    if (!validateFn(vm, args[1], "Argument")) return false;

    Closure *closure = AS_CLOSURE(args[1]);
    if (closure->fn->arity > 1) {
        RETURN_ERROR("Function cannot take more than one parameter.");
    }

    RETURN_OBJ(MSCDjuruFrom(vm, closure));
}

DEF_PRIMITIVE(djuru_abort) {
    vm->djuru->error = args[1];

    // If the error is explicitly null, it's not really an abort.
    return IS_NULL(args[1]);
}

// Transfer execution to [djuru] coming from the current djuru whose stack has
// [args].
//
// [isCall] is true if [djuru] is being called and not transferred.
//
// [hasValue] is true if a value in [args] is being passed to the new djuru.
// Otherwise, `null` is implicitly being passed.
static bool runDjuru(MVM *vm, Djuru *djuru, const Value *args, bool isCall,
                     bool hasValue, const char *verb) {

    if (MSCHasError(djuru)) {
        RETURN_ERROR_FMT("Cannot $ an aborted djuru.", verb);
    }

    if (isCall) {
        // You can't call a called djuru, but you can transfer directly to it,
        // which is why this check is gated on `isCall`. This way, after resuming a
        // suspended djuru, it will run and then return to the djuru that called it
        // and so on.
        if (djuru->caller != NULL) RETURN_ERROR("Djuru has already been called.");

        if (djuru->state == DJURU_ROOT) RETURN_ERROR("Cannot call root djuru.");

        // Remember who ran it.
        djuru->caller = vm->djuru;
    }

    if (djuru->numOfFrames == 0) {
        RETURN_ERROR_FMT("Cannot $ a finished djuru.", verb);
    }

    // When the calling djuru resumes, we'll store the result of the call in its
    // stack. If the call has two arguments (the djuru and the value), we only
    // need one slot for the result, so discard the other slot now.
    if (hasValue) vm->djuru->stackTop--;

    if (djuru->numOfFrames == 1 &&
        djuru->frames[0].ip == djuru->frames[0].closure->fn->code.data) {
        // The djuru is being started for the first time. If its function takes a
        // parameter, bind an argument to it.
        if (djuru->frames[0].closure->fn->arity == 1) {
            djuru->stackTop[0] = hasValue ? args[1] : NULL_VAL;
            djuru->stackTop++;
        }
    } else {
        // The djuru is being resumed, make yield() or transfer() return the result.
        djuru->stackTop[-1] = hasValue ? args[1] : NULL_VAL;
    }

    vm->djuru = djuru;
    return false;
}

DEF_PRIMITIVE(djuru_call) {
    return runDjuru(vm, AS_DJURU(args[0]), args, true, false, "call");
}

DEF_PRIMITIVE(djuru_call1) {
    return runDjuru(vm, AS_DJURU(args[0]), args, true, true, "call");
}

DEF_PRIMITIVE(djuru_current) {
    RETURN_OBJ(vm->djuru);
}

DEF_PRIMITIVE(djuru_error) {
    RETURN_VAL(AS_DJURU(args[0])->error);
}

DEF_PRIMITIVE(djuru_isDone) {
    Djuru *running = AS_DJURU(args[0]);
    RETURN_BOOL(running->numOfFrames == 0 || MSCHasError(running));
}

DEF_PRIMITIVE(djuru_suspend) {
    // Switching to a null djuru tells the interpreter to stop and exit.
    vm->djuru = NULL;
    vm->apiStack = NULL;
    return false;
}

DEF_PRIMITIVE(djuru_transfer) {
    return runDjuru(vm, AS_DJURU(args[0]), args, false, false, "transfer to");
}

DEF_PRIMITIVE(djuru_transfer1) {
    return runDjuru(vm, AS_DJURU(args[0]), args, false, true, "transfer to");
}

DEF_PRIMITIVE(djuru_transferError) {
    runDjuru(vm, AS_DJURU(args[0]), args, false, true, "transfer to");
    vm->djuru->error = args[1];
    return false;
}

DEF_PRIMITIVE(djuru_try) {
    runDjuru(vm, AS_DJURU(args[0]), args, true, false, "try");

    // If we're switching to a valid djuru to try, remember that we're trying it.
    if (!MSCHasError(vm->djuru)) vm->djuru->state = DJURU_TRY;
    return false;
}

DEF_PRIMITIVE(djuru_try1) {
    runDjuru(vm, AS_DJURU(args[0]), args, true, true, "try");

    // If we're switching to a valid djuru to try, remember that we're trying it.
    if (!MSCHasError(vm->djuru)) vm->djuru->state = DJURU_TRY;
    return false;
}

DEF_PRIMITIVE(djuru_yield) {
    Djuru *current = vm->djuru;
    vm->djuru = current->caller;

    // Unhook this djuru from the one that called it.
    current->caller = NULL;
    current->state = DJURU_OTHER;

    if (vm->djuru != NULL) {
        // Make the caller's run method return null.
        vm->djuru->stackTop[-1] = NULL_VAL;
    }

    return false;
}

DEF_PRIMITIVE(djuru_yield1) {
    Djuru *current = vm->djuru;
    vm->djuru = current->caller;

    // Unhook this djuru from the one that called it.
    current->caller = NULL;
    current->state = DJURU_OTHER;

    if (vm->djuru != NULL) {
        // Make the caller's run method return the argument passed to yield.
        vm->djuru->stackTop[-1] = args[1];

        // When the yielding djuru resumes, we'll store the result of the yield
        // call in its stack. Since Djuru.segin(value) has two arguments (the Djuru
        // class and the value) and we only need one slot for the result, discard
        // the other slot now.
        current->stackTop--;
    }

    return false;
}

DEF_PRIMITIVE(fn_new) {
    if (!validateFn(vm, args[1], "Argument")) return false;

    // The block argument is already a function, so just return it.
    RETURN_VAL(args[1]);
}

DEF_PRIMITIVE(fn_arity) {
    RETURN_NUM(AS_CLOSURE(args[0])->fn->arity);
}

static void call_fn(MVM *vm, const Value *args, int numArgs) {
    // +1 to include the function itself.
    callFunction(vm, vm->djuru, AS_CLOSURE(args[0]), numArgs + 1);
}

#define DEF_FN_CALL(numArgs)                                                   \
    DEF_PRIMITIVE(fn_call##numArgs)                                            \
    {                                                                          \
      call_fn(vm, args, numArgs);                                              \
      return false;                                                            \
    }

DEF_FN_CALL(0)

DEF_FN_CALL(1)

DEF_FN_CALL(2)

DEF_FN_CALL(3)

DEF_FN_CALL(4)

DEF_FN_CALL(5)

DEF_FN_CALL(6)

DEF_FN_CALL(7)

DEF_FN_CALL(8)

DEF_FN_CALL(9)

DEF_FN_CALL(10)

DEF_FN_CALL(11)

DEF_FN_CALL(12)

DEF_FN_CALL(13)

DEF_FN_CALL(14)

DEF_FN_CALL(15)

DEF_FN_CALL(16)

DEF_PRIMITIVE(fn_toString) {
    RETURN_VAL(CONST_STRING(vm, "<tii>"));
}

// Creates a new list of size args[1], with all elements initialized to args[2].
DEF_PRIMITIVE(list_filled) {
    if (!validateInt(vm, args[1], "Size")) return false;
    if (AS_NUM(args[1]) < 0) RETURN_ERROR("Size cannot be negative.");

    uint32_t size = (uint32_t) AS_NUM(args[1]);
    List *list = MSCListFrom(vm, size);

    for (uint32_t i = 0; i < size; i++) {
        list->elements.data[i] = args[2];
    }

    RETURN_OBJ(list);
}

DEF_PRIMITIVE(list_new) {
    RETURN_OBJ(MSCListFrom(vm, 0));
}

DEF_PRIMITIVE(list_add) {
    MSCWriteValueBuffer(vm, &AS_LIST(args[0])->elements, args[1]);
    RETURN_VAL(args[1]);
}

// Adds an element to the list and then returns the list itself. This is called
// by the compiler when compiling list literals instead of using add() to
// minimize stack churn.
DEF_PRIMITIVE(list_addCore) {
    MSCWriteValueBuffer(vm, &AS_LIST(args[0])->elements, args[1]);

    // Return the list.
    RETURN_VAL(args[0]);
}

// Adds an other list element to the list and then returns the list itself. This is called
// by the compiler when compiling list literals instead of using addAll() to
// minimize stack churn.
DEF_PRIMITIVE(list_addAllCore) {
    if (!IS_LIST(args[1])) RETURN_ERROR("Spread element should be a list instance");
    List *other = AS_LIST(args[1]);
    for (int i = 0; i < other->elements.count; i++) {
        MSCWriteValueBuffer(vm, &AS_LIST(args[0])->elements, other->elements.data[i]);
    }
    // Return the list.
    RETURN_VAL(args[0]);
}

DEF_PRIMITIVE(list_clear) {
    MSCFreeValueBuffer(vm, &AS_LIST(args[0])->elements);
    RETURN_NULL;
}

DEF_PRIMITIVE(list_count) {
    RETURN_NUM(AS_LIST(args[0])->elements.count);
}

DEF_PRIMITIVE(list_insert) {
    List *list = AS_LIST(args[0]);

    // count + 1 here so you can "insert" at the very end.
    uint32_t index = validateIndex(vm, args[1], (uint32_t) list->elements.count + 1, "Index");
    if (index == UINT32_MAX) return false;

    MSCListInsert(list, vm, args[2], index);
    RETURN_VAL(args[2]);
}

DEF_PRIMITIVE(list_iterate) {
    List *list = AS_LIST(args[0]);
    if (!validateInt(vm, args[2], "Step")) return false;
    int32_t step = (int32_t) AS_NUM(args[2]);

    // If we're starting the iteration, return the first index.
    if (IS_NULL(args[1])) {
        if (list->elements.count == 0) RETURN_FALSE;

        if (step > 0) RETURN_NUM(0);
        else
            RETURN_NUM(list->elements.count - 1);
    }

    if (!validateInt(vm, args[1], "Iterator")) return false;

    // Stop if we're out of bounds.
    double index = AS_NUM(args[1]);
    // double absIndex = step > 0 ? index: -index;
    if ((step > 0 && (index < 0 || index >= list->elements.count - 1)) || (step < 0 && (index < 1 || index > list->elements.count - 1))) RETURN_FALSE;

    // Otherwise, move to the next index.
    RETURN_NUM(index + step);
}

DEF_PRIMITIVE(list_iteratorValue) {
    List *list = AS_LIST(args[0]);
    uint32_t index = validateIndex(vm, args[1], (uint32_t) list->elements.count, "Iterator");
    if (index == UINT32_MAX) return false;
    RETURN_VAL(list->elements.data[index]);
}

DEF_PRIMITIVE(list_removeAt) {
    List *list = AS_LIST(args[0]);
    uint32_t index = validateIndex(vm, args[1], (uint32_t) list->elements.count, "Index");
    if (index == UINT32_MAX) return false;

    RETURN_VAL(MSCListRemoveAt(list, vm, index));
}

DEF_PRIMITIVE(list_removeValue) {
    List *list = AS_LIST(args[0]);
    int index = MSCListIndexOf(list, vm, args[1]);
    if (index == -1) RETURN_NULL;
    RETURN_VAL(MSCListRemoveAt(list, vm, index));
}

DEF_PRIMITIVE(list_indexOf) {
    List *list = AS_LIST(args[0]);
    RETURN_NUM(MSCListIndexOf(list, vm, args[1]));
}

DEF_PRIMITIVE(list_swap) {
    List *list = AS_LIST(args[0]);
    uint32_t indexA = validateIndex(vm, args[1], (uint32_t) list->elements.count, "Index 0");
    if (indexA == UINT32_MAX) return false;
    uint32_t indexB = validateIndex(vm, args[2], (uint32_t) list->elements.count, "Index 1");
    if (indexB == UINT32_MAX) return false;

    Value a = list->elements.data[indexA];
    list->elements.data[indexA] = list->elements.data[indexB];
    list->elements.data[indexB] = a;

    RETURN_NULL;
}

DEF_PRIMITIVE(list_subscript) {
    List *list = AS_LIST(args[0]);

    if (IS_NUM(args[1])) {
        uint32_t index = validateIndex(vm, args[1], (uint32_t) list->elements.count,
                                       "Subscript");
        if (index == UINT32_MAX) return false;

        RETURN_VAL(list->elements.data[index]);
    }
    if (!IS_RANGE(args[1])) {
        RETURN_ERROR("Subscript must be a number or a range.");
    }

    int step;
    uint32_t count = (uint32_t) list->elements.count;
    uint32_t start = calculateRange(vm, AS_RANGE(args[1]), &count, &step);
    if (start == UINT32_MAX) return false;

    List *result = MSCListFrom(vm, count);
    for (uint32_t i = 0; i < count; i++) {
        result->elements.data[i] = list->elements.data[start + i * step];
    }
    RETURN_OBJ(result);

}

DEF_PRIMITIVE(list_subscriptSetter) {
    List *list = AS_LIST(args[0]);
    uint32_t index = validateIndex(vm, args[1], (uint32_t) (list->elements.count),
                                   "Subscript");
    if (index == UINT32_MAX) return false;

    list->elements.data[index] = args[2];
    RETURN_VAL(args[2]);
}

DEF_PRIMITIVE(map_new) {
    RETURN_OBJ(MSCMapFrom(vm));
}

DEF_PRIMITIVE(map_subscript) {
    if (!validateKey(vm, args[1])) return false;

    Map *map = AS_MAP(args[0]);
    Value value = MSCMapGet(map, args[1]);
    if (IS_UNDEFINED(value)) RETURN_NULL;

    RETURN_VAL(value);
}

DEF_PRIMITIVE(map_subscriptSetter) {
    if (!validateKey(vm, args[1])) return false;

    MSCMapSet(AS_MAP(args[0]), vm, args[1], args[2]);
    RETURN_VAL(args[2]);
}

// Adds an entry to the map and then returns the map itself. This is called by
// the compiler when compiling map literals instead of using [_]=(_) to
// minimize stack churn.
DEF_PRIMITIVE(map_addCore) {
    if (!validateKey(vm, args[1])) return false;

    MSCMapSet(AS_MAP(args[0]), vm, args[1], args[2]);

    // Return the map itself.
    RETURN_VAL(args[0]);
}

DEF_PRIMITIVE(map_addAllCore) {
    Map *map = AS_MAP(args[0]);
    if (!IS_MAP(args[1])) {
        RETURN_ERROR("Only map can be spread in an other map");
    }
    Map *other = AS_MAP(args[1]);

    MSCMapAddAll(map, vm, other);
    // Return the map itself.
    RETURN_VAL(args[0]);
}

DEF_PRIMITIVE(map_clear) {
    MSCMapClear(AS_MAP(args[0]), vm);
    RETURN_NULL;
}

DEF_PRIMITIVE(map_containsKey) {
    if (!validateKey(vm, args[1])) return false;

    RETURN_BOOL(!IS_UNDEFINED(MSCMapGet(AS_MAP(args[0]), args[1])));
}

DEF_PRIMITIVE(map_count) {
    RETURN_NUM(AS_MAP(args[0])->count);
}

DEF_PRIMITIVE(map_iterate) {
    Map *map = AS_MAP(args[0]);
    if (!validateInt(vm, args[2], "Step")) return false;
    int32_t step = (int32_t) AS_NUM(args[2]);

    if (map->count == 0) RETURN_FALSE;

    // If we're starting the iteration, start at the first or the last used entry.
    int index = step < 0 ? map->capacity - 1 : 0;

    // Otherwise, start one past the last entry we stopped at or the first entry.
    if (!IS_NULL(args[1])) {
        if (!validateInt(vm, args[1], "Iterator")) return false;

        // if (AS_NUM(args[1]) <= 0) RETURN_FALSE;
        index = (int) AS_NUM(args[1]);
        if (index < 0 || index >= map->capacity) RETURN_FALSE;

        // Advance the iterator.
        index += step;
    }

    // Find a used entry, if any.
    if (step > 0) {
        for (; index < map->capacity; index++) {

            if (!IS_UNDEFINED(map->entries[index].key)) {
                RETURN_NUM(index);
            }
        }
    } else {
        for (; index >= 0; index--) {
            if (!IS_UNDEFINED(map->entries[index].key)) RETURN_NUM(index);
        }
    }

    // If we get here, walked all of the entries.
    RETURN_FALSE;
}

DEF_PRIMITIVE(map_remove) {
    if (!validateKey(vm, args[1])) return false;

    RETURN_VAL(MSCMapRemove(AS_MAP(args[0]), vm, args[1]));
}

DEF_PRIMITIVE(map_keyIteratorValue) {
    Map *map = AS_MAP(args[0]);
    uint32_t index = validateIndex(vm, args[1], map->capacity, "Iterator");
    if (index == UINT32_MAX) return false;

    MapEntry *entry = &map->entries[index];
    if (IS_UNDEFINED(entry->key)) {
        RETURN_ERROR("Invalid map iterator.");
    }

    RETURN_VAL(entry->key);
}

DEF_PRIMITIVE(map_valueIteratorValue) {
    Map *map = AS_MAP(args[0]);
    uint32_t index = validateIndex(vm, args[1], map->capacity, "Iterator");
    if (index == UINT32_MAX) return false;

    MapEntry *entry = &map->entries[index];
    if (IS_UNDEFINED(entry->key)) {
        RETURN_ERROR("Invalid map iterator.");
    }
    RETURN_VAL(entry->value);
}

DEF_PRIMITIVE(null_not) {
    RETURN_VAL(TRUE_VAL);
}

DEF_PRIMITIVE(null_toString) {
    RETURN_VAL(CONST_STRING(vm, "gansan"));
}

DEF_PRIMITIVE(num_fromString) {
    if (!validateString(vm, args[1], "Argument")) return false;

    String *string = AS_STRING(args[1]);

    // Corner case: Can't parse an empty string.
    if (string->length == 0) RETURN_NULL;

    errno = 0;
    char *end;
    double number = strtod(string->value, &end);

    // Skip past any trailing whitespace.
    while (*end != '\0' && isspace((unsigned char) *end)) end++;

    if (errno == ERANGE) RETURN_ERROR("Number literal is too large.");

    // We must have consumed the entire string. Otherwise, it contains non-number
    // characters and we can't parse it.
    if (end < string->value + string->length) RETURN_NULL;

    RETURN_NUM(number);
}

// Defines a primitive on Num that calls infix [op] and returns [type].
#define DEF_NUM_CONSTANT(name, value)                                          \
    DEF_PRIMITIVE(num_##name)                                                  \
    {                                                                          \
      RETURN_NUM(value);                                                       \
    }

DEF_NUM_CONSTANT(infinity, INFINITY)

DEF_NUM_CONSTANT(nan, MSC_DOUBLE_NAN)

DEF_NUM_CONSTANT(pi, 3.14159265358979323846264338327950288)

DEF_NUM_CONSTANT(tau, 6.28318530717958647692528676655900577)

DEF_NUM_CONSTANT(largest, DBL_MAX)

DEF_NUM_CONSTANT(smallest, DBL_MIN)

DEF_NUM_CONSTANT(maxSafeInteger, 9007199254740991.0)

DEF_NUM_CONSTANT(minSafeInteger, -9007199254740991.0)

// Defines a primitive on Num that calls infix [op] and returns [type].
#define DEF_NUM_INFIX(name, op, type)                                          \
    DEF_PRIMITIVE(num_##name)                                                  \
    {                                                                          \
      if (!validateNum(vm, args[1], "Right operand")) return false;            \
      RETURN_##type(AS_NUM(args[0]) op AS_NUM(args[1]));                       \
    }

DEF_NUM_INFIX(minus, -, NUM)

DEF_NUM_INFIX(plus, +, NUM)

DEF_NUM_INFIX(multiply, *, NUM)

DEF_NUM_INFIX(divide, /, NUM)

DEF_NUM_INFIX(lt, <, BOOL)

DEF_NUM_INFIX(gt, >, BOOL)

DEF_NUM_INFIX(lte, <=, BOOL)

DEF_NUM_INFIX(gte, >=, BOOL)

// Defines a primitive on Num that call infix bitwise [op].
#define DEF_NUM_BITWISE(name, op)                                              \
    DEF_PRIMITIVE(num_bitwise##name)                                           \
    {                                                                          \
      if (!validateNum(vm, args[1], "Right operand")) return false;            \
      uint32_t left = (uint32_t)AS_NUM(args[0]);                               \
      uint32_t right = (uint32_t)AS_NUM(args[1]);                              \
      RETURN_NUM(left op right);                                               \
    }

DEF_NUM_BITWISE(And, &)

DEF_NUM_BITWISE(Or, |)

DEF_NUM_BITWISE(Xor, ^)

DEF_NUM_BITWISE(LeftShift, <<)

DEF_NUM_BITWISE(RightShift, >>)

// Defines a primitive method on Num that returns the result of [fn].
#define DEF_NUM_FN(name, fn)                                                   \
    DEF_PRIMITIVE(num_##name)                                                  \
    {                                                                          \
      RETURN_NUM(fn(AS_NUM(args[0])));                                         \
    }

DEF_NUM_FN(abs, fabs)

DEF_NUM_FN(acos, acos)

DEF_NUM_FN(asin, asin)

DEF_NUM_FN(atan, atan)

DEF_NUM_FN(cbrt, cbrt)

DEF_NUM_FN(ceil, ceil)

DEF_NUM_FN(cos, cos)

DEF_NUM_FN(floor, floor)

DEF_NUM_FN(unary_minus, -)

DEF_NUM_FN(unary_plus, +)

DEF_NUM_FN(round, round)

DEF_NUM_FN(sin, sin)

DEF_NUM_FN(sqrt, sqrt)

DEF_NUM_FN(tan, tan)

DEF_NUM_FN(log, log)

DEF_NUM_FN(log2, log2)

DEF_NUM_FN(exp, exp)

DEF_PRIMITIVE(num_mod) {
    if (!validateNum(vm, args[1], "Right operand")) return false;
    RETURN_NUM(fmod(AS_NUM(args[0]), AS_NUM(args[1])));
}

DEF_PRIMITIVE(num_eqeq) {
    if (!IS_NUM(args[1])) RETURN_FALSE;
    RETURN_BOOL(AS_NUM(args[0]) == AS_NUM(args[1]));
}

DEF_PRIMITIVE(num_bangeq) {
    if (!IS_NUM(args[1])) RETURN_TRUE;
    RETURN_BOOL(AS_NUM(args[0]) != AS_NUM(args[1]));
}

DEF_PRIMITIVE(num_bitwiseNot) {
    // Bitwise operators always work on 32-bit unsigned ints.
    RETURN_NUM(~(uint32_t) AS_NUM(args[0]));
}

DEF_PRIMITIVE(num_dotDot) {
    if (!validateNum(vm, args[1], "Right hand side of range")) return false;

    double from = AS_NUM(args[0]);
    double to = AS_NUM(args[1]);
    RETURN_VAL(MSCRangeFrom(vm, from, to, true));
}

DEF_PRIMITIVE(num_dotDotDot) {
    if (!validateNum(vm, args[1], "Right hand side of range")) return false;

    double from = AS_NUM(args[0]);
    double to = AS_NUM(args[1]);
    RETURN_VAL(MSCRangeFrom(vm, from, to, false));
}

DEF_PRIMITIVE(num_atan2) {
    if (!validateNum(vm, args[1], "x value")) return false;

    RETURN_NUM(atan2(AS_NUM(args[0]), AS_NUM(args[1])));
}

DEF_PRIMITIVE(num_min) {
    if (!validateNum(vm, args[1], "Other value")) return false;

    double value = AS_NUM(args[0]);
    double other = AS_NUM(args[1]);
    RETURN_NUM(value <= other ? value : other);
}

DEF_PRIMITIVE(num_max) {
    if (!validateNum(vm, args[1], "Other value")) return false;

    double value = AS_NUM(args[0]);
    double other = AS_NUM(args[1]);
    RETURN_NUM(value > other ? value : other);
}

DEF_PRIMITIVE(num_clamp) {
    if (!validateNum(vm, args[1], "Min value")) return false;
    if (!validateNum(vm, args[2], "Max value")) return false;

    double value = AS_NUM(args[0]);
    double min = AS_NUM(args[1]);
    double max = AS_NUM(args[2]);
    double result = (value < min) ? min : ((value > max) ? max : value);
    RETURN_NUM(result);
}

DEF_PRIMITIVE(num_pow) {
    if (!validateNum(vm, args[1], "Power value")) return false;

    RETURN_NUM(pow(AS_NUM(args[0]), AS_NUM(args[1])));
}

DEF_PRIMITIVE(num_fraction) {
    double unused;
    RETURN_NUM(modf(AS_NUM(args[0]), &unused));
}

DEF_PRIMITIVE(num_isInfinity) {
    RETURN_BOOL(isinf(AS_NUM(args[0])));
}

DEF_PRIMITIVE(num_isInteger) {
    double value = AS_NUM(args[0]);
    if (isnan(value) || isinf(value)) RETURN_FALSE;
    RETURN_BOOL(trunc(value) == value);
}

DEF_PRIMITIVE(num_isNan) {
    RETURN_BOOL(isnan(AS_NUM(args[0])));
}

DEF_PRIMITIVE(num_sign) {
    double value = AS_NUM(args[0]);
    if (value > 0) {
        RETURN_NUM(1);
    } else if (value < 0) {
        RETURN_NUM(-1);
    } else {
        RETURN_NUM(0);
    }
}

DEF_PRIMITIVE(num_toString) {
    RETURN_VAL(MSCStringFromNum(vm, AS_NUM(args[0])));
}

DEF_PRIMITIVE(num_truncate) {
    double integer;
    modf(AS_NUM(args[0]), &integer);
    RETURN_NUM(integer);
}

DEF_PRIMITIVE(object_same) {
    RETURN_BOOL(MSCValuesEqual(args[1], args[2]));
}

DEF_PRIMITIVE(object_not) {
    RETURN_VAL(FALSE_VAL);
}

DEF_PRIMITIVE(object_eqeq) {
    RETURN_BOOL(MSCValuesEqual(args[0], args[1]));
}

DEF_PRIMITIVE(object_bangeq) {
    RETURN_BOOL(!MSCValuesEqual(args[0], args[1]));
}

DEF_PRIMITIVE(object_is) {

    if (!IS_CLASS(args[1])) {
        RETURN_ERROR("Right operand must be a class.");
    }

    Class *classObj = MSCGetClass(vm, args[0]);
    Class *baseClassObj = AS_CLASS(args[1]);

    // Walk the superclass chain looking for the class.
    do {
        if (baseClassObj == classObj) RETURN_BOOL(true);

        classObj = classObj->superclass;
    } while (classObj != NULL);

    RETURN_BOOL(false);
}

DEF_PRIMITIVE(object_toString) {
    Object *obj = AS_OBJ(args[0]);
    Value name = OBJ_VAL(obj->classObj->name);
    RETURN_VAL(MSCStringFormatted(vm, "instance of @", name));
}

DEF_PRIMITIVE(object_type) {
    RETURN_OBJ(MSCGetClass(vm, args[0]));
}

DEF_PRIMITIVE(range_from) {
    RETURN_NUM(AS_RANGE(args[0])->from);
}

DEF_PRIMITIVE(range_to) {
    RETURN_NUM(AS_RANGE(args[0])->to);
}

DEF_PRIMITIVE(range_min) {
    Range *range = AS_RANGE(args[0]);
    RETURN_NUM(fmin(range->from, range->to));
}

DEF_PRIMITIVE(range_max) {
    Range *range = AS_RANGE(args[0]);
    RETURN_NUM(fmax(range->from, range->to));
}

DEF_PRIMITIVE(range_isInclusive) {
    RETURN_BOOL(AS_RANGE(args[0])->isInclusive);
}

DEF_PRIMITIVE(range_iterate) {
    Range *range = AS_RANGE(args[0]);
    if (!validateInt(vm, args[2], "Step")) return false;
    int32_t step = (int32_t) AS_NUM(args[2]);

    // Special case: empty range.
    if (range->from == range->to && !range->isInclusive) RETURN_FALSE;

    // Start the iteration.
    if (IS_NULL(args[1])) {
        if (step >= 0) RETURN_NUM(range->from);
        else {
            if (range->isInclusive) {
                RETURN_NUM(range->to);
            }
            RETURN_NUM((range->to > range->from ? range->to - 1 : range->to + 1));
        }
    }

    if (!validateNum(vm, args[1], "Iterator")) return false;

    double iterator = AS_NUM(args[1]);

    // Iterate towards [to] from [from].
    if (range->from < range->to) {
        iterator += step;
        if ((step >= 0 && iterator > range->to) || (step < 0 && iterator < range->from)) {
            RETURN_FALSE;
        }
    } else {
        iterator -= step;
        if ((step >= 0 && iterator < range->to) || (step < 0 && iterator > range->from)) {
            RETURN_FALSE;
        }
    }

    if (!range->isInclusive && iterator == range->to) RETURN_FALSE;

    RETURN_NUM(iterator);
}

DEF_PRIMITIVE(range_iteratorValue) {
    // Assume the iterator is a number so that is the value of the range.
    RETURN_VAL(args[1]);
}

DEF_PRIMITIVE(range_toString) {
    Range *range = AS_RANGE(args[0]);

    Value from = MSCStringFromNum(vm, range->from);
    MSCPushRoot(vm->gc, AS_OBJ(from));

    Value to = MSCStringFromNum(vm, range->to);
    MSCPushRoot(vm->gc, AS_OBJ(to));

    Value result = MSCStringFormatted(vm, "@$@", from,
                                      range->isInclusive ? ".." : "...", to);

    MSCPopRoot(vm->gc);
    MSCPopRoot(vm->gc);
    RETURN_VAL(result);
}


DEF_PRIMITIVE(string_fromCodePoint) {
    if (!validateInt(vm, args[1], "Code point")) return false;

    int codePoint = (int) AS_NUM(args[1]);
    if (codePoint < 0) {
        RETURN_ERROR("Code point cannot be negative.");
    } else if (codePoint > 0x10ffff) {
        RETURN_ERROR("Code point cannot be greater than 0x10ffff.");
    }

    RETURN_VAL(MSCStringFromCodePoint(vm, codePoint));
}

DEF_PRIMITIVE(string_fromByte) {
    if (!validateInt(vm, args[1], "Byte")) return false;
    int byte = (int) AS_NUM(args[1]);
    if (byte < 0) {
        RETURN_ERROR("Byte cannot be negative.");
    } else if (byte > 0xff) {
        RETURN_ERROR("Byte cannot be greater than 0xff.");
    }
    RETURN_VAL(MSCStringFromByte(vm, (uint8_t) byte));
}

DEF_PRIMITIVE(string_byteAt) {
    String *string = AS_STRING(args[0]);

    uint32_t index = validateIndex(vm, args[1], string->length, "Index");
    if (index == UINT32_MAX) return false;

    RETURN_NUM((uint8_t) string->value[index]);
}

DEF_PRIMITIVE(string_byteCount) {
    RETURN_NUM(AS_STRING(args[0])->length);
}

DEF_PRIMITIVE(string_codePointAt) {
    String *string = AS_STRING(args[0]);

    uint32_t index = validateIndex(vm, args[1], string->length, "Index");
    if (index == UINT32_MAX) return false;

    // If we are in the middle of a UTF-8 sequence, indicate that.
    const uint8_t *bytes = (uint8_t *) string->value;
    if ((bytes[index] & 0xc0) == 0x80) RETURN_NUM(-1);

    // Decode the UTF-8 sequence.
    RETURN_NUM(MSCUtf8Decode((uint8_t *) string->value + index,
                             string->length - index));
}

DEF_PRIMITIVE(string_contains) {
    if (!validateString(vm, args[1], "Argument")) return false;

    String *string = AS_STRING(args[0]);
    String *search = AS_STRING(args[1]);

    RETURN_BOOL(MSCStringFind(string, search, 0) != UINT32_MAX);
}

DEF_PRIMITIVE(string_endsWith) {
    if (!validateString(vm, args[1], "Argument")) return false;

    String *string = AS_STRING(args[0]);
    String *search = AS_STRING(args[1]);

    // Edge case: If the search string is longer then return false right away.
    if (search->length > string->length) RETURN_FALSE;

    RETURN_BOOL(memcmp(string->value + string->length - search->length,
                       search->value, search->length) == 0);
}

DEF_PRIMITIVE(string_indexOf1) {
    if (!validateString(vm, args[1], "Argument")) return false;

    String *string = AS_STRING(args[0]);
    String *search = AS_STRING(args[1]);

    uint32_t index = MSCStringFind(string, search, 0);
    RETURN_NUM(index == UINT32_MAX ? -1 : (int) index);
}

DEF_PRIMITIVE(string_indexOf2) {
    if (!validateString(vm, args[1], "Argument")) return false;

    String *string = AS_STRING(args[0]);
    String *search = AS_STRING(args[1]);
    uint32_t start = validateIndex(vm, args[2], string->length, "Start");
    if (start == UINT32_MAX) return false;

    uint32_t index = MSCStringFind(string, search, start);
    RETURN_NUM(index == UINT32_MAX ? -1 : (int) index);
}

DEF_PRIMITIVE(string_iterate) {
    String *string = AS_STRING(args[0]);
    if (!validateInt(vm, args[2], "Step")) return false;
    int32_t step = (int32_t) AS_NUM(args[2]);

    // If we're starting the iteration, return the first index.
    if (IS_NULL(args[1])) {
        if (string->length == 0) RETURN_FALSE;
        if (step >= 0) RETURN_NUM(0);
        else
            RETURN_NUM(string->length - 1);
    }

    if (!validateInt(vm, args[1], "Iterator")) return false;

    if (AS_NUM(args[1]) < 0) RETURN_FALSE;
    uint32_t index = (uint32_t) AS_NUM(args[1]);

    // Advance to the beginning of the next UTF-8 sequence.
    int stepAbs = step > 0 ? step : -step;
    for (int j = 0; j < stepAbs; j++) {
        do {
            if (step >= 0) index++;
            else index--;
            if (index >= string->length) RETURN_FALSE;
        } while ((string->value[index] & 0xc0) == 0x80);
    }

    RETURN_NUM(index);
}

DEF_PRIMITIVE(string_iterateByte) {
    String *string = AS_STRING(args[0]);
    if (!validateInt(vm, args[2], "Step")) return false;
    int32_t step = (int32_t) AS_NUM(args[2]);
    // If we're starting the iteration, return the first index.
    if (IS_NULL(args[1])) {
        if (string->length == 0) RETURN_FALSE;
        if (step >= 0) RETURN_NUM(0);
        else
            RETURN_NUM(string->length - 1);
    }

    if (!validateInt(vm, args[1], "Iterator")) return false;

    if (AS_NUM(args[1]) < 0) RETURN_FALSE;
    uint32_t index = (uint32_t) AS_NUM(args[1]);

    // Advance to the next byte.
    index += step;
    if (index >= string->length) RETURN_FALSE;

    RETURN_NUM(index);
}

DEF_PRIMITIVE(string_iteratorValue) {
    String *string = AS_STRING(args[0]);
    uint32_t index = validateIndex(vm, args[1], string->length, "Iterator");
    if (index == UINT32_MAX) return false;

    RETURN_VAL(MSCStringFromCodePointAt(string, vm, index));
}

DEF_PRIMITIVE(string_startsWith) {
    if (!validateString(vm, args[1], "Argument")) return false;

    String *string = AS_STRING(args[0]);
    String *search = AS_STRING(args[1]);

    // Edge case: If the search string is longer then return false right away.
    if (search->length > string->length) RETURN_FALSE;

    RETURN_BOOL(memcmp(string->value, search->value, search->length) == 0);
}

DEF_PRIMITIVE(string_plus) {
    if (!validateString(vm, args[1], "Right operand")) return false;
    RETURN_VAL(MSCStringFormatted(vm, "@@", args[0], args[1]));
}

DEF_PRIMITIVE(string_subscript) {
    String *string = AS_STRING(args[0]);

    if (IS_NUM(args[1])) {
        uint32_t index = validateIndex(vm, args[1], string->length, "Subscript");
        if (index == -1) return false;

        RETURN_VAL(MSCStringFromCodePointAt(string, vm, index));
    }

    if (!IS_RANGE(args[1])) {
        RETURN_ERROR("Subscript must be a number or a range.");
    }

    int step;
    uint32_t count = string->length;
    int start = calculateRange(vm, AS_RANGE(args[1]), &count, &step);
    if (start == -1) return false;

    RETURN_VAL(MSCStringFromRange(string, vm, start, count, step));
}

DEF_PRIMITIVE(string_toString) {
    RETURN_VAL(args[0]);
}

DEF_PRIMITIVE(string_compareTo)
{
    if (!validateString(vm, args[1], "Argument")) return false;

    String* str1 = AS_STRING(args[0]);
    String* str2 = AS_STRING(args[1]);

    size_t len1 = str1->length;
    size_t len2 = str2->length;

    // Get minimum length for comparison.
    size_t minLen = (len1 <= len2) ? len1 : len2;
    int res = memcmp(str1->value, str2->value, minLen);

    // If result is non-zero, just return that.
    if (res) RETURN_NUM(res);

    // If the lengths are the same, the strings must be equal
    if (len1 == len2) RETURN_NUM(0);

    // Otherwise the shorter string will come first.
    res = (len1 < len2) ? -1 : 1;
    RETURN_NUM(res);
}


DEF_PRIMITIVE(system_clock) {
    RETURN_NUM((double) clock() / CLOCKS_PER_SEC);
}

DEF_PRIMITIVE(system_gc) {
    MSCCollectGarbage(vm);
    RETURN_NULL;
}

DEF_PRIMITIVE(system_writeString) {
    if (vm->config.writeFn != NULL) {
        vm->config.writeFn(vm, AS_CSTRING(args[1]));
    }

    RETURN_VAL(args[1]);
}


// Creates either the Object or Class class in the core module with [name].
Class *defineClass(MVM *vm, Module *module, const char *name) {

    String *nameString = AS_STRING(MSCStringFromConstChars(vm, name));
    MSCPushRoot(vm->gc, (Object *) nameString);
    Class *classObj = MSCSingleClass(vm, 0, nameString);

    MSCDefineVariable(vm, module, name, nameString->length, OBJ_VAL(classObj), NULL);
    // printf("Count::::::---- %d\n", module->variableNames.count);
    // String** ptr = module->variableNames.data;
    // printf("Var:: [%s]\n", (*ptr)->value);
    /*while((ptr++) < module->variableNames.data + module->variableNames.count) {
        printf("Var:: %s", (*ptr)->value);
    }*/
    MSCPopRoot(vm->gc);
    return classObj;
}

static void showMethods(MVM *vm, Class *obj, const char *name) {
    printf("%s methods: [", name);
    for (int i = 0; i < obj->methods.count; i++) {
        printf("%d to %s; ", i, vm->methodNames.data[i]->value);
    }
    printf("]\n");
}

void load(MVM *vm) {
    Module *coreModule = MSCModuleFrom(vm, NULL);
    MSCPushRoot(vm->gc, (Object *) coreModule);
    // The core module's key is null in the module map.
    MSCMapSet(vm->modules, vm, NULL_VAL, OBJ_VAL(coreModule));
    MSCPopRoot(vm->gc); // coreModule.

    // Define the root Object class. This has to be done a little specially
    // because it has no superclass.
    vm->core.objectClass = defineClass(vm, coreModule, "Baa");

    PRIMITIVE(vm->core.objectClass, "!", object_not);
    PRIMITIVE(vm->core.objectClass, "==(_)", object_eqeq);
    PRIMITIVE(vm->core.objectClass, "!=(_)", object_bangeq);
    PRIMITIVE(vm->core.objectClass, "ye(_)", object_is);
    PRIMITIVE(vm->core.objectClass, "sebenma", object_toString);
    PRIMITIVE(vm->core.objectClass, "suku", object_type);

    // Now we can define Class, which is a subclass of Object.
    vm->core.classClass = defineClass(vm, coreModule, "Kulu");
    MSCBindSuperclass(vm->core.classClass, vm, vm->core.objectClass);

    PRIMITIVE(vm->core.classClass, "togo", class_name);
    PRIMITIVE(vm->core.classClass, "fakulu", class_supertype);
    PRIMITIVE(vm->core.classClass, "sebenma", class_toString);


    // Finally, we can define Object's metaclass which is a subclass of Class.
    Class *objectMetaclass = defineClass(vm, coreModule, "Fen metaclass");

    // Wire up the metaclass relationships now that all three classes are built.
    vm->core.objectClass->obj.classObj = objectMetaclass;
    objectMetaclass->obj.classObj = vm->core.classClass;
    vm->core.classClass->obj.classObj = vm->core.classClass;

    // Do vm->core after wiring up the metaclasses so objectMetaclass doesn't get
    // collected.
    MSCBindSuperclass(objectMetaclass, vm, vm->core.classClass);

    PRIMITIVE(objectMetaclass, "sukuKelen(_,_)", object_same);
    MSCInterpret(vm, NULL, coreModuleSource);


    vm->core.boolClass = AS_CLASS(MSCFindVariable(vm, coreModule, "Tienya"));
    PRIMITIVE(vm->core.boolClass, "sebenma", bool_toString);
    PRIMITIVE(vm->core.boolClass, "!", bool_not);

    vm->core.djuruClass = AS_CLASS(MSCFindVariable(vm, coreModule, "Djuru"));
    PRIMITIVE(vm->core.djuruClass->obj.classObj, "kura(_)", djuru_new);
    PRIMITIVE(vm->core.djuruClass->obj.classObj, "tike(_)", djuru_abort);
    PRIMITIVE(vm->core.djuruClass->obj.classObj, "sissanTa", djuru_current);
    PRIMITIVE(vm->core.djuruClass->obj.classObj, "djo()", djuru_suspend);
    PRIMITIVE(vm->core.djuruClass->obj.classObj, "mine()", djuru_yield);
    PRIMITIVE(vm->core.djuruClass->obj.classObj, "mine(_)", djuru_yield1);
    PRIMITIVE(vm->core.djuruClass, "weele()", djuru_call);
    PRIMITIVE(vm->core.djuruClass, "weele(_)", djuru_call1);
    PRIMITIVE(vm->core.djuruClass, "fili", djuru_error);
    PRIMITIVE(vm->core.djuruClass, "ok", djuru_isDone);
    PRIMITIVE(vm->core.djuruClass, "alaTeme()", djuru_transfer);
    PRIMITIVE(vm->core.djuruClass, "alaTeme(_)", djuru_transfer1);
    PRIMITIVE(vm->core.djuruClass, "filiLaTeme(_)", djuru_transferError);
    PRIMITIVE(vm->core.djuruClass, "aladie()", djuru_try);
    PRIMITIVE(vm->core.djuruClass, "aladie(_)", djuru_try1);

    vm->core.fnClass = AS_CLASS(MSCFindVariable(vm, coreModule, "Tii"));
    PRIMITIVE(vm->core.fnClass->obj.classObj, "kura(_)", fn_new);

    PRIMITIVE(vm->core.fnClass, "arity", fn_arity);

    FUNCTION_CALL(vm->core.fnClass, "weele()", fn_call0);
    FUNCTION_CALL(vm->core.fnClass, "weele(_)", fn_call1);
    FUNCTION_CALL(vm->core.fnClass, "weele(_,_)", fn_call2);
    FUNCTION_CALL(vm->core.fnClass, "weele(_,_,_)", fn_call3);
    FUNCTION_CALL(vm->core.fnClass, "weele(_,_,_,_)", fn_call4);
    FUNCTION_CALL(vm->core.fnClass, "weele(_,_,_,_,_)", fn_call5);
    FUNCTION_CALL(vm->core.fnClass, "weele(_,_,_,_,_,_)", fn_call6);
    FUNCTION_CALL(vm->core.fnClass, "weele(_,_,_,_,_,_,_)", fn_call7);
    FUNCTION_CALL(vm->core.fnClass, "weele(_,_,_,_,_,_,_,_)", fn_call8);
    FUNCTION_CALL(vm->core.fnClass, "weele(_,_,_,_,_,_,_,_,_)", fn_call9);
    FUNCTION_CALL(vm->core.fnClass, "weele(_,_,_,_,_,_,_,_,_,_)", fn_call10);
    FUNCTION_CALL(vm->core.fnClass, "weele(_,_,_,_,_,_,_,_,_,_,_)", fn_call11);
    FUNCTION_CALL(vm->core.fnClass, "weele(_,_,_,_,_,_,_,_,_,_,_,_)", fn_call12);
    FUNCTION_CALL(vm->core.fnClass, "weele(_,_,_,_,_,_,_,_,_,_,_,_,_)", fn_call13);
    FUNCTION_CALL(vm->core.fnClass, "weele(_,_,_,_,_,_,_,_,_,_,_,_,_,_)", fn_call14);
    FUNCTION_CALL(vm->core.fnClass, "weele(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_)", fn_call15);
    FUNCTION_CALL(vm->core.fnClass, "weele(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_)", fn_call16);

    PRIMITIVE(vm->core.fnClass, "sebenma", fn_toString);

    vm->core.nullClass = AS_CLASS(MSCFindVariable(vm, coreModule, "Gansan"));
    PRIMITIVE(vm->core.nullClass, "!", null_not);
    PRIMITIVE(vm->core.nullClass, "sebenma", null_toString);

    vm->core.numClass = AS_CLASS(MSCFindVariable(vm, coreModule, "Diat"));
    PRIMITIVE(vm->core.numClass->obj.classObj, "kaboSebenna(_)", num_fromString);
    PRIMITIVE(vm->core.numClass->obj.classObj, "INF", num_infinity);
    PRIMITIVE(vm->core.numClass->obj.classObj, "NAN", num_nan);
    PRIMITIVE(vm->core.numClass->obj.classObj, "PI", num_pi);
    PRIMITIVE(vm->core.numClass->obj.classObj, "TAU", num_tau);
    PRIMITIVE(vm->core.numClass->obj.classObj, "BELEBELE", num_largest);
    PRIMITIVE(vm->core.numClass->obj.classObj, "FITINI", num_smallest);
    PRIMITIVE(vm->core.numClass->obj.classObj, "maxSafeInteger", num_maxSafeInteger);
    PRIMITIVE(vm->core.numClass->obj.classObj, "minSafeInteger", num_minSafeInteger);
    PRIMITIVE(vm->core.numClass, "-(_)", num_minus);
    PRIMITIVE(vm->core.numClass, "+(_)", num_plus);
    PRIMITIVE(vm->core.numClass, "*(_)", num_multiply);
    PRIMITIVE(vm->core.numClass, "/(_)", num_divide);
    PRIMITIVE(vm->core.numClass, "<(_)", num_lt);
    PRIMITIVE(vm->core.numClass, ">(_)", num_gt);
    PRIMITIVE(vm->core.numClass, "<=(_)", num_lte);
    PRIMITIVE(vm->core.numClass, ">=(_)", num_gte);
    PRIMITIVE(vm->core.numClass, "&(_)", num_bitwiseAnd);
    PRIMITIVE(vm->core.numClass, "|(_)", num_bitwiseOr);
    PRIMITIVE(vm->core.numClass, "^(_)", num_bitwiseXor);
    PRIMITIVE(vm->core.numClass, "<<(_)", num_bitwiseLeftShift);
    PRIMITIVE(vm->core.numClass, ">>(_)", num_bitwiseRightShift);

    PRIMITIVE(vm->core.numClass, "abs", num_abs);
    PRIMITIVE(vm->core.numClass, "acos", num_acos);
    PRIMITIVE(vm->core.numClass, "asin", num_asin);
    PRIMITIVE(vm->core.numClass, "atan", num_atan);
    PRIMITIVE(vm->core.numClass, "cbrt", num_cbrt);
    PRIMITIVE(vm->core.numClass, "ceil", num_ceil);
    PRIMITIVE(vm->core.numClass, "cos", num_cos);
    PRIMITIVE(vm->core.numClass, "floor", num_floor);
    PRIMITIVE(vm->core.numClass, "-", num_unary_minus);
    PRIMITIVE(vm->core.numClass, "+", num_unary_plus);
    PRIMITIVE(vm->core.numClass, "round", num_round);
    PRIMITIVE(vm->core.numClass, "min(_)", num_min);
    PRIMITIVE(vm->core.numClass, "max(_)", num_max);
    PRIMITIVE(vm->core.numClass, "clamp(_,_)", num_clamp);
    PRIMITIVE(vm->core.numClass, "sin", num_sin);
    PRIMITIVE(vm->core.numClass, "sqrt", num_sqrt);
    PRIMITIVE(vm->core.numClass, "tan", num_tan);
    PRIMITIVE(vm->core.numClass, "log", num_log);
    PRIMITIVE(vm->core.numClass, "log2", num_log2);
    PRIMITIVE(vm->core.numClass, "exp", num_exp);
    PRIMITIVE(vm->core.numClass, "%(_)", num_mod);
    PRIMITIVE(vm->core.numClass, "~", num_bitwiseNot);
    PRIMITIVE(vm->core.numClass, "..(_)", num_dotDot);
    PRIMITIVE(vm->core.numClass, "...(_)", num_dotDotDot);
    PRIMITIVE(vm->core.numClass, "atan(_)", num_atan2);
    PRIMITIVE(vm->core.numClass, "pow(_)", num_pow);
    PRIMITIVE(vm->core.numClass, "fraction", num_fraction);
    PRIMITIVE(vm->core.numClass, "yeInf", num_isInfinity);
    PRIMITIVE(vm->core.numClass, "yeInt", num_isInteger);
    PRIMITIVE(vm->core.numClass, "yeNaN", num_isNan);
    PRIMITIVE(vm->core.numClass, "tagamasere", num_sign);
    PRIMITIVE(vm->core.numClass, "truncate", num_truncate);

    PRIMITIVE(vm->core.numClass, "sebenma", num_toString);

    // These are defined just so that 0 and -0 are equal, which is specified by
    // IEEE 754 even though they have different bit representations.
    PRIMITIVE(vm->core.numClass, "==(_)", num_eqeq);
    PRIMITIVE(vm->core.numClass, "!=(_)", num_bangeq);

    vm->core.stringClass = AS_CLASS(MSCFindVariable(vm, coreModule, "Seben"));
    PRIMITIVE(vm->core.stringClass->obj.classObj, "kaboCodePointna(_)", string_fromCodePoint);
    PRIMITIVE(vm->core.stringClass->obj.classObj, "kaboBytela(_)", string_fromByte);
    PRIMITIVE(vm->core.stringClass, "+(_)", string_plus);
    PRIMITIVE(vm->core.stringClass, "[_]", string_subscript);
    PRIMITIVE(vm->core.stringClass, "byteYoro_(_)", string_byteAt);
    PRIMITIVE(vm->core.stringClass, "byteHakan_", string_byteCount);
    PRIMITIVE(vm->core.stringClass, "codePointYoro_(_)", string_codePointAt);
    PRIMITIVE(vm->core.stringClass, "bAkono(_)", string_contains);
    PRIMITIVE(vm->core.stringClass, "beBanNiinAye(_)", string_endsWith);
    PRIMITIVE(vm->core.stringClass, "aDayoro(_)", string_indexOf1);
    PRIMITIVE(vm->core.stringClass, "uDayoro(_,_)", string_indexOf2);
    PRIMITIVE(vm->core.stringClass, "iterate(_,_)", string_iterate);
    PRIMITIVE(vm->core.stringClass, "iterateByte_(_,_)", string_iterateByte);
    PRIMITIVE(vm->core.stringClass, "iteratorValue(_)", string_iteratorValue);
    PRIMITIVE(vm->core.stringClass, "beDamineNiin(_)", string_startsWith);
    PRIMITIVE(vm->core.stringClass, "sebenma", string_toString);
    PRIMITIVE(vm->core.stringClass, "sunma(_)", string_compareTo);

    vm->core.listClass = AS_CLASS(MSCFindVariable(vm, coreModule, "Walan"));
    PRIMITIVE(vm->core.listClass->obj.classObj, "lafaa(_,_)", list_filled);
    PRIMITIVE(vm->core.listClass->obj.classObj, "kura()", list_new);
    PRIMITIVE(vm->core.listClass, "[_]", list_subscript);
    PRIMITIVE(vm->core.listClass, "[_]=(_)", list_subscriptSetter);
    PRIMITIVE(vm->core.listClass, "aFaraAkan(_)", list_add);
    PRIMITIVE(vm->core.listClass, "aFaraAkan_(_)", list_addCore);
    PRIMITIVE(vm->core.listClass, "aBeeFaraAkan_(_)", list_addAllCore);
    PRIMITIVE(vm->core.listClass, "diossi()", list_clear);
    PRIMITIVE(vm->core.listClass, "hakan", list_count);
    PRIMITIVE(vm->core.listClass, "aDon(_,_)", list_insert);
    PRIMITIVE(vm->core.listClass, "iterate(_,_)", list_iterate);
    PRIMITIVE(vm->core.listClass, "iteratorValue(_)", list_iteratorValue);
    PRIMITIVE(vm->core.listClass, "aBoOyorola(_)", list_removeAt);
    PRIMITIVE(vm->core.listClass, "aBoye(_)", list_removeValue);
    PRIMITIVE(vm->core.listClass, "aDayoro(_)", list_indexOf);
    PRIMITIVE(vm->core.listClass, "falen(_,_)", list_swap);// swap

    vm->core.mapClass = AS_CLASS(MSCFindVariable(vm, coreModule, "Wala"));
    PRIMITIVE(vm->core.mapClass->obj.classObj, "kura()", map_new);
    PRIMITIVE(vm->core.mapClass, "[_]", map_subscript);
    PRIMITIVE(vm->core.mapClass, "[_]=(_)", map_subscriptSetter);
    PRIMITIVE(vm->core.mapClass, "aFaraAkan_(_,_)", map_addCore);
    PRIMITIVE(vm->core.mapClass, "aBeeFaraAkan_(_)", map_addAllCore);
    PRIMITIVE(vm->core.mapClass, "diossi()", map_clear);
    PRIMITIVE(vm->core.mapClass, "bAkono(_)", map_containsKey);
    PRIMITIVE(vm->core.mapClass, "hakan", map_count);// count
    PRIMITIVE(vm->core.mapClass, "aBoye(_)", map_remove);
    PRIMITIVE(vm->core.mapClass, "iterate(_,_)", map_iterate);
    PRIMITIVE(vm->core.mapClass, "keyIteratorValue_(_)", map_keyIteratorValue);
    PRIMITIVE(vm->core.mapClass, "valueIteratorValue_(_)", map_valueIteratorValue);

    vm->core.rangeClass = AS_CLASS(MSCFindVariable(vm, coreModule, "Funan"));
    PRIMITIVE(vm->core.rangeClass, "kabo", range_from);
    PRIMITIVE(vm->core.rangeClass, "kata", range_to);
    PRIMITIVE(vm->core.rangeClass, "fitini", range_min);
    PRIMITIVE(vm->core.rangeClass, "dan", range_max);
    PRIMITIVE(vm->core.rangeClass, "kunBala", range_isInclusive);
    PRIMITIVE(vm->core.rangeClass, "iterate(_,_)", range_iterate);
    PRIMITIVE(vm->core.rangeClass, "iteratorValue(_)", range_iteratorValue);
    PRIMITIVE(vm->core.rangeClass, "sebenma", range_toString);


    Class *systemClass = AS_CLASS(MSCFindVariable(vm, coreModule, "A"));
    PRIMITIVE(systemClass->obj.classObj, "waati()", system_clock);
    PRIMITIVE(systemClass->obj.classObj, "gc()", system_gc);
    PRIMITIVE(systemClass->obj.classObj, "seben_(_)", system_writeString);
    /*
    showMethods(vm, vm->core.objectClass, "Baa");
    showMethods(vm, vm->core.classClass, "Kulu");
    showMethods(vm, vm->core.numClass, "Diat");
    showMethods(vm, vm->core.stringClass, "Seben");
     */

    // While bootstrapping the core types and running the core module, a number
    // of string objects have been created, many of which were instantiated
    // before stringClass was stored in the VM. Some of them *must* be created
    // first -- the ObjClass for string itself has a reference to the ObjString
    // for its name.
    //
    // These all currently have a NULL classObj pointer, so go back and assign
    // them now that the string class is known.
    for (Object *obj = vm->gc->first; obj != NULL; obj = obj->next) {
        if (obj->type == OBJ_STRING) obj->classObj = vm->core.stringClass;
    }
}

void MSCInitCore(Core *core, MVM *vm) {
    core->stringClass = NULL;
    core->objectClass = NULL;
    core->fnClass = NULL;
    core->djuruClass = NULL;
    core->numClass = NULL;
    core->classClass = NULL;
    core->listClass = NULL;
    core->boolClass = NULL;
    core->mapClass = NULL;
    core->nullClass = NULL;
    core->rangeClass = NULL;
    load(vm);
}


