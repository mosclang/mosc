//
// Created by Mahamadou DOUMBIA [OML DSI] on 03/02/2022.
//

#include "../src/api/msc.h"

static MVM* vm = NULL;

static void print(MVM *_, const char *text) {
    printf("%s", text);
}

static bool errorPrint(MVM *vm, MSCError type, const char *module_name, int line, const char *message) {
    printf("Error at %s > %d: %s\n", module_name, line, message);
    return true;
}

static const char *readSource(const char *name) {
    FILE *file = fopen(name, "r");
    const char *buffer = NULL;
    if (file) {
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        fseek (file, 0, SEEK_SET);
        buffer = malloc((size_t) size);
        if (buffer) {
            fread((void *) buffer, 1, (size_t) size, file);
        }
        fclose(file);
    }
    return buffer;
}


static MVM* initVM() {

    MSCConfig config;
    MSCInitConfig(&config);
    config.errorHandler = errorPrint;
    config.writeFn = print;

   return MSCNewVM(&config);
}

//The endpoint we call from the browser
int mosc_compile(const char* input) {
    MVM* vm = initVM();
    MSCInterpretResult result = MSCInterpret(vm, "compile", input);
    MSCFreeVM(vm);
    return (int)result;
}

//Main not used, but required. We call mosc_compile directly.
int main(int argc, const char* argv[]) {
    return 0;
}