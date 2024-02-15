//
// Created by Mahamadou DOUMBIA [OML DSI] on 08/02/2024.
//

#ifndef MOSC_CALL_CALLS_EXTERN_H
#define MOSC_CALL_CALLS_EXTERN_H
#include "../../src/api/msc.h"
#include <string.h>

MSCExternMethodFn callCallsExternBindMethod(const char* signature);
int callCallsExternRunTests(MVM* vm);
#endif //MOSC_CALL_CALLS_EXTERN_H
