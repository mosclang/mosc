//
// Created by Mahamadou DOUMBIA [OML DSI] on 11/01/2022.
//

#include "GC.h"
#include <time.h>
#include "../runtime/MVM.h"
#include "../runtime/debuger.h"


/*// The behavior of realloc() when the size is 0 is implementation defined. It
// may return a non-NULL pointer which must not be dereferenced but nevertheless
// should be freed. To prevent that, we avoid calling realloc() with a zero
// size.
static void* defaultReallocate(void* ptr, size_t newSize, void* _)
{
    if (newSize == 0)
    {
        free(ptr);
        return nullptr;
    }

    return realloc(ptr, newSize);
}
 */
void MSCGCCollect(GC *gc) {


#if MSC_DEBUG_TRACE_MEMORY || MSC_DEBUG_TRACE_GC
    printf("-- gc start --\n");

    size_t before = gc->bytesAllocated;
    double startTime = (double) clock() / CLOCKS_PER_SEC;
#endif

    // Mark all reachable objects.

    // Reset gc. As we mark objects, their size will be counted again so that
    // we can track how much memory is in use without needing to know the size
    // of each *freed* object.
    //
    // This is important because when freeing an unmarked object, we don't always
    // know how much memory it is using. For example, when freeing an instance,
    // we need to know its class to know how big it is, but its class may have
    // already been freed.
    gc->bytesAllocated = 0;

    MSCGrayObject((Object *) gc->vm->modules, gc->vm);


    // Temporary roots.
    for (int i = 0; i < gc->numTempRoots; i++) {
        MSCGrayObject(gc->tempRoots[i], gc->vm);
    }

    if (gc->vm->djuru != NULL) {
        MSCGrayObject((Object *) gc->vm->djuru, gc->vm);
    }
    // The handles.
    for (MSCHandle *handle = gc->vm->handles;
         handle != NULL;
         handle = handle->next) {
        MSCGrayValue(gc->vm, handle->value);
    }

    // Any object the compiler is using (if there is one).
    if(gc->vm->compiler != NULL) MSCMarkCompiler(gc->vm->compiler, gc->vm);

    // Method names.
    MSCBlackenSymbolTable(gc->vm, &gc->vm->methodNames);
    // Now that we have grayed the roots, do a depth-first search over all of the
    // reachable objects.
    MSCBlackenObjects(gc);

    // Collect the white objects.
    Object **obj = &gc->first;
    while (*obj != NULL) {
        if (!((*obj)->isDark)) {

            // This object wasn't reached, so remove it from the list and free it.
            Object *unreached = *obj;
            *obj = unreached->next;
            MSCFreeObject(unreached, gc->vm);
        } else {
            // This object was reached, so unmark it (for the next GC) and move on to
            // the next.
            (*obj)->isDark = false;
            obj = &(*obj)->next;
        }
    }

    // Calculate the next gc point, gc is the current allocation plus
    // a configured percentage of the current allocation.
    gc->nextGC = gc->bytesAllocated + ((gc->bytesAllocated * gc->vm->config.heapGrowthPercent) / 100);
    if (gc->nextGC < gc->vm->config.minHeapSize) gc->nextGC = gc->vm->config.minHeapSize;

#if MSC_DEBUG_TRACE_MEMORY || MSC_DEBUG_TRACE_GC
    double elapsed = ((double) clock() / CLOCKS_PER_SEC) - startTime;
    // Explicit cast because size_t has different sizes on 32-bit and 64-bit and
    // we need a consistent type for the format string.
    printf("GC %lu before, %lu after (%lu collected), next at %lu. Took %.3fms.\n",
           (unsigned long) before,
           (unsigned long) gc->bytesAllocated,
           (unsigned long) (before - gc->bytesAllocated),
           (unsigned long) gc->nextGC,
           elapsed * 1000.0);
#endif
}


void MSCBlackenObjects(GC *gc) {
    while (gc->grayCount > 0) {
        // Pop an item from the gray stack.
        Object *obj = gc->gray[--gc->grayCount];
        MSCBlackenObject(obj, gc->vm);
    }
}

void MSCPopRoot(GC *gc) {
    ASSERT(gc->numTempRoots > 0, "No temporary roots to release.");
    gc->numTempRoots--;
}

void MSCPushRoot(GC *gc, Object *obj) {
    ASSERT(obj != NULL, "Can't root nullptr.");
    ASSERT(gc->numTempRoots < MSC_MAX_TEMP_ROOTS, "Too many temporary roots.");

    gc->tempRoots[gc->numTempRoots++] = obj;
}

GC *MSCNewGC(MVM *vm) {
    void *userData = vm->config.userData;
    MSCReallocator reallocate = defaultReallocate;
    GC *gc = (GC *) reallocate(NULL, sizeof(*gc), userData);
    gc->reallocator = vm->config.reallocateFn ? vm->config.reallocateFn : reallocate;
    // gc->reallocator = defaultReallocate;
    gc->bytesAllocated = 0;
    gc->nextGC = vm->config.initialHeapSize;
    gc->grayCapacity = 4;
    gc->grayCount = 0;
    gc->numTempRoots = 0;
    gc->userData = userData;
    gc->vm = vm;
    gc->first = NULL;
    // gc->gray = nullptr;
    gc->gray = (Object **) reallocate(NULL, gc->grayCapacity * sizeof(Object *), userData);
    return gc;
}

void MSCFreeGC(GC *gc) {
    Object *obj = gc->first;
    while (obj != NULL) {
        Object *next = obj->next;
        MSCFreeObject(obj, gc->vm);
        obj = next;
    }

    // Free up the GC gray set.
    gc->gray = (Object **) gc->reallocator(gc->gray, 0, gc->userData);
    // DEALLOCATE(gc->vm, gc);
    // gc->vm = NULL;
}

void *MSCReallocate(GC *gc, void *memory, size_t oldSize, size_t newSize) {
#if MSC_DEBUG_TRACE_MEMORY
    // Explicit cast because size_t has different sizes on 32-bit and 64-bit and
// we need a consistent type for the format string.
printf("reallocate %p %lu -> %lu\n",
     memory, (unsigned long)oldSize, (unsigned long)newSize);
#endif

    // If new bytes are being allocated, add them to the total count. If objects
    // are being completely deallocated, we don't track that (since we don't
    // track the original size). Instead, that will be handled while marking
    // during the next GC.
    gc->bytesAllocated += newSize - oldSize;

#if MSC_DEBUG_TRACE_GC
    // Since collecting calls gc function to free things, make sure we don't
// recurse.
if (newSize > 0) MSCGCCollect(gc);
#else
    if (newSize > 0 && gc->bytesAllocated > gc->nextGC) MSCGCCollect(gc);
#endif

    return gc->reallocator(memory, newSize, gc->userData);
}
