#ifndef pythowon_scanner_h
#define pythowon_scanner_h

typedef enum {                  //TODO: add const token (maybe?)
    // single char tokens
    TOKEN_LPAREN, TOKEN_RPAREN,
    TOKEN_LBRACE, TOKEN_RBRACE,
    TOKEN_LBRACK, TOKEN_RBRACK,
    TOKEN_COMMA, TOKEN_DOT,
    TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_PERCENT, TOKEN_SEMI,
    TOKEN_SLASH, TOKEN_STAR,
    TOKEN_COLON,

    // single/double char tokens
    TOKEN_EXCLAM, TOKEN_EXCLAM_EQ,
    TOKEN_EQ, TOKEN_EQ_EQ, TOKEN_GREATER,
    TOKEN_GREATER_EQ, TOKEN_LESS,
    TOKEN_LESS_EQ, TOKEN_LSHIFT, TOKEN_RSHIFT,

    // literals
    TOKEN_IDENTIFIER, TOKEN_STR, TOKEN_NUM,
    TOKEN_INFINITY, TOKEN_NAN,                  //TODO: implement infinity/NaN

    // keywords
    TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE,
    TOKEN_FALSE, TOKEN_FOR, TOKEN_DEF, TOKEN_IF,
    TOKEN_NONE, TOKEN_OR, TOKEN_PRINT, TOKEN_RETURN,  //TODO: implement print in standard library instead of here
    TOKEN_SUPER, TOKEN_THIS, TOKEN_TRUE, TOKEN_VAR,
    TOKEN_WHILE, TOKEN_EXTENDS, TOKEN_SWITCH,
    TOKEN_CASE, TOKEN_DEFAULT, TOKEN_CONTINUE,
    TOKEN_BREAK,TOKEN_IN,

    TOKEN_ERROR, TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

typedef struct {
    const char* start;
    const char* current;
    int line;

    char* currentString;
    int currentStringLength;
    int currentChar;
} Scanner;

extern Scanner scanner;

void initScanner(const char* source);
Token scanToken(void);

#endif
