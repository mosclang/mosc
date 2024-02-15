//
// Created by Mahamadou DOUMBIA [OML DSI] on 13/01/2022.
//


#include <math.h>
#include "Primitive.h"
#include "../runtime/MVM.h"

bool validateKey(Djuru *djuru, Value arg) {
    if (MSCMapIsValidKey(arg)) return true;

    RETURN_ERROR("Key must be a value type.");
}

bool validateFn(Djuru *djuru, Value arg, const char *argName) {
    if (IS_CLOSURE(arg)) return true;
    RETURN_ERROR_FMT("$ must be a function.", argName);
}

bool validateNum(Djuru *djuru, Value arg, const char *argName) {
    if (IS_NUM(arg)) return true;
    RETURN_ERROR_FMT("$ must be a number.", argName);
}

bool validateIntValue(Djuru *djuru, double value, const char *argName) {
    if (trunc(value) == value) return true;
    RETURN_ERROR_FMT("$ must be an integer.", argName);
}

bool validateInt(Djuru *djuru, Value arg, const char *argName) {
    // Make sure it's a number first.
    if (!validateNum(djuru, arg, argName)) return false;
    return validateIntValue(djuru, AS_NUM(arg), argName);
}

bool validateString(Djuru *djuru, Value arg, const char *argName) {
    if (IS_STRING(arg)) return true;
    RETURN_ERROR_FMT("$ must be a string.", argName);
}

static uint32_t validateIndexValue(Djuru* djuru, double value, uint32_t count, const char* argName) {
    if (!validateIntValue(djuru, value, argName)) return UINT32_MAX;

    // Negative indices count from the end.
    if (value < 0) value = count + value;

    // Check bounds.
    if (value >= 0 && value < count) return (uint32_t)value;

    djuru->error = MSCStringFormatted(djuru->vm, "$ out of bounds.", argName);
    return UINT32_MAX;
}
uint32_t validateIndex(Djuru* vm, Value arg, uint32_t count, const char* argName) {
    if (!validateNum(vm, arg, argName)) return UINT32_MAX;
    return validateIndexValue(vm, AS_NUM(arg), count, argName);
}

uint32_t calculateRange(Djuru* djuru, Range* range, uint32_t* length,
                        int* step)
{
    *step = 0;

    // Edge case: an empty range is allowed at the end of a sequence. This way,
    // list[0..-1] and list[0...list.count] can be used to copy a list even when
    // empty.
    if (range->from == *length &&
        range->to == (range->isInclusive ? -1.0 : (double)*length))
    {
        *length = 0;
        return 0;
    }

    uint32_t from = validateIndexValue(djuru, range->from, *length, "Range start");
    if (from == UINT32_MAX) return UINT32_MAX;

    // Bounds check the end manually to handle exclusive ranges.
    double value = range->to;
    if (!validateIntValue(djuru, value, "Range end")) return UINT32_MAX;

    // Negative indices count from the end.
    if (value < 0) value = *length + value;

    // Convert the exclusive range to an inclusive one.
    if (!range->isInclusive) {
        // An exclusive range with the same start and end points is empty.
        if (value == from)
        {
            *length = 0;
            return from;
        }

        // Shift the endpoint to make it inclusive, handling both increasing and
        // decreasing ranges.
        value += value >= from ? -1 : 1;
    }

    // Check bounds.
    if (value < 0 || value >= *length) {
        djuru->error = CONST_STRING(djuru->vm, "Range end out of bounds.");
        return UINT32_MAX;
    }

    uint32_t to = (uint32_t)value;
    *length = (uint32_t)(abs((int)(from - to)) + 1);
    *step = from < to ? 1 : -1;
    return from;
}