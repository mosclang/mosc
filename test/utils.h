//
// Created by Mahamadou DOUMBIA [OML DSI] on 07/02/2024.
//

#ifndef MOSC_UTILS_H
#define MOSC_UTILS_H

#include "../src/api/msc.h"
#include <string.h>

#define MSC_TERR_USAGE 64
#define MSC_TERR_COMPILATION 65
#define MSC_TERR_SOFTWARE 70
#define MSC_TERR_NODATA 66
#define MSC_TERR_IO 74

#include "./path.h"



int tryHandlingArgs(int argc, const char *argv[]);

MSCLoadModuleResult loadModule(MVM *vm, const char *name);

const char *resolveModule(MVM *vm,
                          const char *importer, const char *name);

void print(MVM *_, const char *text);

bool errorPrint(MVM *vm, MSCError type, const char *module_name, int line, const char *message);

char *readSource(const char *name);

MSCInterpretResult runFile(MVM *vm, const char *path);

bool isModuleAnAPITest(const char *module);

#endif //MOSC_UTILS_H
