#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "object.h"
#include "memory.h"
#include "value.h"
#include "vm.h"

void initValueArray(ValueArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void writeValueArray(ValueArray* array, Value value) {
  if (array->capacity < (array->count + 1)) {
    int oldCapacity = array->capacity;
    array->capacity = GROW_CAPACITY(oldCapacity);
    array->values = GROW_ARRAY(Value, array->values,
                               oldCapacity, array->capacity);
  }

  array->values[array->count] = value;
  array->count++;
}

void freeValueArray(ValueArray* array) {
  FREE_ARRAY(Value, array->values, array->capacity);
  initValueArray(array);
}

void printValue(Value value) {
    switch (value.type) {
        case VAL_BOOL: printf(AS_BOOL(value) ? "true" : "false"); break;
        case VAL_NONE: printf("none"); break;
        case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
        case VAL_INTEGER: printf("%ld", AS_INTEGER(value)); break;
        case VAL_OBJ: printObject(value); break;
        case VAL_EMPTY: printf("<empty>"); break;

        default: break;
  }
}

bool valuesEqual(Value a, Value b) {
    if (IS_INTEGER(a) && IS_DOUBLE(b)) {
        fprintf(stdout, "a: %f, b: %f\n", AS_NUMBER(a), AS_NUMBER(b));
        return (double)AS_INTEGER(a) == AS_NUMBER(b);
    } else if (IS_DOUBLE(a) && IS_INTEGER(b)) {
        fprintf(stdout, "a: %f, b: %f\n", AS_NUMBER(a), AS_NUMBER(b));
        return AS_NUMBER(a) == (double)AS_INTEGER(b);
    }

    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_BOOL:    return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NONE:    return true;
        case VAL_NUMBER:  return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_INTEGER: return AS_INTEGER(a) == AS_INTEGER(b);
        case VAL_OBJ:     return AS_OBJ(a) == AS_OBJ(b);
        case VAL_EMPTY:   return true;
        default:          return false;
    }
}

static uint32_t hashInt(ulong value) {
    value = ((value >> 16) ^ value) * 0x45d9f3b;
    value = ((value >> 16) ^ value) * 0x45d9f3b;
    value = (value >> 16) ^ value;
    return (uint32_t)value;
}

static uint32_t hashDouble(double value) {
    union BitCast {
        double value;
        uint32_t ints[2];
    };

    union BitCast cast;
    cast.value = value + 1.0;
    return cast.ints[0] + cast.ints[1];
}

uint32_t hashValue(Value value) {
    switch (value.type) {
        case VAL_BOOL:    return AS_BOOL(value) ? 3 : 5;
        case VAL_NONE:    return 7;
        case VAL_NUMBER:  return hashDouble(AS_NUMBER(value));
        case VAL_INTEGER: return hashInt(AS_INTEGER(value));
        case VAL_OBJ:     return AS_STRING(value)->hash;
        case VAL_EMPTY:   return 0;
        default:          return 0;
    }
}

ObjString* asString(Value value) {
    switch (value.type) {
        case VAL_BOOL: {
            bool v = AS_BOOL(value);
            if (v) {
                return copyString("true", 4);
            } else {
                return copyString("false", 5);
            }
        }
        case VAL_NONE: return copyString("none", 4);
        case VAL_INTEGER: {
            ulong v = AS_INTEGER(value);
            char* chars = malloc(32);
            int length = snprintf(chars, 32, "%ld", v);
            ObjString* str = copyString(chars, length);
            free(chars);
            return str;
        }
        case VAL_NUMBER: {
            double v = AS_NUMBER(value);
            char* chars = malloc(32);
            int length = snprintf(chars, 32, "%g", v);
            ObjString* str = copyString(chars, length);
            free(chars);
            return str;
        }
        case VAL_OBJ: return AS_STRING(value);
        default: runtimeError("Invalid value to asString\n"); break;
    }
    return AS_STRING(OBJ_VAL(""));
}

Value asBool(Value value) {
    switch (value.type) {
        case VAL_BOOL: return value;
        case VAL_NONE: runtimeError("Cannot convert none to bool"); break;
        case VAL_INTEGER: {
            ulong v = AS_INTEGER(value);
            if (v > 0) {
                return BOOL_VAL(true);
            } else {
                return BOOL_VAL(false);
            }
        }
        case VAL_NUMBER: {
            double v = AS_NUMBER(value);
            if (v > 0) {
                if ((v - (int)v) != 0) {
                    return BOOL_VAL(false);
                } else {
                    return BOOL_VAL(true);
                }
                return BOOL_VAL(true);
            } else {
                return BOOL_VAL(false);
            }
        }
        case VAL_OBJ: {
            const ObjString* str = AS_STRING(value);
            if (strcmp(str->chars, "true") == 0) return BOOL_VAL(true);
            if (strcmp(str->chars, "false") == 0) return BOOL_VAL(false);
            if (str->length == 1) {
                return BOOL_VAL(false);
            } else {
                return BOOL_VAL(true);
            }
        }
        default: runtimeError("Invalid value to asBool\n"); break;
    }
    return BOOL_VAL(false);
}
