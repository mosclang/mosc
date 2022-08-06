//
// Created by Mahamadou DOUMBIA [OML DSI] on 11/01/2022.
//

#ifndef CPMSC_HELPER_H
#define CPMSC_HELPER_H

#include <stdlib.h>
#include "../api/msc.h"
// We need buffers of a few different types. To avoid lots of casting between
// void* and back, we'll use the preprocessor as a poor man's generics and let
// it generate a few type-specific ones.
#define DECLARE_BUFFER(name, type)                                             \
    typedef struct                                                             \
    {                                                                          \
      type* data;                                                              \
      int count;                                                               \
      int capacity;                                                            \
    } name##Buffer;                                                            \
    void MSCInit##name##Buffer(name##Buffer* buffer);                         \
    void MSCFree##name##Buffer(MVM* vm, name##Buffer* buffer);            \
    void MSCFill##name##Buffer(MVM* vm, name##Buffer* buffer, type data,   \
                                int count);                                    \
    void MSCWrite##name##Buffer(MVM* vm, name##Buffer* buffer, type data)

// This should be used once for each type instantiation, somewhere in a .c file.
#define DEFINE_BUFFER(name, type)                                                     \
    void MSCInit##name##Buffer(name##Buffer* buffer)                                  \
    {                                                                          \
      buffer->data = NULL;                                                  \
      buffer->capacity = 0;                                                    \
      buffer->count = 0;                                                        \
    }                                                                                                               \
                                                                                                                    \
    void MSCFree##name##Buffer(MVM* vm, name##Buffer* buffer)                                        \
    {                                                                                                               \
        MSCReallocate(vm->gc, buffer->data, 0, 0);                                                                 \
        buffer->data = NULL;                                                                                     \
        buffer->capacity = 0;                                                                                       \
        buffer->count = 0;                                                                                          \
    }                                                                                                               \
                                                                                                                    \
    void MSCFill##name##Buffer(MVM* vm, name##Buffer* buffer, type data,                             \
                                int count)                                                                          \
    {                                                                                                               \
       if (buffer->capacity < buffer->count + count)                                                                \
        {                                                                                                           \
            int capacity = powerOf2Ceil(buffer->count + count);                                            \
            buffer->data = (type*)MSCReallocate(vm->gc, buffer->data,                                              \
                                                  buffer->capacity * sizeof(type), capacity * sizeof(type));        \
            buffer->capacity = capacity;                                                                            \
        }                                                                                                           \
                                                                                                                    \
        for (int i = 0; i < count; i++)                                                                             \
        {                                                                                                           \
            buffer->data[buffer->count++] = data;                                                                   \
        }                                                                                                           \
    }                                                                              \
                                                                               \
    void MSCWrite##name##Buffer(MVM* vm, name##Buffer * buffer, type data)  \
    {                                                                          \
      MSCFill##name##Buffer(vm, buffer, data, 1);                             \
    }






    // forward declaration
    typedef struct sString  String;


    DECLARE_BUFFER(String, String*);
    DECLARE_BUFFER(Int, int);
    DECLARE_BUFFER(Byte, uint8_t);

    typedef StringBuffer SymbolTable;

    int powerOf2Ceil(int n);

    /*template<typename T>
     void fillBuffer(MVM *vm, Buffer<T> *buffer, T data, int count);

     template<typename T>
     void freeBuffer(MVM *vm, Buffer<T> *buffer);

     template<typename T>
     void writeBuffer(MVM *vm, Buffer<T> *buffer, T data);*/

    void MSCBlackenSymbolTable(MVM *vm, SymbolTable *table);

    void MSCSymbolTableInit(SymbolTable *symbols);
    void MSCSymbolTableClear(MVM* vm, SymbolTable *MSCSymbols);
    int MSCSymbolTableFind(const SymbolTable *symbols, const char *name, size_t length);
    int MSCSymbolTableAdd(MVM* vm, SymbolTable *symbols, const char *name, size_t length);
    int MSCSymbolTableEnsure(MVM* vm, SymbolTable *symbols, const char *name, size_t length);


    // Validates that [value] is within `[0, count)`. Also allows
    // negative indices which map backwards from the end. Returns the valid positive
    // index value. If invalid, returns `UINT32_MAX`.

    uint32_t MSCValidateIndex(uint32_t count, int64_t value);

    int MSCUtf8DecodeNumBytes(uint8_t byte);

    int MSCUtf8Decode(const uint8_t *bytes, uint32_t length);

    int MSCUtf8EncodeNumBytes(int value);

    int MSCUtf8Encode(int value, uint8_t *bytes);




#endif //CPMSC_HELPER_H