//
// Created by Mahamadou DOUMBIA [OML DSI] on 11/01/2022.
//


#include "Helper.h"
#include "../memory/Value.h"
#include "../runtime/MVM.h"

DEFINE_BUFFER(String,String*);
DEFINE_BUFFER(Int, int);
DEFINE_BUFFER(Byte, uint8_t);

int MSCUtf8EncodeNumBytes(int value)
{
    ASSERT(value >= 0, "Cannot encode a negative value.");

    if (value <= 0x7f) return 1;
    if (value <= 0x7ff) return 2;
    if (value <= 0xffff) return 3;
    if (value <= 0x10ffff) return 4;
    return 0;
}
int MSCUtf8Encode(int value, uint8_t* bytes)
{
    if (value <= 0x7f)
    {
        // Single byte (i.e. fits in ASCII).
        *bytes = value & 0x7f;
        return 1;
    }
    else if (value <= 0x7ff)
    {
        // Two byte sequence: 110xxxxx 10xxxxxx.
        *bytes = 0xc0 | ((value & 0x7c0) >> 6);
        bytes++;
        *bytes = 0x80 | (value & 0x3f);
        return 2;
    }
    else if (value <= 0xffff)
    {
        // Three byte sequence: 1110xxxx 10xxxxxx 10xxxxxx.
        *bytes = 0xe0 | ((value & 0xf000) >> 12);
        bytes++;
        *bytes = 0x80 | ((value & 0xfc0) >> 6);
        bytes++;
        *bytes = 0x80 | (value & 0x3f);
        return 3;
    }
    else if (value <= 0x10ffff)
    {
        // Four byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx.
        *bytes = 0xf0 | ((value & 0x1c0000) >> 18);
        bytes++;
        *bytes = 0x80 | ((value & 0x3f000) >> 12);
        bytes++;
        *bytes = 0x80 | ((value & 0xfc0) >> 6);
        bytes++;
        *bytes = 0x80 | (value & 0x3f);
        return 4;
    }

    // Invalid Unicode value. See: http://tools.ietf.org/html/rfc3629
    UNREACHABLE();
    return 0;
}

int MSCUtf8Decode(const uint8_t* bytes, uint32_t length)
{
    // Single byte (i.e. fits in ASCII).
    if (*bytes <= 0x7f) return *bytes;

    int value;
    uint32_t remainingBytes;
    if ((*bytes & 0xe0) == 0xc0)
    {
        // Two byte sequence: 110xxxxx 10xxxxxx.
        value = *bytes & 0x1f;
        remainingBytes = 1;
    }
    else if ((*bytes & 0xf0) == 0xe0)
    {
        // Three byte sequence: 1110xxxx    10xxxxxx 10xxxxxx.
        value = *bytes & 0x0f;
        remainingBytes = 2;
    }
    else if ((*bytes & 0xf8) == 0xf0)
    {
        // Four byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx.
        value = *bytes & 0x07;
        remainingBytes = 3;
    }
    else
    {
        // Invalid UTF-8 sequence.
        return -1;
    }

    // Don't read past the end of the buffer on truncated UTF-8.
    if (remainingBytes > length - 1) return -1;

    while (remainingBytes > 0)
    {
        bytes++;
        remainingBytes--;

        // Remaining bytes must be of form 10xxxxxx.
        if ((*bytes & 0xc0) != 0x80) return -1;

        value = value << 6 | (*bytes & 0x3f);
    }

    return value;
}

int MSCUtf8DecodeNumBytes(uint8_t byte)
{
    // If the byte starts with 10xxxxx, it's the middle of a UTF-8 sequence, so
    // don't count it at all.
    if ((byte & 0xc0) == 0x80) return 0;

    // The first byte's high bits tell us how many bytes are in the UTF-8
    // sequence.
    if ((byte & 0xf8) == 0xf0) return 4;
    if ((byte & 0xf0) == 0xe0) return 3;
    if ((byte & 0xe0) == 0xc0) return 2;
    return 1;
}

// From: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2Float
int powerOf2Ceil(int n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;

    return n;
}

void MSCBlackenSymbolTable(MVM* vm, SymbolTable* symbolTable)
{
    for (int i = 0; i < symbolTable->count; i++) {
        MSCGrayObject((Object*)symbolTable->data[i], vm);
    }
    // Keep track of how much memory is still in use.
    vm->gc->bytesAllocated += symbolTable->capacity * sizeof(*symbolTable->data);
}
/*template <typename T>
void msc::vm::fillBuffer(MVM *vm, Buffer<T> *buffer, T data, int count) {
    if (buffer->capacity < buffer->count + count)
    {
        int capacity = powerOf2Ceil(buffer->count + count);
        buffer->data = (T*)vm->gc->reallocate(vm, buffer->data,
                                              buffer->capacity * sizeof(T), capacity * sizeof(T));
        buffer->capacity = capacity;
    }

    for (int i = 0; i < count; i++)
    {
        buffer->data[buffer->count++] = data;
    }
}
template <typename T>
void msc::vm::freeBuffer(msc::vm::MVM *vm, Buffer<T> *buffer) {
    vm->gc->reallocate(vm, buffer->data, 0, 0);
    buffer->data = nullptr;
    buffer->capacity = 0;
    buffer->count = 0;
}
template <typename T>
void msc::vm::writeBuffer(msc::vm::MVM* vm, Buffer<T>* buffer, T data) {
    fillBuffer(vm, buffer, data, 1);
}
*/


int MSCSymbolTableFind(const SymbolTable *symbols, const char *name, size_t length) {
    // See if the symbol is already defined.
    // TODO: O(n). Do something better.
    for (int i = 0; i < symbols->count; i++) {
        if (MSCStringEqualsCString(symbols->data[i], name, length)) return i;
    }

    return -1;
}

int MSCSymbolTableAdd(MVM* vm, SymbolTable *symbols, const char *name, size_t length) {
    String* symbol = AS_STRING(MSCStringFromCharsWithLength(vm, name, length));
    MSCPushRoot(vm->gc, &symbol->obj);
    MSCWriteStringBuffer(vm, symbols, symbol);
    MSCPopRoot(vm->gc);
    return symbols->count - 1;
}

int MSCSymbolTableEnsure(MVM* vm, SymbolTable *symbols, const char *name, size_t length) {
    // See if the symbol is already defined.
    int existing = MSCSymbolTableFind(symbols, name, length);
    if (existing != -1) return existing;

    // New symbol, so add it.
    return MSCSymbolTableAdd(vm, symbols, name, length);
}

uint32_t MSCValidateIndex(uint32_t count, int64_t value) {
    // Negative indices count from the end.
    if (value < 0) value = count + value;

    // Check bounds.
    if (value >= 0 && value < count) return (uint32_t)value;

    return UINT32_MAX;
}

void MSCSymbolTableInit(SymbolTable *symbols) {
    MSCInitStringBuffer(symbols);
}

void MSCSymbolTableClear(MVM* vm, SymbolTable *symbols) {
    MSCFreeStringBuffer(vm, symbols);
}



