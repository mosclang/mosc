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
    Value *apiStack;

};

void MSCFinalizeExtern(MVM *vm, Extern *externObj);

MSCHandle* MSCMakeHandle(MVM* vm, Value value);

int MSCGetMapCount(MVM *vm, int slot);


int MSCDefineVariable(MVM *vm, Module *module, const char *name, int nameLength, Value value, int *line);


int MSCDeclareVariable(MVM *vm, Module *module, const char *name, size_t length, int line);

Closure *MSCCompileSource(MVM *vm, const char *module, const char *source, bool isExpression, bool printErrors);


Value MSCGetModuleVariable(MVM* vm, Value moduleName, Value variableName);
Module *MSCGetModule(MVM *vm, Value name);

Value MSCFindVariable(MVM *vm, Module *module, const char *name);

Value* MSCSlotAtUnsafe(MVM* vm, int slot);

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

// Pushes [closure] onto [fiber]'s callstack to invoke it. Expects [numArgs]
// arguments (including the receiver) to be on the top of the stack already.
static inline void callFunction(MVM *vm, Djuru *djuru,
                                Closure *closure, int numArgs) {
    // Grow the call frame array if needed.
    if (djuru->numOfFrames + 1 > djuru->frameCapacity) {
        int max = djuru->frameCapacity * 2;
        djuru->frames = (CallFrame *) MSCReallocate(vm->gc, djuru->frames,
                                                    sizeof(CallFrame) * djuru->frameCapacity, sizeof(CallFrame) * max);
        djuru->frameCapacity = max;
    }

    // Grow the stack if needed.
    int stackSize = (int) (djuru->stackTop - djuru->stack);
    int needed = stackSize + closure->fn->maxSlots;
    MSCEnsureStack(djuru, vm, needed);

    MSCAppendCallFrame(djuru, closure, djuru->stackTop - numArgs);
}

static inline bool isFalsyValue(Value value) {
    return IS_FALSE(value) || IS_NULL(value);
}


#endif //CPMSC_MVM_H
