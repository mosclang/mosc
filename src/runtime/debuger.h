//
// Created by Mahamadou DOUMBIA [OML DSI] on 12/01/2022.
//

#ifndef CPMSC_DEBUGER_H
#define CPMSC_DEBUGER_H


#include "../memory/Value.h"


void MSCDebugPrintStackTrace(MVM *vm);

void MSCDumpValue(Value value);

void MSCDumpObject(Object *value);

void MSCDumpStack(Djuru *fiber);

int MSCDumpInstruction(MVM *vm, Function *fn, int i);

void MSCDumpCode(MVM *vm, Function *fn);
void MSCDumpSymbolTable(const SymbolTable* table);


#endif //CPMSC_DEBUGER_H
