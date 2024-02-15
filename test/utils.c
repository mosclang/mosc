//
// Created by Mahamadou DOUMBIA [OML DSI] on 07/02/2024.
//
#include "utils.h"


int tryHandlingArgs(int argc, const char *argv[]) {
    if (argc < 2) {
        printf("Expected a test file name: `mosct <file>`\n");
        return MSC_TERR_USAGE;
    }
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        printf("Test runner with mosc version %s\n", MSC_VERSION_STRING);
        return 1;
    }
    return 0;
}

char *readSource(const char *path) {
    FILE *file = fopen(path, "rb");
    char *buffer = NULL;
    if (file == NULL) {
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    buffer = malloc((size_t) size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Unable to read the file: \"%s\"", path);
        exit(MSC_TERR_IO);
    }
    size_t bytesRead = fread((void *) buffer, 1, (size_t) size, file);
    if (bytesRead < size) {
        fprintf(stderr, "Unable to read the file: \"%s\"", path);
        exit(MSC_TERR_IO);
    }
    fclose(file);
    buffer[bytesRead] = '\0';
    return buffer;
}

bool errorPrint(MVM *vm, MSCError type, const char *module, int line, const char *message) {
    switch (type) {
        case ERROR_COMPILE:
            fprintf(stderr, "[%s line %d] %s\n", module, line, message);
            break;

        case ERROR_RUNTIME:
            fprintf(stderr, "%s\n", message);
            break;

        case ERROR_STACK_TRACE:
            fprintf(stderr, "[%s line %d] in %s\n", module, line, message);
            break;
    }
}

void print(MVM *_, const char *text) {
    printf("%s", text);
}

void readModuleComplete(MVM *vm, const char *module, MSCLoadModuleResult result) {
    if (result.source) {
        free((void *) result.source);
        result.source = NULL;
    }
}

MSCLoadModuleResult loadModule(MVM *vm, const char *name) {
//source may or may not be null
    MSCLoadModuleResult result = {0};

#ifdef MOSC_TRY
    return result;
#endif

    Path *filePath = pathNew(name);

    // Add a ".wren" file extension.
    pathAppendString(filePath, ".wren");

    char *source = readSource(filePath->chars);
    pathFree(filePath);

    result.source = source;
    result.onComplete = readModuleComplete;
    return result;
}

const char *resolveModule(MVM *vm,
                          const char *importer, const char *module) {
    // Logical import strings are used as-is and need no resolution.
    if (pathType(module) == PATH_TYPE_SIMPLE) return module;

    // Get the directory containing the importing module.
    Path *path = pathNew(importer);
    pathDirName(path);

    // Add the relative import path.
    pathJoin(path, module);

    pathNormalize(path);
    char *resolved = pathToString(path);

    pathFree(path);
    return resolved;
}

MSCInterpretResult runFile(MVM *vm, const char *path) {
    char *source = readSource(path);
    if (source == NULL) {
        fprintf(stderr, "Could not find file \"%s\".\n", path);
        exit(MSC_TERR_NODATA);
    }

    // If it looks like a relative path, make it explicitly relative so that we
    // can distinguish it from logical paths.
    Path *module = pathNew(path);
    if (pathType(module->chars) == PATH_TYPE_SIMPLE) {
        Path *relative = pathNew(".");
        pathJoin(relative, path);

        pathFree(module);
        module = relative;
    }

    pathRemoveExtension(module);
    MSCInterpretResult result = MSCInterpret(vm, module->chars, source);

    pathFree(module);
    free(source);

    return result;
}

bool isModuleAnAPITest(const char *module) {
    if (strncmp(module, "test/api", 8) == 0) return true;
    if (strncmp(module, "test/benchmark", 14) == 0) return true;
    return false;
}