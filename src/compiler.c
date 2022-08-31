#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"
#include "object.h"
#include "value.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =, +=, /=, <<=, ... You get the point
 //   PREC_CONDITIONAL, // ?:
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // ==, !=
    PREC_COMPARISON,  // <, >, <=, >=
    PREC_SHIFT,       // <<, >>
    PREC_TERM,        // +, -
    PREC_FACTOR,      // *, /, %
    PREC_UNARY,       // !, -
    PREC_CALL,        // ., ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct {
    Token name;
    int depth;
} Local;

typedef struct {
    Local locals[UINT8_MAX+1];     //TODO: increase to UINT16_MAX
    int localCount;
    int scopeDepth;
} Compiler;

Parser parser;
Compiler* current = NULL;
Chunk* compilingChunk;
Table stringConstants;

static Chunk* currentChunk(void) {
    return compilingChunk;
}

static void errorAt(const Token* token, const char* message) {
    if (parser.panicMode) return;
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // empty
    } else {
        fprintf(stderr, " at '%*.s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void error(const char* message) {
    errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char* message) {
    errorAt(&parser.current, message);
}

static void advance(void) {
    parser.previous = parser.current;

    for (;;) {
        parser.current = scanToken();
        if(parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

static bool check(TokenType type) {
    return parser.current.type == type;
}

static bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void emitReturn(void) {
    emitByte(OP_RETURN);
}

static uint8_t emitConstant(Value value) {
    int index = addConstant(currentChunk(), value);
    if (index > UINT16_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    } else {
        writeConstant(index, currentChunk(), parser.previous.line);
        return (uint8_t)index;
    }
}

static void initCompiler(Compiler* compiler) {
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    current = compiler;
}

static void endCompiler(void) {
    emitReturn();
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif
}

static void beginScope(void) {
    current->scopeDepth++;
}

static void endScope(void) {
    current->scopeDepth--;

    while (current->localCount > 0 &&
           current->locals[current->localCount - 1].depth >
               current->scopeDepth) {
        emitByte(OP_POP);
        current->localCount--;
    }
}

static void expression(void);
static void statement(void);
static void declaration(void);
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static uint8_t identifierConstant(const Token* name) {   //could be more efficient but we have 65535 slots per chunk
    Value string = OBJ_VAL(copyString(name->start, name->length));
    Value indexValue;
    if (tableGet(&stringConstants, string, &indexValue)) {
        pop();
    }

    uint8_t index = emitConstant(string);
    tableSet(&stringConstants, string, NUMBER_VAL((double)index));
    return index;
}

static bool identifiersEqual(const Token* a, const Token* b) {
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(const Compiler* compiler, const Token* name) {
      for (int i = compiler->localCount - 1; i >= 0; i--) {
        const Local* local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                error("Cannot read a local variable from within it's own initializer.");
            }
            return i;
        }
    }

    return -1;
}

static void addLocal(Token name) {
    if (current->localCount == (UINT8_MAX+1)) {
        error("Too many local variables in function.");
        return;
    }

    Local* local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
}

static void declareVariable(void) {
    if (current->scopeDepth == 0) return;

    const Token* name = &parser.previous;
    for (int i = current->localCount - 1; i >= 0; i--) {
        const Local* local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }

    addLocal(*name);
}

static uint8_t parseVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();
    if (current->scopeDepth > 0) return 0;

    return identifierConstant(&parser.previous);
}

static void markInitialized(void) {
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t global) {
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }

    emitBytes(OP_DEF_GLOBAL, global);
}

static void binary(bool canAssign) {
    TokenType opType = parser.previous.type;
    const ParseRule* rule = getRule(opType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (opType) {
        case TOKEN_EXCLAM_EQ:    emitBytes(OP_EQUAL, OP_NOT); break;
        case TOKEN_EQ_EQ:   emitByte(OP_EQUAL); break;
        case TOKEN_GREATER:       emitByte(OP_GREATER); break;
        case TOKEN_GREATER_EQ: emitBytes(OP_LESS, OP_NOT); break;
        case TOKEN_LESS:          emitByte(OP_LESS); break;
        case TOKEN_LESS_EQ:    emitBytes(OP_GREATER, OP_NOT); break;
        case TOKEN_PLUS: emitByte(OP_ADD); break;
        case TOKEN_MINUS: emitByte(OP_SUBTRACT); break;
        case TOKEN_STAR: emitByte(OP_MULTIPLY); break;
        case TOKEN_SLASH: emitByte(OP_DIVIDE); break;
        case TOKEN_PERCENT: emitByte(OP_MODULO); break;
        case TOKEN_LSHIFT: emitByte(OP_LEFTSHIFT); break;
        case TOKEN_RSHIFT: emitByte(OP_RIGHTSHIFT); break;
        default: return;
    }
}

static void literal(bool canAssign) {
    switch (parser.previous.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_TRUE: emitByte(OP_TRUE); break;
        case TOKEN_NONE: emitByte(OP_NONE); break;
        default: break;
    }
}

static void grouping(bool canAssign) {
    expression();
    consume(TOKEN_RPAREN, "Expect ')' after expression.");
}

static void number(bool canAssign) {
    short dots = 0;
    for (int i = 0; i < (parser.previous.length+1); i++) {
        if (parser.previous.start[i] == '.') {
            dots++;
        }
    }
    if (dots == 0) {
        ulong value = strtoul(parser.previous.start, NULL, 10);
        emitConstant(INTEGER_VAL(value));
    } else if (dots == 1) {
        double value = strtod(parser.previous.start, NULL);
        emitConstant(NUMBER_VAL(value));
        fprintf(stdout, "double: %g\n", value);
    } else {
        error("Numbers may only have one decimal point.");
    }
}

static void string(bool canAssign) {
    emitConstant(OBJ_VAL(newString(parser.previous.start,
                                    parser.previous.length)));
}

static void namedVariable(Token name, bool canAssign) {
    uint8_t getOp;
    uint8_t setOp;
    int arg = resolveLocal(current, &name);
    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }
    if (canAssign && match(TOKEN_EQ)) {
        expression();
        emitBytes(setOp, (uint8_t)arg);
    } else {
        emitBytes(getOp, (uint8_t)arg);
    }
}

static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

static void unary(bool canAssign) {
    TokenType operatorType = parser.previous.type;

    parsePrecedence(PREC_UNARY);

    switch (operatorType) {
        case TOKEN_EXCLAM: emitByte(OP_NOT); break;
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return;
    }
}

ParseRule rules[] = {  //    prefix   | infix  |  precedence
    [TOKEN_LPAREN]        = {grouping,  NULL,     PREC_NONE},
    [TOKEN_RPAREN]        = {NULL,      NULL,     PREC_NONE},
    [TOKEN_LBRACE]        = {NULL,      NULL,     PREC_NONE},
    [TOKEN_RBRACE]        = {NULL,      NULL,     PREC_NONE},
    [TOKEN_LBRACK]        = {NULL,      NULL,     PREC_NONE},
    [TOKEN_RBRACK]        = {NULL,      NULL,     PREC_NONE},
    [TOKEN_COMMA]         = {NULL,      NULL,     PREC_NONE},
    [TOKEN_DOT]           = {NULL,      NULL,     PREC_NONE},
    [TOKEN_MINUS]         = {unary,     binary,   PREC_TERM},
    [TOKEN_PLUS]          = {NULL,      binary,   PREC_TERM},
    [TOKEN_SEMI]          = {NULL,      NULL,     PREC_NONE},
    [TOKEN_SLASH]         = {NULL,      binary,   PREC_FACTOR},
    [TOKEN_STAR]          = {NULL,      binary,   PREC_FACTOR},
    [TOKEN_PERCENT]       = {NULL,      binary,   PREC_FACTOR},
    [TOKEN_EXCLAM]        = {unary,     NULL,     PREC_NONE},
    [TOKEN_EXCLAM_EQ]     = {NULL,      binary,   PREC_EQUALITY},
    [TOKEN_EQ]            = {NULL,      NULL,     PREC_NONE},
    [TOKEN_EQ_EQ]         = {NULL,      binary,   PREC_COMPARISON},
    [TOKEN_GREATER]       = {NULL,      binary,   PREC_COMPARISON},
    [TOKEN_GREATER_EQ]    = {NULL,      binary,   PREC_COMPARISON},
    [TOKEN_LESS]          = {NULL,      binary,   PREC_COMPARISON},
    [TOKEN_LESS_EQ]       = {NULL,      binary,   PREC_COMPARISON},
    [TOKEN_LSHIFT]        = {NULL,      binary,   PREC_SHIFT},
    [TOKEN_RSHIFT]        = {NULL,      binary,   PREC_SHIFT},
    [TOKEN_IDENTIFIER]    = {variable,  NULL,     PREC_NONE},
    [TOKEN_STR]           = {string,    NULL,     PREC_NONE},
    [TOKEN_NUM]           = {number,    NULL,     PREC_NONE},
    [TOKEN_AND]           = {NULL,      NULL,     PREC_NONE},
    [TOKEN_CLASS]         = {NULL,      NULL,     PREC_NONE},
    [TOKEN_ELSE]          = {NULL,      NULL,     PREC_NONE},
    [TOKEN_FALSE]         = {literal,   NULL,     PREC_NONE},
    [TOKEN_FOR]           = {NULL,      NULL,     PREC_NONE},
    [TOKEN_DEF]           = {NULL,      NULL,     PREC_NONE},
    [TOKEN_IF]            = {NULL,      NULL,     PREC_NONE},
    [TOKEN_NONE]          = {literal,   NULL,     PREC_NONE},
    [TOKEN_OR]            = {NULL,      NULL,     PREC_NONE},
    [TOKEN_PRINT]         = {NULL,      NULL,     PREC_NONE},
    [TOKEN_RETURN]        = {NULL,      NULL,     PREC_NONE},
    [TOKEN_SUPER]         = {NULL,      NULL,     PREC_NONE},
    [TOKEN_THIS]          = {NULL,      NULL,     PREC_NONE},
    [TOKEN_TRUE]          = {literal,   NULL,     PREC_NONE},
    [TOKEN_VAR]           = {NULL,      NULL,     PREC_NONE},
    [TOKEN_WHILE]         = {NULL,      NULL,     PREC_NONE},
    [TOKEN_EXTENDS]       = {NULL,      NULL,     PREC_NONE},
    [TOKEN_ERROR]         = {NULL,      NULL,     PREC_NONE},
    [TOKEN_EOF]           = {NULL,      NULL,     PREC_NONE},
};

static void parsePrecedence(Precedence prec) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }

    bool canAssign = prec <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (prec <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && match(TOKEN_EQ)) {
        error("Invalid assignment target.");
    }
}

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

static void expression(void) {
    parsePrecedence(PREC_ASSIGNMENT);
}

static void block(void) {
    while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RBRACE, "Expected '}' at end of block.");
}

static void expressionStatement(void) {
    expression();
    consume(TOKEN_SEMI, "Expected ';' after expression.");
    emitByte(OP_POP);
}

static void varDeclaration(void) {
    uint8_t global = parseVariable("Expect variable name.");

    if (match(TOKEN_EQ)) {
        expression();
    } else {
        emitByte(OP_NONE);
    }

    consume(TOKEN_SEMI, "Expected ';' after variable declaration.");

    defineVariable(global);
}

static void printStatement(void) {
    expression();
    consume(TOKEN_SEMI, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

static void synchronize(void) {
    parser.panicMode = false;

    while (parser.previous.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMI) return;
        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_DEF:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN: return;
            default: ;
        }

        advance();
    }
}

static void declaration(void) {
    if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }

    if (parser.panicMode) synchronize();
}

static void statement(void) {
    if (match(TOKEN_PRINT)) {
        printStatement();
    } else if (match(TOKEN_LBRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}

bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    Compiler compiler;
    initCompiler(&compiler);
    compilingChunk = chunk;
    parser.hadError = false;
    parser.panicMode = false;
    initTable(&stringConstants);

    advance();

    while (!match(TOKEN_EOF)) {
        declaration();
    }

    endCompiler();
    freeTable(&stringConstants);
    return !parser.hadError;
}
