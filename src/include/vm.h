#ifndef pythowon_vm_h
#define pythowon_vm_h

#include "chunk.h"
#include "table.h"

typedef struct {
    Chunk* chunk;
    uint32_t* ip;
    Value* stack;
    int stackCount;
    int stackCapacity;
    Table globals;
    Table strings;
    Obj* objects;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void initVM(void);
void freeVM(void);
InterpretResult interpret(const char* source);
void runtimeError(const char* format, ...);
void push(Value value);
Value pop(void);
Value peek(int distance);

#endif
