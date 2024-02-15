//
// Created by Mahamadou DOUMBIA [OML DSI] on 08/02/2024.
//

#include "call_calls_extern.h"
#include "../../src/runtime/debuger.h"

static void api(Djuru *djuru) {
    // Grow the slot array. This should trigger the stack to be moved.
    MSCEnsureSlots(djuru, 2);
    MSCSetSlotNewList(djuru, 0);

    for (int i = 1; i < 10; i++) {
        MSCSetSlotDouble(djuru, 1, i);
        MSCInsertInList(djuru, 0, -1, 1);
    }
}

MSCExternMethodFn callCallsExternBindMethod(const char *signature) {
    if (strcmp(signature, "dialen CallCallsExtern.api()") == 0) return api;

    return NULL;
}

int callCallsExternRunTests(MVM *vm) {
    Djuru *djuru = MSCGetCurrentDjuru(vm);

    MSCEnsureSlots(djuru, 1);
    MSCGetVariable(djuru, "./test/api/call_calls_extern", "CallCallsExtern", 0);
    MSCHandle *apiClass = MSCGetSlotHandle(djuru, 0);
    MSCHandle *call = MSCMakeCallHandle(vm, "weele(_)");

    MSCEnsureSlots(djuru, 2);
    MSCSetSlotHandle(djuru, 0, apiClass);
    MSCSetSlotString(djuru, 1, "parameter");

    printf("slots before %d\n", MSCGetSlotCount(djuru));
    MSCCall(djuru, call);

    // We should have a single slot count for the return.
    printf("slots after %d\n", MSCGetSlotCount(djuru));
    MSCReleaseHandle(djuru, call);
    MSCReleaseHandle(djuru, apiClass);
    return 0;
}