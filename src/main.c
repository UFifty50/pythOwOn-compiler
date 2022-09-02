#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

void sigCtrlC(int signum);

static void repl(void) {
    char line[1024];
    for (;;) {
        printf("PythOwOn <<< ");

   //     signal(SIGINT, sigCtrlC);

         if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
    //        break;
        }

        interpret(line);
    }
}

void sigCtrlC(int signum) {
    printf("Ctrl-C detected, do you want to exit (y/n)? ");
    int c = getchar();
    if (c == 'y') {
        exit(1);
    } else {
        printf("Ok, continuing...\n");
        repl();
    }
}

static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

static void runFile(const char* path) {
    char* source = readFile(path);
    InterpretResult result = interpret(source);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);  //TODO: Error messages/stacktrace
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, const char* argv[]) {
    initVM();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
    }

    freeVM();
    return 0;
}
