#ifndef pythowon_chunk_h
#define pythowon_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_NONE,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_DEF_GLOBAL,
    OP_SET_GLOBAL,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_LEFTSHIFT,
    OP_RIGHTSHIFT,
    OP_MODULO,
    OP_NEGATE,
    OP_PRINT,
    OP_JUMP,
    OP_JUMP_FALSE,
    OP_JUMP_LONG,
    OP_JUMP_FALSE_LONG,
    OP_LOOP,
    OP_LOOP_LONG,
    OP_DUP,
    OP_CALL,
    OP_RETURN,
} OpCode;

typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
void writeConstant(int index, Chunk* chunk, int line);
int addConstant(Chunk* chunk, Value value);

#endif
