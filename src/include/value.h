#ifndef pythowon_value_h
#define pythowon_value_h

#include <stdio.h>

#include "common.h"
#include "scanner.h"

#define IS_BOOL(value)      ((value).type == VAL_BOOL)
#define IS_NONE(value)      ((value).type == VAL_NONE)
#define IS_DOUBLE(value)    ((value).type == VAL_NUMBER)
#define IS_NUMBER(value)    ((value).type == VAL_NUMBER || \
                             (value).type == VAL_INTEGER)
#define IS_INTEGER(value)   ((value).type == VAL_INTEGER)
#define IS_OBJ(value)       ((value).type == VAL_OBJ)
#define IS_EMPTY(value)     ((value).type == VAL_EMPTY)

#define AS_BOOL(value)      ((value).as.boolean)
#define AS_NUMBER(value)    ((value).as.number)
#define AS_INTEGER(value)   ((value).as.integer)
#define AS_OBJ(value)       ((value).as.obj)

#define BOOL_VAL(value)     ((Value){VAL_BOOL, {.boolean = value}})
#define NONE_VAL            ((Value){VAL_NONE, {.integer = 0}})
#define NUMBER_VAL(value)   ((Value){VAL_NUMBER, {.number = value}})
#define INTEGER_VAL(value)  ((Value){VAL_INTEGER, {.integer = value}})
#define OBJ_VAL(object)     ((Value){VAL_OBJ, {.obj = (Obj*)object}})
#define EMPTY_VAL           ((Value){VAL_EMPTY, {.integer = 0}})

typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef enum {
    VAL_BOOL,
    VAL_NONE,
    VAL_NUMBER,
    VAL_INTEGER,
    VAL_OBJ,
    VAL_EMPTY
} ValueType;

typedef struct {
    ValueType type;
    union {
        ulong integer;
        bool boolean;
        double number;
        Obj* obj;
    } as;
    // TODO: maybe implement `ObjString* (*asString)();`?
} Value;

#define ISSIGNED(X) _Generic((X), \
                short : true, \
                int : true, \
                long : true, \
                long long : true, \
                unsigned short : false, \
                unsigned int : false, \
                ulong : false, \
                ulong long : false, \
                float : true, \
                double : true, \
                long double : true \
               )

typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);
ObjString* asString(Value value);
Value asBool(Value value);
uint32_t hashValue(Value value);

#endif
