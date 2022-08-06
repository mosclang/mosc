//
// Created by Mahamadou DOUMBIA [OML DSI] on 11/01/2022.
//

#ifndef CPMSC_GC_H
#define CPMSC_GC_H


#include <stddef.h>
#include <stdio.h>
#include "../memory/Value.h"


#define MSC_MAX_TEMP_ROOTS 8


typedef struct {

    size_t bytesAllocated;
    size_t nextGC;
    Object *first;
    Object **gray;
    int grayCount;
    int grayCapacity;
    MSCReallocator reallocator;
    Object *tempRoots[MSC_MAX_TEMP_ROOTS];
    int numTempRoots;
    void *userData;
    MVM *vm;
} GC;

GC *MSCNewGC(MVM *vm);

void MSCFreeGC(GC *gc);

void MSCGCCollect(GC *gc);


void MSCBlackenObjects(GC *gc);

void MSCPushRoot(GC *gc, Object *obj);

void MSCPopRoot(GC *gc);

void *MSCReallocate(GC *gc, void *memory, size_t oldSize, size_t newSize);
// static void * defaultReallocate(void* ptr, size_t newSize, void* _);


// The behavior of realloc() when the size is 0 is implementation defined. It
// may return a non-NULL pointer which must not be dereferenced but nevertheless
// should be freed. To prevent that, we avoid calling realloc() with a zero
// size.
static void *defaultReallocate(void *ptr, size_t newSize, void *_) {
    if (newSize == 0) {
        free(ptr);
        return NULL;
    }

    return realloc(ptr, newSize);
}

#endif //CPMSC_GC_H
