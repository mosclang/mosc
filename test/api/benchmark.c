//
// Created by Mahamadou DOUMBIA [OML DSI] on 14/02/2024.
//

#include <string.h>
#include <time.h>

#include "benchmark.h"
#include "../../src/memory/Value.h"
#include "../../src/runtime/MVM.h"

static void arguments(Djuru* djuru)
{
    double result = 0;

    result += MSCGetSlotDouble(djuru, 1);
    result += MSCGetSlotDouble(djuru, 2);
    result += MSCGetSlotDouble(djuru, 3);
    result += MSCGetSlotDouble(djuru, 4);

    MSCSetSlotDouble(djuru, 0, result);
}

const char* testScript =
        "kulu Test {\n"
                "  dialen method(a, b, c, d) { a + b + c + d }\n"
                "}\n";

static void call(Djuru* djuru)
{
    int iterations = (int)MSCGetSlotDouble(djuru, 1);

    // Since the VM is not re-entrant, we can't call from within this foreign
    // method. Instead, make a new VM to run the call test in.


    MSCHandle* method = MSCMakeCallHandle(djuru->vm, "method(_,_,_,_)");

    MSCEnsureSlots(djuru, 1);
    MSCGetVariable(djuru, "./test/benchmark/api_reentrancy", "Test", 0);
    MSCHandle* testClass = MSCGetSlotHandle(djuru, 0);

    double startTime = (double)clock() / CLOCKS_PER_SEC;

    double result = 0;
    for (int i = 0; i < iterations; i++)
    {
        MSCEnsureSlots(djuru, 5);
        MSCSetSlotHandle(djuru, 0, testClass);
        MSCSetSlotDouble(djuru, 1, 1.0);
        MSCSetSlotDouble(djuru, 2, 2.0);
        MSCSetSlotDouble(djuru, 3, 3.0);
        MSCSetSlotDouble(djuru, 4, 4.0);
        // printf("Djuru state:::before call %p from %p -- %p\n", djuru, djuru->caller, djuru->vm->djuru);

        MSCInterpretResult res =  MSCCall(djuru, method);
        if(MSCHasError(djuru)) {
            MSCSetSlotDouble(djuru, 0, 0);
            MSCReleaseHandle(djuru->vm, testClass);
            MSCReleaseHandle(djuru->vm, method);
            return;
        }
        // printf("Djuru state::: after call %d %p ==> %d\n", djuru->state, djuru->vm->djuru, res);
        result += MSCGetSlotDouble(djuru, 0);
    }

    double elapsed = (double)clock() / CLOCKS_PER_SEC - startTime;

    MSCReleaseHandle(djuru->vm, testClass);
    MSCReleaseHandle(djuru->vm, method);

    if (result == (1.0 + 2.0 + 3.0 + 4.0) * iterations)
    {
        MSCSetSlotDouble(djuru, 0, elapsed);
    }
    else
    {
        // Got the wrong result.
        MSCSetSlotBool(djuru, 0, false);
    }
}

MSCExternMethodFn benchmarkBindMethod(const char* signature)
{
    if (strcmp(signature, "dialen Benchmark.arguments(_,_,_,_)") == 0) return arguments;
    if (strcmp(signature, "dialen Benchmark.call(_)") == 0) return call;

    return NULL;
}