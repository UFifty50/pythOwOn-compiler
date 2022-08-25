#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

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

typedef void (*ParseFn)();

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

Parser parser;
Chunk* compilingChunk;

static Chunk* currentChunk() {
    return compilingChunk;
}

static void errorAt(Token* token, const char* message) {
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

static void advance() {
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

static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void emitReturn() {
    emitByte(OP_RETURN);
}

static void emitConstant(Value value) {
    if (AS_NUMBER(value) <= UINT16_MAX) {
        writeConstant(currentChunk(), value, parser.previous.line);
    } else {
        error("Too many constants in one chunk.");
    }
}

static void endCompiler() {
    emitReturn();
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif
}

static void expression();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static void binary() {
    TokenType opType = parser.previous.type;
    ParseRule* rule = getRule(opType);
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
        default: return;
    }
}

static void literal() {
    switch (parser.previous.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_TRUE: emitByte(OP_TRUE); break;
        case TOKEN_NONE: emitByte(OP_NONE); break;
        default: break;
    }
}

static void grouping() {
    expression();
    consume(TOKEN_RPAREN, "Expect ')' after expression.");
}

static void number() {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

static void string() {
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1,
                                    parser.previous.length - 2))); //TODO: add support for escape sequences
}

static void unary() {
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
    [TOKEN_LSHIFT]        = {NULL,      NULL,     PREC_NONE},
    [TOKEN_RSHIFT]        = {NULL,      NULL,     PREC_NONE},
    [TOKEN_IDENTIFIER]    = {NULL,      NULL,     PREC_NONE},
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

    prefixRule();

    while (prec <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }
}

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    compilingChunk = chunk;
    parser.hadError = false;
    parser.panicMode = false;

    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    endCompiler();
    return !parser.hadError;
}
