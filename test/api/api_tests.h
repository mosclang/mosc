//
// Created by Mahamadou DOUMBIA [OML DSI] on 08/02/2024.
//

#ifndef MOSC_API_TESTS_H
#define MOSC_API_TESTS_H

#include "../../src/api/msc.h"
#include <string.h>

int runAPITests(MVM* vm, const char* inTestName);

MSCExternMethodFn apiTestBindForeignMethod(
        MVM* vm, const char* module, const char* className,
        bool isStatic, const char* signature);

MSCExternClassMethods apiTestBindForeignClass(MVM* vm, const char* module, const char* className);

#endif //MOSC_API_TESTS_H
