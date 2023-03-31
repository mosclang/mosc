//
// Created by Mahamadou DOUMBIA [OML DSI] on 12/01/2022.
//

#include "debuger.h"
#include "MVM.h"
#include "../common/opcodes.h"


static int dumpInstruction(MVM *vm, Function *fn, int i, int *lastLine) {
    int start = i;
    uint8_t *bytecode = fn->code.data;
    Opcode code = (Opcode) bytecode[i];

    int line = fn->debug->sourceLines.data[i];
    if (lastLine == NULL || *lastLine != line) {
        printf("%4d:", line);
        if (lastLine != NULL) *lastLine = line;
    } else {
        printf("     ");
    }

    printf(" %04d  ", i++);

#define READ_BYTE() (bytecode[i++])
#define READ_SHORT() (i += 2, (bytecode[i - 2] << 8) | bytecode[i - 1])

#define BYTE_INSTRUCTION(name)                                               \
      printf("%-16s %5d\n", name, READ_BYTE());                                \
      break

    switch (code) {
        case OP_CONSTANT: {
            int constant = READ_SHORT();
            printf("%-16s %5d '", "CONSTANT", constant);
            MSCDumpValue(fn->constants.data[constant]);
            printf("'\n");
            break;
        }

        case OP_NULL:
            printf("NULL\n");
            break;
        case OP_FALSE:
            printf("FALSE\n");
            break;
        case OP_TRUE:
            printf("TRUE\n");
            break;

        case OP_LOAD_LOCAL_0:
            printf("LOAD_LOCAL_0\n");
            break;
        case OP_LOAD_LOCAL_1:
            printf("LOAD_LOCAL_1\n");
            break;
        case OP_LOAD_LOCAL_2:
            printf("LOAD_LOCAL_2\n");
            break;
        case OP_LOAD_LOCAL_3:
            printf("LOAD_LOCAL_3\n");
            break;
        case OP_LOAD_LOCAL_4:
            printf("LOAD_LOCAL_4\n");
            break;
        case OP_LOAD_LOCAL_5:
            printf("LOAD_LOCAL_5\n");
            break;
        case OP_LOAD_LOCAL_6:
            printf("LOAD_LOCAL_6\n");
            break;
        case OP_LOAD_LOCAL_7:
            printf("LOAD_LOCAL_7\n");
            break;
        case OP_LOAD_LOCAL_8:
            printf("LOAD_LOCAL_8\n");
            break;

        case OP_LOAD_LOCAL:
        BYTE_INSTRUCTION("LOAD_LOCAL");
        case OP_STORE_LOCAL:
        BYTE_INSTRUCTION("STORE_LOCAL");
        case OP_LOAD_UPVALUE:
        BYTE_INSTRUCTION("LOAD_UPVALUE");
        case OP_STORE_UPVALUE:
        BYTE_INSTRUCTION("STORE_UPVALUE");

        case OP_LOAD_MODULE_VAR: {
            int slot = READ_SHORT();
            printf("%-16s %5d '%s'\n", "LOAD_MODULE_VAR", slot,
                   fn->module->variableNames.data[slot]->value);
            break;
        }

        case OP_STORE_MODULE_VAR: {
            int slot = READ_SHORT();
            printf("%-16s %5d '%s'\n", "STORE_MODULE_VAR", slot,
                   fn->module->variableNames.data[slot]->value);
            break;
        }

        case OP_FIELD:
        BYTE_INSTRUCTION("FIELD");
        case OP_LOAD_FIELD_THIS:
        BYTE_INSTRUCTION("LOAD_FIELD_THIS");
        case OP_STORE_FIELD_THIS:
        BYTE_INSTRUCTION("STORE_FIELD_THIS");
        case OP_LOAD_FIELD:
        BYTE_INSTRUCTION("LOAD_FIELD");
        case OP_STORE_FIELD:
        BYTE_INSTRUCTION("STORE_FIELD");

        case OP_POP:
            printf("POP\n");
            break;

        case OP_CALL: {
            int numArgs = READ_SHORT() + 1;
            printf("CALL(%d)\n", numArgs);
            break;
        }
        case OP_CALL_0:
        case OP_CALL_1:
        case OP_CALL_2:
        case OP_CALL_3:
        case OP_CALL_4:
        case OP_CALL_5:
        case OP_CALL_6:
        case OP_CALL_7:
        case OP_CALL_8:
        case OP_CALL_9:
        case OP_CALL_10:
        case OP_CALL_11:
        case OP_CALL_12:
        case OP_CALL_13:
        case OP_CALL_14:
        case OP_CALL_15:
        case OP_CALL_16: {
            int numArgs = bytecode[i - 1] - OP_CALL_0;
            int symbol = READ_SHORT();
            printf("CALL_%-11d %5d '%s'\n", numArgs, symbol,
                   vm->methodNames.data[symbol]->value);
            break;
        }

        case OP_SUPER_0:
        case OP_SUPER_1:
        case OP_SUPER_2:
        case OP_SUPER_3:
        case OP_SUPER_4:
        case OP_SUPER_5:
        case OP_SUPER_6:
        case OP_SUPER_7:
        case OP_SUPER_8:
        case OP_SUPER_9:
        case OP_SUPER_10:
        case OP_SUPER_11:
        case OP_SUPER_12:
        case OP_SUPER_13:
        case OP_SUPER_14:
        case OP_SUPER_15:
        case OP_SUPER_16: {
            int numArgs = bytecode[i - 1] - OP_SUPER_0;
            int symbol = READ_SHORT();
            int superclass = READ_SHORT();
            printf("SUPER_%-10d %5d '%s' %5d\n", numArgs, symbol,
                   vm->methodNames.data[symbol]->value, superclass);
            break;
        }

        case OP_JUMP: {
            int offset = READ_SHORT();
            printf("%-16s %5d to %d\n", "JUMP", offset, i + offset);
            break;
        }

        case OP_LOOP: {
            int offset = READ_SHORT();
            printf("%-16s %5d to %d\n", "LOOP", offset, i - offset);
            break;
        }

        case OP_JUMP_IF: {
            int offset = READ_SHORT();
            printf("%-16s %5d to %d\n", "JUMP_IF", offset, i + offset);
            break;
        }

        case OP_AND: {
            int offset = READ_SHORT();
            printf("%-16s %5d to %d\n", "AND", offset, i + offset);
            break;
        }

        case OP_OR: {
            int offset = READ_SHORT();
            printf("%-16s %5d to %d\n", "OR", offset, i + offset);
            break;
        }

        case OP_CLOSE_UPVALUE:
            printf("CLOSE_UPVALUE\n");
            break;
        case OP_RETURN:
            printf("RETURN\n");
            break;

        case OP_CLOSURE: {
            int constant = READ_SHORT();
            printf("%-16s %5d ", "CLOSURE", constant);
            MSCDumpValue(fn->constants.data[constant]);
            printf(" ");
            Function *loadedFn = AS_FUNCTION(fn->constants.data[constant]);
            for (int j = 0; j < loadedFn->numUpvalues; j++) {
                int isLocal = READ_BYTE();
                int index = READ_BYTE();
                if (j > 0) printf(", ");
                printf("%s %d", isLocal ? "local" : "upvalue", index);
            }
            printf("\n");
            break;
        }

        case OP_CONSTRUCT:
            printf("CONSTRUCT\n");
            break;
        case OP_EXTERN_CONSTRUCT:
            printf("EXTERN_CONSTRUCT\n");
            break;

        case OP_CLASS: {
            int numFields = READ_BYTE();
            printf("%-16s %5d fields\n", "CLASS", numFields);
            break;
        }

        case OP_EXTERN_CLASS:
            printf("EXTERN_CLASS\n");
            break;
        case OP_END_CLASS:
            printf("END_CLASS\n");
            break;

        case OP_METHOD_INSTANCE: {
            int symbol = READ_SHORT();
            printf("%-16s %5d '%s'\n", "METHOD_INSTANCE", symbol,
                   vm->methodNames.data[symbol]->value);
            break;
        }

        case OP_METHOD_STATIC: {
            int symbol = READ_SHORT();
            printf("%-16s %5d '%s'\n", "METHOD_STATIC", symbol,
                   vm->methodNames.data[symbol]->value);
            break;
        }

        case OP_END_MODULE:
            printf("END_MODULE\n");
            break;

        case OP_IMPORT_MODULE: {
            int name = READ_SHORT();
            printf("%-16s %5d '", "IMPORT_MODULE", name);
            MSCDumpValue(fn->constants.data[name]);
            printf("'\n");
            break;
        }

        case OP_IMPORT_VARIABLE: {
            int variable = READ_SHORT();
            printf("%-16s %5d '", "IMPORT_VARIABLE", variable);
            MSCDumpValue(fn->constants.data[variable]);
            printf("'\n");
            break;
        }

        case OP_END:
            printf("END\n");
            break;

        default:
            printf("UKNOWN! [%d]\n", bytecode[i - 1]);
            break;
    }

    // Return how many bytes this instruction takes, or -1 if it's an END.
    if (code == OP_END) return -1;
    return i - start;

#undef READ_BYTE
#undef READ_SHORT
}

void MSCDebugPrintStackTrace(MVM *vm) {
    // Bail if the host doesn't enable printing errors.
    if (vm->config.errorHandler == NULL) return;

    Djuru *fiber = vm->djuru;
    if (IS_STRING(fiber->error)) {
        vm->config.errorHandler(vm, ERROR_RUNTIME,
                                NULL, -1, AS_CSTRING(fiber->error));
    } else {
        // TODO: Print something a little useful here. Maybe the name of the error's
        // class?
        vm->config.errorHandler(vm, ERROR_RUNTIME,
                                NULL, -1, "[error object]");
    }

    for (int i = fiber->numOfFrames - 1; i >= 0; i--) {
        CallFrame *frame = &fiber->frames[i];
        Function *fn = frame->closure->fn;

        // Skip over stub functions for calling methods from the C API.
        if (fn->module == NULL) continue;

        // The built-in core module has no name. We explicitly omit it from stack
        // traces since we don't want to highlight to a user the implementation
        // detail of what part of the core module is written in C and what is MSC.
        if (fn->module->name == NULL) continue;

        // -1 because IP has advanced past the instruction that it just executed.
        int line = fn->debug->sourceLines.data[frame->ip - fn->code.data - 1];
        vm->config.errorHandler(vm, ERROR_STACK_TRACE,
                                fn->module->name->value, line,
                                fn->debug->name);
    }
}

void MSCDumpObject(Object *obj) {
    switch (obj->type) {
        case OBJ_CLASS:
            printf("[class %s %p]", ((Class *) obj)->name->value, obj);
            break;
        case OBJ_CLOSURE:
            printf("[closure %p]", obj);
            break;
        case OBJ_THREAD:
            printf("[djuru %p]", obj);
            break;
        case OBJ_FN:
            printf("[fn %p]", obj);
            break;
        case OBJ_EXTERN:
            printf("[foreign %p]", obj);
            break;
        case OBJ_INSTANCE:
            printf("[instance of %s %p]", obj->classObj->name->value, obj);
            break;
        case OBJ_LIST:
            printf("[list %p]", obj);
            break;
        case OBJ_MAP:
            printf("[map %p]", obj);
            break;
        case OBJ_MODULE:
            printf("[module %p]", obj);
            break;
        case OBJ_RANGE:
            printf("[range(%f, %f) %p]", ((Range *) obj)->from, ((Range *) obj)->to, obj);
            break;
        case OBJ_STRING:
            printf("%s", ((String *) obj)->value);
            break;
        case OBJ_UPVALUE:
            printf("[upvalue %p]", obj);
            break;
        default:
            printf("[unknown object %d]", obj->type);
            break;
    }
}

void MSCDumpValue(Value value) {
#if MSC_NAN_TAGGING
    if (IS_NUM(value)) {
        printf("%.14g", AS_NUM(value));
    } else if (IS_OBJ(value)) {
        MSCDumpObject(AS_OBJ(value));
    } else {
        switch (GET_TAG(value)) {
            case TAG_FALSE:
                printf("Galon");
                break;
            case TAG_NAN:
                printf("NaN");
                break;
            case TAG_NULL:
                printf("Gansan");
                break;
            case TAG_TRUE:
                printf("Tien");
                break;
            case TAG_UNDEFINED:
                UNREACHABLE();
        }
    }
#else
    switch (value.type)
    {
        case VAL_FALSE:     printf("false"); break;
        case VAL_NULL:      printf("null"); break;
        case VAL_NUM:       printf("%.14g", AS_NUM(value)); break;
        case VAL_TRUE:      printf("true"); break;
        case VAL_OBJ:       MSCDumpObject(AS_OBJ(value)); break;
        case VAL_UNDEFINED: UNREACHABLE();
    }
#endif
}

/*
static int simpleInstruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}


static void disassembleChunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}
static int disassembleInstruction(Chunk* chunk, int offset) {
    printf("%04d ", offset);

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case Opcode::OP_RETURN :
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}
*/

void MSCDumpStack(Djuru *fiber) {
    printf("(fiber %p) ", fiber);
    for (Value *slot = fiber->stack; slot < fiber->stackTop; slot++) {
        MSCDumpValue(*slot);
        printf(" | ");
    }
    printf("\n");
}

int MSCDumpInstruction(MVM *vm, Function *fn, int i) {
    return dumpInstruction(vm, fn, i, NULL);
}

void MSCDumpCode(MVM *vm, Function *fn) {
    printf("%s: %s\n",
           fn->module->name == NULL ? "<core>" : fn->module->name->value,
           fn->debug->name);

    int i = 0;
    int lastLine = -1;
    for (;;) {
        int offset = dumpInstruction(vm, fn, i, &lastLine);
        if (offset == -1) break;
        i += offset;
    }

    printf("\n");
}


void MSCDumpSymbolTable(const SymbolTable *symbols) {
    // See if the symbol is already defined.
    // TODO: O(n). Do something better.
    for (int i = 0; i < symbols->count; i++) {
       printf("Symb %d = %s", i, symbols->data[i]->value);
    }

}
