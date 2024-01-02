//
// Created by Mahamadou DOUMBIA [OML DSI] on 11/01/2022.
//

#include "Value.h"
#include "GC.h"
#include "../runtime/MVM.h"
#include "../builtin/Core.h"
#include "../runtime/debuger.h"
#include <math.h>
#include <stdarg.h>


DEFINE_BUFFER(Value, Value);

DEFINE_BUFFER(Method, Method);

DEFINE_BUFFER(Field, Field);

#define INITIAL_CALL_FRAMES 4


static void initObj(MVM *vm, Object *obj, ObjType type, Class *classObj) {
    obj->type = type;
    obj->isDark = false;
    obj->classObj = classObj;
    obj->next = vm->gc->first;
    vm->gc->first = obj;
}

/** End of object implementation */

// Calculates and stores the hash code for [string].
static void hashString(String *string) {
    // FNV-1a hash. See: http://www.isthe.com/chongo/tech/comp/fnv/
    uint32_t hash = 2166136261u;

    // This is O(n) on the length of the string, but we only call this when a new
    // string is created. Since the creation is also O(n) (to copy/initialize all
    // the bytes), we allow this here.
    for (uint32_t i = 0; i < string->length; i++) {
        hash ^= string->value[i];
        hash *= 16777619;
    }
    string->hash = hash;
}

static inline uint32_t hashBits(uint64_t hash) {
    // From v8's ComputeLongHash() which in turn cites:
    // Thomas Wang, Integer Hash Functions.
    // http://www.concentric.net/~Ttwang/tech/inthash.htm
    hash = ~hash + (hash << 18);  // hash = (hash << 18) - hash - 1;
    hash = hash ^ (hash >> 31);
    hash = hash * 21;  // hash = (hash + (hash << 2)) + (hash << 4);
    hash = hash ^ (hash >> 11);
    hash = hash + (hash << 6);
    hash = hash ^ (hash >> 22);
    return (uint32_t) (hash & 0x3fffffff);
}

// Generates a hash code for [num].
static inline uint32_t hashNumber(double num) {
    // Hash the raw bits of the value.
    return hashBits(MSCDoubleToBits(num));
}

// Generates a hash code for [object].
static uint32_t hashObject(Object *object) {
    switch (object->type) {
        case OBJ_CLASS:
            // Classes just use their name.
            return hashObject((Object *) ((Class *) object)->name);

            // Allow bare (non-closure) functions so that we can use a map to find
            // existing constants in a function's constant table. This is only used
            // internally. Since user code never sees a non-closure function, they
            // cannot use them as map keys.
        case OBJ_FN: {
            Function *fn = (Function *) object;
            return hashNumber(fn->arity) ^ hashNumber(fn->code.count);
        }
        case OBJ_RANGE: {
            Range* range = (Range*)object;
            return hashNumber(range->from) ^ hashNumber(range->to);
        }

        case OBJ_STRING:
            return ((String *) object)->hash;
        default:
            ASSERT(false, "Only immutable objects can be hashed.");
            return 0;
    }
}


// Generates a hash code for [value], which must be one of the built-in
// immutable types: null, bool, class, num, range, or string.
static uint32_t hashValue(Value value) {
    // TODO: We'll probably want to randomize this at some point.

#if MSC_NAN_TAGGING
    if (IS_OBJ(value)) return hashObject(AS_OBJ(value));

    // Hash the raw bits of the unboxed value.
    return hashBits(value);
#else
    switch (value.type) {
        case VAL_FALSE:
            return 0;
        case VAL_NULL:
            return 1;
        case VAL_NUM:
            return hashNumber(AS_NUM(value));
        case VAL_TRUE:
            return 2;
        case VAL_OBJ:
            return hashObject(AS_OBJ(value));
        default:
            UNREACHABLE();
    }
    return 0;
#endif
}

// Looks for an entry with [key] in an array of [capacity] [entries].
//
// If found, sets [result] to point to it and returns `true`. Otherwise,
// returns `false` and points [result] to the entry where the key/value pair
// should be inserted.
static bool findEntry(MapEntry *entries, uint32_t capacity, Value key,
                      MapEntry **result) {
    // If there is no entry array (an empty map), we definitely won't find it.
    if (capacity == 0) return false;

    // Figure out where to insert it in the table. Use open addressing and
    // basic linear probing.
    uint32_t startIndex = hashValue(key) % capacity;
    uint32_t index = startIndex;

    // If we pass a tombstone and don't end up finding the key, its entry will
    // be re-used for the insert.
    MapEntry *tombstone = NULL;

    // Walk the probe sequence until we've tried every slot.
    do {
        MapEntry *entry = &entries[index];

        if (IS_UNDEFINED(entry->key)) {
            // If we found an empty slot, the key is not in the table. If we found a
            // slot that contains a deleted key, we have to keep looking.
            if (IS_FALSE(entry->value)) {
                // We found an empty slot, so we've reached the end of the probe
                // sequence without finding the key. If we passed a tombstone, then
                // that's where we should insert the item, otherwise, put it here at
                // the end of the sequence.
                *result = tombstone != NULL ? tombstone : entry;
                return false;
            } else {
                // We found a tombstone. We need to keep looking in case the key is
                // after it, but we'll use this entry as the insertion point if the
                // key ends up not being found.
                if (tombstone == NULL) tombstone = entry;
            }
        } else if (MSCValuesEqual(entry->key, key)) {
            // We found the key.
            *result = entry;
            return true;
        }

        // Try the next slot.
        index = (index + 1) % capacity;
    } while (index != startIndex);

    // If we get here, the table is full of tombstones. Return the first one we
    // found.
    ASSERT(tombstone != NULL, "Map should have tombstones or empty entries.");
    *result = tombstone;
    return false;
}

Class *MSCSingleClass(MVM *vm, int numFields, String *name) {
    Class *classObj = ALLOCATE(vm, Class);
    initObj(vm, &classObj->obj, OBJ_CLASS, NULL);
    classObj->superclass = NULL;
    classObj->numFields = numFields;
    classObj->name = name;

    MSCPushRoot(vm->gc, (Object *) classObj);
    MSCInitMethodBuffer(&classObj->methods);
    MSCInitFieldBuffer(&classObj->fields);
    MSCPopRoot(vm->gc);

    return classObj;
}

Class *MSCClassFrom(MVM *vm, Class *parent, int numOfFields, String *name) {
    // this->init(vm, name, numOfFields);

// Create the metaclass.
    Value metaclassName = MSCStringFormatted(vm, "@ metaclass", OBJ_VAL(name));
    MSCPushRoot(vm->gc, AS_OBJ(metaclassName));

    Class *metaclass = MSCSingleClass(vm, 0, AS_STRING(metaclassName));
    metaclass->obj.classObj = vm->core.classClass;
    MSCPopRoot(vm->gc);


    MSCPushRoot(vm->gc, (Object *) metaclass);
    // Metaclasses always inherit Class and do not parallel the non-metaclass
    // hierarchy.
    MSCBindSuperclass(metaclass, vm, vm->core.classClass);

    Class *thisClass = MSCSingleClass(vm, numOfFields, name);

    // Make sure the class isn't collected while the inherited methods are being
    // bound.
    MSCPushRoot(vm->gc, (Object *) thisClass);
    thisClass->obj.classObj = metaclass;

    MSCBindSuperclass(thisClass, vm, parent);

    MSCPopRoot(vm->gc);
    MSCPopRoot(vm->gc);

    return thisClass;
}

void MSCBlackenClass(Class *thisClass, MVM *vm) {
    // Object::blacken(vm);
    // The metaclass.
    // if(this->classObj != NULL) MSCGrayObject((Object*)this->classObj, vm);
    MSCGrayObject((Object *) thisClass->obj.classObj, vm);
    // The superclass.
    MSCGrayObject((Object *) thisClass->superclass, vm);

    // Method function objects.
    for (int i = 0; i < thisClass->methods.count; i++) {
        if (thisClass->methods.data[i].type == METHOD_BLOCK) {
            MSCGrayObject((Object *) thisClass->methods.data[i].as.closure, vm);
        }
    }

    MSCGrayObject((Object *) thisClass->name, vm);

    // Keep track of how much memory is still in use.
    vm->gc->bytesAllocated += sizeof(Class);
    vm->gc->bytesAllocated += thisClass->methods.capacity * sizeof(Method);
    vm->gc->bytesAllocated += thisClass->fields.capacity * sizeof(Field);
}


void MSCBindSuperclass(Class *thisClass, MVM *vm, Class *superclass) {
    ASSERT(superclass != NULL, "Must have superclass.");
    // Inherit fields from its superclass.

    for (int i = 0; i < superclass->numFields; i++) {
        MSCBindField(thisClass, vm, superclass->numFields + i, superclass->fields.data[i]);
    }
    thisClass->superclass = superclass;
    // Include the superclass in the total number of fields.
    if (thisClass->numFields != -1) {
        thisClass->numFields += superclass->numFields;
    } else {
        ASSERT(superclass->numFields == 0,
               "A foreign class cannot inherit from a class with fields.");
    }

    // Inherit methods from its superclass.
    for (int i = 0; i < superclass->methods.count; i++) {
        MSCBindMethod(thisClass, vm, i, superclass->methods.data[i]);
    }

}

void MSCBindMethod(Class *thisClass, MVM *vm, int symbol, Method method) {
    // Make sure the buffer is big enough to contain the symbol's index.
    if (symbol >= thisClass->methods.count) {
        Method noMethod;
        noMethod.type = METHOD_NONE;
        MSCFillMethodBuffer(vm, &thisClass->methods, noMethod,
                            symbol - thisClass->methods.count + 1);
    }

    thisClass->methods.data[symbol] = method;
}

void MSCBindField(Class *thisClass, MVM *vm, int symbol, Field field) {
    // Make sure the buffer is big enough to contain the symbol's index.
    if (symbol >= thisClass->fields.count) {
        Field noField;
        noField.isStatic = false;
        noField.defaultValue = NULL_VAL;
        MSCFillFieldBuffer(vm, &thisClass->fields, noField,
                           symbol - thisClass->fields.count + 1);
    }
    thisClass->fields.data[symbol] = field;
}

void MSCInitClass(Class *thisClass, MVM *vm, String *name, int numOfFields) {
    thisClass->numFields = numOfFields;
    thisClass->name = name;
    thisClass->superclass = NULL;
    thisClass->name = name;
    // thisClass->attributes = NULL_VAL;

    MSCPushRoot(vm->gc, (Object *) thisClass);
    MSCInitMethodBuffer(&thisClass->methods);
    MSCPopRoot(vm->gc);
}


void MSCBlackenObject(Object *thisObj, MVM *vm) {
#if MSC_DEBUG_TRACE_MEMORY
    printf("mark ");
  MSCDumpValue(OBJ_VAL(thisObj));
  printf(" @ %p\n", thisObj);
#endif
    // Traverse the object's fields.
    switch (thisObj->type) {
        case OBJ_CLASS:
            MSCBlackenClass((Class *) thisObj, vm);
            break;
        case OBJ_CLOSURE:
            MSCBlackenClosure((Closure *) thisObj, vm);
            break;
        case OBJ_THREAD:
            MSCBlackenDjuru((Djuru *) thisObj, vm);
            break;
        case OBJ_FN:
            MSCBlackenFunction((Function *) thisObj, vm);
            break;
        case OBJ_EXTERN:
            MSCBlackenExtern((Extern *) thisObj, vm);
            break;
        case OBJ_INSTANCE:
            MSCBlackenInstance((Instance *) thisObj, vm);
            break;
        case OBJ_LIST:
            MSCBlackenList((List *) thisObj, vm);
            break;
        case OBJ_MAP:
            MSCBlackenMap((Map *) thisObj, vm);
            break;
        case OBJ_MODULE:
            MSCBlackenModule((Module *) thisObj, vm);
            break;
        case OBJ_RANGE:
            MSCBlackenRange((Range *) thisObj, vm);
            break;
        case OBJ_STRING:
            MSCBlackenString((String *) thisObj, vm);
            break;
        case OBJ_UPVALUE:
            MSCBlackenUpvalue((Upvalue *) thisObj, vm);
            break;
    }
}

void MSCFreeObject(Object *thisObj, MVM *vm) {
#if MSC_DEBUG_TRACE_MEMORY
    printf("free ");
  MSCDumpValue(OBJ_VAL(thisObj));
  printf(" @ %p\n", thisObj);
#endif
    switch (thisObj->type) {
        case OBJ_CLASS:
            MSCFreeMethodBuffer(vm, &((Class *) thisObj)->methods);
            break;

        case OBJ_THREAD: {
            DEALLOCATE(vm, ((Djuru *) thisObj)->frames);
            DEALLOCATE(vm, ((Djuru *) thisObj)->stack);
            break;
        }

        case OBJ_FN: {
            Function *fn = (Function *) thisObj;

            MSCFreeValueBuffer(vm, &fn->constants);
            MSCFreeByteBuffer(vm, &fn->code);
            MSCFreeIntBuffer(vm, &fn->debug->sourceLines);
            DEALLOCATE(vm, fn->debug->name);
            DEALLOCATE(vm, fn->debug);
            break;
        }

        case OBJ_EXTERN:
            MSCFinalizeExtern(vm, (Extern *) thisObj);
            break;

        case OBJ_LIST:
            MSCFreeValueBuffer(vm, &((List *) thisObj)->elements);
            break;

        case OBJ_MAP:
            DEALLOCATE(vm, ((Map *) thisObj)->entries);
            break;

        case OBJ_MODULE:
            MSCFreeStringBuffer(vm, &((Module *) thisObj)->variableNames);
            MSCFreeValueBuffer(vm, &((Module *) thisObj)->variables);
            break;

        case OBJ_CLOSURE:
            // ((Closure*)this)->_free(vm);
            break;
        case OBJ_INSTANCE:
            // ((Instance*)this)->_free(vm);
            break;
        case OBJ_STRING:
            // ((String*)this)->_free(vm);
            break;
        case OBJ_UPVALUE:
            // ((Upvalue*)this)->_free(vm);
            break;
        case OBJ_RANGE:
            break;
    }
    // delete this;
    DEALLOCATE(vm, thisObj);
}


void MSCGrayObject(Object *thisObj, MVM *vm) {
    if (thisObj == NULL) {
        return;
    }
    // Stop if the object is already darkened so we don't get stuck in a cycle.
    if (thisObj->isDark) return;
    // It's been reached.
    thisObj->isDark = true;
    // Add it to the gray list so it can be recursively explored for
    // more marks later.
    if (vm->gc->grayCount >= vm->gc->grayCapacity) {
        vm->gc->grayCapacity = vm->gc->grayCount * 2;
        vm->gc->gray = (Object **) vm->gc->reallocator(vm->gc->gray,
                                                       vm->gc->grayCapacity * sizeof(Object *),
                                                       vm->gc->userData);
    }

    vm->gc->gray[vm->gc->grayCount++] = thisObj;
}


void MSCBlackenClosure(Closure *closure, MVM *vm) {
    // Object::blacken(vm);
    // Mark the function.
    MSCGrayObject((Object *) closure->fn, vm);
    // Mark the upvalues.
    for (int i = 0; i < closure->fn->numUpvalues; i++) {
        MSCGrayObject((Object *) closure->upvalues[i], vm);
    }
    // Keep track of how much memory is still in use.
    vm->gc->bytesAllocated += sizeof(Closure);
    vm->gc->bytesAllocated += sizeof(Upvalue *) * closure->fn->numUpvalues;
}

Closure *MSCClosureFrom(MVM *vm, Function *fn) {
    Closure *closure = ALLOCATE_FLEX(vm, Closure,
                                     Upvalue*, fn->numUpvalues);
    initObj(vm, &closure->obj, OBJ_CLOSURE, vm->core.fnClass);
    closure->fn = fn;
    // closure->upvalues = ALLOCATE_ARRAY(vm, Upvalue*, fn->numUpvalues);
    // Clear the upvalue array. We need to do this in case a GC is triggered
    // after the closure is created but before the upvalue array is populated.
    for (int i = 0; i < fn->numUpvalues; i++) closure->upvalues[i] = NULL;
    return closure;
}


Upvalue *MSCUpvalueFrom(MVM *vm, Value *value) {
    Upvalue *upvalue = ALLOCATE(vm, Upvalue);
    // Upvalues are never used as first-class objects, so don't need a class.
    initObj(vm, &upvalue->obj, OBJ_UPVALUE, NULL);

    upvalue->value = value;
    upvalue->closed = NULL_VAL;
    upvalue->next = NULL;
    return upvalue;
}

void MSCBlackenUpvalue(Upvalue *upvalue, MVM *vm) {
    // Object::blacken(vm);
    // Mark the closed-over object (in case it is closed).
    MSCGrayValue(vm, upvalue->closed);

    // Keep track of how much memory is still in use.
    vm->gc->bytesAllocated += sizeof(Upvalue);
}


Djuru *MSCDjuruFrom(MVM *vm, Closure *closure) {
    // Allocate the arrays before the fiber in case it triggers a GC.
    CallFrame *frames = ALLOCATE_ARRAY(vm, CallFrame, INITIAL_CALL_FRAMES);
    // Add one slot for the unused implicit receiver slot that the compiler
    // assumes all functions have.
    int stackCapacity = closure == NULL ? 1 : powerOf2Ceil(closure->fn->maxSlots + 1);
    Djuru *thread = ALLOCATE(vm, Djuru);
    initObj(vm, &thread->obj, OBJ_THREAD, vm->core.djuruClass);
    MSCPushRoot(vm->gc, (Object *) thread);

    Value *stack = ALLOCATE_ARRAY(vm, Value, stackCapacity);
    thread->stack = stack;
    thread->stackTop = thread->stack;
    thread->stackCapacity = stackCapacity;

    thread->frames = frames;
    thread->frameCapacity = INITIAL_CALL_FRAMES;
    thread->numOfFrames = 0;

    thread->openUpvalues = NULL;
    thread->caller = NULL;
    thread->error = NULL_VAL;
    thread->state = DJURU_OTHER;

    if (closure != NULL) {
        // Initialize the first call frame.
        MSCAppendCallFrame(thread, closure, thread->stack);

        // The first slot always holds the closure.
        thread->stackTop[0] = OBJ_VAL(closure);
        thread->stackTop++;
    }

    MSCPopRoot(vm->gc);
    // printf("Creating Djuru... %p\n", thread);
    return thread;

}

void MSCBlackenDjuru(Djuru *djuru, MVM *vm) {
    // Object::blacken(vm);
    // Stack functions.
    for (int i = 0; i < djuru->numOfFrames; i++) {
        MSCGrayObject((Object *) djuru->frames[i].closure, vm);
    }

    // Stack variables.
    for (Value *slot = djuru->stack; slot < djuru->stackTop; slot++) {
        MSCGrayValue(vm, *slot);
    }

    // Open upvalues.
    Upvalue *upvalue = djuru->openUpvalues;
    while (upvalue != NULL) {
        MSCGrayObject((Object *) upvalue, vm);
        upvalue = upvalue->next;
    }

    // The caller.
    MSCGrayObject((Object *) djuru->caller, vm);
    MSCGrayValue(vm, djuru->error);

    // Keep track of how much memory is still in use.
    vm->gc->bytesAllocated += sizeof(Djuru);
    vm->gc->bytesAllocated += djuru->frameCapacity * sizeof(CallFrame);
    vm->gc->bytesAllocated += djuru->stackCapacity * sizeof(Value);
}


void MSCEnsureStack(Djuru *djuru, MVM *vm, int needed) {
    if (djuru->stackCapacity >= needed) return;

    int capacity = powerOf2Ceil(needed);
    Value *oldStack = djuru->stack;
    djuru->stack = (Value *) MSCReallocate(vm->gc, djuru->stack,
                                           sizeof(Value) * djuru->stackCapacity,
                                           sizeof(Value) * capacity);
    djuru->stackCapacity = capacity;

    // If the reallocation moves the stack, then we need to recalculate every
    // pointer that points into the old stack to into the same relative distance
    // in the new stack. We have to be a little careful about how these are
    // calculated because pointer subtraction is only well-defined within a
    // single array, hence the slightly redundant-looking arithmetic below.
    if (djuru->stack != oldStack) {
        // Top of the stack.
        if (vm->apiStack >= oldStack && vm->apiStack <= djuru->stackTop) {
            vm->apiStack = djuru->stack + (vm->apiStack - oldStack);
        }

        // Stack pointer for each call frame.
        for (int i = 0; i < djuru->numOfFrames; i++) {
            CallFrame *frame = &djuru->frames[i];
            frame->stackStart = djuru->stack + (frame->stackStart - oldStack);
        }

        // Open upvalues.
        for (Upvalue *upvalue = djuru->openUpvalues;
             upvalue != NULL;
             upvalue = upvalue->next) {
            upvalue->value = djuru->stack + (upvalue->value - oldStack);
        }

        djuru->stackTop = djuru->stack + (djuru->stackTop - oldStack);
    }
}

// Adds a new [CallFrame] to [fiber] invoking [closure] whose stack starts at
// [stackStart].
void MSCAppendCallFrame(Djuru *djuru, Closure *closure, Value *stackStart) {

    // The caller should have ensured we already have enough capacity.
    ASSERT(djuru->frameCapacity > djuru->numOfFrames, "No memory for call frame.");

    CallFrame *frame = &djuru->frames[djuru->numOfFrames++];
    frame->stackStart = stackStart;
    frame->closure = closure;
    frame->ip = closure->fn->code.data;

}

bool MSCHasError(Djuru *djuru) {
    return !IS_NULL(djuru->error);
}

void MSCBlackenExtern(Extern *externObj, MVM *vm) {
    // Object::blacken(vm);
}


Extern *MSCExternFrom(MVM *vm, Class *objClass, size_t size) {
    Extern *object = ALLOCATE_FLEX(vm, Extern, uint8_t, size);
    initObj(vm, &object->obj, OBJ_EXTERN, objClass);
    // Zero out the bytes.
    memset(object->data, 0, size);
    return object;
}

Instance *MSCInstanceFrom(MVM *vm, Class *classObj) {
    Instance *instance = ALLOCATE_FLEX(vm, Instance,
                                       Value, classObj->numFields);
    initObj(vm, &instance->obj, OBJ_INSTANCE, classObj);
    // Initialize fields to null.
    for (int i = 0; i < classObj->numFields; i++) {
        Field field = classObj->fields.data[i];
        if (!field.isStatic) {
            instance->fields[i] = classObj->fields.data[i].defaultValue;
        }

    }
    return instance;
}

void MSCBlackenInstance(Instance *instance, MVM *vm) {
    // Object::blacken(vm);
    MSCGrayObject((Object *) instance->obj.classObj, vm);
    // Mark the fields.
    for (int i = 0; i < instance->obj.classObj->numFields; i++) {
        MSCGrayValue(vm, instance->fields[i]);
    }

    // Keep track of how much memory is still in use.
    vm->gc->bytesAllocated += sizeof(Instance);
    vm->gc->bytesAllocated += sizeof(Value) * instance->obj.classObj->numFields;
}


List *MSCListFrom(MVM *vm, int numElements) {
    // Allocate this before the list object in case it triggers a GC which would
    // free the list.
    Value *elements = NULL;
    if (numElements > 0) {
        elements = ALLOCATE_ARRAY(vm, Value, numElements);
    }
    List *list = ALLOCATE(vm, List);
    initObj(vm, &list->obj, OBJ_LIST, vm->core.listClass);

    list->elements.capacity = numElements;
    list->elements.count = numElements;
    list->elements.data = elements;
    return list;
}

void MSCBlackenList(List *list, MVM *vm) {
    // Object::blacken(vm);
    // Mark the elements.
    MSCGrayBuffer(vm, &list->elements);

    // Keep track of how much memory is still in use.
    vm->gc->bytesAllocated += sizeof(List);
    vm->gc->bytesAllocated += sizeof(Value) * list->elements.capacity;
}


void MSCListInsert(List *list, MVM *vm, Value value, int index) {
    if (IS_OBJ(value)) MSCPushRoot(vm->gc, AS_OBJ(value));

    // Add a slot at the end of the list.
    MSCWriteValueBuffer(vm, &list->elements, NULL_VAL);

    if (IS_OBJ(value)) MSCPopRoot(vm->gc);

    // Shift the existing elements down.
    for (int i = list->elements.count - 1; i > index; i--) {
        list->elements.data[i] = list->elements.data[i - 1];
    }

    // Store the new element.
    list->elements.data[index] = value;
}

int MSCListIndexOf(List *list, MVM *vm, Value value) {
    int count = list->elements.count;
    for (int i = 0; i < count; i++) {
        Value item = list->elements.data[i];
        if (MSCValuesEqual(item, value)) {
            return i;
        }
    }
    return -1;
}

Value MSCListRemoveAt(List *list, MVM *vm, uint32_t index) {
    Value removed = list->elements.data[index];

    if (IS_OBJ(removed)) MSCPushRoot(vm->gc, AS_OBJ(removed));

    // Shift items up.
    for (int i = index; i < list->elements.count - 1; i++) {
        list->elements.data[i] = list->elements.data[i + 1];
    }

    // If we have too much excess capacity, shrink it.
    if (list->elements.capacity / LIST_GROW_FACTOR >= list->elements.count) {
        list->elements.data = (Value *) MSCReallocate(vm->gc, list->elements.data,
                                                      sizeof(Value) * list->elements.capacity,
                                                      sizeof(Value) *
                                                      (list->elements.capacity / LIST_GROW_FACTOR));
        list->elements.capacity /= LIST_GROW_FACTOR;
    }

    if (IS_OBJ(removed)) MSCPopRoot(vm->gc);

    list->elements.count--;
    return removed;
}

Map *MSCMapFrom(MVM *vm) {
    Map *map = ALLOCATE(vm, Map);
    initObj(vm, &map->obj, OBJ_MAP, vm->core.mapClass != NULL ? vm->core.mapClass : NULL);
    map->capacity = 0;
    map->count = 0;
    map->entries = NULL;
    return map;
}

void MSCBlackenMap(Map *map, MVM *vm) {
    // Object::blacken(vm);
    // Mark the entries.
    for (uint32_t i = 0; i < map->capacity; i++) {
        MapEntry *entry = &map->entries[i];
        if (IS_UNDEFINED(entry->key)) continue;

        MSCGrayValue(vm, entry->key);
        MSCGrayValue(vm, entry->value);
    }

    // Keep track of how much memory is still in use.
    vm->gc->bytesAllocated += sizeof(Map);
    vm->gc->bytesAllocated += sizeof(MapEntry) * map->capacity;
}


bool insertEntries(MapEntry *entries, uint32_t capacitry, Value key, Value value) {
    ASSERT(entries != NULL, "Should ensure capacity before inserting.");

    MapEntry *entry;
    if (findEntry(entries, capacitry, key, &entry)) {
        // Already present, so just replace the value.
        entry->value = value;
        return false;
    } else {
        entry->key = key;
        entry->value = value;
        return true;
    }
}

void MSCMapResize(Map *map, MVM *vm, uint32_t capacity) {
// Create the new empty hash table.
    MapEntry *entries = ALLOCATE_ARRAY(vm, MapEntry, capacity);
    for (uint32_t i = 0; i < capacity; i++) {
        entries[i].key = UNDEFINED_VAL;
        entries[i].value = FALSE_VAL;
    }

    // Re-add the existing entries.
    if (map->capacity > 0) {
        for (uint32_t i = 0; i < map->capacity; i++) {
            MapEntry *entry = &map->entries[i];

            // Don't copy empty entries or tombstones.
            if (IS_UNDEFINED(entry->key)) continue;

            insertEntries(entries, capacity, entry->key, entry->value);
        }
    }

    // Replace the array.
    DEALLOCATE(vm, map->entries);
    map->entries = entries;
    map->capacity = capacity;
}

Value MSCMapGet(Map *map, Value key) {
    MapEntry *entry;
    if (findEntry(map->entries, map->capacity, key, &entry)) return entry->value;

    return UNDEFINED_VAL;
}

void MSCMapAddAll(Map *map, MVM *vm, Map *other) {
    for (uint32_t i = 0; i < other->capacity; i++) {
        MapEntry *entry = &other->entries[i];
        if (IS_UNDEFINED(entry->key)) continue;
        MSCMapSet(map, vm, entry->key, entry->value);
    }
}

void MSCMapSet(Map *map, MVM *vm, Value key, Value value) {
// If the map is getting too full, make room first.
    if (map->count + 1 > map->capacity * MAP_LOAD_PERCENT / 100) {
        // Figure out the new hash table size.
        uint32_t capacity = map->capacity * MAP_GROW_FACTOR;
        if (capacity < MAP_MIN_CAPACITY) capacity = MAP_MIN_CAPACITY;

        MSCMapResize(map, vm, capacity);
    }
    if (insertEntries(map->entries, map->capacity, key, value)) {
        // A new key was added.
        map->count++;
    }
}

void MSCMapClear(Map *map, MVM *vm) {
    DEALLOCATE(vm, map->entries);
    map->entries = NULL;
    map->capacity = 0;
    map->count = 0;
}

Value MSCMapRemove(Map *map, MVM *vm, Value key) {
    MapEntry *entry;
    if (!findEntry(map->entries, map->capacity, key, &entry)) return NULL_VAL;

    // Remove the entry from the map. Set map value to true, which marks it as a
    // deleted slot. When searching for a key, we will stop on empty slots, but
    // continue past deleted slots.
    Value value = entry->value;
    entry->key = UNDEFINED_VAL;
    entry->value = TRUE_VAL;

    if (IS_OBJ(value)) MSCPushRoot(vm->gc, AS_OBJ(value));
    map->count--;

    if (map->count == 0) {
        // Removed the last item, so free the array.
        MSCMapClear(map, vm);
    } else if (map->capacity > MAP_MIN_CAPACITY &&
               map->count < map->capacity / MAP_GROW_FACTOR * MAP_LOAD_PERCENT / 100) {
        uint32_t capacity = map->capacity / MAP_GROW_FACTOR;
        if (capacity < MAP_MIN_CAPACITY) capacity = MAP_MIN_CAPACITY;

        // The map is getting empty, so shrink the entry array back down.
        // TODO: Should we do map less aggressively than we grow?
        MSCMapResize(map, vm, capacity);
    }

    if (IS_OBJ(value)) MSCPopRoot(vm->gc);
    return value;
}


void MSCBlackenModule(Module *module, MVM *vm) {
    // Object::blacken(vm);
    // Top-level variables.
    for (int i = 0; i < module->variables.count; i++) {
        MSCGrayValue(vm, module->variables.data[i]);
    }

    MSCBlackenSymbolTable(vm, &module->variableNames);


    if (module->name != NULL) MSCGrayObject((Object *) module->name, vm);
    // Keep track of how much memory is still in use.
    vm->gc->bytesAllocated += sizeof(Module);
}


Module *MSCModuleFrom(MVM *vm, String *name) {
    Module *module = ALLOCATE(vm, Module);
    // Modules are never used as first-class objects, so don't need a class.
    initObj(vm, &module->obj, OBJ_MODULE, NULL);
    MSCPushRoot(vm->gc, (Object *) module);
    MSCSymbolTableInit(&module->variableNames);
    MSCInitValueBuffer(&module->variables);
    module->name = name;
    MSCPopRoot(vm->gc);
    return module;
}


Value MSCStringFromCharsWithLength(MVM *vm, const char *text, uint32_t length) {
    // Allow NULL if the string is empty since byte buffers don't allocate any
    // characters for a zero-length string.
    String *string = MSCStringNew(vm, text, length);
    return OBJ_VAL(string);
}

Value MSCStringFromConstChars(MVM *vm, const char *cstr) {
    return MSCStringFromCharsWithLength(vm, cstr, (uint32_t) strlen(cstr));
}

String *MSCStringNew(MVM *vm, const char *text, uint32_t length) {
    ASSERT(length == 0 || text != NULL, "Unexpected NULL string.");
    String *string = MSCStringAllocate(vm, length);
    // Copy the string (if given one).
    if (length > 0 && text != NULL) memcpy(string->value, text, length);
    // printf("\nInit string %s", this->value);
    hashString(string);
    return string;
}

String *MSCStringAllocate(MVM *vm, uint32_t length) {
    String *string = ALLOCATE_FLEX(vm, String, char, length + 1);
    initObj(vm, &string->obj, OBJ_STRING, vm->core.stringClass);
    string->length = (int) length;
    string->value[length] = '\0';

    return string;
}

void MSCBlackenString(String *string, MVM *vm) {
    // Object::blacken(vm);
    vm->gc->bytesAllocated += sizeof(String) + string->length + 1;
}


Value MSCStringFromNum(MVM *vm, double value) {
    // Edge case: If the value is NaN or infinity, different versions of libc
    // produce different outputs (some will format it signed and some won't). To
    // get reliable output, handle it ourselves.
    if (isnan(value)) return CONST_STRING(vm, "nan");
    if (isinf(value)) {
        if (value > 0.0) {
            return CONST_STRING(vm, "infinity");
        } else {
            return CONST_STRING(vm, "-infinity");
        }
    }

    // This is large enough to hold any double converted to a string using
    // "%.14g". Example:
    //
    //     -1.12345678901234e-1022
    //
    // So we have:
    //
    // + 1 char for sign
    // + 1 char for digit
    // + 1 char for "."
    // + 14 chars for decimal digits
    // + 1 char for "e"
    // + 1 char for "-" or "+"
    // + 4 chars for exponent
    // + 1 char for "\0"
    // = 24
    char buffer[24];
    int length = sprintf(buffer, "%.14g", value);
    return MSCStringFromCharsWithLength(vm, buffer, (uint32_t) length);
}


Value MSCStringFromCodePoint(MVM *vm, int value) {
    int length = MSCUtf8EncodeNumBytes(value);
    ASSERT(length != 0, "Value out of range.");
    String *string = MSCStringAllocate(vm, (uint32_t) length);
    MSCUtf8Encode(value, (uint8_t *) string->value);
    hashString(string);
    return OBJ_VAL(string);
}

Value MSCStringFromByte(MVM *vm, uint8_t value) {
    int length = 1;
    String *string = MSCStringAllocate(vm, (uint32_t) length);
    string->value[0] = value;
    hashString(string);
    return OBJ_VAL(string);
}

Value MSCStringFormatted(MVM *vm, const char *format, ...) {
    va_list argList;

    // Calculate the length of the result string. Do this up front so we can
    // create the final string with a single allocation.
    va_start(argList, format);
    size_t totalLength = 0;
    for (const char *c = format; *c != '\0'; c++) {
        switch (*c) {
            case '$':
                totalLength += strlen(va_arg(argList,
                                             const char*));
                break;

            case '@':
                totalLength += AS_STRING(va_arg(argList, Value))->length;
                break;

            default:
                // Any other character is interpreted literally.
                totalLength++;
        }
    }
    va_end(argList);

    // Concatenate the string.
    String *result = MSCStringAllocate(vm, (uint32_t) totalLength);

    va_start(argList, format);
    char *start = result->value;
    for (const char *c = format; *c != '\0'; c++) {
        switch (*c) {
            case '$': {
                const char *string = va_arg(argList,
                                            const char*);
                size_t length = strlen(string);
                memcpy(start, string, length);
                start += length;
                break;
            }

            case '@': {
                String *string = AS_STRING(va_arg(argList, Value));
                memcpy(start, string->value, string->length);
                start += string->length;
                break;
            }

            default:
                // Any other character is interpreted literally.
                *start++ = *c;
        }
    }
    va_end(argList);

    hashString(result);

    return OBJ_VAL(result);
}

Value MSCStringFromCodePointAt(String *string, MVM *vm, uint32_t index) {
    ASSERT(index < string->length, "Index out of bounds.");

    int codePoint = MSCUtf8Decode((uint8_t *) string->value + index,
                                  string->length - index);
    if (codePoint == -1) {
        // If it isn't a valid UTF-8 sequence, treat it as a single raw byte.
        char bytes[2];
        bytes[0] = string->value[index];
        bytes[1] = '\0';
        return MSCStringFromCharsWithLength(vm, bytes, 1);
    }

    return MSCStringFromCodePoint(vm, codePoint);
}


// Uses the Boyer-Moore-Horspool string matching algorithm.
uint32_t MSCStringFind(String *string, String *needle, uint32_t start) {
    // Edge case: An empty needle is always found.
    if (needle->length == 0) return start;

    // If the needle goes past the haystack it won't be found.
    if (start + needle->length > string->length) return UINT32_MAX;

    // If the startIndex is too far it also won't be found.
    if (start >= string->length) return UINT32_MAX;

    // Pre-calculate the shift table. For each character (8-bit value), we
    // determine how far the search window can be advanced if that character is
    // the last character in the haystack where we are searching for the needle
    // and the needle doesn't match there.
    uint32_t shift[UINT8_MAX];
    uint32_t needleEnd = needle->length - 1;

    // By default, we assume the character is not the needle at all. In that case
    // case, if a match fails on that character, we can advance one whole needle
    // width since.
    /*for (uint32_t index = 0; index < UINT8_MAX; index++)
    {
        shift[index] = needle->length;
    }*/

    for (uint32_t index = 0; index < UINT8_MAX; index++) {
        shift[index] = needle->length;
    }

    // Then, for every character in the needle, determine how far it is from the
    // end. If a match fails on that character, we can advance the window such
    // that it the last character in it lines up with the last place we could
    // find it in the needle.
    for (uint32_t index = 0; index < needleEnd; index++) {
        char c = needle->value[index];
        shift[(uint8_t) c] = needleEnd - index;
    }

    // Slide the needle across the haystack, looking for the first match or
    // stopping if the needle goes off the end.
    char lastChar = needle->value[needleEnd];
    uint32_t range = string->length - needle->length;

    for (uint32_t index = start; index <= range;) {
        // Compare the last character in the this's window to the last character
        // in the needle. If it matches, see if the whole needle matches.
        char c = string->value[index + needleEnd];
        if (lastChar == c &&
            memcmp(string->value + index, needle->value, needleEnd) == 0) {
            // Found a match.
            return index;
        }

        // Otherwise, slide the needle forward.
        index += shift[(uint8_t) c];
    }

    // Not found.
    return UINT32_MAX;
}

Value MSCStringFromRange(String *thisString, MVM *vm, int start, uint32_t count, int step) {
    uint8_t *from = (uint8_t *) thisString->value;
    int length = 0;
    for (uint32_t i = 0; i < count; i++) {
        length += MSCUtf8DecodeNumBytes(from[start + i * step]);
    }

    String *result = MSCStringAllocate(vm, (uint32_t) length);
    result->value[length] = '\0';

    uint8_t *to = (uint8_t *) result->value;
    for (uint32_t i = 0; i < count; i++) {
        int index = start + i * step;
        int codePoint = MSCUtf8Decode(from + index, thisString->length - index);

        if (codePoint != -1) {
            to += MSCUtf8Encode(codePoint, to);
        }
    }

    hashString(result);
    return OBJ_VAL(result);
}

Function *MSCFunctionFrom(MVM *vm, Module *module, int maxSlots) {
    FnDebug *debug = ALLOCATE(vm, FnDebug);
    debug->name = NULL;
    MSCInitIntBuffer(&debug->sourceLines);
    Function *fn = ALLOCATE(vm, Function);
    initObj(vm, &fn->obj, OBJ_FN, vm->core.fnClass);
    MSCInitValueBuffer(&fn->constants);
    MSCInitByteBuffer(&fn->code);
    fn->module = module;
    fn->boundToClass = NULL;
    fn->maxSlots = maxSlots;
    fn->numUpvalues = 0;
    fn->arity = 0;
    fn->debug = debug;
    return fn;
}

void MSCBlackenFunction(Function *function, MVM *vm) {
    // Object::blacken(vm);
    // Mark the constants.
    MSCGrayBuffer(vm, &function->constants);

    // Keep track of how much memory is still in use.
    vm->gc->bytesAllocated += sizeof(Function);
    vm->gc->bytesAllocated += sizeof(uint8_t) * function->code.capacity;
    vm->gc->bytesAllocated += sizeof(Value) * function->constants.capacity;

    // The debug line number buffer.
    vm->gc->bytesAllocated += sizeof(int) * function->code.capacity;
}


void MSCFunctionBindName(Function *fn, MVM *vm, const char *name, int length) {
    fn->debug->name = ALLOCATE_ARRAY(vm, char, length + 1);
    memcpy(fn->debug->name, name, length);
    fn->debug->name[length] = '\0';
}


void MSCGrayValue(MVM *vm, Value value) {
    if (!IS_OBJ(value)) return;
    MSCGrayObject(AS_OBJ(value), vm);
}

void MSCGrayBuffer(MVM *vm, ValueBuffer *buffer) {
    for (int i = 0; i < buffer->count; i++) {
        MSCGrayValue(vm, buffer->data[i]);
    }
}


Class *MSCGetClass(MVM *vm, Value value) {
    return MSCGetClassInline(vm, value);
}

bool MSCValuesEqual(Value a, Value b) {
    if (MSCValuesSame(a, b)) return true;

    // If we get here, it's only possible for two heap-allocated immutable objects
    // to be equal.
    if (!IS_OBJ(a) || !IS_OBJ(b)) return false;

    Object *aObj = AS_OBJ(a);
    Object *bObj = AS_OBJ(b);

    // Must be the same type.
    if (aObj->type != bObj->type) return false;

    switch (aObj->type) {
        case OBJ_RANGE: {
            Range *aRange = (Range *) aObj;
            Range *bRange = (Range *) bObj;
            return aRange->from == bRange->from &&
                   aRange->to == bRange->to &&
                   aRange->isInclusive == bRange->isInclusive;
        }
        case OBJ_STRING: {
            String *aString = (String *) aObj;
            String *bString = (String *) bObj;
            return aString->hash == bString->hash &&
                   MSCStringEqualsCString(aString, bString->value, bString->length);
        }

        default:
            // All other types are only equal if they are same, which they aren't if
            // we get here.
            return false;
    }
}

/*
msc::vm::Value::Value() = default;

msc::vm::Value::Value(msc::vm::ValueType type): type(type) {

}

msc::vm::Value::Value(msc::vm::ValueType type, double value): type(type) {
    this->as.num = value;
}

msc::vm::Value::Value(msc::vm::ValueType type, msc::vm::Object* value): type(type) {
    this->as.obj = value;
}
*/
Value MSCRangeFrom(MVM *vm, double from, double to, bool isInclusive) {
    Range *range = ALLOCATE(vm, Range);
    initObj(vm, &range->obj, OBJ_RANGE, vm->core.rangeClass);
    range->from = from;
    range->to = to;
    range->isInclusive = isInclusive;

    return OBJ_VAL(range);
}


void MSCBlackenRange(Range *range, MVM *vm) {
    vm->gc->bytesAllocated += sizeof(Range);
}
