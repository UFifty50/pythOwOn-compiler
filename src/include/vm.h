#ifndef pythowon_vm_h
#define pythowon_vm_h

#include "chunk.h"


typedef struct {
    Chunk* chunk;
    uint8_t* ip;
    Value* stack;
    int stackCount;
    int stackCapacity;
    Obj* objects;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char* source);
void runtimeError(const char* format, ...);
void push(Value value);
Value pop();
Value peek(int distance);

#endif
