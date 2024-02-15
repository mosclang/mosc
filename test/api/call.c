//
// Created by Mahamadou DOUMBIA [OML DSI] on 08/02/2024.
//

#include "call.h"
#include "../../src/runtime/MVM.h"

int callRunTests(MVM* vm)
{
    Djuru* djuru = MSCGetCurrentDjuru(vm);
    MSCEnsureSlots(djuru, 1);
    MSCGetVariable(djuru, "./test/api/call", "Call", 0);
    MSCHandle* callClass = MSCGetSlotHandle(djuru, 0);

    MSCHandle* noParams = MSCMakeCallHandle(vm, "noParams");
    MSCHandle* zero = MSCMakeCallHandle(vm, "zero()");
    MSCHandle* one = MSCMakeCallHandle(vm, "one(_)");
    MSCHandle* two = MSCMakeCallHandle(vm, "two(_,_)");
    MSCHandle* unary = MSCMakeCallHandle(vm, "-");
    MSCHandle* binary = MSCMakeCallHandle(vm, "-(_)");
    MSCHandle* subscript = MSCMakeCallHandle(vm, "[_,_]");
    MSCHandle* subscriptSet = MSCMakeCallHandle(vm, "[_,_]=(_)");

    // Different arity.
    MSCEnsureSlots(djuru, 1);
    MSCSetSlotHandle(djuru, 0, callClass);
    MSCCall(djuru, noParams);

    MSCEnsureSlots(djuru, 1);
    MSCSetSlotHandle(djuru, 0, callClass);
    MSCCall(djuru, zero);

    MSCEnsureSlots(djuru, 2);
    MSCSetSlotHandle(djuru, 0, callClass);
    MSCSetSlotDouble(djuru, 1, 1.0);
    MSCCall(djuru, one);

    MSCEnsureSlots(djuru, 3);
    MSCSetSlotHandle(djuru, 0, callClass);
    MSCSetSlotDouble(djuru, 1, 1.0);
    MSCSetSlotDouble(djuru, 2, 2.0);
    MSCCall(djuru, two);

    // Operators.
    MSCEnsureSlots(djuru, 1);
    MSCSetSlotHandle(djuru, 0, callClass);
    MSCCall(djuru, unary);

    MSCEnsureSlots(djuru, 2);
    MSCSetSlotHandle(djuru, 0, callClass);
    MSCSetSlotDouble(djuru, 1, 1.0);
    MSCCall(djuru, binary);

    MSCEnsureSlots(djuru, 3);
    MSCSetSlotHandle(djuru, 0, callClass);
    MSCSetSlotDouble(djuru, 1, 1.0);
    MSCSetSlotDouble(djuru, 2, 2.0);
    MSCCall(djuru, subscript);

    MSCEnsureSlots(djuru, 4);
    MSCSetSlotHandle(djuru, 0, callClass);
    MSCSetSlotDouble(djuru, 1, 1.0);
    MSCSetSlotDouble(djuru, 2, 2.0);
    MSCSetSlotDouble(djuru, 3, 3.0);
    MSCCall(djuru, subscriptSet);

    // Returning a value.
    MSCHandle* getValue = MSCMakeCallHandle(vm, "getValue()");
    MSCEnsureSlots(djuru, 1);
    MSCSetSlotHandle(djuru, 0, callClass);
    MSCCall(djuru, getValue);
    printf("slots after call: %d\n", MSCGetSlotCount(djuru));
    MSCHandle* value = MSCGetSlotHandle(djuru, 0);

    // Different argument types.
    MSCEnsureSlots(djuru, 3);
    MSCSetSlotHandle(djuru, 0, callClass);
    MSCSetSlotBool(djuru, 1, true);
    MSCSetSlotBool(djuru, 2, false);
    MSCCall(djuru, two);

    MSCEnsureSlots(djuru, 3);
    MSCSetSlotHandle(djuru, 0, callClass);
    MSCSetSlotDouble(djuru, 1, 1.2);
    MSCSetSlotDouble(djuru, 2, 3.4);
    MSCCall(djuru, two);

    MSCEnsureSlots(djuru, 3);
    MSCSetSlotHandle(djuru, 0, callClass);
    MSCSetSlotString(djuru, 1, "string");
    MSCSetSlotString(djuru, 2, "another");
    MSCCall(djuru, two);

    MSCEnsureSlots(djuru, 3);
    MSCSetSlotHandle(djuru, 0, callClass);
    MSCSetSlotNull(djuru, 1);
    MSCSetSlotHandle(djuru, 2, value);
    MSCCall(djuru, two);

    // Truncate a string, or allow null bytes.
    MSCEnsureSlots(djuru, 3);
    MSCSetSlotHandle(djuru, 0, callClass);
    MSCSetSlotBytes(djuru, 1, "string", 3);
    MSCSetSlotBytes(djuru, 2, "b\0y\0t\0e", 7);
    MSCCall(djuru, two);

    // Call ignores with extra temporary slots on stack.
    MSCEnsureSlots(djuru, 10);
    MSCSetSlotHandle(djuru, 0, callClass);
    for (int i = 1; i < 10; i++)
    {
        MSCSetSlotDouble(djuru, i, i * 0.1);
    }
    MSCCall(djuru, one);

    MSCReleaseHandle(djuru, callClass);
    MSCReleaseHandle(djuru, noParams);
    MSCReleaseHandle(djuru, zero);
    MSCReleaseHandle(djuru, one);
    MSCReleaseHandle(djuru, two);
    MSCReleaseHandle(djuru, getValue);
    MSCReleaseHandle(djuru, value);
    MSCReleaseHandle(djuru, unary);
    MSCReleaseHandle(djuru, binary);
    MSCReleaseHandle(djuru, subscript);
    MSCReleaseHandle(djuru, subscriptSet);

    return 0;
}