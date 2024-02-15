//
// Created by Mahamadou DOUMBIA [OML DSI] on 10/02/2022.
//
#include "Fan.h"

#if MSC_OPT_FAN

#include <string.h>

#include "../runtime/MVM.h"
#include "Fan.msc.inc"


void metaCompile(Djuru *djuru) {
    const char *source = MSCGetSlotString(djuru, 1);
    bool isExpression = MSCGetSlotBool(djuru, 3);
    bool printErrors = MSCGetSlotBool(djuru, 4);

    const char *module;
    if (MSCGetSlotType(djuru, 2) != MSC_TYPE_NULL) {
        module = MSCGetSlotString(djuru, 2);
        if (MSCHasModule(djuru, module)) {
            MSCSetSlotString(djuru, 0, "Security: Cannot compile into an existing named module.");
            MSCAbortDjuru(djuru, 0);
        }
    } else {
        // TODO: Allow passing in module?
        // Look up the module surrounding the callsite. This is brittle. The -3 walks
        // up the callstack assuming that the meta module has 2 levels of
        // indirection before hitting the user's code. Any change to meta may require
        // this constant to be tweaked.
        Djuru *currentFiber = djuru;
        Function *fn = currentFiber->frames[currentFiber->numOfFrames - 3].closure->fn;
        module = fn->module->name->value;
    }

    Closure *closure = MSCCompileSource(djuru->vm, module, source,
                                        isExpression, printErrors);
    // Return the result. We can't use the public API for this since we have a
    // bare ObjClosure*.
    if (closure == NULL) {
        MSCSetSlot(djuru, 0, NULL_VAL);
    } else {
        MSCSetSlot(djuru, 0, OBJ_VAL(closure));
    }
}

void metaGetModuleVariables(Djuru *djuru) {
    MSCEnsureSlots(djuru, 3);

    Value moduleValue = MSCMapGet(djuru->vm->modules, MSCGetSlot(djuru, 1));
    if (IS_UNDEFINED(moduleValue)) {
        MSCSetSlot(djuru, 0, NULL_VAL);
        return;
    }

    Module *module = AS_MODULE(moduleValue);
    List *names = MSCListFrom(djuru->vm, module->variableNames.count);
    MSCSetSlot(djuru, 0, OBJ_VAL(names));
    // Initialize the elements to null in case a collection happens when we
    // allocate the strings below.
    for (int i = 0; i < names->elements.count; i++) {
        names->elements.data[i] = NULL_VAL;
    }

    for (int i = 0; i < names->elements.count; i++) {
        names->elements.data[i] = OBJ_VAL(module->variableNames.data[i]);
    }
}

const char *MSCFanSource() {
    return FanModuleSource;
}

MSCExternMethodFn MSCFanBindExternMethod(MVM *vm,
                                         const char *className,
                                         bool isStatic,
                                         const char *signature) {
    // There is only one foreign method in the meta module.
    ASSERT(strcmp(className, "Fan") == 0, "Should be in Fan class.");
    ASSERT(isStatic, "Should be static.");

    if (strcmp(signature, "compile_(_,_,_,_)") == 0) {
        return metaCompile;
    }

    if (strcmp(signature, "getModuleVariables_(_)") == 0) {
        return metaGetModuleVariables;
    }

    ASSERT(false, "Unknown method.");
    return NULL;
}

#endif