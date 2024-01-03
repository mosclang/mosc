//
// Created by Mahamadou DOUMBIA [OML DSI] on 13/01/2022.
//

#ifndef CPMSC_MSC_H
#define CPMSC_MSC_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>


// The MSC semantic version number components.
#define MSC_VERSION_MAJOR 0
#define MSC_VERSION_MINOR 8
#define MSC_VERSION_PATCH 2

// A human-friendly string representation of the version.
#define MSC_VERSION_STRING "0.8.2"

// A monotonically increasing numeric representation of the version number. Use
// this if you want to do range checks over versions.
#define MSC_VERSION_NUMBER (MSC_VERSION_MAJOR * 1000000 +                    \
                             MSC_VERSION_MINOR * 1000 +                       \
                             MSC_VERSION_PATCH)

#ifndef MSC_API
#if defined(_MSC_VER) && defined(MSC_API_DLLEXPORT)
#define MSC_API __declspec( dllexport )
#else
#define MSC_API
#endif
#endif //MSC_API

typedef struct MVM MVM;

typedef struct MSCHandle MSCHandle;


typedef void *(*MSCReallocator)(void *memory, size_t newSize, void *userData);

// A function callable from MSC code, but implemented in C.
typedef void (*MSCExternMethodFn)(MVM *vm);

// A finalizer function for freeing resources owned by an instance of a foreign
// class. Unlike most foreign methods, finalizers do not have access to the VM
// and should not interact with it since it's in the middle of a garbage
// collection.
typedef void (*MSCFinalizerFn)(void *data);

// Gives the host a chance to canonicalize the imported module name,
// potentially taking into account the (previously resolved) name of the module
// that contains the import. Typically, this is used to implement relative
// imports.
typedef const char *(*MSCResolveModuleFn)(MVM *vm,
                                          const char *importer, const char *name);

// Forward declare
struct MSCLoadModuleResult;

// Called after loadModuleFn is called for module [name]. The original returned result
// is handed back to you in this callback, so that you can free memory if appropriate.
typedef void (*MSCLoadModuleCompleteFn)(MVM *vm, const char *name, struct MSCLoadModuleResult result);

typedef struct MSCLoadModuleResult {
    const char *source;
    MSCLoadModuleCompleteFn onComplete;
    void *userData;

} MSCLoadModuleResult;

// Loads and returns the source code for the module [name].
typedef MSCLoadModuleResult (*MSCLoadModuleFn)(MVM *vm, const char *name);

// Returns a pointer to a foreign method on [className] in [module] with
// [signature].
typedef MSCExternMethodFn (*MSCBindExternMethodFn)(MVM *vm, const char *module, const char *className, bool isStatic,
                                                   const char *signature);

// Displays a string of text to the user.
typedef void (*MSCWriteFn)(MVM *vm, const char *text);

typedef enum {
    MSC_TYPE_BOOL,
    MSC_TYPE_NUM,
    MSC_TYPE_EXTERN,
    MSC_TYPE_LIST,
    MSC_TYPE_MAP,
    MSC_TYPE_NULL,
    MSC_TYPE_STRING,

    MSC_TYPE_UNKNOWN
} MSCType;

typedef enum {
    ERROR_COMPILE,
    ERROR_RUNTIME,
    ERROR_STACK_TRACE
} MSCError;

typedef bool (*MSCErrorHandler)(MVM *vm, MSCError type, const char *module_name, int line, const char *message);

typedef enum InterpretResult {
    RESULT_COMPILATION_ERROR,
    RESULT_RUNTIME_ERROR,
    RESULT_SUCCESS
} MSCInterpretResult;
typedef struct {
    // The callback invoked when the foreign object is created.
    //
    // This must be provided. Inside the body of this, it must call
    // [setSlotNewForeign()] exactly once.
    MSCExternMethodFn allocate;

    // The callback invoked when the garbage collector is about to collect a
    // foreign object's memory.
    //
    // This may be `NULL` if the foreign class does not need to finalize.
    MSCFinalizerFn finalize;
} MSCExternClassMethods;

// Returns a pair of pointers to the foreign methods used to allocate and
// finalize the data for instances of [className] in resolved [module].
typedef MSCExternClassMethods (*MSCBindExternClassFn)(
        MVM *vm, const char *module, const char *className);


typedef struct {

    MSCReallocator reallocateFn;
    MSCResolveModuleFn resolveModuleFn;
    MSCLoadModuleFn loadModuleFn;
    MSCBindExternMethodFn bindExternMethodFn;
    MSCBindExternClassFn bindExternClassFn;
    MSCErrorHandler errorHandler;
    // The callback to use to display text when `System.print()` or the other
    // related functions are called.
    //
    // If this is `NULL`, Mosc discards any printed text.
    MSCWriteFn writeFn;

    // If zero, defaults to 1MB.
    size_t minHeapSize;
    // If zero, defaults to 10MB.
    size_t initialHeapSize;
    int heapGrowthPercent;
    void *userData;
} MSCConfig;

MSC_API void MSCInitConfig(MSCConfig *config);

MSC_API void MSCVMSetConfig(MVM* vm, MSCConfig *config);
MSC_API void MSCVMSetWriteFn(MVM* vm, MSCWriteFn fn);
MSC_API void MSCVMSetErrorHandler(MVM* vm, MSCErrorHandler fn);
// Get the current Mosc version number.
//
// Can be used to range checks over versions.
MSC_API int MSCGetVersionNumber();

// Initializes [configuration] with all of its default values.
//
// Call this before setting the particular fields you care about.
// MSC_API void MSCConfig::MSCConfig(MSCReallocator reallocator);

// Creates a new Mosc virtual machine using the given [configuration]. Mosc
// will copy the configuration data, so the argument passed to this can be
// freed after calling this. If [configuration] is `NULL`, uses a default
// configuration.
MSC_API MVM *MSCNewVM(MSCConfig *configuration);

// Disposes of all resources is use by [vm], which was previously created by a
// call to [MSCNewVM].
MSC_API void MSCFreeVM(MVM *vm);

// Immediately run the garbage collector to free unused memory.
MSC_API void MSCCollectGarbage(MVM *vm);

// Runs [source], a string of Mosc source code in a new fiber in [vm] in the
// context of resolved [module].
MSC_API MSCInterpretResult MSCInterpret(MVM *vm, const char *module,
                                        const char *source);

// Creates a handle that can be used to invoke a method with [signature] on
// using a receiver and arguments that are set up on the stack.
//
// This handle can be used repeatedly to directly invoke that method from C
// code using [MSCCall].
//
// When you are done with this handle, it must be released using
// [MSCReleaseHandle].
MSC_API MSCHandle *MSCMakeCallHandle(MVM *vm, const char *signature);


// Calls [method], using the receiver and arguments previously set up on the
// stack.
//
// [method] must have been created by a call to [MSCMakeCallHandle]. The
// arguments to the method must be already on the stack. The receiver should be
// in slot 0 with the remaining arguments following it, in order. It is an
// error if the number of arguments provided does not match the method's
// signature.
//
// After this returns, you can access the return value from slot 0 on the stack.
MSC_API MSCInterpretResult MSCCall(MVM *vm, MSCHandle *method);

// Releases the reference stored in [handle]. After calling this, [handle] can
// no longer be used.
MSC_API void MSCReleaseHandle(MVM *vm, MSCHandle *handle);

// The following functions are intended to be called from foreign methods or
// finalizers. The interface Mosc provides to a foreign method is like a
// register machine: you are given a numbered array of slots that values can be
// read from and written to. Values always live in a slot (unless explicitly
// captured using MSCGetSlotHandle(), which ensures the garbage collector can
// find them.
//
// When your foreign function is called, you are given one slot for the receiver
// and each argument to the method. The receiver is in slot 0 and the arguments
// are in increasingly numbered slots after that. You are free to read and
// write to those slots as you want. If you want more slots to use as scratch
// space, you can call MScEnsureSlots() to add more.
//
// When your function returns, every slot except slot zero is discarded and the
// value in slot zero is used as the return value of the method. If you don't
// store a return value in that slot yourself, it will retain its previous
// value, the receiver.
//
// While Mosc is dynamically typed, C is not. This means the C interface has to
// support the various types of primitive values a Mosc variable can hold: bool,
// double, string, etc. If we supported this for every operation in the C API,
// there would be a combinatorial explosion of functions, like "get a
// double-valued element from a list", "insert a string key and double value
// into a map", etc.
//
// To avoid that, the only way to convert to and from a raw C value is by going
// into and out of a slot. All other functions work with values already in a
// slot. So, to add an element to a list, you put the list in one slot, and the
// element in another. Then there is a single API function MSCInsertInList()
// that takes the element out of that slot and puts it into the list.
//
// The goal of this API is to be easy to use while not compromising performance.
// The latter means it does not do type or bounds checking at runtime except
// using assertions which are generally removed from release builds. C is an
// unsafe language, so it's up to you to be careful to use it correctly. In
// return, you get a very fast FFI.

// Returns the number of slots available to the current foreign method.
MSC_API int MSCGetSlotCount(MVM *vm);

// Ensures that the foreign method stack has at least [numSlots] available for
// use, growing the stack if needed.
//
// Does not shrink the stack if it has more than enough slots.
//
// It is an error to call this from a finalizer.
MSC_API void MSCEnsureSlots(MVM *vm, int numSlots);

// Gets the type of the object in [slot].
MSC_API MSCType MSCGetSlotType(MVM *vm, int slot);

// Reads a boolean value from [slot].
//
// It is an error to call this if the slot does not contain a boolean value.
MSC_API bool MSCGetSlotBool(MVM *vm, int slot);

// Reads a byte array from [slot].
//
// The memory for the returned string is owned by Mosc. You can inspect it
// while in your foreign method, but cannot keep a pointer to it after the
// function returns, since the garbage collector may reclaim it.
//
// Returns a pointer to the first byte of the array and fill [length] with the
// number of bytes in the array.
//
// It is an error to call this if the slot does not contain a string.
MSC_API const char *MSCGetSlotBytes(MVM *vm, int slot, int *length);

// Reads a number from [slot].
//
// It is an error to call this if the slot does not contain a number.
MSC_API double MSCGetSlotDouble(MVM *vm, int slot);

// Reads a foreign object from [slot] and returns a pointer to the foreign data
// stored with it.
//
// It is an error to call this if the slot does not contain an instance of a
// foreign class.
MSC_API void *MSCGetSlotExtern(MVM *vm, int slot);

// Reads a string from [slot].
//
// The memory for the returned string is owned by Mosc. You can inspect it
// while in your foreign method, but cannot keep a pointer to it after the
// function returns, since the garbage collector may reclaim it.
//
// It is an error to call this if the slot does not contain a string.
MSC_API const char *MSCGetSlotString(MVM *vm, int slot);

// Creates a handle for the value stored in [slot].
//
// This will prevent the object that is referred to from being garbage collected
// until the handle is released by calling [MSCReleaseHandle()].
MSC_API MSCHandle *MSCGetSlotHandle(MVM *vm, int slot);

// Stores the boolean [value] in [slot].
MSC_API void MSCSetSlotBool(MVM *vm, int slot, bool value);

// Stores the array [length] of [bytes] in [slot].
//
// The bytes are copied to a new string within Mosc's heap, so you can free
// memory used by them after this is called.
MSC_API void MSCSetSlotBytes(MVM *vm, int slot, const char *bytes, size_t length);

// Stores the numeric [value] in [slot].
MSC_API void MSCSetSlotDouble(MVM *vm, int slot, double value);

// Creates a new instance of the foreign class stored in [classSlot] with [size]
// bytes of raw storage and places the resulting object in [slot].
//
// This does not invoke the foreign class's constructor on the new instance. If
// you need that to happen, call the constructor from Mosc, which will then
// call the allocator foreign method. In there, call this to create the object
// and then the constructor will be invoked when the allocator returns.
//
// Returns a pointer to the foreign object's data.
MSC_API void *MSCSetSlotNewExtern(MVM *vm, int slot, int classSlot, size_t size);

// Stores a new empty list in [slot].
MSC_API void MSCSetSlotNewList(MVM *vm, int slot);

// Stores a new empty map in [slot].
MSC_API void MSCSetSlotNewMap(MVM *vm, int slot);

// Stores null in [slot].
MSC_API void MSCSetSlotNull(MVM *vm, int slot);

// Stores the string [text] in [slot].
//
// The [text] is copied to a new string within Mosc's heap, so you can free
// memory used by it after this is called. The length is calculated using
// [strlen()]. If the string may contain any null bytes in the middle, then you
// should use [MSCSetSlotBytes()] instead.
MSC_API void MSCSetSlotString(MVM *vm, int slot, const char *text);


// Stores the value captured in [handle] in [slot].
//
// This does not release the handle for the value.
MSC_API void MSCSetSlotHandle(MVM *vm, int slot, MSCHandle *handle);

// Returns the number of elements in the list stored in [slot].
MSC_API int MSCGetListCount(MVM *vm, int slot);

// Reads element [index] from the list in [listSlot] and stores it in
// [elementSlot].
MSC_API void MSCGetListElement(MVM *vm, int listSlot, int index, int elementSlot);

// Sets the value stored at [index] in the list at [listSlot],
// to the value from [elementSlot].
MSC_API void MSCSetListElement(MVM *vm, int listSlot, int index, int elementSlot);


// Takes the value stored at [elementSlot] and inserts it into the list stored
// at [listSlot] at [index].
//
// As in Mosc, negative indexes can be used to insert from the end. To append
// an element, use `-1` for the index.
MSC_API void MSCInsertInList(MVM *vm, int listSlot, int index, int elementSlot);


// Returns the number of entries in the map stored in [slot].
MSC_API int MSCMapCount(MVM *vm, int slot);

// Returns true if the key in [keySlot] is found in the map placed in [mapSlot].
MSC_API bool MSCMapContainsKey(MVM *vm, int mapSlot, int keySlot);

// Retrieves a value with the key in [keySlot] from the map in [mapSlot] and
// stores it in [valueSlot].
MSC_API void MSCGetMapValue(MVM *vm, int mapSlot, int keySlot, int valueSlot);

// Takes the value stored at [valueSlot] and inserts it into the map stored
// at [mapSlot] with key [keySlot].
MSC_API void MSCSetMapValue(MVM *vm, int mapSlot, int keySlot, int valueSlot);

// Removes a value from the map in [mapSlot], with the key from [keySlot],
// and place it in [removedValueSlot]. If not found, [removedValueSlot] is
// set to null, the same behaviour as the Mosc Map API.
MSC_API void MSCRemoveMapValue(MVM *vm, int mapSlot, int keySlot,
                               int removedValueSlot);

// Looks up the top level variable with [name] in resolved [module] and stores
// it in [slot].
MSC_API void MSCGetVariable(MVM *vm, const char *module, const char *name,
                            int slot);

// Looks up the top level variable with [name] in resolved [module],
// returns false if not found. The module must be imported at the time,
// use MSCHasModule to ensure that before calling.
MSC_API bool MSCHasVariable(MVM *vm, const char *module, const char *name);

// Returns true if [module] has been imported/resolved before, false if not.
MSC_API bool MSCHasModule(MVM *vm, const char *module);

// Sets the current djuru to be aborted, and uses the value in [slot] as the
// runtime error object.
MSC_API void MSCAbortDjuru(MVM *vm, int slot);

// Returns the user data associated with the MVM.
MSC_API void *MSCGetUserData(MVM *vm);

// Sets user data associated with the MVM.
MSC_API void MSCSetUserData(MVM *vm, void *userData);

#endif //CPMSC_MSC_H
