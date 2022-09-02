#ifndef pythowon_vm_h
#define pythowon_vm_h

#include "chunk.h"
#include "table.h"
#include "object.h"

#define FRAMES_MAX 255

typedef struct {
    ObjFunction* function;
    uint8_t* ip;
    Value* slots;
} CallFrame;

typedef struct {
    CallFrame frames[FRAMES_MAX];
    int frameCount;
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
void runtimeError(const char* errorType, const char* format, ...);
void push(Value value);
Value pop(void);
Value peek(int distance);

#endif
