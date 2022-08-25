#ifndef pythowon_value_h
#define pythowon_value_h

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef enum {
    VAL_BOOL,
    VAL_NONE,
    VAL_NUMBER,
    VAL_INTEGER,
    VAL_OBJ
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        unsigned long integer;
        Obj* obj;
    } as;
} Value;

#define IS_BOOL(value)      ((value).type == VAL_BOOL)
#define IS_NONE(value)      ((value).type == VAL_NONE)
#define IS_NUMBER(value)    ((value).type == VAL_NUMBER || \
                             (value).type == VAL_INTEGER)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)

#define AS_BOOL(value)      ((value).as.boolean)
#define AS_NUMBER(value)    ((value).as.number)
#define AS_INTEGER(value)    ((value).as.integer)
#define AS_OBJ(value)     ((value).as.obj)

#define BOOL_VAL(value)     ((Value){VAL_BOOL, {.boolean = value}})
#define NONE_VAL            ((Value){VAL_NONE, {.number = 0}})
#define NUMBER_VAL(value)   ((Value){VAL_NUMBER, {.number = value}})
#define INTEGER_VAL(value)   ((Value){VAL_INTEGER, {.integer = value}})
#define OBJ_VAL(object)   ((Value){VAL_OBJ, {.obj = (Obj*)object}})

#define ISSIGNED(X) _Generic((X), \
                short : true, \
                int : true, \
                long : true, \
                long long : true, \
                unsigned short : false, \
                unsigned int : false, \
                unsigned long : false, \
                unsigned long long : false, \
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

#endif
