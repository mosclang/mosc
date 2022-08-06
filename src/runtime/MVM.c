//
// Created by Mahamadou DOUMBIA [OML DSI] on 11/01/2022.
//

#include "MVM.h"
#include "../builtin/Primitive.h"
#include "debuger.h"

#if MSC_OPT_FAN

#include "../meta/Fan.h"

#endif
#if MSC_OPT_KUNFE

#include "../meta/Kunfe.h"
#include "../memory/Value.h"
#include "../helpers/Helper.h"
#include "../api/msc.h"

#endif

static void printFunctionCode(Function *fn) {
    /*uint8_t *ptr = fn->code.data;
    printf("\n%s[", fn->debug->name);
    while ((ptr) < fn->code.data + fn->code.count) {
        printf("%d, ", *ptr++);
    }
    printf("]\n");*/


}

// Looks up a foreign method in [moduleName] on [className] with [signature].
//
// This will try the host's foreign method binder first. If that fails, it
// falls back to handling the built-in modules.
static MSCExternMethodFn findExternMethod(MVM *vm,
                                          const char *moduleName,
                                          const char *className,
                                          bool isStatic,
                                          const char *signature) {
    MSCExternMethodFn method = NULL;

    if (vm->config.bindExternMethodFn != NULL) {
        method = vm->config.bindExternMethodFn(vm, moduleName, className, isStatic,
                                               signature);
    }

    // If the host didn't provide it, see if it's an optional one.
    if (method == NULL) {

#if MSC_OPT_FAN
        if (strcmp(moduleName, "fan") == 0) {
            method = MSCFanBindExternMethod(vm, className, isStatic, signature);
        }
#endif
#if MSC_OPT_KUNFE
        if (strcmp(moduleName, "kunfe") == 0) {
            method = MSCKunfeBindExternMethod(vm, className, isStatic, signature);
        }
#endif

    }

    return method;
}

// Captures the local variable [local] into an [Upvalue]. If that local is
// already in an upvalue, the existing one will be used. (This is important to
// ensure that multiple closures closing over the same variable actually see
// the same variable.) Otherwise, it will create a new open upvalue and add it
// the fiber's list of upvalues.
static Upvalue *captureUpvalue(MVM *vm, Djuru *djuru, Value *local) {
    // If there are no open upvalues at all, we must need a new one.
    if (djuru->openUpvalues == NULL) {
        djuru->openUpvalues = MSCUpvalueFrom(vm, local);
        return djuru->openUpvalues;
    }

    Upvalue *prevUpvalue = NULL;
    Upvalue *upvalue = djuru->openUpvalues;

    // Walk towards the bottom of the stack until we find a previously existing
    // upvalue or pass where it should be.
    while (upvalue != NULL && upvalue->value > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    // Found an existing upvalue for vm local.
    if (upvalue != NULL && upvalue->value == local) return upvalue;

    // We've walked past vm local on the stack, so there must not be an
    // upvalue for it already. Make a new one and link it in in the right
    // place to keep the list sorted.
    Upvalue *createdUpvalue = MSCUpvalueFrom(vm, local);
    if (prevUpvalue == NULL) {
        // The new one is the first one in the list.
        djuru->openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    createdUpvalue->next = upvalue;
    return createdUpvalue;
}

// Closes any open upvalues that have been created for stack slots at [last]
// and above.
static void closeUpvalues(Djuru *fiber, const Value *last) {
    while (fiber->openUpvalues != NULL &&
           fiber->openUpvalues->value >= last) {
        Upvalue *upvalue = fiber->openUpvalues;

        // Move the value into the upvalue itself and point the upvalue to it.
        upvalue->closed = *upvalue->value;
        upvalue->value = &upvalue->closed;

        // Remove it from the open upvalue list.
        fiber->openUpvalues = upvalue->next;
    }
}

inline static bool checkArity(MVM *vm, Value value, int numArgs) {
    ASSERT(IS_CLOSURE(value), "Receiver must be a closure.");
    Function *fn = AS_CLOSURE(value)->fn;

    // We only care about missing arguments, not extras. The "- 1" is because
    // numArgs includes the receiver, the function itself, which we don't want to
    // count.
    if (numArgs - 1 >= fn->arity) return true;

    vm->djuru->error = CONST_STRING(vm, "Function expects more arguments.");
    return false;
}

// Aborts the current fiber with an appropriate method not found error for a
// method with [symbol] on [classObj].
static void methodNotFound(MVM *vm, Class *classObj, int symbol) {
    vm->djuru->error = MSCStringFormatted(vm, "@ does not implement '$'.",
                                          OBJ_VAL(classObj->name), vm->methodNames.data[symbol]->value);
}


// Handles the current fiber having aborted because of an error.
//
// Walks the call chain of fibers, aborting each one until it hits a fiber that
// handles the error. If none do, tells the VM to stop.
static void runtimeError(MVM *vm) {
    ASSERT(MSCHasError(vm->djuru), "Should only call vm after an error.");

    Djuru *current = vm->djuru;
    Value error = current->error;

    while (current != NULL) {
        // Every fiber along the call chain gets aborted with the same error.
        current->error = error;

        // If the caller ran vm fiber using "try", give it the error and stop.
        if (current->state == DJURU_TRY) {
            // Make the caller's try method return the error message.
            current->caller->stackTop[-1] = vm->djuru->error;
            vm->djuru = current->caller;
            return;
        }

        // Otherwise, unhook the caller since we will never resume and return to it.
        Djuru *caller = current->caller;
        current->caller = NULL;
        current = caller;
    }

    // If we got here, nothing caught the error, so show the stack trace.
    MSCDebugPrintStackTrace(vm);
    vm->djuru = NULL;
    vm->apiStack = NULL;
}

void MSCVMSetConfig(MVM *vm, MSCConfig *config) {
    if (config->writeFn != NULL) {
        vm->config.writeFn = config->writeFn;
    }
    if (config->errorHandler != NULL) {
        vm->config.errorHandler = config->errorHandler;
    }
    if (config->bindExternClassFn != NULL) {
        vm->config.bindExternClassFn = config->bindExternClassFn;
    }
    if (config->bindExternMethodFn != NULL) {
        vm->config.bindExternMethodFn = config->bindExternMethodFn;
    }
    if (config->resolveModuleFn != NULL) {
        vm->config.resolveModuleFn = config->resolveModuleFn;
    }
    if (config->loadModuleFn != NULL) {
        vm->config.loadModuleFn = config->loadModuleFn;
    }
    if (config->userData != NULL) {
        vm->config.userData = config->userData;
    }
    if (config->reallocateFn != NULL) {
        vm->config.reallocateFn = config->reallocateFn;
    }
}

void MSCVMSetWriteFn(MVM *vm, MSCWriteFn fn) {
    vm->config.writeFn = fn;
}

void MSCVMSetErrorHandler(MVM *vm, MSCErrorHandler fn) {
    vm->config.errorHandler = fn;
}

void *MSCGetUserData(MVM *vm) {
    return vm->config.userData;
}

// Sets user data associated with the MVM.
void MSCSetUserData(MVM *vm, void *userData) {
    vm->config.userData = userData;
}

void MSCInitConfig(MSCConfig *config) {
    config->reallocateFn = defaultReallocate;
    config->resolveModuleFn = NULL;
    config->loadModuleFn = NULL;
    config->bindExternMethodFn = NULL;
    config->bindExternClassFn = NULL;
    config->writeFn = NULL;
    config->errorHandler = NULL;
    config->initialHeapSize = 1024 * 1024 * 10;
    config->minHeapSize = 1024 * 1024;
    config->heapGrowthPercent = 50;
    config->userData = NULL;
}

MVM *MSCNewVM(MSCConfig *config) {


    MSCReallocator reallocate = defaultReallocate;
    void *userData = NULL;
    if (config != NULL) {
        userData = config->userData;
        reallocate = config->reallocateFn ? config->reallocateFn : defaultReallocate;
    }

    MVM *vm = (MVM *) reallocate(NULL, sizeof(*vm), userData);
    memset(vm, 0, sizeof(MVM));
    // Copy the configuration if given one.
    if (config != NULL) {
        memcpy(&vm->config, config, sizeof(MSCConfig));
        vm->config.reallocateFn = reallocate;
    } else {
        MSCInitConfig(&vm->config);
    }

    vm->gc = MSCNewGC(vm);

    vm->modules = MSCMapFrom(vm);
    MSCSymbolTableInit(&vm->methodNames);
    MSCInitCore(&vm->core, vm);
    return vm;
}

void MSCFreeVM(MVM *vm) {
    ASSERT(vm->methodNames.count > 0, "VM appears to have already been freed.");

    // Free all of the GC objects.
    MSCFreeGC(vm->gc);

    // Tell the user if they didn't free any handles. We don't want to just free
    // them here because the host app may still have pointers to them that they
    // may try to use. Better to tell them about the bug early.
    ASSERT(vm->handles == NULL, "All handles have not been released.");

    MSCSymbolTableClear(vm, &vm->methodNames);

}

void MSCCollectGarbage(MVM *vm) {
    MSCGCCollect(vm->gc);
}

int MSCGetVersionNumber() {
    return MSC_VERSION_NUMBER;
}

void MSCFinalizeExtern(MVM *vm, Extern *externObj) {
    // TODO: Don't look up every time.
    int symbol = MSCSymbolTableFind(&vm->methodNames, "<finalize>", 10);
    ASSERT(symbol != -1, "Should have defined <finalize> symbol.");

    // If there are no finalizers, don't finalize it.
    if (symbol == -1) return;

    // If the class doesn't have a finalizer, bail out.
    Class *classObj = externObj->obj.classObj;
    if (symbol >= classObj->methods.count) return;

    Method *method = &classObj->methods.data[symbol];
    if (method->type == METHOD_NONE) return;

    ASSERT(method->type == METHOD_EXTERN, "Finalizer should be foreign.");

    //MSCFinalizerFn finalizer = (MSCFinalizerFn)method->as.foreign;
    //finalizer(foreign->data);
}

MSCHandle *MSCMakeCallHandle(MVM *vm, const char *signature) {
    ASSERT(signature != NULL, "Signature cannot be NULL.");

    int signatureLength = (int) strlen(signature);
    ASSERT(signatureLength > 0, "Signature cannot be empty.");

    // Count the number parameters the method expects.
    int numParams = 0;
    if (signature[signatureLength - 1] == ')') {
        for (int i = signatureLength - 1; i > 0 && signature[i] != '('; i--) {
            if (signature[i] == '_') numParams++;
        }
    }

    /*// Count subscript arguments.
    if (signature[0] == '[') {
        for (int i = 0; i < signatureLength && signature[i] != ']'; i++)
        {
            if (signature[i] == '_') numParams++;
        }
    }*/

    // Add the signatue to the method table.
    int method = MSCSymbolTableEnsure(vm, &vm->methodNames,
                                      signature, (size_t) signatureLength);

    // Create a little stub function that assumes the arguments are on the stack
    // and calls the method.
    Function *fn = MSCFunctionFrom(vm, NULL, numParams + 1);

    // Wrap the function in a closure and then in a handle. Do vm here so it
    // doesn't get collected as we fill it in.
    MSCHandle *value = MSCMakeHandle(vm, OBJ_VAL(fn));
    value->value = OBJ_VAL(MSCClosureFrom(vm, fn));

    MSCWriteByteBuffer(vm, &fn->code, (uint8_t) (OP_CALL_0 + numParams));
    MSCWriteByteBuffer(vm, &fn->code, (uint8_t) ((method >> 8) & 0xff));
    MSCWriteByteBuffer(vm, &fn->code, (uint8_t) (method & 0xff));
    MSCWriteByteBuffer(vm, &fn->code, OP_RETURN);
    MSCWriteByteBuffer(vm, &fn->code, OP_END);
    MSCFillIntBuffer(vm, &fn->debug->sourceLines, 0, 5);
    MSCFunctionBindName(fn, vm, signature, signatureLength);
    return value;
}

static Value getModuleVariable(MVM *vm, Module *module, Value variableName) {
    String *variable = AS_STRING(variableName);
    int variableEntry = MSCSymbolTableFind(&module->variableNames,
                                           variable->value,
                                           variable->length);
    // It's a runtime error if the imported variable does not exist.
    if (variableEntry != UINT32_MAX) {
        return module->variables.data[variableEntry];
    }
    vm->djuru->error = MSCStringFormatted(vm,
                                          "Could not find a variable named '@' in module '@'.",
                                          variableName, OBJ_VAL(module->name));
    return NULL_VAL;
}

// Completes the process for creating a new class.
//
// The class attributes instance and the class itself should be on the
// top of the fiber's stack.
//
// This process handles moving the attribute data for a class from
// compile time to runtime, since it now has all the attributes associated
// with a class, including for methods.
static void endClass(MVM *vm) {
    // Pull the attributes and class off the stack
    Value attributes = vm->djuru->stackTop[-2];
    Value classValue = vm->djuru->stackTop[-1];

    // Remove the stack items
    vm->djuru->stackTop -= 2;

    // Class *classObj = AS_CLASS(classValue);
    //classObj->attributes = attributes;
}

// Verifies that [superclassValue] is a valid object to inherit from. That
// means it must be a class and cannot be the class of any built-in type.
//
// Also validates that it doesn't result in a class with too many fields and
// the other limitations foreign classes have.
//
// If successful, returns `null`. Otherwise, returns a string for the runtime
// error message.
static Value validateSuperclass(MVM *vm, Value name, Value superclassValue, int numFields) {
    // Make sure the superclass is a class.
    if (!IS_CLASS(superclassValue)) {
        return MSCStringFormatted(vm,
                                  "Class '@' cannot inherit from a non-class object.",
                                  name);
    }

    // Make sure it doesn't inherit from a sealed built-in type. Primitive methods
    // on these classes assume the instance is one of the other Obj___ types and
    // will fail horribly if it's actually an ObjInstance.
    Class *superclass = AS_CLASS(superclassValue);
    if (superclass == vm->core.classClass ||
        superclass == vm->core.djuruClass ||
        superclass == vm->core.fnClass || // Includes OBJ_CLOSURE.
        superclass == vm->core.listClass ||
        superclass == vm->core.mapClass ||
        superclass == vm->core.rangeClass ||
        superclass == vm->core.stringClass ||
        superclass == vm->core.boolClass ||
        superclass == vm->core.nullClass ||
        superclass == vm->core.numClass) {
        return MSCStringFormatted(vm,
                                  "Class '@' cannot inherit from built-in class '@'.",
                                  name, OBJ_VAL(superclass->name));
    }

    if (superclass->numFields == -1) {
        return MSCStringFormatted(vm,
                                  "Class '@' cannot inherit from foreign class '@'.",
                                  name, OBJ_VAL(superclass->name));
    }

    if (numFields == -1 && superclass->numFields > 0) {
        return MSCStringFormatted(vm,
                                  "Foreign class '@' may not inherit from a class with fields.",
                                  name);
    }

    if (superclass->numFields + numFields > MAX_FIELDS) {
        return MSCStringFormatted(vm,
                                  "Class '@' may not have more than 255 fields, including inherited "
                                          "ones.", name);
    }

    return NULL_VAL;
}

static void bindExternClass(MVM *vm, Class *classObj, Module *module) {
    MSCExternClassMethods methods;
    methods.allocate = NULL;
    methods.finalize = NULL;

    // Check the optional built-in module first so the host can override it.

    if (vm->config.bindExternClassFn != NULL) {
        methods = vm->config.bindExternClassFn(vm, module->name->value,
                                               classObj->name->value);
    }

    // If the host didn't provide it, see if it's a built in optional module.
    if (methods.allocate == NULL && methods.finalize == NULL) {
#if MSC_OPT_KUNFE
        if (strcmp(module->name->value, "kunfe") == 0) {
            methods = MSCKunfeBindExternClass(vm, module->name->value,
                                              classObj->name->value);
        }
#endif
    }

    Method method;
    method.type = METHOD_EXTERN;

    // Add the symbol even if there is no allocator so we can ensure that the
    // symbol itself is always in the symbol table.
    int symbol = MSCSymbolTableEnsure(vm, &vm->methodNames, "<allocate>", 10);
    if (methods.allocate != NULL) {
        method.as.foreign = methods.allocate;
        MSCBindMethod(classObj, vm, symbol, method);
    }

    // Add the symbol even if there is no finalizer so we can ensure that the
    // symbol itself is always in the symbol table.
    symbol = MSCSymbolTableEnsure(vm, &vm->methodNames, "<finalize>", 10);
    if (methods.finalize != NULL) {
        method.as.foreign = (MSCExternMethodFn) methods.finalize;
        MSCBindMethod(classObj, vm, symbol, method);
    }
}

// Creates a new class.
//
// If [numFields] is -1, the class is a foreign class. The name and superclass
// should be on top of the fiber's stack. After calling vm, the top of the
// stack will contain the new class.
//
// Aborts the current fiber if an error occurs.
static void createClass(MVM *vm, int numFields, Module *module) {
    // Pull the name and superclass off the stack.
    Value name = vm->djuru->stackTop[-2];
    Value superclass = vm->djuru->stackTop[-1];
    // We have two values on the stack and we are going to leave one, so discard
    // the other slot.
    vm->djuru->stackTop--;

    vm->djuru->error = validateSuperclass(vm, name, superclass, numFields);
    if (MSCHasError(vm->djuru)) return;

    Class *classObj = MSCClassFrom(vm, AS_CLASS(superclass), numFields, AS_STRING(name));

    vm->djuru->stackTop[-1] = OBJ_VAL(classObj);

    if (numFields == -1) bindExternClass(vm, classObj, module);
}

// Defines [methodValue] as a method on [classObj].
//
// Handles both foreign methods where [methodValue] is a string containing the
// method's signature and Wren methods where [methodValue] is a function.
//
// Aborts the current fiber if the method is a foreign method that could not be
// found.
static void bindMethod(MVM *vm, int methodType, int symbol,
                       Module *module, Class *classObj, Value methodValue) {

    const char *className = classObj->name->value;
    if (methodType == OP_METHOD_STATIC) classObj = classObj->obj.classObj;
    Method method;
    if (IS_STRING(methodValue)) {

        const char *name = AS_CSTRING(methodValue);
        method.type = METHOD_EXTERN;
        method.as.foreign = findExternMethod(vm, module->name->value,
                                             className,
                                             methodType == OP_METHOD_STATIC,
                                             name);

        if (method.as.foreign == NULL) {
            vm->djuru->error = MSCStringFormatted(vm,
                                                  "Could not find foreign method '@' for class $ in module '$'.",
                                                  methodValue, classObj->name->value, module->name->value);
            return;
        }

    } else {
        method.as.closure = AS_CLOSURE(methodValue);
        method.type = METHOD_BLOCK;
        if (methodType != OP_METHOD_STATIC)
            //printf("Will bind class method %s %s: ::: %p:: %p -- %p\n", method.as.closure->fn->debug->name, className,
            //      classObj, classObj->superclass,
            //     core->objectClass);
            // Patch up the bytecode now that we know the superclass.
            MSCBindMethodCode(classObj, method.as.closure->fn);
    }

    MSCBindMethod(classObj, vm, symbol, method);
}


// Returns `NULL` if no module with that name has been loaded.
Module *MSCGetModule(MVM *vm, Value name) {
    Value moduleValue = MSCMapGet(vm->modules, name);
    return !IS_UNDEFINED(moduleValue) ? AS_MODULE(moduleValue) : NULL;
}

static Closure *compileInModule(MVM *vm, Value name, const char *source,
                                bool isExpression, bool printErrors) {
    // See if the module has already been loaded.
    Module *module = MSCGetModule(vm, name);
    if (module == NULL) {
        module = MSCModuleFrom(vm, AS_STRING(name));

        // It's possible for the MSCMapSet below to resize the modules map,
        // and trigger a GC while doing so. When vm happens it will collect
        // the module we've just created. Once in the map it is safe.
        MSCPushRoot(vm->gc, (Object *) module);

        // Store it in the VM's module registry so we don't load the same module
        // multiple times.
        MSCMapSet(vm->modules, vm, name, OBJ_VAL(module));

        MSCPopRoot(vm->gc);

        // Implicitly import the core module.
        Module *coreModule = MSCGetModule(vm, NULL_VAL);
        if (coreModule != NULL) {
            for (int i = 0; i < coreModule->variables.count; i++) {
                MSCDefineVariable(vm, module,
                                  coreModule->variableNames.data[i]->value,
                                  coreModule->variableNames.data[i]->length,
                                  coreModule->variables.data[i], NULL);
            }
        }
    }

    Function *fn = MSCCompile(vm, module, source, isExpression, printErrors);

    if (fn == NULL) {
        // TODO: Should we still store the module even if it didn't compile?
        return NULL;
    }
    // Functions are always wrapped in closures.
    MSCPushRoot(vm->gc, (Object *) fn);
    Closure *closure = MSCClosureFrom(vm, fn);
    MSCPopRoot(vm->gc); // fn.

    return closure;
}

// Let the host resolve an imported module name if it wants to.
static Value resolveModule(MVM *vm, Value name) {
    // If the host doesn't care to resolve, leave the name alone.
    if (vm->config.resolveModuleFn == NULL) return name;

    Djuru *fiber = vm->djuru;
    Function *fn = fiber->frames[fiber->numOfFrames - 1].closure->fn;
    String *importer = fn->module->name;

    const char *resolved = vm->config.resolveModuleFn(vm, importer->value,
                                                      AS_CSTRING(name));
    if (resolved == NULL) {
        vm->djuru->error = MSCStringFormatted(vm,
                                              "Could not resolve module '@' imported from '@'.",
                                              name, OBJ_VAL(importer));
        return NULL_VAL;
    }

    // If they resolved to the exact same string, we don't need to copy it.
    if (resolved == AS_CSTRING(name)) return name;
    // Copy the string into a MSC String object.
    name = MSCStringFromConstChars(vm, resolved);
    DEALLOCATE(vm, (char *) resolved);
    return name;
}

static void callExtern(MVM *vm, Djuru *fiber,
                       MSCExternMethodFn foreign, int numArgs) {
    ASSERT(vm->apiStack == NULL, "Cannot already be in foreign call.");
    vm->apiStack = fiber->stackTop - numArgs;

    foreign(vm);

    // Discard the stack slots for the arguments and temporaries but leave one
    // for the result.
    fiber->stackTop = vm->apiStack + 1;

    vm->apiStack = NULL;
}

static void createExtern(MVM *vm, Djuru *djuru, Value *stack) {
    Class *classObj = AS_CLASS(stack[0]);
    ASSERT(classObj->numFields == -1, "Class must be a foreign class.");

    // TODO: Don't look up every time.
    int symbol = MSCSymbolTableFind(&vm->methodNames, "<allocate>", 10);
    ASSERT(symbol != -1, "Should have defined <allocate> symbol.");

    ASSERT(classObj->methods.count > symbol, "Class should have allocator.");
    Method *method = &classObj->methods.data[symbol];
    ASSERT(method->type == METHOD_EXTERN, "Allocator should be foreign.");

    // Pass the constructor arguments to the allocator as well.
    ASSERT(vm->apiStack == NULL, "Cannot already be in foreign call.");
    vm->apiStack = stack;

    method->as.foreign(vm);

    vm->apiStack = NULL;
}


static Value importModule(MVM *vm, Value name) {
    name = resolveModule(vm, name);
    // If the module is already loaded, we don't need to do anything.
    Value existing = MSCMapGet(vm->modules, name);
    if (!IS_UNDEFINED(existing)) return existing;

    MSCPushRoot(vm->gc, AS_OBJ(name));
    MSCLoadModuleResult result = {0};
    // const char *source = NULL;

    // Let the host try to provide the module.
    if (vm->config.loadModuleFn != NULL) {
        result = vm->config.loadModuleFn(vm, AS_CSTRING(name));
    }

    // If the host didn't provide it, see if it's a built in optional module.
    if (result.source == NULL) {
        result.onComplete = NULL;
        String *nameString = AS_STRING(name);
#if MSC_OPT_FAN
        if (strcmp(nameString->value, "fan") == 0) result.source = MSCFanSource();
#endif
#if MSC_OPT_KUNFE
        if (strcmp(nameString->value, "kunfe") == 0) result.source = MSCKunfeSource();
#endif

    }

    if (result.source == NULL) {
        vm->djuru->error = MSCStringFormatted(vm, "Could not load module '@'.", name);
        MSCPopRoot(vm->gc);
        return NULL_VAL;
    }

    Closure *moduleClosure = compileInModule(vm, name, result.source, false, true);

    // Now that we're done, give the result back in case there's cleanup to do.
    if (result.onComplete) result.onComplete(vm, AS_CSTRING(name), result);

    if (moduleClosure == NULL) {
        vm->djuru->error = MSCStringFormatted(vm,
                                              "Could not compile module '@'.", name);
        MSCPopRoot(vm->gc);
        return NULL_VAL;
    }

    MSCPopRoot(vm->gc);

    // Return the closure that executes the module.
    return OBJ_VAL(moduleClosure);
}


static MSCInterpretResult runInterpreter(MVM *vm, Djuru *djuru) {
#if __cplusplus > 199711L
#define register      // Deprecated in C++11.
#endif  // #if __cplusplus > 199711L
    // Remember the current djuru so we can find it if a GC happens.
    vm->djuru = djuru;
    djuru->state = DJURU_ROOT;

    // Hoist these into local variables. They are accessed frequently in the loop
    // but assigned less frequently. Keeping them in locals and updating them when
    // a call frame has been pushed or popped gives a large speed boost.
    register CallFrame *frame;
    register Value *stackStart;
    register uint8_t *ip;
    register Function *fn;

    // These macros are designed to only be invoked within vm function.
#define PUSH(value)  (*djuru->stackTop++ = value)
#define POP()        (*(--djuru->stackTop))
#define DROP()       (djuru->stackTop--)
#define PEEK()       (*(djuru->stackTop - 1))
#define PEEK2()      (*(djuru->stackTop - 2))
#define READ_BYTE()  (*ip++)
#define READ_SHORT() (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))

    // Use vm before a CallFrame is pushed to store the local variables back
    // into the current one.
#define STORE_FRAME() frame->ip = ip

    // Use vm after a CallFrame has been pushed or popped to refresh the local
    // variables.
#define LOAD_FRAME()                                                         \
      do                                                                       \
      {                                                                        \
        frame = &djuru->frames[djuru->numOfFrames - 1];                          \
        stackStart = frame->stackStart;                                        \
        ip = frame->ip;                                                        \
        fn = frame->closure->fn;                                               \
      } while (false)

    // Terminates the current djuru with error string [error]. If another calling
    // djuru is willing to catch the error, transfers control to it, otherwise
    // exits the interpreter.
#define RUNTIME_ERROR()                                                      \
      do                                                                       \
      {                                                                        \
        STORE_FRAME();                                                          \
        runtimeError(vm);                                                      \
        if (vm->djuru == NULL) return RESULT_RUNTIME_ERROR;               \
        djuru = vm->djuru;                                                     \
        LOAD_FRAME();                                                          \
        DISPATCH();                                                            \
      } while (false)

#if MSC_DEBUG_TRACE_INSTRUCTIONS
    // Prints the stack and instruction before each instruction is executed.
#define DEBUG_TRACE_INSTRUCTIONS()                                         \
        do                                                                     \
        {                                                                      \
          MSCDumpStack(djuru);                                                \
          MSCDumpInstruction(vm, fn, (int)(ip - fn->code.data));              \
        } while (false)
#else
#define DEBUG_TRACE_INSTRUCTIONS() do { } while (false)
#endif

#if MSC_COMPUTED_GOTO

    static void *dispatchTable[] = {
#define OPCODE(name, _) &&code_##name,

#include "../common/codes.h"

#undef OPCODE
    };

#define INTERPRET_LOOP    DISPATCH();
#define CASE_CODE(name)   code_##name

#define DISPATCH()                                                           \
      do                                                                       \
      {                                                                        \
        DEBUG_TRACE_INSTRUCTIONS();                                            \
        goto *dispatchTable[instruction = (Opcode)READ_BYTE()];                  \
      } while (false)

#else

#define INTERPRET_LOOP                                                       \
      loop:                                                                    \
        DEBUG_TRACE_INSTRUCTIONS();                                            \
        instruction = (Opcode)READ_BYTE()     ;                                  \
        /*printf("Inst:: %d", instruction);*/                                       \
        switch (instruction)

#define CASE_CODE(name)  case OP_##name
#define DISPATCH()       goto loop

#endif

    LOAD_FRAME();

    Opcode instruction;
    INTERPRET_LOOP
    {
        CASE_CODE(LOAD_LOCAL_0):
        CASE_CODE(LOAD_LOCAL_1):
        CASE_CODE(LOAD_LOCAL_2):
        CASE_CODE(LOAD_LOCAL_3):
        CASE_CODE(LOAD_LOCAL_4):
        CASE_CODE(LOAD_LOCAL_5):
        CASE_CODE(LOAD_LOCAL_6):
        CASE_CODE(LOAD_LOCAL_7):
        CASE_CODE(LOAD_LOCAL_8):
        PUSH(stackStart[instruction - OP_LOAD_LOCAL_0]);
        DISPATCH();

        CASE_CODE(LOAD_LOCAL):
        PUSH(stackStart[READ_BYTE()]);
        DISPATCH();

        CASE_CODE(LOAD_FIELD_THIS):
        {
            uint8_t field = READ_BYTE();
            Value receiver = stackStart[0];
            ASSERT(IS_INSTANCE(receiver), "Receiver should be instance.");
            Instance *instance = AS_INSTANCE(receiver);
            ASSERT(field < instance->obj.classObj->numFields, "Out of bounds field.");
            PUSH(instance->fields[field]);
            DISPATCH();
        }

        CASE_CODE(POP):

        DROP();

        DISPATCH();
        CASE_CODE(NULL):
        PUSH(NULL_VAL);
        DISPATCH();
        CASE_CODE(VOID):
        PUSH(NULL_VAL);
        DISPATCH();
        CASE_CODE(FALSE):
        PUSH(FALSE_VAL);
        DISPATCH();
        CASE_CODE(TRUE):
        PUSH(TRUE_VAL);
        DISPATCH();

        CASE_CODE(STORE_LOCAL):
        stackStart[READ_BYTE()] = PEEK();
        DISPATCH();

        CASE_CODE(CONSTANT):

        PUSH(fn->constants.data[READ_SHORT()]);
        DISPATCH();
        CASE_CODE(CALL):
        {
            int numArgs = READ_SHORT() + 1;
            Value *args = djuru->stackTop - numArgs;
            Closure *closure = AS_CLOSURE(args[0]);
            STORE_FRAME();
            callFunction(vm, djuru, closure, numArgs);
            LOAD_FRAME();
            DISPATCH();
        }


        {
            // The opcodes for doing method and superclass calls share a lot of Opcode.
            // However, doing an if() test in the middle of the instruction sequence
            // to handle the bit that is special to super calls makes the non-super
            // call path noticeably slower.
            //
            // Instead, we do vm old school using an explicit goto to share Opcode for
            // everything at the tail end of the call-handling Opcode that is the same
            // between normal and superclass calls.
            int numArgs;
            int symbol;

            Value *args;
            Class *classObj;

            Method *method;

            CASE_CODE(CALL_0):
            CASE_CODE(CALL_1):
            CASE_CODE(CALL_2):
            CASE_CODE(CALL_3):
            CASE_CODE(CALL_4):
            CASE_CODE(CALL_5):
            CASE_CODE(CALL_6):
            CASE_CODE(CALL_7):
            CASE_CODE(CALL_8):
            CASE_CODE(CALL_9):
            CASE_CODE(CALL_10):
            CASE_CODE(CALL_11):
            CASE_CODE(CALL_12):
            CASE_CODE(CALL_13):
            CASE_CODE(CALL_14):
            CASE_CODE(CALL_15):
            CASE_CODE(CALL_16):
            method = NULL;
            // Add one for the implicit receiver argument.
            numArgs = instruction - OP_CALL_0 + 1;
            symbol = READ_SHORT();
            // The receiver is the first argument.
            args = djuru->stackTop - numArgs;
            classObj = MSCGetClassInline(vm, args[0]);
            goto completeCall;

            CASE_CODE(SUPER_0):
            CASE_CODE(SUPER_1):
            CASE_CODE(SUPER_2):
            CASE_CODE(SUPER_3):
            CASE_CODE(SUPER_4):
            CASE_CODE(SUPER_5):
            CASE_CODE(SUPER_6):
            CASE_CODE(SUPER_7):
            CASE_CODE(SUPER_8):
            CASE_CODE(SUPER_9):
            CASE_CODE(SUPER_10):
            CASE_CODE(SUPER_11):
            CASE_CODE(SUPER_12):
            CASE_CODE(SUPER_13):
            CASE_CODE(SUPER_14):
            CASE_CODE(SUPER_15):
            CASE_CODE(SUPER_16):
            method = NULL;
            // Add one for the implicit receiver argument.
            numArgs = instruction - OP_SUPER_0 + 1;
            symbol = READ_SHORT();

            // The receiver is the first argument.
            args = djuru->stackTop - numArgs;

            // The superclass is stored in a constant.
            classObj = AS_CLASS(fn->constants.data[READ_SHORT()]);
            goto completeCall;

            completeCall:
            // If the class's method table doesn't include the symbol, bail.
            if (method == NULL && (symbol >= classObj->methods.count ||
                                   (method = &classObj->methods.data[symbol])->type == METHOD_NONE)) {
                methodNotFound(vm, classObj, symbol);
                RUNTIME_ERROR();
            }

            switch (method->type) {
                case METHOD_PRIMITIVE:
                    // printf("NumArgs:: %d %f %f\n", numArgs, args[0].as.num, args[1].as.num);
                    if (method->as.primitive(vm, args)) {
                        // The result is now in the first arg slot. Discard the other
                        // stack slots.
                        djuru->stackTop -= numArgs - 1;
                    } else {
                        // An error, djuru switch, or call frame change occurred.
                        STORE_FRAME();
                        // If we don't have a djuru to switch to, stop interpreting.
                        djuru = vm->djuru;
                        if (djuru == NULL) return RESULT_SUCCESS;
                        if (MSCHasError(djuru)) RUNTIME_ERROR();
                        LOAD_FRAME();
                    }
                    break;

                case METHOD_FUNCTION_CALL:
                    if (!checkArity(vm, args[0], numArgs)) {
                        RUNTIME_ERROR();
                        break;
                    }

                    STORE_FRAME();
                    method->as.primitive(vm, args);
                    LOAD_FRAME();
                    break;

                case METHOD_EXTERN:
                    callExtern(vm, djuru, method->as.foreign, numArgs);
                    if (MSCHasError(djuru)) RUNTIME_ERROR();
                    break;

                case METHOD_BLOCK:
                    STORE_FRAME();
                    callFunction(vm, djuru, method->as.closure, numArgs);
                    LOAD_FRAME();
                    break;

                case METHOD_NONE:
                    UNREACHABLE();
                    break;
            }
            DISPATCH();
        }

        CASE_CODE(LOAD_UPVALUE):
        {
            Upvalue **upvalues = frame->closure->upvalues;
            PUSH(*upvalues[READ_BYTE()]->value);
            // printf("LOAD UPVALUE %d", instruction);
            DISPATCH();
        }

        CASE_CODE(STORE_UPVALUE):
        {
            Upvalue **upvalues = frame->closure->upvalues;
            *upvalues[READ_BYTE()]->value = PEEK();
            DISPATCH();
        }

        CASE_CODE(LOAD_MODULE_VAR):
        {
            PUSH(fn->module->variables.data[READ_SHORT()]);
            DISPATCH();
        }
        CASE_CODE(STORE_MODULE_VAR):
        {
            fn->module->variables.data[READ_SHORT()] = PEEK();
            DISPATCH();
        }

        CASE_CODE(STORE_FIELD_THIS):
        {
            uint8_t field = READ_BYTE();
            Value receiver = stackStart[0];
            ASSERT(IS_INSTANCE(receiver), "Receiver should be instance.");
            Instance *instance = AS_INSTANCE(receiver);
            ASSERT(field < instance->obj.classObj->numFields, "Out of bounds field.");
            instance->fields[field] = PEEK();
            DISPATCH();
        }


        CASE_CODE(FIELD):
        {
            uint8_t field = READ_BYTE();
            Value receiver = POP();
            ASSERT(IS_CLASS(receiver), "Receiver should be a class.");
            Class *objClass = AS_CLASS(receiver);
            // ASSERT(field < objClass->classObj->numFields, "Out of bounds field.");
            Field fieldValue = {
                    false,
                    POP()
            };
            MSCBindField(objClass, vm, field, fieldValue);
            DISPATCH();
        }
        CASE_CODE(LOAD_FIELD):
        {
            uint8_t field = READ_BYTE();
            Value receiver = POP();
            ASSERT(IS_INSTANCE(receiver), "Receiver should be instance.");
            Instance *instance = AS_INSTANCE(receiver);
            ASSERT(field < instance->obj.classObj->numFields, "Out of bounds field.");
            PUSH(instance->fields[field]);
            DISPATCH();
        }

        CASE_CODE(STORE_FIELD):
        {
            uint8_t field = READ_BYTE();
            Value receiver = PEEK2();
            ASSERT(IS_INSTANCE(receiver), "Receiver should be instance.");
            Instance *instance = AS_INSTANCE(receiver);
            ASSERT(field < instance->obj.classObj->numFields, "Out of bounds field.");
            Value value = POP();
            instance->fields[field] = value;
            *(djuru->stackTop - 1) = value;
            DISPATCH();
        }

        CASE_CODE(JUMP):
        {
            uint16_t offset = READ_SHORT();
            ip += offset;
            DISPATCH();
        }

        CASE_CODE(LOOP):
        {
            // Jump back to the top of the loop.
            uint16_t offset = READ_SHORT();
            ip -= offset;
            DISPATCH();
        }

        CASE_CODE(JUMP_IF):
        {
            uint16_t offset = READ_SHORT();
            Value condition = POP();

            if (isFalsyValue(condition)) ip += offset;
            DISPATCH();
        }

        CASE_CODE(AND):
        {
            uint16_t offset = READ_SHORT();
            Value condition = PEEK();

            if (isFalsyValue(condition)) {
                // Short-circuit the right hand side.
                ip += offset;
            } else {
                // Discard the condition and evaluate the right hand side.
                DROP();
            }
            DISPATCH();
        }

        CASE_CODE(OR):
        {
            uint16_t offset = READ_SHORT();
            Value condition = PEEK();

            if (isFalsyValue(condition)) {
                // Discard the condition and evaluate the right hand side.
                DROP();
            } else {
                // Short-circuit the right hand side.
                ip += offset;
            }
            DISPATCH();
        }

        CASE_CODE(CLOSE_UPVALUE):
        // Close the upvalue for the local if we have one.
        closeUpvalues(djuru, djuru->stackTop - 1);
        DROP();
        DISPATCH();

        CASE_CODE(RETURN):
        {
            Value result = POP();
            djuru->numOfFrames--;

            // Close any upvalues still in scope.
            closeUpvalues(djuru, stackStart);

            // If the djuru is complete, end it.
            if (djuru->numOfFrames == 0) {
                // See if there's another djuru to return to. If not, we're done.
                if (djuru->caller == NULL) {
                    // Store the final result value at the beginning of the stack so the
                    // C API can get it.
                    djuru->stack[0] = result;
                    djuru->stackTop = djuru->stack + 1;
                    return RESULT_SUCCESS;
                }

                Djuru *resumingFiber = djuru->caller;
                djuru->caller = NULL;
                djuru = resumingFiber;
                vm->djuru = resumingFiber;

                // Store the result in the resuming djuru.
                djuru->stackTop[-1] = result;
            } else {
                // Store the result of the block in the first slot, which is where the
                // caller expects it.
                stackStart[0] = result;

                // Discard the stack slots for the call frame (leaving one slot for the
                // result).
                djuru->stackTop = frame->stackStart + 1;
            }

            LOAD_FRAME();
            DISPATCH();
        }

        CASE_CODE(CONSTRUCT):
        ASSERT(IS_CLASS(stackStart[0]), "'vm' should be a class.");
        stackStart[0] = OBJ_VAL(MSCInstanceFrom(vm, AS_CLASS(stackStart[0])));
        DISPATCH();

        CASE_CODE(EXTERN_CONSTRUCT):
        ASSERT(IS_CLASS(stackStart[0]), "'vm' should be a class.");
        createExtern(vm, djuru, stackStart);
        if (MSCHasError(djuru)) RUNTIME_ERROR();
        DISPATCH();

        CASE_CODE(CLOSURE):
        {
            // Create the closure and push it on the stack before creating upvalues
            // so that it doesn't get collected.

            Function *function = AS_FUNCTION(fn->constants.data[READ_SHORT()]);

            Closure *closure = MSCClosureFrom(vm, function);

            PUSH(OBJ_VAL(closure));

            // Capture upvalues, if any.
            for (int i = 0; i < function->numUpvalues; i++) {
                uint8_t isLocal = READ_BYTE();
                uint8_t index = READ_BYTE();
                if (isLocal) {
                    // Make an new upvalue to close over the parent's local variable.
                    closure->upvalues[i] = captureUpvalue(vm, djuru,
                                                          frame->stackStart + index);
                } else {
                    // Use the same upvalue as the current call frame.
                    closure->upvalues[i] = frame->closure->upvalues[index];
                }
            }
            DISPATCH();
        }

        CASE_CODE(END_CLASS):
        {

            endClass(vm);
            if (MSCHasError(djuru)) RUNTIME_ERROR();
            DISPATCH();
        }

        CASE_CODE(CLASS):
        {
            createClass(vm, READ_BYTE(), NULL);
            if (MSCHasError(djuru)) RUNTIME_ERROR();
            DISPATCH();
        }

        CASE_CODE(EXTERN_CLASS):
        {

            createClass(vm, -1, fn->module);
            if (MSCHasError(djuru)) RUNTIME_ERROR();
            DISPATCH();
        }

        CASE_CODE(METHOD_INSTANCE):
        CASE_CODE(METHOD_STATIC):
        {

            uint16_t symbol = READ_SHORT();

            Class *classObj = AS_CLASS(PEEK());
            Value method = PEEK2();
            bindMethod(vm, instruction, symbol, fn->module, classObj, method);
            if (MSCHasError(djuru)) RUNTIME_ERROR();
            DROP();
            DROP();
            DISPATCH();
        }

        CASE_CODE(END_MODULE):
        {

            vm->lastModule = fn->module;
            PUSH(NULL_VAL);
            DISPATCH();
        }

        CASE_CODE(IMPORT_MODULE):
        {

            // Make a slot on the stack for the module's djuru to place the return
            // value. It will be popped after vm djuru is resumed. Store the
            // imported module's closure in the slot in case a GC happens when
            // invoking the closure.
            PUSH(importModule(vm, fn->constants.data[READ_SHORT()]));
            if (MSCHasError(djuru)) RUNTIME_ERROR();

            // If we get a closure, call it to execute the module body.
            if (IS_CLOSURE(PEEK())) {
                STORE_FRAME();
                Closure *closure = AS_CLOSURE(PEEK());
                callFunction(vm, djuru, closure, 1);
                LOAD_FRAME();
            } else {
                // The module has already been loaded. Remember it so we can import
                // variables from it if needed.
                vm->lastModule = AS_MODULE(PEEK());
            }

            DISPATCH();
        }

        CASE_CODE(IMPORT_VARIABLE):
        {

            Value variable = fn->constants.data[READ_SHORT()];
            ASSERT(vm->lastModule != NULL, "Should have already imported module.");
            Value result = getModuleVariable(vm, vm->lastModule, variable);
            if (MSCHasError(djuru)) RUNTIME_ERROR();

            PUSH(result);
            DISPATCH();
        }

        CASE_CODE(END):
        // A CODE_END should always be preceded by a CODE_RETURN. If we get here,
        // the compiler generated wrong Opcode.
        UNREACHABLE();
    }

    // We should only exit vm function from an explicit return from CODE_RETURN
    // or a runtime error.
    UNREACHABLE();
    return RESULT_RUNTIME_ERROR;

#undef READ_BYTE
#undef READ_SHORT
    return RESULT_COMPILATION_ERROR;
}

MSCInterpretResult MSCCall(MVM *vm, MSCHandle *method) {
    ASSERT(method != NULL, "Method cannot be NULL.");
    ASSERT(IS_CLOSURE(method->value), "Method must be a method handle.");
    ASSERT(vm->djuru != NULL, "Must set up arguments for call first.");
    ASSERT(vm->apiStack != NULL, "Must set up arguments for call first.");
    ASSERT(vm->djuru->numOfFrames == 0, "Can not call from a foreign method.");

    Closure *closure = AS_CLOSURE(method->value);

    ASSERT(vm->djuru->stackTop - vm->djuru->stack >= closure->fn->arity,
           "Stack must have enough arguments for method.");

    // Clear the API stack. Now that MSCCall() has control, we no longer need
    // it. We use vm being non-null to tell if re-entrant calls to foreign
    // methods are happening, so it's important to clear it out now so that you
    // can call foreign methods from within calls to MSCCall().
    vm->apiStack = NULL;

    // Discard any extra temporary slots. We take for granted that the stub
    // function has exactly one slot for each argument.
    vm->djuru->stackTop = &vm->djuru->stack[closure->fn->maxSlots];

    callFunction(vm, vm->djuru, closure, 0);
    MSCInterpretResult result = runInterpreter(vm, vm->djuru);

    // If the call didn't abort, then set up the API stack to point to the
    // beginning of the stack so the host can access the call's return value.
    if (vm->djuru != NULL) vm->apiStack = vm->djuru->stack;

    return result;
}


Value MSCGetModuleVariable(MVM *vm, Value moduleName, Value variableName) {
    Module *module = MSCGetModule(vm, moduleName);
    if (module == NULL) {
        vm->djuru->error = MSCStringFormatted(vm, "Module '@' is not loaded.",
                                              moduleName);
        return NULL_VAL;
    }
    return getModuleVariable(vm, module, variableName);
}

Value MSCFindVariable(MVM *vm, Module *module, const char *name) {
    int symbol = MSCSymbolTableFind(&module->variableNames, name, strlen(name));
    return module->variables.data[symbol];
}

MSCHandle *MSCMakeHandle(MVM *vm, Value value) {
    if (IS_OBJ(value)) MSCPushRoot(vm->gc, AS_OBJ(value));

    // Make a handle for it.
    MSCHandle *handle = ALLOCATE(vm, MSCHandle);
    handle->value = value;

    if (IS_OBJ(value)) MSCPopRoot(vm->gc);

    // Add it to the front of the linked list of handles.
    if (vm->handles != NULL) vm->handles->prev = handle;
    handle->prev = NULL;
    handle->next = vm->handles;
    vm->handles = handle;

    return handle;
}

void MSCReleaseHandle(MVM *vm, MSCHandle *handle) {
    ASSERT(handle != NULL, "Handle cannot be NULL.");

    // Update the VM's head pointer if we're releasing the first handle.
    if (vm->handles == handle) vm->handles = handle->next;

    // Unlink it from the list.
    if (handle->prev != NULL) handle->prev->next = handle->next;
    if (handle->next != NULL) handle->next->prev = handle->prev;

    // Clear it out. This isn't strictly necessary since we're going to free it,
    // but it makes for easier debugging.
    handle->prev = NULL;
    handle->next = NULL;
    handle->value = NULL_VAL;
    DEALLOCATE(vm, handle);
}

int MSCGetSlotCount(MVM *vm) {
    if (vm->apiStack == NULL) return 0;

    return (int) (vm->djuru->stackTop - vm->apiStack);
}

// Ensures that [slot] is a valid index into the API's stack of slots.
static void validateApiSlot(MVM *vm, int slot) {
    ASSERT(slot >= 0, "Slot cannot be negative.");
    ASSERT(slot < MSCGetSlotCount(vm), "Not that many slots.");
}

Value *MSCSlotAtUnsafe(MVM *vm, int slot) {
    validateApiSlot(vm, slot);
    return &vm->apiStack[slot];
}

void MSCEnsureSlots(MVM *vm, int numSlots) {
    // If we don't have a djuru accessible, create one for the API to use.
    if (vm->apiStack == NULL) {
        vm->djuru = MSCDjuruFrom(vm, NULL);
        vm->apiStack = vm->djuru->stack;
    }

    int currentSize = (int) (vm->djuru->stackTop - vm->apiStack);
    if (currentSize >= numSlots) return;

    // Grow the stack if needed.
    int needed = (int) (vm->apiStack - vm->djuru->stack) + numSlots;
    MSCEnsureStack(vm->djuru, vm, needed);

    vm->djuru->stackTop = vm->apiStack + numSlots;
}


void MSCAbortDjuru(MVM *vm, int slot) {
    validateApiSlot(vm, slot);
    vm->djuru->error = vm->apiStack[slot];
}


// Gets the type of the object in [slot].
MSCType MSCGetSlotType(MVM *vm, int slot) {
    validateApiSlot(vm, slot);
    if (IS_BOOL(vm->apiStack[slot])) return MSC_TYPE_BOOL;
    if (IS_NUM(vm->apiStack[slot])) return MSC_TYPE_NUM;
    if (IS_FOREIGN(vm->apiStack[slot])) return MSC_TYPE_EXTERN;
    if (IS_LIST(vm->apiStack[slot])) return MSC_TYPE_LIST;
    if (IS_MAP(vm->apiStack[slot])) return MSC_TYPE_MAP;
    if (IS_NULL(vm->apiStack[slot])) return MSC_TYPE_NULL;
    if (IS_STRING(vm->apiStack[slot])) return MSC_TYPE_STRING;

    return MSC_TYPE_UNKNOWN;
}

bool MSCGetSlotBool(MVM *vm, int slot) {
    validateApiSlot(vm, slot);
    ASSERT(IS_BOOL(vm->apiStack[slot]), "Slot must hold a bool.");

    return AS_BOOL(vm->apiStack[slot]);
}

const char *MSCGetSlotBytes(MVM *vm, int slot, int *length) {
    validateApiSlot(vm, slot);
    ASSERT(IS_STRING(vm->apiStack[slot]), "Slot must hold a string.");

    String *string = AS_STRING(vm->apiStack[slot]);
    *length = string->length;
    return string->value;
}

double MSCGetSlotDouble(MVM *vm, int slot) {
    validateApiSlot(vm, slot);
    ASSERT(IS_NUM(vm->apiStack[slot]), "Slot must hold a number.");

    return AS_NUM(vm->apiStack[slot]);
}

void *MSCGetSlotExtern(MVM *vm, int slot) {
    validateApiSlot(vm, slot);
    ASSERT(IS_FOREIGN(vm->apiStack[slot]),
           "Slot must hold a foreign instance.");
    return AS_EXTERN(vm->apiStack[slot])->data;
}

const char *MSCGetSlotString(MVM *vm, int slot) {
    validateApiSlot(vm, slot);
    ASSERT(IS_STRING(vm->apiStack[slot]), "Slot must hold a string.");

    return AS_CSTRING(vm->apiStack[slot]);
}

MSCHandle *MSCGetSlotHandle(MVM *vm, int slot) {
    validateApiSlot(vm, slot);
    return MSCMakeHandle(vm, vm->apiStack[slot]);
}

// Stores [value] in [slot] in the foreign call stack.
static void setSlot(MVM *vm, int slot, Value value) {
    validateApiSlot(vm, slot);
    vm->apiStack[slot] = value;
}

void MSCSetSlotBool(MVM *vm, int slot, bool value) {
    setSlot(vm, slot, BOOL_VAL(value));
}

void MSCSetSlotBytes(MVM *vm, int slot, const char *bytes, size_t length) {
    ASSERT(bytes != NULL, "Byte array cannot be NULL.");
    setSlot(vm, slot, MSCStringFromCharsWithLength(vm, bytes, (uint32_t) length));
}

void MSCSetSlotDouble(MVM *vm, int slot, double value) {
    setSlot(vm, slot, NUM_VAL(value));
}

void *MSCSetSlotNewExtern(MVM *vm, int slot, int classSlot, size_t size) {
    validateApiSlot(vm, slot);
    validateApiSlot(vm, classSlot);
    ASSERT(IS_CLASS(vm->apiStack[classSlot]), "Slot must hold a class.");

    Class *classObj = AS_CLASS(vm->apiStack[classSlot]);
    ASSERT(classObj->numFields == -1, "Class must be a foreign class.");

    Extern *foreign = MSCExternFrom(vm, classObj, size);
    vm->apiStack[slot] = OBJ_VAL(foreign);

    return (void *) foreign->data;
}

void MSCSetSlotNewList(MVM *vm, int slot) {
    setSlot(vm, slot, OBJ_VAL(MSCListFrom(vm, 0)));
}

void MSCSetSlotNewMap(MVM *vm, int slot) {
    setSlot(vm, slot, OBJ_VAL(MSCMapFrom(vm)));
}

void MSCSetSlotNull(MVM *vm, int slot) {
    setSlot(vm, slot, NULL_VAL);
}

void MSCSetSlotString(MVM *vm, int slot, const char *text) {
    ASSERT(text != NULL, "String cannot be NULL.");

    setSlot(vm, slot, MSCStringFromConstChars(vm, text));
}

void MSCSetSlotHandle(MVM *vm, int slot, MSCHandle *handle) {
    ASSERT(handle != NULL, "Handle cannot be NULL.");

    setSlot(vm, slot, handle->value);
}

int MSCGetListCount(MVM *vm, int slot) {
    validateApiSlot(vm, slot);
    ASSERT(IS_LIST(vm->apiStack[slot]), "Slot must hold a list.");

    ValueBuffer elements = AS_LIST(vm->apiStack[slot])->elements;
    return elements.count;
}

void MSCGetListElement(MVM *vm, int listSlot, int index, int elementSlot) {
    validateApiSlot(vm, listSlot);
    validateApiSlot(vm, elementSlot);
    ASSERT(IS_LIST(vm->apiStack[listSlot]), "Slot must hold a list.");

    ValueBuffer elements = AS_LIST(vm->apiStack[listSlot])->elements;

    uint32_t usedIndex = MSCValidateIndex((uint32_t) elements.count, index);
    ASSERT(usedIndex != UINT32_MAX, "Index out of bounds.");

    vm->apiStack[elementSlot] = elements.data[usedIndex];
}

void MSCSetListElement(MVM *vm, int listSlot, int index, int elementSlot) {
    validateApiSlot(vm, listSlot);
    validateApiSlot(vm, elementSlot);
    ASSERT(IS_LIST(vm->apiStack[listSlot]), "Slot must hold a list.");

    List *list = AS_LIST(vm->apiStack[listSlot]);

    uint32_t usedIndex = MSCValidateIndex((uint32_t) list->elements.count, index);
    ASSERT(usedIndex != UINT32_MAX, "Index out of bounds.");

    list->elements.data[usedIndex] = vm->apiStack[elementSlot];
}

void MSCInsertInList(MVM *vm, int listSlot, int index, int elementSlot) {
    validateApiSlot(vm, listSlot);
    validateApiSlot(vm, elementSlot);
    ASSERT(IS_LIST(vm->apiStack[listSlot]), "Must insert into a list.");

    List *list = AS_LIST(vm->apiStack[listSlot]);

    // Negative indices count from the end.
    // We don't use MSCValidateIndex here because insert allows 1 past the end.
    if (index < 0) index = list->elements.count + 1 + index;

    ASSERT(index <= list->elements.count, "Index out of bounds.");
    MSCListInsert(list, vm, vm->apiStack[elementSlot], index);
}

int MSCGetMapCount(MVM *vm, int slot) {
    validateApiSlot(vm, slot);
    ASSERT(IS_MAP(vm->apiStack[slot]), "Slot must hold a map.");

    Map *map = AS_MAP(vm->apiStack[slot]);
    return map->count;
}

bool MSCMapContainsKey(MVM *vm, int mapSlot, int keySlot) {
    validateApiSlot(vm, mapSlot);
    validateApiSlot(vm, keySlot);
    ASSERT(IS_MAP(vm->apiStack[mapSlot]), "Slot must hold a map.");

    Value key = vm->apiStack[keySlot];
    ASSERT(MSCMapIsValidKey(key), "Key must be a value type");
    if (!validateKey(vm, key)) return false;

    Map *map = AS_MAP(vm->apiStack[mapSlot]);
    Value value = MSCMapGet(map, key);

    return !IS_UNDEFINED(value);
}

void MSCGetMapValue(MVM *vm, int mapSlot, int keySlot, int valueSlot) {
    validateApiSlot(vm, mapSlot);
    validateApiSlot(vm, keySlot);
    validateApiSlot(vm, valueSlot);
    ASSERT(IS_MAP(vm->apiStack[mapSlot]), "Slot must hold a map.");

    Map *map = AS_MAP(vm->apiStack[mapSlot]);
    Value value = MSCMapGet(map, vm->apiStack[keySlot]);
    if (IS_UNDEFINED(value)) {
        value = NULL_VAL;
    }

    vm->apiStack[valueSlot] = value;
}

void MSCSetMapValue(MVM *vm, int mapSlot, int keySlot, int valueSlot) {
    validateApiSlot(vm, mapSlot);
    validateApiSlot(vm, keySlot);
    validateApiSlot(vm, valueSlot);
    ASSERT(IS_MAP(vm->apiStack[mapSlot]), "Must insert into a map.");

    Value key = vm->apiStack[keySlot];
    ASSERT(MSCMapIsValidKey(key), "Key must be a value type");

    if (!validateKey(vm, key)) {
        return;
    }

    Value value = vm->apiStack[valueSlot];
    Map *map = AS_MAP(vm->apiStack[mapSlot]);

    MSCMapSet(map, vm, key, value);
}

void MSCRemoveMapValue(MVM *vm, int mapSlot, int keySlot,
                       int removedValueSlot) {
    validateApiSlot(vm, mapSlot);
    validateApiSlot(vm, keySlot);
    ASSERT(IS_MAP(vm->apiStack[mapSlot]), "Slot must hold a map.");

    Value key = vm->apiStack[keySlot];
    if (!validateKey(vm, key)) {
        return;
    }

    Map *map = AS_MAP(vm->apiStack[mapSlot]);
    Value removed = MSCMapRemove(map, vm, key);
    setSlot(vm, removedValueSlot, removed);
}

void MSCGetVariable(MVM *vm, const char *module, const char *name,
                    int slot) {
    ASSERT(module != NULL, "Module cannot be NULL.");
    ASSERT(name != NULL, "Variable name cannot be NULL.");

    Value moduleName = MSCStringFormatted(vm, "$", module);
    MSCPushRoot(vm->gc, AS_OBJ(moduleName));
    Module *moduleObj = MSCGetModule(vm, moduleName);
    ASSERT(moduleObj != NULL, "Could not find module.");

    MSCPopRoot(vm->gc);
    int variableSlot = MSCSymbolTableFind(&moduleObj->variableNames,
                                          name, strlen(name));
    ASSERT(variableSlot != -1, "Could not find variable.");

    setSlot(vm, slot, moduleObj->variables.data[variableSlot]);
}

bool MSCHasVariable(MVM *vm, const char *module, const char *name) {
    ASSERT(module != NULL, "Module cannot be NULL.");
    ASSERT(name != NULL, "Variable name cannot be NULL.");

    Value moduleName = MSCStringFormatted(vm, "$", module);
    MSCPushRoot(vm->gc, AS_OBJ(moduleName));

    //We don't use MSCHasModule since we want to use the module object.
    Module *moduleObj = MSCGetModule(vm, moduleName);
    ASSERT(moduleObj != NULL, "Could not find module.");

    MSCPopRoot(vm->gc); // moduleName.

    int variableSlot = MSCSymbolTableFind(&moduleObj->variableNames,
                                          name, strlen(name));

    return variableSlot != -1;
}

bool MSCHasModule(MVM *vm, const char *module) {
    ASSERT(module != NULL, "Module cannot be NULL.");

    Value moduleName = MSCStringFormatted(vm, "$", module);
    MSCPushRoot(vm->gc, AS_OBJ(moduleName));

    Module *moduleObj = MSCGetModule(vm, moduleName);

    MSCPopRoot(vm->gc); // moduleName.
    return moduleObj != NULL;
}


int MSCDeclareVariable(MVM *vm, Module *module, const char *name,
                       size_t length, int line) {
    if (module->variables.count == MAX_MODULE_VARS) return -2;

    // Implicitly defined variables get a "value" that is the line where the
    // variable is first used. We'll use that later to report an error on the
    // right line.
    MSCWriteValueBuffer(vm, &module->variables, NUM_VAL(line));
    return MSCSymbolTableAdd(vm, &module->variableNames, name, length);
}


int MSCDefineVariable(MVM *vm, Module *module, const char *name, int nameLength, Value value,
                      int *line) {
    if (module->variables.count == MAX_MODULE_VARS) return -2;

    if (IS_OBJ(value)) MSCPushRoot(vm->gc, AS_OBJ(value));

    // See if the variable is already explicitly or implicitly declared.
    int symbol = MSCSymbolTableFind(&module->variableNames, name, (size_t) nameLength);

    if (symbol == -1) {
        // Brand new variable.
        symbol = MSCSymbolTableAdd(vm, &module->variableNames, name, (size_t) nameLength);
        // printf("Defining new variable:: %s %d [%s]\n", name, symbol, module->variableNames.data[0]->value);
        MSCWriteValueBuffer(vm, &module->variables, value);
    } else if (IS_NUM(module->variables.data[symbol])) {
        // An implicitly declared variable's value will always be a number.
        // Now we have a real definition.
        if (line) *line = (int) AS_NUM(module->variables.data[symbol]);
        module->variables.data[symbol] = value;

        // If vm was a localname we want to error if it was
        // referenced before vm definition.
        //???if (isLocalName(name)) symbol = -3;
    } else {
        // Already explicitly declared.
        symbol = -1;
    }

    if (IS_OBJ(value)) MSCPopRoot(vm->gc);

    return symbol;
}


Closure *MSCCompileSource(MVM *vm, const char *module, const char *source,
                          bool isExpression, bool printErrors) {
    Value nameValue = NULL_VAL;
    if (module != NULL) {
        nameValue = MSCStringFromConstChars(vm, module);
        MSCPushRoot(vm->gc, AS_OBJ(nameValue));
    }

    Closure *closure = compileInModule(vm, nameValue, source,
                                       isExpression, printErrors);

    if (module != NULL) MSCPopRoot(vm->gc); // nameValue.

    return closure;
}


MSCInterpretResult MSCInterpret(MVM *vm, const char *module,
                                const char *source) {

    Closure *closure = MSCCompileSource(vm, module, source, false, true);

    if (closure == NULL) return RESULT_COMPILATION_ERROR;

    MSCPushRoot(vm->gc, (Object *) closure);
    Djuru *thread = MSCDjuruFrom(vm, closure);
    MSCPopRoot(vm->gc); // closure.
    vm->apiStack = NULL;

    // return RESULT_SUCCESS;
    return runInterpreter(vm, thread);
}

