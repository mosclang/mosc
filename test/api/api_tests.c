//
// Created by Mahamadou DOUMBIA [OML DSI] on 08/02/2024.
//
#include "api_tests.h"
#include "call.h"
#include "benchmark.h"
#include "call_calls_extern.h"
static const char *testName = NULL;

MSCExternMethodFn apiTestBindForeignMethod(
        MVM *vm, const char *module, const char *className,
        bool isStatic, const char *signature) {
    if (strncmp(module, "./test/", 7) != 0) return NULL;
    char fullName[256];
    fullName[0] = '\0';
    if (isStatic) strcat(fullName, "dialen ");
    strcat(fullName, className);
    strcat(fullName, ".");
    strcat(fullName, signature);

    MSCExternMethodFn method = NULL;


    method = callCallsExternBindMethod(fullName);
    if (method != NULL) return method;

    method = benchmarkBindMethod(fullName);
    if (method != NULL) return method;
    /*method = errorBindMethod(fullName);
    if (method != NULL) return method;

    method = getVariableBindMethod(fullName);
    if (method != NULL) return method;

    method = foreignClassBindMethod(fullName);
    if (method != NULL) return method;

    method = handleBindMethod(fullName);
    if (method != NULL) return method;

    method = listsBindMethod(fullName);
    if (method != NULL) return method;

    method = mapsBindMethod(fullName);
    if (method != NULL) return method;

    method = newVMBindMethod(fullName);
    if (method != NULL) return method;

    method = resolutionBindMethod(fullName);
    if (method != NULL) return method;

    method = slotsBindMethod(fullName);
    if (method != NULL) return method;

    method = userDataBindMethod(fullName);
    if (method != NULL) return method;
*/
    fprintf(stderr,
            "Unknown foreign method '%s' for test '%s'\n", fullName, testName);
    exit(1);
    return NULL;
}

MSCExternClassMethods apiTestBindForeignClass(MVM *vm, const char *module, const char *className) {
    MSCExternClassMethods methods = {NULL, NULL};
    if (strncmp(module, "./test/api", 7) != 0) return methods;

    /*externClassBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;

    resetStackAfterExternConstructBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;

    slotsBindClass(className, &methods);
    if (methods.allocate != NULL) return methods;*/

    fprintf(stderr,
            "Unknown foreign class '%s' for test '%s'\n", className, testName);
    exit(1);
}

int runAPITests(MVM *vm, const char *inTestName) {
    testName = inTestName;
    if (strstr(inTestName, "/call.msc") != NULL) {
        return callRunTests(vm);
    } else if (strstr(inTestName, "/call_calls_extern.msc") != NULL) {
        return callCallsExternRunTests(vm);
    }
    /*else if (strstr(inTestName, "/call_msc_call_root.msc") != NULL) {
        return callMSCCallRootRunTests(vm);
    } else if (strstr(inTestName, "/reset_stack_after_call_abort.msc") != NULL) {
        return resetStackAfterCallAbortRunTests(vm);
    } else if (strstr(inTestName, "/reset_stack_after_extern_construct.msc") != NULL) {
        return resetStackAfterExternConstructRunTests(vm);
    }*/

    return 0;
}