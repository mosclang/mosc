//
// Created by Mahamadou DOUMBIA [OML DSI] on 11/01/2022.
//


#include "Compiler.h"
#include "../runtime/MVM.h"
#include "../runtime/debuger.h"
#include "../helpers/Helper.h"
#include "../memory/Value.h"
#include "Parser.h"
#include "Token.h"


// The stack effect of each opcode. The index in the array is the opcode, and
// the value is the stack effect of that instruction.
static const int stackEffects[] = {
#define OPCODE(_, effect) effect,

#include "../common/codes.h"

#undef OPCODE
};


typedef enum {
    PREC_NONE,
    PREC_LOWEST,
    PREC_ASSIGNMENT,    // =
    PREC_CONDITIONAL,   // ?:
    PREC_LOGICAL_OR,    // ||
    PREC_LOGICAL_AND,   // &&
    PREC_EQUALITY,      // == !=
    PREC_IS,            // is
    PREC_COMPARISON,    // < > <= >=
    PREC_BITWISE_OR,    // |
    PREC_BITWISE_XOR,   // ^
    PREC_BITWISE_AND,   // &
    PREC_BITWISE_SHIFT, // << >>
    PREC_RANGE,         // ..
    PREC_TERM,          // + -
    PREC_FACTOR,        // * / %
    PREC_UNARY,         // unary - ! ~ ...(spread or rest element)
    PREC_CALL,          // . () []
    PREC_PRIMARY
} Precedence;

typedef struct {

    // The name of the local variable. This points directly into the original
    // source code string.
    const char *name;

    // The length of the local variable's name.
    int length;

    // The depth in the scope chain that this variable was declared at. Zero is
    // the outermost scope--parameters for a method, or the first local block in
    // top level code. One is the scope within that, etc.
    int depth;

    // If this local variable is being used as an upvalue.
    bool isUpvalue;
} Local;

typedef struct {

    // True if this upvalue is capturing a local variable from the enclosing
    // function. False if it's capturing an upvalue.
    bool isLocal;

    // The index of the local or upvalue being captured in the enclosing function.
    int index;
} CompilerUpvalue;

typedef struct sLoop Loop;
// Bookkeeping information for the current loop being compiled.
struct sLoop {
    // Index of the instruction that the loop should jump back to.
    int start;

    // Index of the argument for the CODE_JUMP_IF instruction used to exit the
    // loop. Stored so we can patch it once we know where the loop ends.
    int exitJump;

    // Index of the first instruction of the body of the loop.
    int body;

    // Depth of the scope(s) that need to be exited if a break is hit inside the
    // loop.
    int scopeDepth;

    // The loop enclosing this one, or NULL if this is the outermost loop.
    Loop *enclosing;
};

// The different signature syntaxes for different kinds of methods.
typedef enum {
    // A name followed by a (possibly empty) parenthesized parameter list. Also
    // used for binary operators.
            SIG_GETTER,
    SIG_SETTER,
    SIG_FUNCTION,

    SIG_SUBSCRIPT,
    SIG_SUBSCRIPT_SETTER,
    // A constructor initializer function. This has a distinct signature to
    // prevent it from being invoked directly outside of the constructor on the
    // metaclass.
            SIG_INITIALIZER
} SignatureType;

typedef struct {

    const char *name;
    int length;
    SignatureType type;
    int arity;
} Signature;

typedef struct {
    int slot;
    uint8_t symbol;
    const char *name;
    int length;
} ClassField;

DECLARE_BUFFER(ClassField, ClassField);

DEFINE_BUFFER(ClassField, ClassField);


typedef struct {
    ByteBuffer *code;
    ClassFieldBuffer fieldTrackings;
} ClassFieldPatch;

// Bookkeeping information for compiling a class definition.
typedef struct {

    // The name of the class.
    String *name;

    // Symbol table for the fields of the class.
    SymbolTable fields;

    // Symbols for the methods defined by the class. Used to detect duplicate
    // method definitions.
    IntBuffer methods;
    IntBuffer staticMethods;

    // True if the class being compiled is a foreign class.
    bool isExtern;

    // True if the current method being compiled is static.
    bool inStatic;

    // The signature of the method being compiled.
    Signature *signature;

} ClassInfo;

// Describes where a variable is declared.
typedef enum {
    // A local variable in the current function.
            SCOPE_LOCAL,

    // A local variable declared in an enclosing function.
            SCOPE_UPVALUE,

    // A top-level module variable.
            SCOPE_MODULE
} Scope;

// A reference to a variable and the scope where it is defined. This contains
// enough information to emit correct code to load or store the variable.
typedef struct {

    // The stack slot, upvalue slot, or module symbol defining the variable.
    int index;

    // Where the variable is declared.
    Scope scope;
} Variable;


typedef enum {
    OBJECT_PATTERN,
    ARRAY_PATTERN,
    REST_PATTERN,
    NONE_PATTERN
} PatternType;

typedef struct Pattern Pattern;

DECLARE_BUFFER(Pattern, Pattern);


struct Pattern {
    PatternType type;

    PatternType parent;

    union {
        PatternBuffer object;
        PatternBuffer array;
        Token id;
    } as;

    Variable variable;
    struct Pattern *alias;
};

DEFINE_BUFFER(Pattern, Pattern);

struct Compiler {


    Parser *parser;

    // The compiler for the function enclosing this one, or NULL if it's the
    // top level.
    struct Compiler *parent;

    // The currently in scope local variables.
    Local locals[MAX_LOCALS];

    // The number of local variables currently in scope.
    int numLocals;

    // The upvalues that this function has captured from outer scopes. The count
    // of them is stored in [numUpvalues].
    CompilerUpvalue upvalues[MAX_UPVALUES];

    // The current level of block scope nesting, where zero is no nesting. A -1
    // here means top-level code is being compiled and there is no block scope
    // in effect at all. Any variables declared will be module-level.
    int scopeDepth;

    // The current number of slots (locals and temporaries) in use.
    //
    // We use this and maxSlots to track the maximum number of additional slots
    // a function may need while executing. When the function is called, the
    // fiber will check to ensure its stack has enough room to cover that worst
    // case and grow the stack if needed.
    //
    // This value here doesn't include parameters to the function. Since those
    // are already pushed onto the stack by the caller and tracked there, we
    // don't need to double count them here.
    int numSlots;

    // The current innermost loop being compiled, or NULL if not in a loop.
    Loop *loop;

    // If this is a compiler for a method, keeps track of the class enclosing it.
    ClassInfo *enclosingClass;

    // The function being compiled.
    Function *function;

    // The constants for the function being compiled.
    Map *constants;

    // Whether or not the compiler is for a constructor initializer
    bool isInitializer;

    bool isExtension;

    TokenType dotSource;

    ClassFieldBuffer fieldTrackings;
    // The number of attributes seen while parsing.
    // We track this separately as compile time attributes
    // are not stored, so we can't rely on attributes->count
    // to enforce an error message when attributes are used
    // anywhere other than methods or classes.


};


static void newCompilerUpvalue(CompilerUpvalue *thisValue, bool isLocal, int index) {
    thisValue->index = index;
    thisValue->isLocal = isLocal;
}

static void newLocal(Local *local, const char *name, int length, int depth, bool isUpvalue) {

    local->name = name;
    local->length = length;
    local->depth = depth;
    local->isUpvalue = isUpvalue;
}


static void newLoop(Loop *loop, int start, int exitJump, int body, int scope, Loop *parent) {

    loop->start = start;
    loop->exitJump = exitJump;
    loop->body = body;
    loop->scopeDepth = scope;
    loop->enclosing = parent;
}


static void newSignature(Signature *sign, const char *name, int length, SignatureType type, int arity) {
    sign->length = length;
    sign->name = name;
    sign->type = type;
    sign->arity = arity;
}


static void newClassInfo(ClassInfo *classInfo, String *name, bool isExtern, bool inStatic) {
    // Set up a symbol table for the class's fields. We'll initially compile
    // them to slots starting at zero. When the method is bound to the class, the
    // bytecode will be adjusted by [MSCBindMethod] to take inherited fields
    // into account.

    classInfo->name = name;
    classInfo->inStatic = inStatic;
    classInfo->isExtern = isExtern;

    MSCSymbolTableInit(&classInfo->fields);
    // Set up symbol buffers to track duplicate static and instance methods.
    MSCInitIntBuffer(&classInfo->methods);
    MSCInitIntBuffer(&classInfo->staticMethods);
    classInfo->signature = NULL;
}


static void newVariable(Variable *variable, int index, Scope scope) {
    variable->index = index;
    variable->scope = scope;
}

static Pattern parsePattern(Compiler *compiler, PatternType parent, bool declare);

static void defineMethod(Compiler *compiler, Variable *classVariable, bool isStatic, int methodSymbol);

static void whenExpression(Compiler *compiler, bool canAssign);

static void ifExpression(Compiler *compiler, bool canAssign);

static void ignoreNewlines(Compiler *compiler);

static void parsePrecedence(Compiler *compiler, Precedence precedence);

static bool statement(Compiler *compiler, bool expr);

static bool definition(Compiler *compiler, bool expr);


static bool isDeclared(IntBuffer *methods, int symbol) {
    for (int i = 0; i < methods->count; i++) {
        if (methods->data[i] == symbol) {
            return true;
        }
    }
    return false;
}


static void initCompiler(Compiler *compiler, Parser *parser, Compiler *parent,
                         bool isMethod) {
    compiler->parser = parser;
    compiler->parent = parent;
    compiler->loop = NULL;
    compiler->enclosingClass = NULL;
    compiler->isInitializer = false;
    compiler->isExtension = false;

    // Initialize these to NULL before allocating in case a GC gets triggered in
    // the middle of initializing the compiler.
    compiler->function = NULL;
    compiler->constants = NULL;

    parser->vm->compiler = compiler;


    compiler->numLocals = 1;
    compiler->numSlots = compiler->numLocals;

    if (isMethod) {
        compiler->locals[0].name = "ale";
        compiler->locals[0].length = 3;
    } else {
        compiler->locals[0].name = NULL;
        compiler->locals[0].length = 0;
    }

    compiler->locals[0].depth = -1;
    compiler->locals[0].isUpvalue = false;
    if (parent == NULL) {
        // Compiling top-level code, so the initial scope is module-level.
        compiler->scopeDepth = -1;
    } else {
        // The initial scope for functions and methods is local scope.
        compiler->scopeDepth = 0;
    }

    compiler->function = MSCFunctionFrom(parser->vm, parser->module, compiler->numLocals);
    MSCInitClassFieldBuffer(&compiler->fieldTrackings);

}

void MSCMarkCompiler(Compiler *compiler, MVM *mvm) {

    MSCGrayValue(mvm, compiler->parser->current.value);
    MSCGrayValue(mvm, compiler->parser->previous.value);
    MSCGrayValue(mvm, compiler->parser->next.value);
    Compiler *parent = compiler;
    // Walk up the parent chain to mark the outer compilers too. The VM only
    // tracks the innermost one.
    do {

        MSCGrayObject((Object *) parent->function, mvm);

        MSCGrayObject((Object *) parent->constants, mvm);
        if (parent->enclosingClass != NULL) {
            MSCBlackenSymbolTable(mvm, &parent->enclosingClass->fields);
        }
        parent = parent->parent;
    } while (parent != NULL);
}

static void error(Compiler *compiler, const char *format, ...) {
    Token *token = &compiler->parser->previous;

    // If the parse error was caused by an error token, the lexer has already
    // reported it.
    if (token->type == ERROR_TOKEN) return;

    va_list args;
    va_start(args, format);
    if (token->type == EOL_TOKEN) {
        printError(compiler->parser, token->line, "Error at newline", format, args);
    } else if (token->type == EOF_TOKEN) {
        printError(compiler->parser, token->line,
                   "Error at end of file", format, args);
    } else {
        // Make sure we don't exceed the buffer with a very long token.
        char label[10 + MAX_VARIABLE_NAME + 4 + 1];
        if (token->length <= MAX_VARIABLE_NAME) {
            sprintf(label, "Error at '%.*s'", token->length, token->start);
        } else {
            sprintf(label, "Error at '%.*s...'", MAX_VARIABLE_NAME, token->start);
        }
        printError(compiler->parser, token->line, label, format, args);
    }
    va_end(args);
}

int addConstant(Compiler *compiler, Value constant) {
    if (compiler->parser->hasError) return -1;
    // See if we already have a constant for the value. If so, reuse it.
    if (compiler->constants != NULL) {
        Value existing = MSCMapGet(compiler->constants, constant);
        if (IS_NUM(existing)) return (int) AS_NUM(existing);
    }

    // It's a new constant.
    if (compiler->function->constants.count < MAX_CONSTANTS) {
        if (IS_OBJ(constant)) MSCPushRoot(compiler->parser->vm->gc, AS_OBJ(constant));
        MSCWriteValueBuffer(compiler->parser->vm, &compiler->function->constants,
                            constant);
        if (IS_OBJ(constant)) MSCPopRoot(compiler->parser->vm->gc);

        if (compiler->constants == NULL) {
            compiler->constants = MSCMapFrom(compiler->parser->vm);
        }
        MSCMapSet(compiler->constants, compiler->parser->vm, constant,
                  NUM_VAL(compiler->function->constants.count - 1));
    } else {
        error(compiler, "A function may only contain %d unique constants.", MAX_CONSTANTS);
    }

    return compiler->function->constants.count - 1;
}

void expression(Compiler *compiler) {
    parsePrecedence(compiler, PREC_LOWEST);

}

// Consumes the current token. Emits an error if its type is not [expected].
void consume(Compiler *compiler, TokenType expected,
             const char *errorMessage) {
    nextToken(compiler->parser);
    if (compiler->parser->previous.type != expected) {
        error(compiler, errorMessage);

        // If the next token is the one we want, assume the current one is just a
        // spurious error and discard it to minimize the number of cascaded errors.
        if (compiler->parser->current.type == expected) nextToken(compiler->parser);
    }
}

// Returns the type of the current token.
TokenType peek(Compiler *compiler) {
    return compiler->parser->current.type;
}

bool match(Compiler *compiler, TokenType expected) {
    if (peek(compiler) != expected) return false;

    nextToken(compiler->parser);
    return true;
}

bool matchLine(Compiler *compiler) {
    if (!match(compiler, EOL_TOKEN)) return false;

    while (match(compiler, EOL_TOKEN));
    return true;
}

// Emits one single-byte argument. Returns its index.
int emitByte(Compiler *compiler, int byte) {
    MSCWriteByteBuffer(compiler->parser->vm, &compiler->function->code, (uint8_t) byte);

    // Assume the instruction is associated with the most recently consumed token.
    MSCWriteIntBuffer(compiler->parser->vm, &compiler->function->debug->sourceLines,
                      compiler->parser->previous.line);

    return compiler->function->code.count - 1;
}

// Emits one bytecode instruction.
void emitOp(Compiler *compiler, Opcode instruction) {
    emitByte(compiler, instruction);
    // Keep track of the stack's high water mark.
    compiler->numSlots += stackEffects[instruction];
    if (compiler->numSlots > compiler->function->maxSlots) {
        compiler->function->maxSlots = compiler->numSlots;
    }
}

// Emits one 16-bit argument, which will be written big endian.
void emitShort(Compiler *compiler, int arg) {
    emitByte(compiler, (arg >> 8) & 0xff);
    emitByte(compiler, arg & 0xff);
}

// Emits one bytecode instruction followed by a 16-bit argument, which will be
// written big endian.
void emitShortArg(Compiler *compiler, Opcode instruction, int arg) {
    emitOp(compiler, instruction);
    emitShort(compiler, arg);
}

// Finishes [compiler], which is compiling a function, method, or chunk of top
// level code. If there is a parent compiler, then this emits code in the
// parent compiler to load the resulting function.
Function *endCompiler(Compiler *compiler, const char *debugName, int debugNameLength) {
    /*if (compiler->enclosingClass->signature != NULL) {
        delete (compiler->enclosingClass->signature);
        compiler->enclosingClass->signature = NULL;
    }*/
    // If we hit an error, don't finish the function since it's borked anyway.
    if (compiler->parser->hasError) {
        compiler->parser->vm->compiler = compiler->parent;
        return NULL;
    }

    // Mark the end of the bytecode. Since it may contain multiple early returns,
    // we can't rely on OP_RETURN to tell us we're at the end.
    emitOp(compiler, OP_END);

    MSCFunctionBindName(compiler->function, compiler->parser->vm, debugName, debugNameLength);

    // In the function that contains this one, load the resulting function object.
    if (compiler->parent != NULL) {
        int constant = addConstant(compiler->parent, OBJ_VAL(compiler->function));

        // Wrap the function in a closure. We do this even if it has no upvalues so
        // that the VM can uniformly assume all called objects are closures. This
        // makes creating a function a little slower, but makes invoking them
        // faster. Given that functions are invoked more often than they are
        // created, this is a win.
        emitShortArg(compiler->parent, OP_CLOSURE, constant);

        // Emit arguments for each upvalue to know whether to capture a local or
        // an upvalue.
        for (int i = 0; i < compiler->function->numUpvalues; i++) {
            emitByte(compiler->parent, compiler->upvalues[i].isLocal ? 1 : 0);
            emitByte(compiler->parent, compiler->upvalues[i].index);
        }
        /*ClassInfo *info = getEnclosingClass(compiler);

        if (info != NULL && compiler->fieldTrackings.count > 0) {
            // we are compiling a class, so add this function for pacting purpose
            info->toPatch.push_back({
                                            &compiler->function->code,
                                            compiler->fieldTrackings
                                    });
        }*/
    }
    // Pop this compiler off the stack.
    compiler->parser->vm->compiler = compiler->parent;

#if MSC_DEBUG_DUMP_COMPILED_CODE
    MSCDumpCode(compiler->parser->vm, compiler->function);
#endif

    return compiler->function;
}

Function *MSCCompile(MVM *vm, Module *module, const char *source, bool isExpression,
                     bool printErrors) {
    // Skip the UTF-8 BOM if there is one.
    // if (strncmp(source, "\xEF\xBB\xBF", 3) == 0) source += 3;
    if (strncmp(source, "\xEF\xBB\xBF", 3) == 0) source += 3;

    Parser parser;
    initParser(&parser, vm, module, source);

    parser.printErrors = printErrors;
    parser.hasError = false;

    // Read the first token into next
    nextToken(&parser);
    // return NULL;
    // Copy next -> current
    nextToken(&parser);

    int numExistingVariables = module->variables.count;

    Compiler compiler;
    initCompiler(&compiler, &parser, NULL, false);

    ignoreNewlines(&compiler);
    if (isExpression) {
        expression(&compiler);
        consume(&compiler, EOF_TOKEN, "Expect end of expression.");
    } else {
        while (!match(&compiler, EOF_TOKEN)) {
            definition(&compiler, false);
            bool valid = match(&compiler, SEMI_TOKEN);
            if (!valid) valid = matchLine(&compiler);
            else matchLine(&compiler);
            // If there is no newline, it must be the end of file on the same line.
            if (!valid) {
                consume(&compiler, EOF_TOKEN, "Expect ; or end of file.");
                // eturn NULL;
                break;
            }
        }
        emitOp(&compiler, OP_END_MODULE);
    }

    emitOp(&compiler, OP_RETURN);

    // See if there are any implicitly declared module-level variables that never
    // got an explicit definition. They will have values that are numbers
    // indicating the line where the variable was first used.
    for (int i = numExistingVariables; i < parser.module->variables.count; i++) {
        if (IS_NUM(parser.module->variables.data[i])) {
            // Synthesize a token for the original use site.
            parser.previous.type = ID_TOKEN;
            parser.previous.start = parser.module->variableNames.data[i]->value;
            parser.previous.length = parser.module->variableNames.data[i]->length;
            parser.previous.line = (int) AS_NUM(parser.module->variables.data[i]);
            error(&compiler, "Variable is used but not defined: '%s'\n", parser.module->variableNames.data[i]->value);
        }
    }

    return endCompiler(&compiler, "(script)", 8);
}

// Returns the number of bytes for the arguments to the instruction
// at [ip] in [fn]'s bytecode.
static int getByteCountForArguments(const uint8_t *bytecode,
                                    const Value *constants, int ip) {
    Opcode instruction = (Opcode) bytecode[ip];
    switch (instruction) {
        case OP_NULL:
        case OP_FALSE:
        case OP_TRUE:
        case OP_VOID:
        case OP_POP:
        case OP_CLOSE_UPVALUE:
        case OP_RETURN:
        case OP_END:
        case OP_LOAD_LOCAL_0:
        case OP_LOAD_LOCAL_1:
        case OP_LOAD_LOCAL_2:
        case OP_LOAD_LOCAL_3:
        case OP_LOAD_LOCAL_4:
        case OP_LOAD_LOCAL_5:
        case OP_LOAD_LOCAL_6:
        case OP_LOAD_LOCAL_7:
        case OP_LOAD_LOCAL_8:
        case OP_CONSTRUCT:
        case OP_EXTERN_CONSTRUCT:
        case OP_EXTERN_CLASS:
        case OP_END_MODULE:
        case OP_END_CLASS:
            return 0;

        case OP_LOAD_LOCAL:
        case OP_STORE_LOCAL:
        case OP_LOAD_UPVALUE:
        case OP_STORE_UPVALUE:
        case OP_LOAD_FIELD_THIS:
        case OP_STORE_FIELD_THIS:
        case OP_LOAD_FIELD:
        case OP_STORE_FIELD:
        case OP_FIELD:
        case OP_CLASS:
            return 1;

        case OP_CONSTANT:
        case OP_LOAD_MODULE_VAR:
        case OP_STORE_MODULE_VAR:
        case OP_CALL:
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
        case OP_CALL_16:
        case OP_JUMP:
        case OP_LOOP:
        case OP_JUMP_IF:
        case OP_AND:
        case OP_OR:
        case OP_METHOD_INSTANCE:
        case OP_METHOD_STATIC:
        case OP_IMPORT_MODULE:
        case OP_IMPORT_VARIABLE:
            return 2;

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
        case OP_SUPER_16:
            return 4;

        case OP_CLOSURE: {
            int constant = (bytecode[ip + 1] << 8) | bytecode[ip + 2];
            Function *loadedFn = AS_FUNCTION(constants[constant]);
            // There are two bytes for the constant, then two for each upvalue.
            return 2 + (loadedFn->numUpvalues * 2);
        }
    }

    UNREACHABLE();
    return 0;
}

void MSCBindMethodCode(Class *classObj, Function *fn) {
    ASSERT(fn->boundToClass == NULL, "Trying to rebound a method");
    fn->boundToClass = classObj;

    int ip = 0;
    for (;;) {
        Opcode instruction = (Opcode) fn->code.data[ip];
        switch (instruction) {
            case OP_FIELD:
            case OP_LOAD_FIELD:
            case OP_STORE_FIELD:
            case OP_LOAD_FIELD_THIS:
            case OP_STORE_FIELD_THIS:
                // Shift this class's fields down past the inherited ones. We don't
                // check for overflow here because we'll see if the number of fields
                // overflows when the subclass is created.

                fn->code.data[ip + 1] += classObj->superclass->numFields;
                break;

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
                // Fill in the constant slot with a reference to the superclass.
                int constant = (fn->code.data[ip + 3] << 8) | fn->code.data[ip + 4];
                fn->constants.data[constant] = OBJ_VAL(classObj->superclass);
                break;
            }

            case OP_CLOSURE: {
                // Bind the nested closure too.
                int constant = (fn->code.data[ip + 1] << 8) | fn->code.data[ip + 2];
                MSCBindMethodCode(classObj, AS_FUNCTION(fn->constants.data[constant]));
                break;
            }

            case OP_END:
                return;

            default:
                // Other instructions are unaffected, so just skip over them.
                break;
        }
        ip += 1 + getByteCountForArguments(fn->code.data, fn->constants.data, ip);
    }
}

// Returns the type of the current token.
static TokenType peekNext(Compiler *compiler) {
    return compiler->parser->next.type;
}

// Consumes the current token if its type is [expected]. Returns true if a
// token was consumed.

// Matches one or more newlines. Returns true if at least one was found.

// Discards any newlines starting at the current token.
static void ignoreNewlines(Compiler *compiler) {
    matchLine(compiler);
}

// Consumes the current token. Emits an error if it is not a newline. Then
// discards any duplicate newlines following it.
static void consumeLine(Compiler *compiler, const char *errorMessage) {
    consume(compiler, EOL_TOKEN, errorMessage);
    ignoreNewlines(compiler);
}

static void allowLineBeforeDot(Compiler *compiler) {
    if (peek(compiler) == EOL_TOKEN && peekNext(compiler) == DOT_TOKEN) {
        nextToken(compiler->parser);
    }
}

// Variables and scopes --------------------------------------------------------

// Emits one bytecode instruction followed by a 8-bit argument. Returns the
// index of the argument in the bytecode.
static int emitByteArg(Compiler *compiler, Opcode instruction, int arg) {
    emitOp(compiler, instruction);
    return emitByte(compiler, arg);
}

// Emits [instruction] followed by a placeholder for a jump offset. The
// placeholder can be patched by calling [jumpPatch]. Returns the index of the
// placeholder.
static int emitJump(Compiler *compiler, Opcode instruction) {
    emitOp(compiler, instruction);
    emitByte(compiler, 0xff);
    return emitByte(compiler, 0xff) - 1;
}

// Creates a new constant for the current value and emits the bytecode to load
// it from the constant table.
static void emitConstant(Compiler *compiler, Value value) {
    int constant = addConstant(compiler, value);
    // Compile the code to load the constant.
    emitShortArg(compiler, OP_CONSTANT, constant);
}

// Create a new local variable with [name]. Assumes the current scope is local
// and the name is unique.
static int addLocal(Compiler *compiler, const char *name, int length) {
    Local *local = &compiler->locals[compiler->numLocals];
    local->name = name;
    local->length = length;
    local->depth = compiler->scopeDepth;
    local->isUpvalue = false;
    return compiler->numLocals++;
}

// Appends characters to [name] (and updates [length]) for [numParams] "_"
// surrounded by [leftBracket] and [rightBracket].
static void signatureParameterList(char name[MAX_METHOD_SIGNATURE], int *length,
                                   int numParams, char leftBracket, char rightBracket) {
    name[(*length)++] = leftBracket;
    // This function may be called with too many parameters. When that happens,
    // a compile error has already been reported, but we need to make sure we
    // don't overflow the string too, hence the MAX_PARAMETERS check.
    for (int i = 0; i < numParams && i < MAX_PARAMETERS; i++) {
        if (i > 0) name[(*length)++] = ',';
        name[(*length)++] = '_';
    }
    name[(*length)++] = rightBracket;
}

// Fills [name] with the stringified version of [signature] and updates
// [length] to the resulting length.
static void signatureToString(Signature *signature,
                              char name[MAX_METHOD_SIGNATURE], int *length) {
    *length = 0;

    // Build the full name from the signature.
    memcpy(name + *length, signature->name, (size_t) signature->length);
    *length += signature->length;

    switch (signature->type) {
        case SIG_FUNCTION:
            signatureParameterList(name, length, signature->arity, '(', ')');
            break;
        case SIG_GETTER:
            // The signature is just the name.
            break;

        case SIG_SETTER:
            name[(*length)++] = '=';
            signatureParameterList(name, length, 1, '(', ')');
            break;
        case SIG_SUBSCRIPT:
            signatureParameterList(name, length, signature->arity, '[', ']');
            break;
        case SIG_SUBSCRIPT_SETTER:
            signatureParameterList(name, length, signature->arity - 1, '[', ']');
            name[(*length)++] = '=';
            signatureParameterList(name, length, 1, '(', ')');
            break;
        case SIG_INITIALIZER:
            memcpy(name, "kura ", 5);
            memcpy(name + 5, signature->name, (size_t) signature->length);
            *length = 5 + signature->length;
            signatureParameterList(name, length, signature->arity, '(', ')');
            break;
    }

    name[*length] = '\0';
}

// Gets the symbol for a method [name] with [length].
static int methodSymbol(Compiler *compiler, const char *name, int length) {
    return MSCSymbolTableEnsure(compiler->parser->vm,
                                &compiler->parser->vm->methodNames, name, (size_t) length);
}

/**
 * Generate a setter fnction for a field
 * [LOAD_PARAM? SET_THIS_FIELD]
 * [OP_LOAD_LOCAL_1, OP_STORE_FIELD]
 * @param classVariable
 * @param field
 * @param name
 * @param length
 * @return
 */
static void
emitSetter(Compiler *compiler, Variable *classVariable, int field, const char *name, int length, bool isStatic) {
    Signature setter = {name, length, SIG_SETTER, 1};
    int size = length;
    char fullSignature[MAX_METHOD_SIGNATURE];
    signatureToString(&setter, fullSignature, &size);
    int symbol = methodSymbol(compiler, fullSignature, size);

    if (isDeclared(&compiler->enclosingClass->methods, symbol)) {
        return;
    }

    Compiler fnCompiler;
    initCompiler(&fnCompiler, compiler->parser, compiler, true);
    emitOp(&fnCompiler, OP_LOAD_LOCAL_1);// load the param
    emitByteArg(&fnCompiler, OP_STORE_FIELD_THIS, field);
    emitOp(&fnCompiler, OP_NULL);
    emitOp(&fnCompiler, OP_RETURN);
    endCompiler(&fnCompiler, fullSignature, size);
    defineMethod(compiler, classVariable,
                 isStatic, symbol);
}


/**
 * Generate a getter for a field
 * [LOAD_THIS_FIELD]
 * [OP_LOAD_FIELD]
 * @param classVariable
 * @param field
 * @param name
 * @param length
 * @return
 */
void emitGetter(Compiler *compiler, Variable *classVariable, int field, const char *name, int length, bool isStatic) {
    Signature getter = {name, length, SIG_GETTER, 0};
    int size = length;
    char fullSignature[MAX_METHOD_SIGNATURE];
    signatureToString(&getter, fullSignature, &size);
    int symbol = methodSymbol(compiler, fullSignature, size);

    if (isDeclared(&compiler->enclosingClass->methods, symbol)) {
        return;
    }
    Compiler fnCompiler;
    initCompiler(&fnCompiler, compiler->parser, compiler, true);
    emitByteArg(&fnCompiler, OP_LOAD_FIELD_THIS, field);
    emitOp(&fnCompiler, OP_RETURN);
    endCompiler(&fnCompiler, fullSignature, size);
    defineMethod(compiler, classVariable, isStatic, symbol);
}

// Declares a variable in the current scope whose name is the given token.
//
// If [token] is `NULL`, uses the previously consumed token. Returns its symbol.
int declareVariable(Compiler *compiler, Token *token) {
    if (token == NULL) token = &compiler->parser->previous;

    if (token->length > MAX_VARIABLE_NAME) {
        error(compiler, "Variable name cannot be longer than %d characters.",
              MAX_VARIABLE_NAME);
    }

    // Top-level module scope.
    if (compiler->scopeDepth == -1) {
        int line = -1;
        int symbol = MSCDefineVariable(
                compiler->parser->vm,
                compiler->parser->module,
                token->start, token->length,
                NULL_VAL, &line);

        if (symbol == -1) {
            error(compiler, "Module variable is already defined.");
        } else if (symbol == -2) {
            error(compiler, "Too many module variables defined.");
        } else if (symbol == -3) {
            error(compiler,
                  "Variable '%.*s' referenced before this definition (first use at line %d).",
                  token->length, token->start, line);
        }
        return symbol;
    }

    // See if there is already a variable with this name declared in the current
    // scope. (Outer scopes are OK: those get shadowed.)
    for (int i = compiler->numLocals - 1; i >= 0; i--) {
        Local *local = &compiler->locals[i];

        // Once we escape this scope and hit an outer one, we can stop.
        if (local->depth < compiler->scopeDepth) break;

        if (local->length == token->length &&
            memcmp(local->name, token->start, (size_t) token->length) == 0) {
            error(compiler, "Variable is already declared in this scope.");
            return i;
        }
    }

    if (compiler->numLocals == MAX_LOCALS) {
        error(compiler, "Cannot declare more than %d variables in one scope.",
              MAX_LOCALS);
        return -1;
    }

    return addLocal(compiler, token->start, token->length);
}

// Declares a function in the current scope.
//
// If [token] is `NULL`, uses the previously consumed token. Returns its symbol.
int declareFunction(Compiler *compiler, const char *name, int length) {

    if (length > MAX_VARIABLE_NAME) {
        error(compiler, "Function name cannot be longer than %d characters.",
              MAX_VARIABLE_NAME);
    }

    // Top-level module scope.
    if (compiler->scopeDepth == -1) {
        int line = -1;
        int symbol = MSCDefineVariable(
                compiler->parser->vm,
                compiler->parser->module,
                name, length,
                NULL_VAL, &line);

        if (symbol == -1) {
            error(compiler, "Module function is already defined.");
        } else if (symbol == -2) {
            error(compiler, "Too many module variables defined.");
        } else if (symbol == -3) {
            error(compiler,
                  "Function '%.*s' referenced before this definition (first use at line %d).",
                  length, name, line);
        }
        return symbol;
    }

    // See if there is already a variable with this name declared in the current
    // scope. (Outer scopes are OK: those get shadowed.)
    for (int i = compiler->numLocals - 1; i >= 0; i--) {
        Local *local = &compiler->locals[i];

        // Once we escape this scope and hit an outer one, we can stop.
        if (local->depth < compiler->scopeDepth) break;

        if (local->length == length &&
            memcmp(local->name, name, (size_t) length) == 0) {
            error(compiler, "Function is already declared in this scope.");
            return i;
        }
    }

    if (compiler->numLocals == MAX_LOCALS) {
        error(compiler, "Cannot declare more than %d variables in one scope.",
              MAX_LOCALS);
        return -1;
    }

    return addLocal(compiler, name, length);
}

// Parses a name token and declares a variable in the current scope with that
// name. Returns its slot.
int declareNamedVariable(Compiler *compiler) {
    consume(compiler, ID_TOKEN, "Expect variable name.");
    return declareVariable(compiler, NULL);
}

// Stores a variable with the previously defined symbol in the current scope.
void defineVariable(Compiler *compiler, int symbol) {
    // Store the variable. If it's a local, the result of the initializer is
    // in the correct slot on the stack already so we're done.
    if (compiler->scopeDepth >= 0) return;

    // It's a module-level variable, so store the value in the module slot and
    // then discard the temporary for the initializer.
    emitShortArg(compiler, OP_STORE_MODULE_VAR, symbol);
    emitOp(compiler, OP_POP);
}

// Starts a new local block scope.
void pushScope(Compiler *compiler) {
    compiler->scopeDepth++;
}

// Generates code to discard local variables at [depth] or greater. Does *not*
// actually undeclare variables or pop any scopes, though. This is called
// directly when compiling "break" statements to ditch the local variables
// before jumping out of the loop even though they are still in scope *past*
// the break instruction.
//
// Returns the number of local variables that were eliminated.
int discardLocals(Compiler *compiler, int depth) {
    ASSERT(compiler->scopeDepth > -1, "Cannot exit top-level scope.");

    int local = compiler->numLocals - 1;
    while (local >= 0 && compiler->locals[local].depth >= depth) {
        // If the local was closed over, make sure the upvalue gets closed when it
        // goes out of scope on the stack. We use emitByte() and not emitOp() here
        // because we don't want to track that stack effect of these pops since the
        // variables are still in scope after the break.
        if (compiler->locals[local].isUpvalue) {
            emitByte(compiler, OP_CLOSE_UPVALUE);
        } else {
            emitByte(compiler, OP_POP);
        }

        local--;
    }

    return compiler->numLocals - local - 1;
}

int softDiscardLocals(Compiler *compiler, int depth) {
    ASSERT(compiler->scopeDepth > -1, "Cannot exit top-level scope.");

    int local = compiler->numLocals - 1;
    while (local >= 0 && compiler->locals[local].depth >= depth) {
        local--;
    }

    return compiler->numLocals - local - 1;
}

// Closes the last pushed block scope and discards any local variables declared
// in that scope. This should only be called in a statement context where no
// temporaries are still on the stack.
void popScope(Compiler *compiler) {
    int popped = discardLocals(compiler, compiler->scopeDepth);
    compiler->numLocals -= popped;
    compiler->numSlots -= popped;
    compiler->scopeDepth--;
}

void softPopScope(Compiler *compiler) {
    int popped = softDiscardLocals(compiler, compiler->scopeDepth);
    compiler->numLocals -= popped;
    compiler->numSlots -= popped;
    compiler->scopeDepth--;
}

// Attempts to look up the name in the local variables of [compiler]. If found,
// returns its index, otherwise returns -1.
int resolveLocal(Compiler *compiler, const char *name, int length) {
    // Look it up in the local scopes. Look in reverse order so that the most
    // nested variable is found first and shadows outer ones.
    for (int i = compiler->numLocals - 1; i >= 0; i--) {
        if (compiler->locals[i].length == length &&
            memcmp(name, compiler->locals[i].name, (size_t) length) == 0) {
            return i;
        }
    }

    return -1;
}

// Adds an upvalue to [compiler]'s function with the given properties. Does not
// add one if an upvalue for that variable is already in the list. Returns the
// index of the upvalue.
int addUpvalue(Compiler *compiler, bool isLocal, int index) {
    // Look for an existing one.
    for (int i = 0; i < compiler->function->numUpvalues; i++) {
        CompilerUpvalue *upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) return i;
    }

    // If we got here, it's a new upvalue.
    compiler->upvalues[compiler->function->numUpvalues].isLocal = isLocal;
    compiler->upvalues[compiler->function->numUpvalues].index = index;
    return compiler->function->numUpvalues++;
}

// Attempts to look up [name] in the functions enclosing the one being compiled
// by [compiler]. If found, it adds an upvalue for it to this compiler's list
// of upvalues (unless it's already in there) and returns its index. If not
// found, returns -1.
//
// If the name is found outside of the immediately enclosing function, this
// will flatten the closure and add upvalues to all of the intermediate
// functions so that it gets walked down to this one.
//
// If it reaches a method boundary, this stops and returns -1 since methods do
// not close over local variables.
int findUpvalue(Compiler *compiler, const char *name, int length) {
    // If we are at the top level, we didn't find it.
    if (compiler->parent == NULL) return -1;

    // If we hit the method boundary (and the name isn't a static field), then
    // stop looking for it. We'll instead treat it as a self send.
    // if (name[0] != '_' && compiler->parent->enclosingClass != NULL) return -1;

    // See if it's a local variable in the immediately enclosing function.
    int local = resolveLocal(compiler->parent, name, length);
    if (local != -1) {
        // Mark the local as an upvalue so we know to close it when it goes out of
        // scope.
        compiler->parent->locals[local].isUpvalue = true;

        return addUpvalue(compiler, true, local);
    }

    // See if it's an upvalue in the immediately enclosing function. In other
    // words, if it's a local variable in a non-immediately enclosing function.
    // This "flattens" closures automatically: it adds upvalues to all of the
    // intermediate functions to get from the function where a local is declared
    // all the way into the possibly deeply nested function that is closing over
    // it.
    int upvalue = findUpvalue(compiler->parent, name, length);
    if (upvalue != -1) {
        return addUpvalue(compiler, false, upvalue);
    }

    // If we got here, we walked all the way up the parent chain and couldn't
    // find it.
    return -1;
}

// Look up [name] in the current scope to see what variable it refers to.
// Returns the variable either in local scope, or the enclosing function's
// upvalue list. Does not search the module scope. Returns a variable with
// index -1 if not found.
Variable resolveNonmodule(Compiler *compiler, const char *name, int length) {
    // Look it up in the local scopes.
    Variable variable;
    variable.scope = SCOPE_LOCAL;
    variable.index = resolveLocal(compiler, name, length);
    if (variable.index != -1) return variable;
    // Tt's not a local, so guess that it's an upvalue.
    variable.scope = SCOPE_UPVALUE;
    variable.index = findUpvalue(compiler, name, length);
    return variable;
}

// Look up [name] in the current scope to see what variable it refers to.
// Returns the variable either in module scope, local scope, or the enclosing
// function's upvalue list. Returns a variable with index -1 if not found.
Variable resolveName(Compiler *compiler, const char *name, int length) {
    Variable variable = resolveNonmodule(compiler, name, length);
    if (variable.index != -1) return variable;

    variable.scope = SCOPE_MODULE;
    variable.index = MSCSymbolTableFind(&compiler->parser->module->variableNames,
                                        name, (size_t) length);
    return variable;
}

void loadLocal(Compiler *compiler, int slot) {
    if (slot <= 8) {
        emitOp(compiler, (Opcode) (OP_LOAD_LOCAL_0 + slot));
        return;
    }
    emitByteArg(compiler, OP_LOAD_LOCAL, slot);
}


typedef void (*GrammarFn)(Compiler *, bool canAssign);

typedef void (*SignatureFn)(Compiler *compiler, Signature *signature);

typedef struct {
    GrammarFn prefix;
    GrammarFn infix;
    SignatureFn method;
    Precedence precedence;
    const char *name;
} GrammarRule;

// Forward declarations since the grammar is recursive.
static GrammarRule *getRule(TokenType type);


static void null(Compiler *compiler, bool canAssign) {
    emitOp(compiler, OP_NULL);
}


static void initVariable(Compiler *compiler, Variable *variable) {
// null(this, false);
    if (compiler->scopeDepth != -1) {
        // local variable
        // emit null as slot value
        null(compiler, false);
        return;
    }

}

static void declarePattern(Compiler *compiler, Pattern *pattern) {
    switch (pattern->type) {
        case NONE_PATTERN: {
            if (pattern->alias != NULL) {
                declarePattern(compiler, pattern->alias);
            } else {
                int symbol = declareVariable(compiler, &pattern->as.id);
                newVariable(&pattern->variable, symbol, compiler->scopeDepth == -1 ? SCOPE_MODULE : SCOPE_LOCAL);
                initVariable(compiler, &pattern->variable);
            }
            return;
        }
        case REST_PATTERN: {
            int symbol = declareVariable(compiler, &pattern->as.id);
            newVariable(&pattern->variable, symbol, compiler->scopeDepth == -1 ? SCOPE_MODULE : SCOPE_LOCAL);
            initVariable(compiler, &pattern->variable);
            return;
        }
        case OBJECT_PATTERN:
            for (int i = 0; i < pattern->as.object.count; i++) {
                declarePattern(compiler, &pattern->as.object.data[i]);
            }
            return;
        case ARRAY_PATTERN:
            for (int i = 0; i < pattern->as.array.count; i++) {
                declarePattern(compiler, &pattern->as.array.data[i]);
            }
            return;
    }
}

static Pattern parsePattern(Compiler *compiler, PatternType parent, bool declare) {
    Pattern ret;
    ret.alias = NULL;
    ret.parent = parent;
    if (match(compiler, ID_TOKEN)) {
        ret.type = NONE_PATTERN;
        initToken(&ret.as.id, compiler->parser->previous.type, compiler->parser->previous.start,
                  compiler->parser->previous.length,
                  compiler->parser->previous.line, compiler->parser->previous.value);
        if (parent == OBJECT_PATTERN) {
            // can give alias
            if (match(compiler, COLON_TOKEN)) {
                // parse the alias
                Pattern alias = parsePattern(compiler, NONE_PATTERN, declare);
                ret.alias = ALLOCATE(compiler->parser->vm, Pattern);
                memcpy(ret.alias, &alias, sizeof(Pattern));
                // int symbol = declareVariable(compiler,&alias.as.id);
                // ret.variable = Variable(symbol, compiler->scopeDepth == -1 ? SCOPE_MODULE : SCOPE_LOCAL);
            } else if (declare) {
                int symbol = declareVariable(compiler, &ret.as.id);
                newVariable(&ret.variable, symbol, compiler->scopeDepth == -1 ? SCOPE_MODULE : SCOPE_LOCAL);
                initVariable(compiler, &ret.variable);
            }

        } else if (declare) {
            int symbol = declareVariable(compiler, &ret.as.id);

            newVariable(&ret.variable, symbol, compiler->scopeDepth == -1 ? SCOPE_MODULE : SCOPE_LOCAL);
            initVariable(compiler, &ret.variable);
        }
    } else if (match(compiler, SPREAD_OR_REST_TOKEN)) {
        consume(compiler, ID_TOKEN, "Expected name after rest pattern");
        ret.type = REST_PATTERN;
        initToken(&ret.as.id, compiler->parser->previous.type, compiler->parser->previous.start,
                  compiler->parser->previous.length,
                  compiler->parser->previous.line, compiler->parser->previous.value);
        if (declare) {
            int symbol = declareVariable(compiler, &ret.as.id);
            newVariable(&ret.variable, symbol, compiler->scopeDepth == -1 ? SCOPE_MODULE : SCOPE_LOCAL);
            initVariable(compiler, &ret.variable);
        }

    } else if (match(compiler, LBRACE_TOKEN)) {
        ret.type = OBJECT_PATTERN;
        MSCInitPatternBuffer(&ret.as.object);
        do {
            if (peek(compiler) == RBRACE_TOKEN) break;
            Pattern pattern = parsePattern(compiler, ret.type, declare);
            MSCWritePatternBuffer(compiler->parser->vm, &ret.as.object, pattern);
            if (pattern.type == REST_PATTERN) break;
        } while (match(compiler, COMMA_TOKEN));
        consume(compiler, RBRACE_TOKEN, "Expected '}' after object pattern");
    } else if (match(compiler, LBRACKET_TOKEN)) {
        ret.type = ARRAY_PATTERN;
        MSCInitPatternBuffer(&ret.as.array);
        do {
            if (peek(compiler) == RBRACKET_TOKEN) break;
            Pattern pattern = parsePattern(compiler, ret.type, declare);
            MSCWritePatternBuffer(compiler->parser->vm, &ret.as.array, pattern);
            if (pattern.type == REST_PATTERN) break;
        } while (match(compiler, COMMA_TOKEN));
        consume(compiler, RBRACKET_TOKEN, "Expected ']' after array pattern");
    }
    return ret;
}


static inline Pattern patternWithDeclaration(Compiler *compiler, PatternType parent) {
    return parsePattern(compiler, parent, true);
}

// Pushes the value for a module-level variable implicitly imported from core.
void loadCoreVariable(Compiler *compiler, const char *name) {
    int symbol = MSCSymbolTableFind(&compiler->parser->module->variableNames,
                                    name, strlen(name));

    ASSERT(symbol != -1, "Should have already defined core name.");
    emitShortArg(compiler, OP_LOAD_MODULE_VAR, symbol);
}

// Compiles a method call with [numArgs] for a method with [name] with [length].
void callMethod(Compiler *compiler, int numArgs, const char *name,
                int length) {
    int symbol = methodSymbol(compiler, name, length);
    emitShortArg(compiler, (Opcode) (OP_CALL_0 + numArgs), symbol);
}

void assignVariable(Compiler *compiler, Variable *variable) {
    switch (variable->scope) {
        case SCOPE_LOCAL:
            emitByteArg(compiler, OP_STORE_LOCAL, variable->index);
            break;
        case SCOPE_UPVALUE:
            emitByteArg(compiler, OP_STORE_UPVALUE, variable->index);
            break;
        case SCOPE_MODULE:
            emitShortArg(compiler, OP_STORE_MODULE_VAR, variable->index);
            break;
        default:
            UNREACHABLE();
    }
    emitByte(compiler, OP_POP);
}

void handleDestructuration(Compiler *compiler, Pattern *pattern) {
    switch (pattern->type) {
        case NONE_PATTERN: {
            if (pattern->parent == OBJECT_PATTERN && pattern->alias != NULL) {
                // load map property at source token
                switch (pattern->alias->type) {
                    case OBJECT_PATTERN:
                        handleDestructuration(compiler, pattern->alias);
                        break;
                    case ARRAY_PATTERN:
                        handleDestructuration(compiler, pattern->alias);
                        break;
                    default:
                        assignVariable(compiler, &pattern->alias->variable);
                        break;
                }
                DEALLOCATE(compiler->parser->vm, pattern->alias);
                return;
            }
            assignVariable(compiler, &pattern->variable);
            break;
        }

        case OBJECT_PATTERN: {
            pushScope(compiler);
            int tmpSlot = addLocal(compiler, "tmp_ ", 5);// store parrent in local variable
            emitByteArg(compiler, OP_STORE_LOCAL, tmpSlot);
            null(compiler, false);
            int localMap = addLocal(compiler, "map_m ", 6);
            PatternBuffer elements = pattern->as.object;
            ValueBuffer took;
            MSCInitValueBuffer(&took);
            for (int i = 0; i < elements.count; i++) {
                // load source
                Pattern item = elements.data[i];
                Token *sourceToken = &item.as.id;
                Value str = MSCStringFromCharsWithLength(compiler->parser->vm, sourceToken->start, sourceToken->length);



                // emitByteArg(compiler,OP_STORE_LOCAL, tmp);
                if (item.type == REST_PATTERN) {

                    loadCoreVariable(compiler, "Wala");
                    callMethod(compiler, 0, "kura()", 6);
                    emitByteArg(compiler, OP_STORE_LOCAL, localMap);
                    loadLocal(compiler, tmpSlot);// load initial map
                    callMethod(compiler, 1, "aBeeFaraAkan_(_)", 16);
                    // emitByte(compiler,OP_POP);

                    for (int j = 0; j < took.count; j++) {
                        // remove item from localmap
                        loadLocal(compiler, localMap);
                        emitConstant(compiler, took.data[j]);// load key to remove
                        callMethod(compiler, 1, "aBoye(_)", 8);
                        emitByte(compiler, OP_POP);
                    }
                    assignVariable(compiler, &item.variable);

                    break;
                }
                MSCWriteValueBuffer(compiler->parser->vm, &took, str);
                loadLocal(compiler, tmpSlot);
                emitConstant(compiler, str);
                callMethod(compiler, 1, "[_]", 3);
                handleDestructuration(compiler, &item);
            }
            MSCFreePatternBuffer(compiler->parser->vm, &elements);
            MSCFreeValueBuffer(compiler->parser->vm, &took);
            popScope(compiler);
            break;
        }
        case ARRAY_PATTERN: {
            pushScope(compiler);
            int tmpSlot = addLocal(compiler, "tmp_ ", 5);// store parrent in local variable
            emitByteArg(compiler, OP_STORE_LOCAL, tmpSlot);
            PatternBuffer elements = pattern->as.array;
            for (int i = 0; i < elements.count; i++) {
                Pattern item = elements.data[i];
                if (item.type == REST_PATTERN) {
                    loadCoreVariable(compiler, "Walan");
                    callMethod(compiler, 0, "kura()", 6);
                    // int tmpArr = addLocal(compiler,"tmpArr ", 7);
                    loadLocal(compiler, tmpSlot);// load initial array
                    emitConstant(compiler, NUM_VAL(i));// start index
                    loadLocal(compiler, tmpSlot);// load initial array
                    callMethod(compiler, 0, "hakan", 5);// get array size
                    callMethod(compiler, 1, "...(_)", 6);// create range from i to length
                    callMethod(compiler, 1, "[_]", 3);// sublist
                    callMethod(compiler, 1, "aBeeFaraAkan_(_)", 16);
                    assignVariable(compiler, &item.variable);
                    break;
                }
                loadLocal(compiler, tmpSlot);
                Value index = NUM_VAL(i);
                emitConstant(compiler, index);
                callMethod(compiler, 1, "[_]", 3);
                handleDestructuration(compiler, &item);
            }
            MSCFreePatternBuffer(compiler->parser->vm, &elements);
            popScope(compiler);
            break;
        }

    }

}

void objectDestructurationDeclaration(Compiler *compiler) {
    Pattern objectPattern = patternWithDeclaration(compiler, OBJECT_PATTERN);
    consume(compiler, ASSIGN_TOKEN, "Expected '=' after object destructuration pattern");
    ignoreNewlines(compiler);

    expression(compiler);
    // PatternBuffer elements = objectPattern.as.object;
    handleDestructuration(compiler, &objectPattern);
    // emitOp(OP_POP);
    // popScope(compiler);
}

void arrayDestructurationDeclaration(Compiler *compiler) {
    Pattern pattern = patternWithDeclaration(compiler, ARRAY_PATTERN);
    consume(compiler, ASSIGN_TOKEN, "Expected '=' after array destructuration pattern");
    ignoreNewlines(compiler);
    // pushScope(compiler);
    expression(compiler);
    // PatternBuffer elements = objectPattern.as.object;
    handleDestructuration(compiler, &pattern);
    // emitOp(OP_POP);
    // popScope(compiler);
}

// Compiles a "var" variable definition statement.
void variableDefinition(Compiler *compiler) {
    // Grab its name, but don't declare it yet. A (local) variable shouldn't be
    // in scope in its own initializer.
    if (peek(compiler) == LBRACE_TOKEN) {
        // destructuration
        objectDestructurationDeclaration(compiler);
        return;
    }
    if (peek(compiler) == LBRACKET_TOKEN) {
        arrayDestructurationDeclaration(compiler);
        return;
    }
    consume(compiler, ID_TOKEN, "Expect variable name.");
    Token nameToken = compiler->parser->previous;

    // Compile the initializer.
    if (match(compiler, ASSIGN_TOKEN)) {
        ignoreNewlines(compiler);
        expression(compiler);
    } else {
        // Default initialize it to null.
        null(compiler, false);
    }
    // Now put it in scope.
    int symbol = declareVariable(compiler, &nameToken);
    defineVariable(compiler, symbol);
}

// Returns a signature with [type] whose name is from the last consumed token.
static Signature signatureFromToken(Compiler *compiler, SignatureType type) {
    Signature signature;

    // Get the token for the method name.
    Token *token = &compiler->parser->previous;
    signature.name = token->start;
    signature.length = token->length;
    signature.type = type;
    signature.arity = 0;

    if (signature.length > MAX_METHOD_NAME) {
        error(compiler, "Method names cannot be longer than %d characters.",
              MAX_METHOD_NAME);
        signature.length = MAX_METHOD_NAME;
    }

    return signature;
}


static bool finishBlock(Compiler *compiler, bool expr) {
    // Empty blocks do nothing.
    if (match(compiler, RBRACE_TOKEN)) return false;

    // If there's no line after the "{", it's a single-expression body.
    if (!matchLine(compiler)) {
        expression(compiler);
        consume(compiler, RBRACE_TOKEN, "Expect '}' at end of block.");
        return true;
    }

    // Empty blocks (with just a newline inside) do nothing.
    if (match(compiler, RBRACE_TOKEN)) return false;
    bool isExpr;
    // Compile the definition list.
    do {
        isExpr = definition(compiler, expr);
        match(compiler, SEMI_TOKEN);
        matchLine(compiler);
        if (compiler->parser->previous.type != EOL_TOKEN && compiler->parser->previous.type != SEMI_TOKEN)
            error(compiler, "Expect newline or ; after statement.");
        if (expr && isExpr && peek(compiler) != RBRACE_TOKEN) {
            // pop
            emitOp(compiler, OP_POP);
        }
    } while (peek(compiler) != RBRACE_TOKEN && peek(compiler) != EOF_TOKEN);

    consume(compiler, RBRACE_TOKEN, "Expect '}' at end of block.");
    return expr && isExpr;
}

// Parses a method or function body, after the initial "{" has been consumed.
//
// If [Compiler->isInitializer] is `true`, this is the body of a constructor
// initializer. In that case, this adds the code to ensure it returns `this`.
static void finishBody(Compiler *compiler) {
    bool isExpressionBody = finishBlock(compiler, false);

    if (compiler->isInitializer) {
        // If the initializer body evaluates to a value, discard it.
        if (isExpressionBody) emitOp(compiler, OP_POP);

        // The receiver is always stored in the first local slot.
        emitOp(compiler, OP_LOAD_LOCAL_0);
    } else if (!isExpressionBody) {
        // Implicitly return null in statement bodies.
        emitOp(compiler, OP_NULL);
    }

    emitOp(compiler, OP_RETURN);
}

// Gets the symbol for a method with [signature].
static int signatureSymbol(Compiler *compiler, Signature *signature) {
    // Build the full name from the signature.

    char name[MAX_METHOD_SIGNATURE];
    int length;
    signatureToString(signature, name, &length);

    return methodSymbol(compiler, name, length);
}

static void functionDefinition(Compiler *compiler, bool isStatic) {
    // consume(compiler,ID_TOKEN, "Expect variable name.");
    SignatureFn signatureFn = getRule(compiler->parser->current.type)->method;
    nextToken(compiler->parser);
    const char *className = NULL;
    int classNameLength = 0;
    if (peek(compiler) == DOT_TOKEN) {
        // extension function
        className = compiler->parser->previous.start;
        classNameLength = compiler->parser->previous.length;
        nextToken(compiler->parser);// consume '.'
        signatureFn = getRule(compiler->parser->current.type)->method;
        nextToken(compiler->parser);
        if (signatureFn == NULL) {
            error(compiler, "Expect method definition.");
        }
    } else {
        if (isStatic) {
            error(compiler, "Unexpected token 'dialen' before function definition.");
            return;
        }
        if (signatureFn == NULL) {
            error(compiler, "Expect function definition.");
            return;
        }
    }


    // Build the method signature.
    Signature signature = signatureFromToken(compiler, SIG_FUNCTION);
    int nameLength = signature.length;
    Compiler functionCompiler;
    initCompiler(&functionCompiler, compiler->parser, compiler, className != NULL);
    if (className != NULL) {
        functionCompiler.isExtension = true;
    }
    // Compile the method signature.
    signatureFn(&functionCompiler, &signature);


    if (signature.type == SIG_INITIALIZER) {
        error(compiler, "A constructor cannot be outside a 'kulu' definition");
    }

    // Include the full signature in debug messages in stack traces.
    char fullSignature[MAX_METHOD_SIGNATURE];
    int length;
    signatureToString(&signature, fullSignature, &length);


    int functionSymbol = -1;
    if (className != NULL) {
        functionSymbol = signatureSymbol(compiler, &signature);
    } else {
        functionSymbol = declareFunction(compiler, signature.name, nameLength);
    }
    // declareMethod(compiler,&signature, fullSignature, length);

    consume(compiler, LBRACE_TOKEN, "Expect '{' to begin function body.");
    finishBody(&functionCompiler);
    endCompiler(&functionCompiler, fullSignature, length);


    if (className != NULL) {
        Variable classVariable = resolveName(compiler, className, classNameLength);
        if (classVariable.index == -1) {
            error(compiler, "Cant extend unknown 'kulu' %.*s", classNameLength, className);
        }
        defineMethod(compiler, &classVariable, isStatic, functionSymbol);
    } else {
        // Define the method. For a constructor, this defines the instance
        // initializer method.
        defineVariable(compiler, functionSymbol);
    }

    // delete compiler->enclosingClass->signature;
}


// Replaces the placeholder argument for a previous CODE_JUMP or CODE_JUMP_IF
// instruction with an offset that jumps to the current end of bytecode.
static void patchJump(Compiler *compiler, int offset) {
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = compiler->function->code.count - offset - 2;
    if (jump > MAX_JUMP) error(compiler, "Too much code to jump over.");
    compiler->function->code.data[offset] = (uint8_t) ((jump >> 8) & 0xff);
    compiler->function->code.data[offset + 1] = (uint8_t) (jump & 0xff);
}

// Marks the beginning of a loop. Keeps track of the current instruction so we
// know what to loop back to at the end of the body.
static void startLoop(Compiler *compiler, Loop *loop) {
    loop->enclosing = compiler->loop;
    loop->start = compiler->function->code.count - 1;
    loop->scopeDepth = compiler->scopeDepth;
    compiler->loop = loop;
}

// Emits the [CODE_JUMP_IF] instruction used to test the loop condition and
// potentially exit the loop. Keeps track of the instruction so we can patch it
// later once we know where the end of the body is.
static inline void testExitLoop(Compiler *compiler) {
    compiler->loop->exitJump = emitJump(compiler, OP_JUMP_IF);
}

// Compiles the body of the loop and tracks its extent so that contained "break"
// statements can be handled correctly.
static void loopBody(Compiler *compiler) {
    compiler->loop->body = compiler->function->code.count;
    statement(compiler, false);
}

// Ends the current innermost loop. Patches up all jumps and breaks now that
// we know where the end of the loop is.
static void endLoop(Compiler *compiler) {
    // We don't check for overflow here since the forward jump over the loop body
    // will report an error for the same problem.
    int loopOffset = compiler->function->code.count - compiler->loop->start + 2;
    emitShortArg(compiler, OP_LOOP, loopOffset);

    patchJump(compiler, compiler->loop->exitJump);

    // Find any break placeholder instructions (which will be OP_END in the
    // bytecode) and replace them with real jumps.
    int i = compiler->loop->body;
    while (i < compiler->function->code.count) {
        if (compiler->function->code.data[i] == OP_END) {
            compiler->function->code.data[i] = OP_JUMP;
            patchJump(compiler, i + 1);
            i += 3;
        } else {
            // Skip this instruction and its arguments.
            i += 1 + getByteCountForArguments(compiler->function->code.data,
                                              compiler->function->constants.data, i);
        }
    }

    compiler->loop = compiler->loop->enclosing;
}


/**
 *
 */
static void forStatement(Compiler *compiler) {
    // A for statement like:
    //
    //     seginka(sequence.expression kono i [kay|kaj][niin step ye]) {
    //       A.yira(i)
    //     }
    //
    // Is compiled to bytecode almost as if the source looked like this:
    //
    //     {
    //       nin seq_ = sequence.expression
    //       nin step_ = step || 1;
    //       # nin dir_ = 1 || -1;
    //       nin iter_
    //       while (iter_ = seq_.iterate(iter_, step_)) {
    //         nin i = seq_.iteratorValue(iter_)
    //         A.yira(i)
    //       }
    //     }
    //
    // It's not exactly this, because the synthetic variables `seq_` and `iter_`
    //
    // The important parts are:
    // - The sequence expression is only evaluated once.
    // - The .iterate() method is used to advance the iterator and determine if
    //   it should exit the loop.
    // - The .iteratorValue() method is used to get the value at the current
    //   iterator position.

    // Create a scope for the hidden local variables used for the iterator.
    pushScope(compiler);
    bool withParen = match(compiler, LPAREN_TOKEN);
    // consume(compiler,LPAREN_TOKEN, "Expect '(' after 'for'.");

    // Evaluate the sequence expression and store it in a hidden local variable.
    // The space in the variable name ensures it won't collide with a user-defined
    // variable.
    expression(compiler);

    consume(compiler, IN_TOKEN, "Expect 'in' after loop variable.");
    if (withParen) ignoreNewlines(compiler);

    Pattern *loopPattern = NULL;
    // Remember the name of the loop variable.
    const char *name = NULL;
    int length = -1;
    TokenType next = peek(compiler);
    if (next == LBRACE_TOKEN || next == LBRACKET_TOKEN) {
        Pattern tmp = parsePattern(compiler, next == LBRACE_TOKEN ? OBJECT_PATTERN : ARRAY_PATTERN, false);
        loopPattern = ALLOCATE(compiler->parser->vm, Pattern);
        memcpy(loopPattern, &tmp, sizeof(Pattern));
    } else {
        consume(compiler, ID_TOKEN, "Expect for loop variable name.");
        name = compiler->parser->previous.start;
        length = compiler->parser->previous.length;
    }


    // Verify that there is space to hidden local variables.
    // Note that we expect only two addLocal calls next to each other in the
    // following code.

    if (compiler->numLocals + 3 > MAX_LOCALS) {
        error(compiler,
              "Cannot declare more than %d variables in one scope. (Not enough space for for-loops internal variables)",
              MAX_LOCALS);
        return;
    }


    int seqSlot = addLocal(compiler, "seq ", 4);

    // Create another hidden local for the iterator object.
    null(compiler, false);
    int iterSlot = addLocal(compiler, "iter ", 5);

    // parse the step and direction
    bool up;
    if (peek(compiler) == UP_TOKEN || peek(compiler) == DOWN_TOKEN) {
        up = match(compiler, UP_TOKEN) || !match(compiler, DOWN_TOKEN);
        match(compiler, WITH_TOKEN) ? expression(compiler) : emitConstant(compiler, NUM_VAL(1));
        if (!up) callMethod(compiler, 0, "-", 1);
    } else {
        emitConstant(compiler, NUM_VAL(1));
    }

    int stepSlot = addLocal(compiler, "step ", 5);

    if (withParen) consume(compiler, RPAREN_TOKEN, "Expect ')' after loop expression.");

    Loop loop;
    startLoop(compiler, &loop);

    // Advance the iterator by calling the ".iterate" method on the sequence.
    loadLocal(compiler, seqSlot);
    loadLocal(compiler, iterSlot);
    loadLocal(compiler, stepSlot);
    // Update and test the iterator.
    callMethod(compiler, 2, "iterate(_,_)", 12);
    emitByteArg(compiler, OP_STORE_LOCAL, iterSlot);
    testExitLoop(compiler);

    // Get the current value in the sequence by calling ".iteratorValue".
    loadLocal(compiler, seqSlot);
    loadLocal(compiler, iterSlot);
    callMethod(compiler, 1, "iteratorValue(_)", 16);

    // Bind the loop variable in its own scope. This ensures we get a fresh
    // variable each iteration so that closures for it don't all see the same one.
    pushScope(compiler);
    if (loopPattern == NULL) {
        addLocal(compiler, name, length);
    } else {
        int tmpSlot = addLocal(compiler, "tmpLd ", 6);// save te
        declarePattern(compiler, loopPattern);
        loadLocal(compiler, tmpSlot);
        handleDestructuration(compiler, loopPattern);
        DEALLOCATE(compiler->parser->vm, loopPattern);
    }

    loopBody(compiler);

    // Loop variable.
    popScope(compiler);

    endLoop(compiler);

    // Hidden variables.
    popScope(compiler);
}

static void ifStatement(Compiler *compiler, bool expr) {
    // Compile the condition.
    bool withParen = match(compiler, LPAREN_TOKEN);
    if (withParen) ignoreNewlines(compiler);
    expression(compiler);
    if (withParen) ignoreNewlines(compiler);
    if (withParen) consume(compiler, RPAREN_TOKEN, "Expect ')' after if condition.");

    // Jump to the else branch if the condition is false.
    int ifJump = emitJump(compiler, OP_JUMP_IF);

    // Compile the then branch.
    statement(compiler, expr);
    ignoreNewlines(compiler);
    // Compile the else branch if there is one.
    if (match(compiler, ELSE_TOKEN)) {
        // Jump over the else branch when the if branch is taken.
        int elseJump = emitJump(compiler, OP_JUMP);
        patchJump(compiler, ifJump);

        statement(compiler, expr);

        // Patch the jump over the else.
        patchJump(compiler, elseJump);
    } else {
        patchJump(compiler, ifJump);
    }
}


static void whileStatement(Compiler *compiler) {
    Loop loop;
    startLoop(compiler, &loop);

    // Compile the condition.
    bool withParen = match(compiler, LPAREN_TOKEN);
    // consume(compiler,LPAREN_TOKEN, "Expect '(' after 'while'.");
    expression(compiler);
    if (withParen) consume(compiler, RPAREN_TOKEN, "Expect ')' after while condition.");
    // callMethod(compiler,0, "!", 1);

    testExitLoop(compiler);
    loopBody(compiler);
    endLoop(compiler);
}


static bool statement(Compiler *compiler, bool expr) {
    if (match(compiler, BREAK_TOKEN)) {
        if (compiler->loop == NULL) {
            error(compiler, "Cannot use 'break' outside of a loop.");
            return false;
        }

        // Since we will be jumping out of the scope, make sure any locals in it
        // are discarded first.
        discardLocals(compiler, compiler->loop->scopeDepth + 1);

        // Emit a placeholder instruction for the jump to the end of the body. When
        // we're done compiling the loop body and know where the end is, we'll
        // replace these with `CODE_JUMP` instructions with appropriate offsets.
        // We use `CODE_END` here because that can't occur in the middle of
        // bytecode.
        emitJump(compiler, OP_END);
    } else if (match(compiler, CONTINUE_TOKEN)) {
        if (compiler->loop == NULL) {
            error(compiler, "Cannot use 'continue' outside of a loop.");
            return false;
        }

        // Since we will be jumping out of the scope, make sure any locals in it
        // are discarded first.
        discardLocals(compiler, compiler->loop->scopeDepth + 1);

        // emit a jump back to the top of the loop
        int loopOffset = compiler->function->code.count - compiler->loop->start + 2;
        emitShortArg(compiler, OP_LOOP, loopOffset);
    } else if (match(compiler, FOR_LOOP_TOKEN)) {
        forStatement(compiler);
    } else if (match(compiler, IF_TOKEN)) {
        ifStatement(compiler, expr);
        return expr;
    } else if (match(compiler, RETURN_TOKEN)) {
        // Compile the return value.
        TokenType tokenType = peek(compiler);
        if (tokenType == EOL_TOKEN || tokenType == SEMI_TOKEN) {
            // If there's no expression after return, initializers should
            // return 'this' and regular methods should return null
            // If there's no expression after return, initializers should
            // return 'this' and regular methods should return null
            Opcode result = compiler->isInitializer ? OP_LOAD_LOCAL_0 : OP_NULL;
            emitOp(compiler, result);
        } else {
            if (!match(compiler, WITH_TOKEN)) {
                error(compiler, "Expected 'niin' keyword after 'segin'");
            }
            if (compiler->isInitializer) {
                error(compiler, "A constructor cannot return a value.");
            }
            expression(compiler);
        }
        emitOp(compiler, OP_RETURN);
    } else if (match(compiler, WHILE_TILL_TOKEN)) {
        whileStatement(compiler);
    } else if (match(compiler, LBRACE_TOKEN)) {
        // Block statement.
        int tmpSlot = -1;
        if (expr) {
            pushScope(compiler);
            null(compiler, false);
            tmpSlot = addLocal(compiler, "tmpBlck ", 8);
        }
        pushScope(compiler);
        bool isExpr = finishBlock(compiler, expr);
        if (tmpSlot != -1 && isExpr) {
            // store block result before pop scope
            emitByteArg(compiler, OP_STORE_LOCAL, tmpSlot);
            // Block was an expression, so discard it.
            emitOp(compiler, OP_POP);
        }
        popScope(compiler);
        if (expr) {
            softPopScope(compiler);
        }
        return isExpr;
    } else {

        // Expression statement.
        expression(compiler);
        if (!expr) emitOp(compiler, OP_POP);
        return true;
    }
    return false;
}


// Walks the compiler chain to find the compiler for the nearest class
// enclosing this one. Returns NULL if not currently inside a class definition.
static Compiler *getEnclosingClassCompiler(Compiler *compiler) {
    while (compiler != NULL) {
        if (compiler->enclosingClass != NULL) return compiler;
        compiler = compiler->parent;
    }

    return NULL;
}

// Emits the code to load [variable] onto the stack.
static void loadVariable(Compiler *compiler, Variable *variable) {
    switch (variable->scope) {
        case
            SCOPE_LOCAL:
            loadLocal(compiler, variable->index);
            break;
        case
            SCOPE_UPVALUE:
            emitByteArg(compiler, OP_LOAD_UPVALUE, variable->index);
            break;
        case
            SCOPE_MODULE:
            emitShortArg(compiler, OP_LOAD_MODULE_VAR, variable->index);
            break;
        default:
            UNREACHABLE();
    }
}

// Compiles a read or assignment to [variable].
void bareName(Compiler *compiler, bool canAssign, Variable variable) {
    // If there's an "=" after a bare name, it's a variable assignment.
    if (canAssign && match(compiler, ASSIGN_TOKEN)) {
        // Compile the right-hand side.
        expression(compiler);
        // Emit the store instruction.
        switch (variable.scope) {
            case SCOPE_LOCAL:
                emitByteArg(compiler, OP_STORE_LOCAL, variable.index);
                break;
            case SCOPE_UPVALUE:
                emitByteArg(compiler, OP_STORE_UPVALUE, variable.index);
                break;
            case SCOPE_MODULE:
                emitShortArg(compiler, OP_STORE_MODULE_VAR, variable.index);
                break;
            default:
                UNREACHABLE();
        }
        return;
    }

    // Emit the load instruction.
    loadVariable(compiler, &variable);

    allowLineBeforeDot(compiler);
}

// Compiles an "import" statement.
//
// An import compiles to a series of instructions. Given:
//
//     import "foo" for Bar, Baz
//
// We compile a single IMPORT_MODULE "foo" instruction to load the module
// itself. When that finishes executing the imported module, it leaves the
// ObjModule in vm->lastModule. Then, for Bar and Baz, we:
//
// * Declare a variable in the current scope with that name.
// * Emit an IMPORT_VARIABLE instruction to load the variable's value from the
//   other module.
// * Compile the code to store that value in the variable in this scope.
static void import(Compiler *compiler, bool whole) {
    ignoreNewlines(compiler);
    consume(compiler, STRING_CONST_TOKEN, "Expect a string after 'import'.");
    int moduleConstant = addConstant(compiler, compiler->parser->previous.value);

    // Load the module.
    emitShortArg(compiler, OP_IMPORT_MODULE, moduleConstant);

    // Discard the unused result value from calling the module body's closure.
    emitOp(compiler, OP_POP);
    if (whole) return;
    consume(compiler, IMPORT_TOKEN, "Expected 'nani' token in 'kabo' statement");

    // Compile the comma-separated list of variables to import.
    do {
        ignoreNewlines(compiler);

        consume(compiler, ID_TOKEN, "Expect variable name.");

        // We need to hold onto the source variable,
        // in order to reference it in the import later
        Token sourceVariableToken = compiler->parser->previous;

        // Define a string constant for the original variable name.
        int sourceVariableConstant = addConstant(compiler,
                                                 MSCStringFromCharsWithLength(compiler->parser->vm,
                                                                              sourceVariableToken.start,
                                                                              sourceVariableToken.length));

        // Store the symbol we care about for the variable
        int slot = -1;
        if (match(compiler, AS_TOKEN)) {
            //import "module" for Source as Dest
            //Use 'Dest' as the name by declaring a new variable for it.
            //This parses a name after the 'as' and defines it.
            slot = declareNamedVariable(compiler);
        } else {
            //import "module" for Source
            //Uses 'Source' as the name directly
            slot = declareVariable(compiler, &sourceVariableToken);
        }

        // Load the variable from the other module.
        emitShortArg(compiler, OP_IMPORT_VARIABLE, sourceVariableConstant);

        // Store the result in the variable here.
        defineVariable(compiler, slot);
    } while (match(compiler, COMMA_TOKEN));

}




// Parses a block body, after the initial "{" has been consumed.
//
// Returns true if it was a expression body, false if it was a statement body.
// (More precisely, returns true if a value was left on the stack. An empty
// block returns false.)

// The VM can only handle a certain number of parameters, so check that we
// haven't exceeded that and give a usable error.
static void validateNumParameters(Compiler *compiler, int numArgs) {
    if (numArgs == MAX_PARAMETERS + 1) {
        // Only show an error at exactly max + 1 so that we can keep parsing the
        // parameters and minimize cascaded errors.
        error(compiler, "Methods cannot have more than %d parameters.",
              MAX_PARAMETERS);
    }
}

// Parses the rest of a comma-separated parameter list after the opening
// delimeter. Updates `arity` in [signature] with the number of parameters.
static void finishParameterList(Compiler *compiler, Signature *signature) {
    do {
        ignoreNewlines(compiler);
        validateNumParameters(compiler, ++signature->arity);
        TokenType next = peek(compiler);
        // Define a local variable in the method for the parameter.
        if (next == LBRACE_TOKEN || next == LBRACKET_TOKEN) {
            // destructuration pattern
            // creat temporary loacl variable
            // null(this, false);
            // addLocal(compiler,"localTemp ", 10);
            Pattern pattern = patternWithDeclaration(compiler, next == LBRACE_TOKEN ? OBJECT_PATTERN : ARRAY_PATTERN);
            loadLocal(compiler, signature->arity);
            handleDestructuration(compiler, &pattern);
            emitOp(compiler, OP_POP);
        } else {
            declareNamedVariable(compiler);
        }
    } while (match(compiler, COMMA_TOKEN));
}

// Parses a comma-separated list of arguments. Modifies [signature] to include
// the arity of the argument list.
static void finishArgumentList(Compiler *compiler, Signature *signature) {
    do {
        ignoreNewlines(compiler);
        validateNumParameters(compiler, ++signature->arity);
        expression(compiler);
    } while (match(compiler, COMMA_TOKEN));
    // Allow a newline before the closing delimiter.
    ignoreNewlines(compiler);
}


// Walks the compiler chain to find the nearest class enclosing this one.
// Returns NULL if not currently inside a class definition.
static ClassInfo *getEnclosingClass(Compiler *compiler) {
    compiler = getEnclosingClassCompiler(compiler);
    return compiler == NULL ? NULL : compiler->enclosingClass;
}

static bool isPrivate(const char *name, int length) {
    return length > 1 && name[0] == '_';
}

static bool staticField(Compiler *compiler, bool canAssign, bool declaring, Variable *classVariable) {
    Compiler *classCompiler = getEnclosingClassCompiler(compiler);
    if (classCompiler == NULL) {
        error(compiler, "Cannot use a static field outside of a class definition.");
        return false;
    }

    // Look up the name in the scope chain.
    Token *token = &compiler->parser->previous;

    // If this is the first time we've seen this static field, implicitly
    // define it as a variable in the scope surrounding the class definition.
    if (resolveLocal(compiler, token->start, token->length) == -1) {
        int symbol = declareVariable(compiler, NULL);

        // Implicitly initialize it to null.
        emitOp(compiler, OP_NULL);
        defineVariable(compiler, symbol);
    }
    // It definitely exists now, so resolve it properly. This is different from
    // the above resolveLocal() call because we may have already closed over it
    // as an upvalue.
    Variable variable = resolveName(compiler, token->start, token->length);
    bareName(compiler, true, variable);
    // if not private static field, expose a get and setter for the field
    if (!isPrivate(token->start, token->length)) {
        emitSetter(compiler, classVariable, variable.index, token->start, token->length, true);
        emitGetter(compiler, classVariable, variable.index, token->start, token->length, true);
    }
    return true;
}

static bool field(Compiler *compiler, bool canAssign, bool declaring, Variable *classVariable) {
    // Initialize it with a fake value so we can keep parsing and minimize the
    // number of cascaded errors.
    int field;

    ClassInfo *enclosingClass = getEnclosingClass(compiler);
    const char *name = NULL;
    int length = 0;
    if (enclosingClass == NULL) {
        error(compiler, "Cannot reference a field outside of a class definition.");
        return false;
    } else if (enclosingClass->isExtern) {
        error(compiler, "Cannot define fields in a extern class.");
        return false;
    } else {
        name = compiler->parser->previous.start;
        length = compiler->parser->previous.length;
        // Look up the field, or implicitly define it.
        field = MSCSymbolTableEnsure(compiler->parser->vm, &enclosingClass->fields,
                                     name,
                                     (size_t) length);

        if (field >= MAX_FIELDS) {
            error(compiler, "A class can only have %d fields.", MAX_FIELDS);
            return false;
        }
    }
    bool isStore = false;
    if (declaring) {

        // If there's an "=" after a field name, it's an assignment.
        if (match(compiler, ASSIGN_TOKEN)) {
            // Compile the right-hand side.
            expression(compiler);
        } else {
            // Default initialize it to null.
            emitOp(compiler, OP_NULL);
        }
    } else if (canAssign && match(compiler, ASSIGN_TOKEN)) {
        isStore = true;
        expression(compiler);
    }
    if (declaring) {
        loadVariable(compiler, classVariable);
        emitByteArg(compiler, OP_FIELD, field);
        // emit getter and setter by default
        if (!isPrivate(name, length)) {
            emitSetter(compiler, classVariable, field, name, length, false);
            emitGetter(compiler, classVariable, field, name, length, false);
        }
    } else {
        // If we're directly inside a method, use a more optimal instruction.
        /*if (compiler->parent != NULL &&
            compiler->parent->enclosingClass == enclosingClass) {
            emitByteArg(compiler,isStore ? OP_STORE_FIELD_THIS : OP_LOAD_FIELD_THIS,
                              field);
        } else {
            loadThis(compiler);
            emitByteArg(compiler,isStore ? OP_STORE_FIELD : OP_LOAD_FIELD, field);

        }*/
        // loadThis(compiler);
        emitByteArg(compiler, isStore ? OP_STORE_FIELD : OP_LOAD_FIELD, field);
    }
    return true;
}


// Declares a method in the enclosing class with [signature].
//
// Reports an error if a method with that signature is already declared.
// Returns the symbol for the method.
static int declareMethod(Compiler *compiler, Signature *signature,
                         const char *name, int length) {

    int symbol = signatureSymbol(compiler, signature);

    // See if the class has already declared method with this signature.
    ClassInfo *classInfo = compiler->enclosingClass;
    IntBuffer *methods = classInfo->inStatic
                         ? &classInfo->staticMethods : &classInfo->methods;
    /*for (int i = 0; i < methods->count; i++) {
        if (methods->data[i] == symbol) {
            const char *staticPrefix = classInfo->inStatic ? "static " : "";
            error(compiler,"Class %s already defines a %smethod '%s'.",
                        &compiler->enclosingClass->name->value, staticPrefix, name);
            break;
        }
    }*/
    if (isDeclared(methods, symbol)) {
        const char *staticPrefix = classInfo->inStatic ? "static " : "";
        error(compiler, "Class %s already defines a %smethod '%s'.",
              &compiler->enclosingClass->name->value, staticPrefix, name);

    }

    MSCWriteIntBuffer(compiler->parser->vm, methods, symbol);
    return symbol;
}

// Compiles a method call with [signature] using [instruction].
void callSignature(Compiler *compiler, Opcode instruction,
                   Signature *signature) {
    if (signature->type == SIG_GETTER) {
        ClassInfo *enclosingClass = getEnclosingClass(compiler);
        if (compiler->dotSource == THIS_TOKEN && enclosingClass != NULL) {
            int fieldSymbol = MSCSymbolTableFind(&enclosingClass->fields, signature->name,
                                                 (size_t) signature->length);
            if (fieldSymbol != -1) {
                // it's a field
                field(compiler, false, false, NULL);
                return;
            }
            /*fieldSymbol = 0xff; // set it to max value
            // emit a get_field code with field symbol
            int symbol = signatureSymbol(compiler,signature);
            int slot = emitByteArg(compiler,OP_GET_FIELD, fieldSymbol);
            emitShort(compiler,symbol);
            trackField(this, slot, signature->name, signature->length,
                       static_cast<uint8_t>(fieldSymbol));
            return;*/
        }

    }
    int symbol = signatureSymbol(compiler, signature);
    if (signature->arity <= 16) {
        emitShortArg(compiler, (Opcode) (instruction + signature->arity), symbol);
    } else {
        emitShortArg(compiler, OP_CALL, symbol);
        emitShort(compiler, signature->arity);
    }

    if (instruction == OP_SUPER_0) {
        // Super calls need to be statically bound to the class's superclass. This
        // ensures we call the right method even when a method containing a super
        // call is inherited by another subclass.
        //
        // We bind it at class definition time by storing a reference to the
        // superclass in a constant. So, here, we create a slot in the constant
        // table and store NULL in it. When the method is bound, we'll look up the
        // superclass then and store it in the constant slot.
        emitShort(compiler, addConstant(compiler, NULL_VAL));
    }
}

// Compiles an (optional) argument list for a method call with [methodSignature]
// and then calls it.
void methodCall(Compiler *compiler, Opcode instruction,
                Signature *signature) {
    // Make a new signature that contains the updated arity and type based on
    // the arguments we find.
    Signature called = {signature->name, signature->length, SIG_GETTER, 0};

    // Parse the argument list, if any.
    if (match(compiler, LPAREN_TOKEN)) {
        called.type = SIG_FUNCTION;
        // Allow new line before an empty argument list
        ignoreNewlines(compiler);
        // Allow empty an argument list.
        if (peek(compiler) != RPAREN_TOKEN) {
            finishArgumentList(compiler, &called);
        }
        consume(compiler, RPAREN_TOKEN, "Expect ')' after arguments.");
    }

    // Parse the block argument, if any.
    if (match(compiler, LBRACE_TOKEN)) {
        // Include the block argument in the arity.
        called.type = SIG_FUNCTION;
        called.arity++;

        Compiler fnCompiler;
        initCompiler(&fnCompiler, compiler->parser, compiler, false);
        // initCompiler(&fnCompiler, compiler->parser, compiler, false);

        // Make a dummy signature to track the arity.
        Signature fnSignature = {"", 0, SIG_FUNCTION, 0};

        // Parse the parameter list, if any.
        if (match(compiler, LPAREN_TOKEN)) {
            finishParameterList(&fnCompiler, &fnSignature);
            consume(compiler, RPAREN_TOKEN, "Expect ')' after function parameters.");
            consume(compiler, ARROW_TOKEN, "Expect '=>' after function parameters.");
        }

        fnCompiler.function->arity = fnSignature.arity;

        finishBody(&fnCompiler);

        // Name the function based on the method its passed to.
        char blockName[MAX_METHOD_SIGNATURE + 15];
        int blockLength;
        signatureToString(&called, blockName, &blockLength);
        memmove(blockName + blockLength, " block argument", 16);

        endCompiler(&fnCompiler, blockName, blockLength + 15);
    }

    // TODO: Allow Grace-style mixfix methods?

    // If this is a super() call for an initializer, make sure we got an actual
    // argument list.
    if (signature->type == SIG_INITIALIZER) {
        if (called.type != SIG_FUNCTION) {
            error(compiler, "A superclass constructor must have an argument list.");
        }

        called.type = SIG_INITIALIZER;
    }
    callSignature(compiler, instruction, &called);
}

// Compiles a call whose name is the previously consumed token. This includes
// method calls with arguments
static void namedCall(Compiler *compiler, bool canAssign, Opcode instruction) {
    // Get the token for the method name.
    Signature signature = signatureFromToken(compiler, SIG_GETTER);

    // printf("Sign:::: %d (%.*s)\n", signature.type, signature.length, signature.name);
    if (canAssign && peek(compiler) == ASSIGN_TOKEN) {
        // x.d = 18; set field or method call

        ignoreNewlines(compiler);
        // Build the setter signature.
        signature.type = SIG_SETTER;
        signature.arity = 1;

        ClassInfo *enclosingClass = getEnclosingClass(compiler);
        if (enclosingClass != NULL && compiler->dotSource == THIS_TOKEN) {
            // we are inside a class method , so use a conditional field setter
            // ale.d = 18;
            // nin aaa = ale;
            // aa.d = 18;
            int fieldSymbol = MSCSymbolTableFind(&enclosingClass->fields, signature.name, (size_t) signature.length);
            if (fieldSymbol != -1) {
                // it's a field
                field(compiler, true, false, NULL);
                return;
            }
            /*consume(ASSIGN_TOKEN, "expected '=' keyword");
            expression(compiler);
            loadThis(compiler);
            fieldSymbol = 0xff; // set it to max value
            // emit a get_field code with field symbol
            int symbol = signatureSymbol(compiler,&signature);
            int slot = emitByteArg(compiler,OP_SET_FIELD, fieldSymbol);
            emitShort(compiler,symbol);
            trackField(this, slot, signature.name, signature.length,
                       static_cast<uint8_t>(fieldSymbol));
            return;*/
        }
        // Compile the assigned value.
        consume(compiler, ASSIGN_TOKEN, "expected '=' keyword");
        expression(compiler);
        callSignature(compiler, instruction, &signature);
    } else {
        methodCall(compiler, instruction, &signature);
        allowLineBeforeDot(compiler);
    }
}


// Loads the receiver of the currently enclosing method. Correctly handles
// functions defined inside methods.
static void loadThis(Compiler *compiler) {
    Variable thisVar = resolveNonmodule(compiler, "ale", 3);
    loadVariable(compiler, &thisVar);
}


// A parenthesized expression.
static void grouping(Compiler *compiler, bool canAssign) {
    expression(compiler);
    consume(compiler, RPAREN_TOKEN, "Expect ')' after expression.");
}

// A list literal.
static void list(Compiler *compiler, bool canAssign) {
    // Instantiate a new list.
    loadCoreVariable(compiler, "Walan");
    callMethod(compiler, 0, "kura()", 6);

    // Compile the list elements. Each one compiles to a ".add()" call.
    do {
        ignoreNewlines(compiler);

        // Stop if we hit the end of the list.
        if (peek(compiler) == RBRACKET_TOKEN) break;
        if (match(compiler, SPREAD_OR_REST_TOKEN)) {
            expression(compiler);
            callMethod(compiler, 1, "aBeeFaraAkan_(_)", 16);
        } else {
            expression(compiler);
            callMethod(compiler, 1, "aFaraAkan_(_)", 13);
        }
        // The element.
    } while (match(compiler, COMMA_TOKEN));

    // Allow newlines before the closing ']'.
    ignoreNewlines(compiler);
    consume(compiler, RBRACKET_TOKEN, "Expect ']' after list elements.");
}

// A map literal.
static bool mapKey(Compiler *compiler) {
    if (match(compiler, ID_TOKEN)) {
        // map key without string wraped it as string
        Token *token = &compiler->parser->previous;
        emitConstant(compiler, MSCStringFromCharsWithLength(compiler->parser->vm, token->start, token->length));
        if (peek(compiler) == COLON_TOKEN) {
            return true;
        }
        // resolve the variable value
        Variable variable = resolveName(compiler, token->start, token->length);
        bareName(compiler, true, variable);
        callMethod(compiler, 2, "aFaraAkan_(_,_)", 15);
        return false;
    }
    if (match(compiler, LBRACKET_TOKEN)) {
        expression(compiler);
        consume(compiler, RBRACKET_TOKEN, "Expect ']' in map key expression");
        return true;
    }
    if (match(compiler, SPREAD_OR_REST_TOKEN)) {
        // spread elements here
        expression(compiler);
        callMethod(compiler, 1, "aBeeFaraAkan_(_)", 16);
        return false;
    }
    parsePrecedence(compiler, PREC_PRIMARY);
    return true;
}

static void map(Compiler *compiler, bool canAssign) {
    // Instantiate a new map.
    loadCoreVariable(compiler, "Wala");
    callMethod(compiler, 0, "kura()", 6);

    // Compile the map elements. Each one is compiled to just invoke the
    // subscript setter on the map.
    do {
        ignoreNewlines(compiler);

        // Stop if we hit the end of the map.
        if (peek(compiler) == RBRACE_TOKEN) break;

        // The key.
        if (mapKey(compiler)) {
            consume(compiler, COLON_TOKEN, "Expect ':' after map key.");
            ignoreNewlines(compiler);

            // The value.
            expression(compiler);
            callMethod(compiler, 2, "aFaraAkan_(_,_)", 15);
        }

    } while (match(compiler, COMMA_TOKEN));

    // Allow newlines before the closing '}'.
    ignoreNewlines(compiler);
    consume(compiler, RBRACE_TOKEN, "Expect '}' after map entries.");
}

// Unary operators like `-foo`.
static void unaryOp(Compiler *compiler, bool canAssign) {
    GrammarRule *rule = getRule(compiler->parser->previous.type);

    ignoreNewlines(compiler);

    // Compile the argument.
    parsePrecedence(compiler, (Precedence) (PREC_UNARY + 1));

    // Call the operator method on the left-hand side.
    callMethod(compiler, 0, rule->name, 1);
}

static void boolean(Compiler *compiler, bool canAssign) {
    emitOp(compiler,
           compiler->parser->previous.type == FALSE_TOKEN ? OP_FALSE : OP_TRUE);
}


// Compiles a variable name or method call with an implicit receiver.
static void name(Compiler *compiler, bool canAssign) {
    // Look for the name in the scope chain up to the nearest enclosing method.
    Token *token = &compiler->parser->previous;
    TokenType next = peek(compiler);
    if (next == DOT_TOKEN) {
        compiler->dotSource = token->type;
    }
    /*if (next == LPAREN_TOKEN || next == LBRACKET_TOKEN || next == LBRACE_TOKEN) {
        // method call
        printf("Call next:: %s", compiler->parser->currentChar);
        namedCall(compiler,false, OP_CALL_0);
    }*/
    Variable variable = resolveNonmodule(compiler, token->start, token->length);
    if (variable.index != -1) {
        bareName(compiler, canAssign, variable);
        return;
    }

    // TODO: The fact that we return above here if the variable is known and parse
    // an optional argument list below if not means that the grammar is not
    // context-free. A line of code in a method like "someName(foo)" is a parse
    // error if "someName" is a defined variable in the surrounding scope and not
    // if it isn't. Fix this. One option is to have "someName(foo)" always
    // resolve to a self-call if there is an argument list, but that makes
    // getters a little confusing.

    // If we're inside a method and the name is lowercase, treat it as a method
    // on this.
    /*if (isLocalName(token->start) && getEnclosingClass(compiler) != NULL) {
        loadThis(compiler);
        namedCall(compiler,canAssign, OP_CALL_0);
        return;
    }*/

    // Otherwise, look for a module-level variable with the name.
    variable.scope = SCOPE_MODULE;
    variable.index = MSCSymbolTableFind(&compiler->parser->module->variableNames,
                                        token->start, (size_t) token->length);
    if (variable.index == -1) {
        // Implicitly define a module-level variable in
        // the hopes that we get a real definition later.
        variable.index = MSCDeclareVariable(
                compiler->parser->vm,
                compiler->parser->module,
                token->start, (size_t) token->length,
                token->line);

        if (variable.index == -2) {
            error(compiler, "Too many module variables defined.");
        }
    }

    bareName(compiler, canAssign, variable);
}

static void void_(Compiler *compiler, bool canAssign) {
    emitOp(compiler, OP_VOID);
}

// A number or string literal.
static void literal(Compiler *compiler, bool canAssign) {
    emitConstant(compiler, compiler->parser->previous.value);
}

// A string literal that contains interpolated expressions.
//
// Interpolation is syntactic sugar for calling ".join()" on a list. So the
// string:
//
//     "a %(b + c) d"
//
// is compiled roughly like:
//
//     ["a ", b + c, " d"].kunben()
static void stringInterpolation(Compiler *compiler, bool canAssign) {
    // Instantiate a new list.
    loadCoreVariable(compiler, "Walan");
    callMethod(compiler, 0, "kura()", 6);

    do {
        // The opening string part.
        literal(compiler, false);
        callMethod(compiler, 1, "aFaraAkan_(_)", 13);

        // The interpolated expression.
        ignoreNewlines(compiler);
        expression(compiler);
        callMethod(compiler, 1, "aFaraAkan_(_)", 13);

        ignoreNewlines(compiler);
    } while (match(compiler, DOLLAR_INTERPOL_TOKEN));

    // The trailing string part.
    consume(compiler, STRING_CONST_TOKEN, "Expect end of string interpolation.");
    literal(compiler, false);
    callMethod(compiler, 1, "aFaraAkan_(_)", 13);

    // The list of interpolated parts.
    callMethod(compiler, 0, "kunBen()", 8);
}

static void super_(Compiler *compiler, bool canAssign) {
    ClassInfo *enclosingClass = getEnclosingClass(compiler);
    if (enclosingClass == NULL && !compiler->isExtension) {
        error(compiler, "Cannot use 'faa' outside of a method.");
    }
    loadThis(compiler);
    // TODO: Super operator calls.
    // TODO: There's no syntax for invoking a superclass constructor with a
    // different name from the enclosing one. Figure that out.

    // See if it's a named super call, or an unnamed one.
    if (match(compiler, DOT_TOKEN)) {
        // Compile the superclass call.
        compiler->dotSource = SUPER_TOKEN;
        consume(compiler, ID_TOKEN, "Expect method name after 'super.'.");
        namedCall(compiler, canAssign, OP_SUPER_0);
    } else if (enclosingClass != NULL) {
        // No explicit name, so use the name of the enclosing method. Make sure we
        // check that enclosingClass isn't NULL first. We've already reported the
        // error, but we don't want to crash here.
        methodCall(compiler, OP_SUPER_0, enclosingClass->signature);
    }
}

static void this_(Compiler *compiler, bool canAssign) {
    if (getEnclosingClass(compiler) == NULL && !compiler->isExtension) {
        error(compiler, "Cannot use 'ale' outside of a method.");
        return;
    }
    compiler->dotSource = THIS_TOKEN;
    loadThis(compiler);
}

// Subscript or "array indexing" operator like `foo[bar]`.
static void subscript(Compiler *compiler, bool canAssign) {
    Signature signature = {"", 0, SIG_SUBSCRIPT, 0};

    // Parse the argument list.
    finishArgumentList(compiler, &signature);
    consume(compiler, RBRACKET_TOKEN, "Expect ']' after arguments.");

    allowLineBeforeDot(compiler);

    if (canAssign && match(compiler, ASSIGN_TOKEN)) {
        signature.type = SIG_SUBSCRIPT_SETTER;

        // Compile the assigned value.
        validateNumParameters(compiler, ++signature.arity);
        expression(compiler);
    }
    callSignature(compiler, OP_CALL_0, &signature);
}

static void call(Compiler *compiler, bool canAssign) {
    ignoreNewlines(compiler);
    consume(compiler, ID_TOKEN, "Expect method or attribute name after '.'.");
    // printf(":::(%.*s)\n", compiler->parser->previous.length, compiler->parser->previous.start);
    namedCall(compiler, canAssign, OP_CALL_0);
}

static void functionCall(Compiler *compiler, bool canAssign) {
    Signature signature = {"", 0, SIG_FUNCTION, 0};

    // Parse the argument list.
    ignoreNewlines(compiler);
    // Allow empty an argument list.
    if (peek(compiler) != RPAREN_TOKEN) {
        finishArgumentList(compiler, &signature);
    }
    consume(compiler, RPAREN_TOKEN, "Expect ')' after arguments.");
    // Parse the block argument, if any.
    if (match(compiler, LBRACE_TOKEN)) {
        // Include the block argument in the arity.
        signature.type = SIG_FUNCTION;
        signature.arity++;

        Compiler fnCompiler;
        initCompiler(&fnCompiler, compiler->parser, compiler, false);
        // initCompiler(&fnCompiler, compiler->parser, compiler, false);

        // Make a dummy signature to track the arity.
        Signature fnSignature = {"", 0, SIG_FUNCTION, 0};

        // Parse the parameter list, if any.
        if (match(compiler, LPAREN_TOKEN)) {
            finishParameterList(&fnCompiler, &fnSignature);
            consume(compiler, RPAREN_TOKEN, "Expect ')' after function parameters.");
            consume(compiler, ARROW_TOKEN, "Expect '=>' after function parameters.");
        }

        fnCompiler.function->arity = fnSignature.arity;

        finishBody(&fnCompiler);

        // Name the function based on the method its passed to.
        char blockName[MAX_METHOD_SIGNATURE + 15];
        int blockLength;
        signatureToString(&signature, blockName, &blockLength);
        memmove(blockName + blockLength, " block argument", 16);

        endCompiler(&fnCompiler, blockName, blockLength + 15);
    }
    emitShortArg(compiler, OP_CALL, signature.arity);
    // callSignature(compiler,OP_CALL_0, &signature);
}

static void lambdaCall(Compiler *compiler, bool canAssign) {
    Signature signature = {"", 0, SIG_FUNCTION, 0};

    // Parse the argument list.
    ignoreNewlines(compiler);
    // Allow empty an argument list.
    if (peek(compiler) != RPAREN_TOKEN) {
        finishArgumentList(compiler, &signature);
    }
    Compiler fnCompiler;
    initCompiler(&fnCompiler, compiler->parser, compiler, false);
    // initCompiler(&fnCompiler, compiler->parser, compiler, false);

    // Make a dummy signature to track the arity.
    Signature fnSignature = {"", 0, SIG_FUNCTION, 0};

    // Parse the parameter list, if any.
    if (match(compiler, LPAREN_TOKEN)) {
        finishParameterList(&fnCompiler, &fnSignature);
        consume(compiler, RPAREN_TOKEN, "Expect ')' after function parameters.");
        consume(compiler, ARROW_TOKEN, "Expect '=>' after function parameters.");
    }

    fnCompiler.function->arity = fnSignature.arity;

    finishBody(&fnCompiler);

    // Name the function based on the method its passed to.
    char blockName[MAX_METHOD_SIGNATURE + 15];
    int blockLength;
    signatureToString(&signature, blockName, &blockLength);
    memmove(blockName + blockLength, " block argument", 16);

    endCompiler(&fnCompiler, blockName, blockLength + 15);


    emitShortArg(compiler, OP_CALL, signature.arity);
    // callSignature(compiler,OP_CALL_0, &signature);
}

static void and_(Compiler *compiler, bool canAssign) {
    ignoreNewlines(compiler);

    // Skip the right argument if the left is false.
    int jump = emitJump(compiler, OP_AND);
    parsePrecedence(compiler, PREC_LOGICAL_AND);
    patchJump(compiler, jump);
}

static void or_(Compiler *compiler, bool canAssign) {
    ignoreNewlines(compiler);

    // Skip the right argument if the left is true.
    int jump = emitJump(compiler, OP_OR);
    parsePrecedence(compiler, PREC_LOGICAL_OR);
    patchJump(compiler, jump);
}

static void conditional(Compiler *compiler, bool canAssign) {
    // Ignore newline after '?'.
    ignoreNewlines(compiler);

    // Jump to the else branch if the condition is false.
    int ifJump = emitJump(compiler, OP_JUMP_IF);

    // Compile the then branch.
    parsePrecedence(compiler, PREC_CONDITIONAL);

    consume(compiler, COLON_TOKEN,
            "Expect ':' after then branch of conditional operator.");
    ignoreNewlines(compiler);

    // Jump over the else branch when the if branch is taken.
    int elseJump = emitJump(compiler, OP_JUMP);

    // Compile the else branch.
    patchJump(compiler, ifJump);

    parsePrecedence(compiler, PREC_ASSIGNMENT);

    // Patch the jump over the else.
    patchJump(compiler, elseJump);
}

void infixOp(Compiler *compiler, bool canAssign) {
    GrammarRule *rule = getRule(compiler->parser->previous.type);

    // An infix operator cannot end an expression.
    ignoreNewlines(compiler);

    // Compile the right-hand side.
    parsePrecedence(compiler, (Precedence) (rule->precedence + 1));

    // Call the operator method on the left-hand side.
    Signature signature = {rule->name, (int) strlen(rule->name), SIG_FUNCTION, 1};
    callSignature(compiler, OP_CALL_0, &signature);
}

// Compiles a method signature for an infix operator.
void infixSignature(Compiler *compiler, Signature *signature) {
    // Add the RHS parameter.
    signature->type = SIG_FUNCTION;
    signature->arity = 1;

    // Parse the parameter name.
    consume(compiler, LPAREN_TOKEN, "Expect '(' after operator name.");
    declareNamedVariable(compiler);
    consume(compiler, RPAREN_TOKEN, "Expect ')' after parameter name.");
}

// Compiles a method signature for an unary operator (i.e. "!").
void unarySignature(Compiler *compiler, Signature *signature) {
    // Do nothing. The name is already complete.
    signature->type = SIG_GETTER;
}

// Compiles a method signature for an operator that can either be unary or
// infix (i.e. "-").
void mixedSignature(Compiler *compiler, Signature *signature) {
    signature->type = SIG_GETTER;

    // If there is a parameter, it's an infix operator, otherwise it's unary.
    if (match(compiler, LPAREN_TOKEN)) {
        // Add the RHS parameter.
        signature->type = SIG_FUNCTION;
        signature->arity = 1;
        // Parse the parameter name.
        declareNamedVariable(compiler);
        consume(compiler, RPAREN_TOKEN, "Expect ')' after parameter name.");
    }
}

// Compiles an optional setter parameter in a method [signature].
//
// Returns `true` if it was a setter.
static bool maybeSetter(Compiler *compiler, Signature *signature) {
    // See if it's a setter.
    if (!match(compiler, ASSIGN_TOKEN)) return false;

    // It's a setter.
    if (signature->type == SIG_SUBSCRIPT) {
        signature->type = SIG_SUBSCRIPT_SETTER;
    } else {
        signature->type = SIG_SETTER;
    }

    // Parse the value parameter.
    consume(compiler, LPAREN_TOKEN, "Expect '(' after '='.");
    declareNamedVariable(compiler);
    consume(compiler, RPAREN_TOKEN, "Expect ')' after parameter name.");

    signature->arity++;

    return true;
}

// Compiles a method signature for a subscript operator.
void subscriptSignature(Compiler *compiler, Signature *signature) {
    signature->type = SIG_SUBSCRIPT;

    // The signature currently has "[" as its name since that was the token that
    // matched it. Clear that out.
    signature->length = 0;

    // Parse the parameters inside the subscript.
    finishParameterList(compiler, signature);
    consume(compiler, RBRACKET_TOKEN, "Expect ']' after parameters.");

    maybeSetter(compiler, signature);
}

// Parses an optional parenthesized parameter list. Updates `type` and `arity`
// in [signature] to match what was parsed.
static void parameterList(Compiler *compiler, Signature *signature) {
    // The parameter list is optional.
    if (!match(compiler, LPAREN_TOKEN)) return;

    signature->type = SIG_FUNCTION;

    // Allow new line before an empty argument list
    ignoreNewlines(compiler);

    // Allow an empty parameter list.
    if (match(compiler, RPAREN_TOKEN)) return;

    finishParameterList(compiler, signature);
    consume(compiler, RPAREN_TOKEN, "Expect ')' after parameters.");
}

// Compiles a function signature for a named function.
void namedSignature(Compiler *compiler, Signature *signature) {
    signature->type = SIG_GETTER;
    // If it's a setter, it can't also have a parameter list.
    if (maybeSetter(compiler, signature)) return;
    // Regular named function with an optional parameter list.
    parameterList(compiler, signature);

}

// Compiles a method signature for a constructor.
void constructorSignature(Compiler *compiler, Signature *signature) {
    consume(compiler, ID_TOKEN, "Expect constructor name after 'construct'.");
    // Capture the name.
    *signature = signatureFromToken(compiler, SIG_INITIALIZER);

    if (match(compiler, ASSIGN_TOKEN)) {
        error(compiler, "A constructor cannot be a setter.");
    }

    if (!match(compiler, LPAREN_TOKEN)) {
        error(compiler, "A constructor cannot be a getter.");
        return;
    }

    // Allow an empty parameter list.
    if (match(compiler, RPAREN_TOKEN)) return;

    finishParameterList(compiler, signature);
    consume(compiler, RPAREN_TOKEN, "Expect ')' after parameters.");
}

// This table defines all of the parsing rules for the prefix and infix
// expressions in the grammar. Expressions are parsed using a Pratt parser.
//
// See: http://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/
#define UNUSED                     { NULL, NULL, NULL, PREC_NONE, NULL }
#define PREFIX(fn)                 { fn, NULL, NULL, PREC_NONE, NULL }
#define INFIX(prec, fn)            { NULL, fn, NULL, prec, NULL }
#define INFIX_OPERATOR(prec, name) { NULL, infixOp, infixSignature, prec, name }
#define PREFIX_OPERATOR(name)      { unaryOp, NULL, unarySignature, PREC_NONE, name }
#define OPERATOR(name)             { unaryOp, infixOp, mixedSignature, PREC_TERM, name }

GrammarRule rules[] = {
        /* NUMBER_CONST_TOKEN           0 */ PREFIX(literal),
        /* TRING_CONST_TOKEN            1  */ PREFIX(literal),
        /* PLUS_TOKEN                   2  */ INFIX_OPERATOR(PREC_TERM, "+"),
        /* MINUS_TOKEN                  3  */ OPERATOR("-"),
        /* MULT_TOKEN                   4  */ INFIX_OPERATOR(PREC_FACTOR, "*"),
        /* DIV_TOKEN                    5  */ INFIX_OPERATOR(PREC_FACTOR, "/"),
        /* MOD_TOKEN                    6  */ INFIX_OPERATOR(PREC_FACTOR, "%"),
        /* DOLLAR_INTERPOL_TOKEN        7  */ PREFIX(stringInterpolation),
        /* LPAREN_TOKEN                 8  */ {grouping, functionCall, NULL, PREC_CALL, NULL},
        /* RPAREN_TOKEN                 9  */ UNUSED,
        /* LBRACE_TOKEN                 10 */ PREFIX(map),
        /* RBRACE_TOKEN                 11 */ UNUSED,
        /* LBRACKET_TOKEN               12 */ {list, subscript, subscriptSignature, PREC_CALL, NULL},
        /* RBRACKET_TOKEN               13 */ UNUSED,
        /* JOB_TOKEN                    14 */ UNUSED,
        /* ARROW_TOKEN                  15 */ UNUSED,

        /* GREAT_TOKEN                  16 */ INFIX_OPERATOR(PREC_COMPARISON, ">"),
        /* GREAT_EQ_TOKEN               17 */ INFIX_OPERATOR(PREC_COMPARISON, ">="),
        /* LESS_TOKEN                   18 */ INFIX_OPERATOR(PREC_COMPARISON, "<"),
        /* LESS_EQ_TOKEN                19 */ INFIX_OPERATOR(PREC_COMPARISON, "<="),
        /* EQUAL_TOKEN                  20 */ INFIX_OPERATOR(PREC_COMPARISON, "=="),
        /* NEQUAL_TOKEN                 21 */ INFIX_OPERATOR(PREC_COMPARISON, "!="),

        /* IF_TOKEN                     22 */ PREFIX(ifExpression),
        /* ELSE_TOKEN                   23 */ UNUSED,
        /* AND_TOKEN                    24 */ INFIX(PREC_LOGICAL_AND, and_),
        /* OR_TOKEN                     25 */ INFIX(PREC_LOGICAL_OR, or_),
        /* BT_XOR_TOKEN                 26 */ INFIX_OPERATOR(PREC_BITWISE_XOR, "^"),
        /* BT_AND_TOKEN                 27 */ INFIX_OPERATOR(PREC_BITWISE_AND, "&"),
        /* BT_OR_TOKEN                  28 */ INFIX_OPERATOR(PREC_BITWISE_OR, "|"),
        /* BT_SHIFT_R_TOKEN             29 */ INFIX_OPERATOR(PREC_BITWISE_SHIFT, ">>"),

        /* BT_SHIFT_L_TOKEN             30 */ INFIX_OPERATOR(PREC_BITWISE_SHIFT, "<<"),
        /* INC_TOKEN                    31 */ UNUSED,
        /* DEC_TOKEN                    32 */ UNUSED,
        /* NOT_TOKEN                    33 */ PREFIX_OPERATOR("!"),
        /* BT_INV_TOKEN                 34 */ PREFIX_OPERATOR("~"),
        /* ASSIGN_TOKEN                 35 */ UNUSED,
        /* SEMI_TOKEN                   36 */ UNUSED,
        /* COMMA_TOKEN                  37 */ UNUSED,
        /* COLON_TOKEN                  38 */ UNUSED,
        /* DOT_TOKEN                    39 */ INFIX(PREC_CALL, call),

        /* SPREAD_OR_REST_TOKEN         40 */ INFIX_OPERATOR(PREC_RANGE, "..."),
        /* RANGE_TOKEN                  41 */ INFIX_OPERATOR(PREC_RANGE, ".."),
        /* VAR_TOKEN                    42 */ UNUSED,
        /* TRUE_TOKEN                   43 */ PREFIX(boolean),
        /* NULL_TOKEN                   44 */ PREFIX(null),
        /* VOID_TOKEN                   45 */ PREFIX(void_),
        /* FALSE_TOKEN                  46 */ PREFIX(boolean),
        /* RETURN_TOKEN                 47 */ UNUSED,
        /* WHILE_TILL_TOKEN             48 */ UNUSED,
        /* WITH_TOKEN                   49 */ UNUSED,

        /* AS_TOKEN                     50 */ UNUSED,
        /* FROM_TOKEN                   51 */ UNUSED,
        /* UP_TOKEN                     52 */ UNUSED,
        /* DOWN_TOKEN                   53 */ UNUSED,
        /* FOR_LOOP_TOKEN               54 */ UNUSED,
        /* IN_TOKEN                     55 */ UNUSED,
        /* YE_TOKEN                     56 */ INFIX_OPERATOR(PREC_IS, "ye"),
        /* DO_TOKEN                     57 */ UNUSED,
        /* CATCH_TOKEN                  58 */ UNUSED,
        /* THROW_TOKEN                  59 */ UNUSED,

        /* BECAUSE_TOKEN                60 */ UNUSED,
        /* IMPORT_TOKEN                 61 */ UNUSED,
        /* CLASS_TOKEN                  62 */ UNUSED,
        /* STATIC_TOKEN                 63 */ UNUSED,

        /* INIT_TOKEN                   64 */ {NULL, NULL, constructorSignature, PREC_NONE, NULL},
        /* WHEN_TOKEN                   65 */ PREFIX(whenExpression),
        /* BREAK_TOKEN                  66 */ UNUSED,
        /* CONTINUE_TOKEN               67 */ UNUSED,
        /* ID_TOKEN                     68 */ {name, NULL, namedSignature, PREC_NONE, NULL},

        /* EOL_TOKEN                    69 */ UNUSED,
        /* EOF_TOKEN                    70 */ UNUSED,
        /* ERROR_TOKEN                  71 */ UNUSED,


        /* THIS_TOKEN                    72 */ PREFIX(this_),
        /* SUPER_TOKEN                   73 */ PREFIX(super_),
        /* EXTERN_TOKEN                  74 */ UNUSED,
        /* MULT_EQ_TOKEN                 75 */ UNUSED,

        /* DIV_EQ_TOKEN                  76 */ UNUSED,

        /* MOD_EQ_TOKEN                  77 */ UNUSED,

        /* PLUS_EQ_TOKEN                 78 */ UNUSED,

        /* MINUS_EQ_TOKEN                79 */ UNUSED,
        /* INVALID_TOKEN                 80 */ UNUSED,

};

// Gets the [GrammarRule] associated with tokens of [type].
static GrammarRule *getRule(TokenType type) {
    return &rules[type];
}

// The main entrypoint for the top-down operator precedence parser.
static void parsePrecedence(Compiler *compiler, Precedence precedence) {
    nextToken(compiler->parser);
    GrammarFn prefix = rules[compiler->parser->previous.type].prefix;
    if (prefix == NULL) {
        error(compiler, "Expected expression.");
        return;
    }

    // Track if the precendence of the surrounding expression is low enough to
    // allow an assignment inside this one. We can't compile an assignment like
    // a normal expression because it requires us to handle the LHS specially --
    // it needs to be an lvalue, not an rvalue. So, for each of the kinds of
    // expressions that are valid lvalues -- names, subscripts, fields, etc. --
    // we pass in whether or not it appears in a context loose enough to allow
    // "=". If so, it will parse the "=" itself and handle it appropriately.
    bool canAssign = precedence <= PREC_CONDITIONAL;
    prefix(compiler, canAssign);

    while (precedence <= rules[compiler->parser->current.type].precedence) {

        nextToken(compiler->parser);
        GrammarFn infix = rules[compiler->parser->previous.type].infix;
        infix(compiler, canAssign);
    }
}


// Creates a matching constructor method for an initializer with [signature]
// and [initializerSymbol].
//
// Construction is a two-stage process in Mosc that involves two separate
// methods. There is a static method that allocates a new instance of the class.
// It then invokes an initializer method on the new instance, forwarding all of
// the constructor arguments to it.
//
// The allocator method always has a fixed implementation:
//
//     CODE_CONSTRUCT - Replace the class in slot 0 with a new instance of it.
//     CODE_CALL      - Invoke the initializer on the new instance.
//
// This creates that method and calls the initializer with [initializerSymbol].
static void createConstructor(Compiler *compiler, Signature *signature,
                              int initializerSymbol) {
    Compiler methodCompiler;
    initCompiler(&methodCompiler, compiler->parser, compiler, true);

    // Allocate the instance.
    emitOp(&methodCompiler, compiler->enclosingClass->isExtern
                            ? OP_EXTERN_CONSTRUCT : OP_CONSTRUCT);

    // Run its initializer.
    emitShortArg(&methodCompiler, (Opcode) (OP_CALL_0 + signature->arity),
                 initializerSymbol);

    // Return the instance.
    emitOp(&methodCompiler, OP_RETURN);

    endCompiler(&methodCompiler, "", 0);
}

// Compiles a method definition inside a class body.
//
// Returns `true` if it compiled successfully, or `false` if the method couldn't
// be parsed.

static bool method(Compiler *compiler, Variable *classVariable, bool isStatic, bool isExtern) {

    compiler->enclosingClass->
            inStatic = isStatic;

    SignatureFn signatureFn = rules[compiler->parser->current.type].method;
    nextToken(compiler->parser);

    if (signatureFn == NULL) {
        error(compiler,
              "Expect method definition.");
        return false;
    }

// Build the method signature.
    Signature signature = signatureFromToken(compiler, SIG_GETTER);
/* if (compiler->enclosingClass->signature != NULL) {
     compiler->enclosingClass->signature = NULL;

 }*/

    compiler->enclosingClass->
            signature = &signature;

    Compiler methodCompiler;
    initCompiler(&methodCompiler, compiler->parser, compiler, true);

    // Compile the method signature.
    signatureFn(&methodCompiler, &signature);
    methodCompiler.isInitializer = signature.type == SIG_INITIALIZER;

    if (isStatic && signature.type == SIG_INITIALIZER) {
        error(compiler,
              "A constructor cannot be static.");
    }

// Include the full signature in debug messages in stack traces.
    char fullSignature[MAX_METHOD_SIGNATURE];
    int length;
    signatureToString(&signature, fullSignature, &length);

    // Check for duplicate methods. Doesn't matter that it's already been
    // defined, error will discard bytecode anyway.
    // Check if the method table already contains this symbol
    int methodSymbol = declareMethod(compiler, &signature, fullSignature, length);

    if (isExtern) {
// Define a constant for the signature.
        emitConstant(compiler, MSCStringFromCharsWithLength(compiler->parser->vm,
                                                            fullSignature, (uint32_t) length));

// We don't need the function we started compiling in the parameter list
// any more.
        methodCompiler.parser->vm->
                compiler = methodCompiler.parent;
    } else {

        consume(compiler, LBRACE_TOKEN,
                "Expect '{' to begin method body.");

        finishBody(&methodCompiler);

        endCompiler(&methodCompiler, fullSignature, length);
    }

// Define the method. For a constructor, this defines the instance
// initializer method.

    defineMethod(compiler, classVariable, isStatic, methodSymbol);

    if (signature.type == SIG_INITIALIZER) {
// Also define a matching constructor method on the metaclass.
        signature.
                type = SIG_FUNCTION;
        int constructorSymbol = signatureSymbol(compiler, &signature);

        createConstructor(compiler,
                          &signature, methodSymbol);

        defineMethod(compiler, classVariable,
                     true, constructorSymbol);
    }
// delete compiler->enclosingClass->signature;
    return true;
}

bool classStatement(Compiler *compiler, Variable *classVariable) {
    matchLine(compiler);
    bool isExtern = match(compiler, EXTERN_TOKEN);
    bool isStatic = match(compiler, STATIC_TOKEN);
    if (
            match(compiler, VAR_TOKEN)
            ) {
        consume(compiler, ID_TOKEN,
                "Exect field name after keyword 'nin'");
        if (isExtern) {
            error(compiler,
                  "Unexpected keyword 'kokan' before 'kulu' field");
            return false;
        }

        return isStatic ?
               staticField(compiler,
                           true, true, classVariable) :
               field(compiler,
                     true, true, classVariable);
    }
    return method(compiler, classVariable, isStatic, isExtern);

}

// Compiles a class definition. Assumes the "class" token has already been
// consumed (along with a possibly preceding "foreign" token).
void classDefinition(Compiler *compiler, bool isExtern) {
    // Create a variable to store the class in.
    Variable classVariable;
    classVariable.scope = compiler->scopeDepth == -1 ? SCOPE_MODULE : SCOPE_LOCAL;
    classVariable.index = declareNamedVariable(compiler);

    // Create shared class name value
    Value classNameString = MSCStringFromCharsWithLength(compiler->parser->vm,
                                                         compiler->parser->previous.start,
                                                         compiler->parser->previous.length);

    // Create class name string to track method duplicates
    String *className = AS_STRING(classNameString);

    // Make a string constant for the name.
    emitConstant(compiler, classNameString);

    // Load the superclass (if there is one).
    if (match(compiler, YE_TOKEN)) {
        parsePrecedence(compiler, PREC_CALL);
    } else {
        // Implicitly inherit from Object.
        loadCoreVariable(compiler, "Baa");
    }

    // Store a placeholder for the number of fields argument. We don't know the
    // count until we've compiled all the methods to see which fields are used.
    int numFieldsInstruction = -1;
    if (isExtern) {
        emitOp(compiler, OP_EXTERN_CLASS);
    } else {
        numFieldsInstruction = emitByteArg(compiler, OP_CLASS, 255);
    }

    // Store it in its name.
    defineVariable(compiler, classVariable.index);

    // Push a local variable scope. Static fields in a class body are hoisted out
    // into local variables declared in this scope. Methods that use them will
    // have upvalues referencing them.
    pushScope(compiler);

    if (match(compiler, SEMI_TOKEN) || peek(compiler) == EOL_TOKEN) {

        // end of class, a class with default constructor
        compiler->enclosingClass = NULL;
        if (!isExtern) {
            compiler->function->code.data[numFieldsInstruction] = 0;
        }
        popScope(compiler);
        return;
    }

    ClassInfo classInfo;
    newClassInfo(&classInfo, className, isExtern, false);



    // Allocate attribute maps if necessary.
    // A method will allocate the methods one if needed


    compiler->enclosingClass = &classInfo;

    // Compile the method definitions.
    consume(compiler, LBRACE_TOKEN, "Expect '{' after class declaration.");
    matchLine(compiler);

    while (!match(compiler, RBRACE_TOKEN)) {
        if (!classStatement(compiler, &classVariable)) break;
        match(compiler, SEMI_TOKEN);
        // Don't require a newline after the last definition.
        if (match(compiler, RBRACE_TOKEN)) break;

        consumeLine(compiler, "Expect newline after definition in class.");
    }

    // If any attributes are present,
    // instantiate a ClassAttributes instance for the class
    // and send it over to CODE_END_CLASS
    /*bool hasAttr = classInfo.classAttributes != NULL ||
                   classInfo.methodAttributes != NULL;
    if (hasAttr) {
        emitClassAttributes(compiler, &classInfo);
        loadVariable(compiler, classVariable);
        // At the moment, we don't have other uses for CODE_END_CLASS,
        // so we put it inside this condition. Later, we can always
        // emit it and use it as needed.
        emitOp(compiler, CODE_END_CLASS);
    }*/

    // Update the class with the number of fields.
    if (!isExtern) {
        compiler->function->code.data[numFieldsInstruction] =
                (uint8_t) classInfo.fields.count;
    }
    /*for (auto patch = classInfo.toPatch.begin();
    patch != classInfo.toPatch.end();
    patch++) {
        ByteBuffer *code = (*patch).code;
        auto fieldTrackings = (*patch).fieldTrackings;
        ClassField *field = fieldTrackings.data;
        while (field < fieldTrackings.data + fieldTrackings.count) {
            int fieldSymbol = MSCSymbolTableFind(&classInfo->fields, field->name, static_cast<size_t>(field->length));
            if (fieldSymbol == -1) {
                // error(compiler,"Undefined field %.*s used", field->length, field->name);
                field++;
                continue;
            }
            code->data[field->slot] = static_cast<uint8_t>(fieldSymbol);
            code->data[field->slot + 1] = 0xff;
            code->data[field->slot + 2] = 0xff;
            field++;
        }
        freeClassFieldBuffer(compiler->parser->vm, &fieldTrackings);
    }*/
    // Clear symbol tables for tracking field and method names.
    MSCSymbolTableClear(compiler->parser->vm, &classInfo.fields);
    MSCFreeIntBuffer(compiler->parser->vm, &classInfo.methods);
    MSCFreeIntBuffer(compiler->parser->vm, &classInfo.staticMethods);

    compiler->enclosingClass = NULL;
    popScope(compiler);
}

static bool definition(Compiler *compiler, bool expr) {
    /*if(matchAttribute(compiler)) {
        definition(compiler);
        return;
    }*/

    if (match(compiler, CLASS_TOKEN)) {
        classDefinition(compiler, false);
        return false;
    } else if (match(compiler, EXTERN_TOKEN)) {
        consume(compiler, CLASS_TOKEN, "Expect 'kulu' after 'extern'.");
        classDefinition(compiler, true);
        return false;
    }

    // disallowAttributes(compiler);

    if (match(compiler, FROM_TOKEN)) {
        import(compiler, false);
    } else if (match(compiler, IMPORT_TOKEN)) {
        import(compiler, true);
    } else if (match(compiler, VAR_TOKEN)) {
        variableDefinition(compiler);
    } else if (match(compiler, JOB_TOKEN)) {
        functionDefinition(compiler, false);
    } else if (match(compiler, STATIC_TOKEN)) {
        consume(compiler, JOB_TOKEN, "Expect 'tii' after 'dialen' token");
        functionDefinition(compiler, true);
    } else {
        return statement(compiler, expr);
    }
    return false;
}


int whenBranch(Compiler *compiler) {
    ignoreNewlines(compiler);
    TokenType currentToken = peek(compiler);
    // printf("Cur:: %d\n", currentToken);
    // nextToken(compiler->parser);
    GrammarFn infix = rules[currentToken].infix;
    if (infix != NULL) {
        nextToken(compiler->parser);
        infix(compiler, false);
        while (PREC_LOWEST <= rules[compiler->parser->current.type].precedence) {
            nextToken(compiler->parser);
            infix = rules[compiler->parser->previous.type].infix;
            infix(compiler, false);
        }
    } else {
        expression(compiler);
        callMethod(compiler, 1, "==(_)", 5);
    }
    consume(compiler, ARROW_TOKEN, "Expect '=>' betwen 'tumamin' case and body");
    return emitJump(compiler, OP_JUMP_IF);
}

static void whenExpression(Compiler *compiler, bool canAssign) {
    bool withParen = match(compiler, LPAREN_TOKEN);
    if (withParen) ignoreNewlines(compiler);
    const char *varName = "tumamin ";
    int length = 8;
    // null(compiler, false);
    // int retSlot = addLocal(compiler, "tumaminR ", 9);
    pushScope(compiler);
    if (match(compiler, VAR_TOKEN)) {
        consume(compiler, ID_TOKEN, "Expected variable name after 'nin' keyword");
        varName = compiler->parser->previous.start;
        length = compiler->parser->previous.length;
        consume(compiler, ASSIGN_TOKEN, "Expected '=' variable name keyword");
    }
    expression(compiler);// tester
    int discriminant = addLocal(compiler, varName, length);
    if (withParen) ignoreNewlines(compiler);
    if (withParen) consume(compiler, RPAREN_TOKEN, "Expect ')' after if condition.");
    consume(compiler, LBRACE_TOKEN, "Expect '{' after 'tumamin' keyword expression");
    int ifJump = -1;
    IntBuffer exitPoint;
    MSCInitIntBuffer(&exitPoint);
    do {
        if (match(compiler, ELSE_TOKEN)) {
            // Jump over the else branch when the if branch is taken.
            int elseJump = emitJump(compiler, OP_JUMP);
            patchJump(compiler, ifJump);
            consume(compiler, ARROW_TOKEN, "Expect '=>' between 'tumamin' case and body");
            // pushScope(compiler);
            statement(compiler, true);
            // emitByteArg(compiler, OP_STORE_LOCAL, retSlot);
            // popScope(compiler);
            // Patch the jump over the else.
            patchJump(compiler, elseJump);
            break;
        }

        if (ifJump != -1) {
            // not the first case, emit else jump
            int elseJump = emitJump(compiler, OP_JUMP);
            patchJump(compiler, ifJump);
            MSCWriteIntBuffer(compiler->parser->vm, &exitPoint, elseJump);
        }
        // Jump to the else branch if the condition is false.
        loadLocal(compiler, discriminant);// load test for next comparison
        ifJump = whenBranch(compiler);
        // pushScope(compiler);
        // parse when branch body
        statement(compiler, true);
        // emitByteArg(compiler, OP_STORE_LOCAL, retSlot);
        // popScope(compiler);
        ignoreNewlines(compiler);
        if (peek(compiler) == RBRACE_TOKEN) {
            // will exit so patch the ifjump
            patchJump(compiler, ifJump);
        }
    } while (peek(compiler) != EOF_TOKEN);
    for (int i = 0; i < exitPoint.count; i++) {
        patchJump(compiler, exitPoint.data[i]);
    }

    ignoreNewlines(compiler);
    consume(compiler, RBRACE_TOKEN, "Expect end of 'toumamin' }");
    MSCFreeIntBuffer(compiler->parser->vm, &exitPoint);
    emitByteArg(compiler, OP_STORE_LOCAL, discriminant);
    emitByte(compiler, OP_POP);
    softPopScope(compiler);
    // printf("RetSlot %d\n", retSlot);
    //MSCDumpCode(compiler->parser->vm, compiler->function);
    //popScope(compiler);

}

static void ifExpression(Compiler *compiler, bool canAssign) {
    ifStatement(compiler, true);
}


// Loads the enclosing class onto the stack and then binds the function already
// on the stack as a method on that class.
static void defineMethod(Compiler *compiler, Variable *classVariable, bool isStatic, int methodSymbol) {
// Load the class. We have to do this for each method because we can't
// keep the class on top of the stack. If there are static fields, they
// will be locals above the initial variable slot for the class on the
// stack. To defineMethodskip past those, we just load the class each time right before
// defining a method.
    loadVariable(compiler, classVariable);

// Define the method.
    Opcode instruction = isStatic ? OP_METHOD_STATIC : OP_METHOD_INSTANCE;
    emitShortArg(compiler, instruction, methodSymbol);
}

static Value consumeLiteral(Compiler *compiler, const char *message) {
    if (match(compiler, FALSE_TOKEN)) return FALSE_VAL;
    if (match(compiler, TRUE_TOKEN)) return TRUE_VAL;
    if (match(compiler, NUMBER_CONST_TOKEN)) return compiler->parser->previous.value;
    if (match(compiler, STRING_CONST_TOKEN)) return compiler->parser->previous.value;
    if (match(compiler, ID_TOKEN)) return compiler->parser->previous.value;

    error(compiler, message);
    nextToken(compiler->parser);
    return NULL_VAL;
}


