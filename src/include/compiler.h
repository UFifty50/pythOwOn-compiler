#ifndef pythowon_compiler_h
#define pythowon_compiler_h

#include "vm.h"
#include "object.h"
#include "scanner.h"

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

extern Parser parser;

bool compile(const char* source, Chunk* chunk);

#endif
