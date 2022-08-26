#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "compiler.h"
#include "memory.h"
#include "common.h"
#include "object.h"
#include "debug.h"
#include "vm.h"

VM vm;

static void resetStack() {
    vm.stackCount = 0;
}

void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}

void initVM() {
    vm.stack = NULL;
    vm.stackCapacity = 0;
    resetStack();
    vm.objects = NULL;
}

void freeVM() {
    freeObjects();
}

void push(Value value) {
    if (vm.stackCapacity < vm.stackCount + 1) {
        int oldCapacity = vm.stackCapacity;
        vm.stackCapacity = GROW_CAPACITY(oldCapacity);
        vm.stack = GROW_ARRAY(Value, vm.stack, oldCapacity, vm.stackCapacity);
    }
    vm.stack[vm.stackCount] = value;
    vm.stackCount++;
}

Value pop() {
    vm.stackCount--;
    return vm.stack[vm.stackCount];
}

Value peek(int distance) {
    return vm.stack[vm.stackCount - 1 - distance];
}

static bool isFalsey(Value value) {
    if (value.type == VAL_NUMBER) return AS_NUMBER(value) < 0 ? true : false;
    return IS_NONE(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenateString() {
    char* b = asString(pop())->chars;
    char* a = asString(pop())->chars;

    int length = (int)(strlen(a) + strlen(b) + 1);
    char* c = strcat(a, b);

    push(OBJ_VAL(copyString(c, length)));
}

static void multiplyString() {
    int a = (int)AS_NUMBER(pop());
    fprintf(stdout, "a: %d\n", a);
    char* b = asString(pop())->chars;
    fprintf(stdout, "b: %s\n", b);
    char* c = malloc(a * strlen(b) + 1);
    for (int i = 0; i < a; i++) {
        strcat(c, b);
    }

    c[a * strlen(b)] = '\0';
    fprintf(stdout, "c: %s\n", c);
    push(OBJ_VAL(copyString(c, a * strlen(b) + 1)));
    free(c);
}

/*
    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length-1, b->chars, b->length);
    chars[length] = '\0';

    fprintf(stdout, "Concatenated %s\n", chars);

    ObjString* result = takeString(chars, length);
    push(OBJ_VAL(result)); */


static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(valueType, op) \
    do { \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        double b = AS_NUMBER(pop()); \
        double a = AS_NUMBER(pop()); \
        push(valueType(a op b)); \
    } while (false)
#define BINARY_OP_INT(op) \
    do { \
        if (!IS_INTEGER(peek(0)) || !IS_INTEGER(peek(1))) { \
            runtimeError("Operands must be Integers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        unsigned long b = AS_INTEGER(pop()); \
        unsigned long a = AS_INTEGER(pop()); \
        push(INTEGER_VAL(a op b)); \
    } while (false)             //TODO: handle invalid values better

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value* slot = vm.stack; slot < vm.stack + vm.stackCount; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_NONE: push(NONE_VAL); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
            case OP_FALSE: push(BOOL_VAL(false)); break;
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER: BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS: BINARY_OP(BOOL_VAL, <); break;
            case OP_ADD: {
                if (IS_STRING(peek(0)) || IS_STRING(peek(1))) {
                    concatenateString();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else {
                    runtimeError(
                        "Operands must be two numbers or one string and one bool/none/number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
            case OP_MULTIPLY: {
                if  (IS_STRING(peek(1))) {
                    multiplyString();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    BINARY_OP(NUMBER_VAL, *);
                }
                break;
            }
            case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;
            case OP_NOT: push(BOOL_VAL(isFalsey(pop()))); break;
            case OP_LEFTSHIFT: BINARY_OP_INT(<<); break;
            case OP_RIGHTSHIFT: BINARY_OP_INT(>>); break;
            case OP_MODULO: BINARY_OP_INT(%); break;
            case OP_NEGATE: {
                if(!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            }
            case OP_RETURN: {
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
            }
            default:
                return INTERPRET_RUNTIME_ERROR;
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
#undef BINARY_OP_INT
}

InterpretResult interpret(const char* source) {
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = run();

    freeChunk(&chunk);
    return result;
}
