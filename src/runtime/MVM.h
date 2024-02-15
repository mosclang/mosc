//
// Created by Mahamadou DOUMBIA [OML DSI] on 11/01/2022.
//

#ifndef CPMSC_MVM_H
#define CPMSC_MVM_H


#include "../memory/Value.h"
#include "../memory/GC.h"
#include "../compiler/Compiler.h"
#include "../builtin/Core.h"
#include "../api/msc.h"


struct MSCHandle {
    Value value;
    struct MSCHandle *prev;
    struct MSCHandle *next;
};

struct MVM {


    Core core;
    MSCConfig config;
    SymbolTable methodNames;
    Compiler *compiler;
    Djuru *djuru;
    Map *modules;
    Module *lastModule;
    GC *gc;
    MSCHandle *handles;

};

void MSCFinalizeExtern(MVM *vm, Extern *externObj);

MSCHandle* MSCMakeHandle(Djuru* vm, Value value);

int MSCGetMapCount(Djuru *vm, int slot);


int MSCDefineVariable(MVM *vm, Module *module, const char *name, int nameLength, Value value, int *line);


int MSCDeclareVariable(MVM *vm, Module *module, const char *name, size_t length, int line);

Closure *MSCCompileSource(MVM *vm, const char *module, const char *source, bool isExpression, bool printErrors);


Value MSCGetModuleVariable(MVM* vm, Value moduleName, Value variableName);
Module *MSCGetModule(MVM *vm, Value name);

Value MSCFindVariable(MVM *vm, Module *module, const char *name);

Value* MSCSlotAtUnsafe(Djuru* vm, int slot);

// Returns the class of [value].
//
// Defined here instead of in Value.h because it's critical that this be
// inlined. That means it must be defined in the header, but the Value.h
// header doesn't have a full definitely of MVM yet.
static inline Class *MSCGetClassInline(MVM *vm, Value value) {

    if (IS_NUM(value)) return vm->core.numClass;
    if (IS_OBJ(value)) return AS_OBJ(value)->classObj;

#if MSC_NAN_TAGGING
    switch (GET_TAG(value)) {
        case TAG_FALSE:
            return vm->core.boolClass;
            break;
        case TAG_NAN:
            return vm->core.numClass;
            break;
        case TAG_NULL:
            return vm->core.nullClass;
            break;
        case TAG_TRUE:
            return vm->core.boolClass;
            break;
        case TAG_UNDEFINED:
            UNREACHABLE();
    }
#else
    switch (value.type) {
        case VAL_FALSE:
            return vm->core.boolClass;
        case VAL_NULL:
            return vm->core.nullClass;
        case VAL_NUM:
            return vm->core.numClass;
        case VAL_TRUE:
            return vm->core.boolClass;
        case VAL_OBJ:
            return AS_OBJ(value)->classObj;
        case VAL_UNDEFINED:
            UNREACHABLE();
    }
#endif

    UNREACHABLE();
    return NULL;
}
// Ensures that [slot] is a valid index into the API's stack of slots.

static inline void validateApiSlot(Djuru *djuru, int slot) {
    ASSERT(slot >= 0, "Slot cannot be negative.");
    ASSERT(slot < MSCGetSlotCount(djuru), "Not that many slots.");
}

// Pushes [closure] onto [fiber]'s callstack to invoke it. Expects [numArgs]
// arguments (including the receiver) to be on the top of the stack already.
static inline void callFunction(Djuru *djuru,
                                Closure *closure, int numArgs) {
    // Grow the stack if needed.
    int stackSize = (int) (djuru->stackTop - djuru->stack);
    int needed = stackSize + closure->fn->maxSlots;
    MSCEnsureStack(djuru, needed);
    MSCPushCallFrame(djuru, closure, djuru->stackTop - numArgs);
}

static inline bool isFalsyValue(Value value) {
    return IS_FALSE(value) || IS_NULL(value);
}

static inline Value MSCGetSlot(Djuru* djuru, int slot) {
    validateApiSlot(djuru, slot);
    return djuru->stackStart[slot];
}

static inline void MSCSetSlot(Djuru* djuru, int slot, Value value) {
    validateApiSlot(djuru, slot);
    djuru->stackStart[slot] = value;
}

#endif //CPMSC_MVM_H
