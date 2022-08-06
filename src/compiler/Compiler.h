//
// Created by Mahamadou DOUMBIA [OML DSI] on 11/01/2022.
//

#ifndef CPMSC_COMPILER_H
#define CPMSC_COMPILER_H


#include "../memory/Value.h"
#include "../common/opcodes.h"

typedef struct Compiler Compiler;

Function *MSCCompile(MVM *vm, Module *module, const char *source, bool isExpression, bool printErrors);
void MSCMarkCompiler(Compiler *compiler, MVM *mvm);
void MSCBindMethodCode(Class *classObj, Function *fn);



#endif //CPMSC_COMPILER_H
