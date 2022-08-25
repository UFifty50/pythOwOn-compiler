#ifndef pythowon_compiler_h
#define pythowon_compiler_h

#include "vm.h"
#include "object.h"

bool compile(const char* source, Chunk* chunk);

#endif
