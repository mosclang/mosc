//
// Created by Mahamadou DOUMBIA [OML DSI] on 03/02/2022.
//


#include "utils.h"
#include "api/api_tests.h"
#include "../src/api/msc.h"
#include "../src/runtime/MVM.h"

static MVM *vm = NULL;


int main(int argc, const char* argv[]) {
    int handled = tryHandlingArgs(argc, argv);
    if(handled) return handled;

    const char *test = argv[1];

    bool apiTest = isModuleAnAPITest(test);
    MSCConfig config;
    MSCInitConfig(&config);
    config.errorHandler = errorPrint;
    config.writeFn = print;
    config.loadModuleFn = loadModule;
    config.resolveModuleFn = resolveModule;
    config.initialHeapSize = 1024 * 1024 * 100;
    if(apiTest) {
        config.bindExternClassFn = apiTestBindForeignClass;
        config.bindExternMethodFn = apiTestBindForeignMethod;
    }

    vm = MSCNewVM(&config);
    MSCInterpretResult result = runFile(vm, test);
    int exitCode = 0;
    if(apiTest) {
        exitCode = runAPITests(vm, test);
    }
    MSCFreeVM(vm);
    if(result == RESULT_COMPILATION_ERROR) return MSC_TERR_COMPILATION;
    if(result == RESULT_RUNTIME_ERROR) return MSC_TERR_SOFTWARE;

    return exitCode;
}