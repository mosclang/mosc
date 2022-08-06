//
// Created by Mahamadou DOUMBIA [OML DSI] on 03/02/2022.
//

#include "../src/api/msc.h"
#include "../src/memory/Value.h"
#include <stdlib.h>


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


int main() {
    // printf("%zu, %zu", sizeof(Djuru),sizeof(Class));
    const char *text = readSource("../test/examples/when_sample2.msc");
    if(text == NULL) {
        printf("Failed to read file");
        return 0;
    }

    MSCConfig config;
    MSCInitConfig(&config);
    config.errorHandler = errorPrint;
    config.writeFn = print;

    MVM *vm = MSCNewVM(&config);
    MSCInterpret(vm, "scritp", text);
    MSCFreeVM(vm);
    free((void *) text);
    return 0;
}