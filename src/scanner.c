#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

Scanner scanner;

void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

static bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

static bool isAtEnd() {
    return *scanner.current == '\0';
}

static char advance() {
    scanner.current++;
    return scanner.current[-1];
}

static char peek() {
    return *scanner.current;
}

static char peekNext() {
    if (isAtEnd()) return '\0';
    return scanner.current[1];
}

static bool match(char expected) {
    if (isAtEnd()) return false;
    if (*scanner.current != expected) return false;
    scanner.current++;
    return true;
}

static Token makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

static Token errorToken(const char* msg) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = msg;
    token.length = (int)strlen(msg);
    token.line = scanner.line;
    return token;
}

static void skipWhitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                scanner.line++;
                advance();
                break;
            case '#':
     //           if (peekNext() == '|') {
              //      while ((peek() != '|' || peekNext() != '#') && !isAtEnd()) advance();
     //               advance();
     //               advance();
     //           } else {
                    while (peek() != '\n' && !isAtEnd()) advance();
      //          }
                break;
            default:
                return;
        }
    }
}

static TokenType checkKeyword(int start, int length, const char* rest, TokenType type) {
    if (scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, length) == 0) {
    return type;
  }

  return TOKEN_IDENTIFIER;
} 

static TokenType identifierType() {
    switch (scanner.start[0]) {
        case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
        case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
        case 'd': return checkKeyword(1, 2, "ef", TOKEN_DEF);
        case 'e':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                case 'l': return checkKeyword(2, 2, "se", TOKEN_ELSE);
                case 'x': return checkKeyword(2, 5, "tends", TOKEN_EXTENDS);
                default: break;
                }
            }
            break;
        case 'f':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
                default: break;
                }
            }
            break;
        case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
        case 'n': return checkKeyword(1, 3, "one", TOKEN_NONE);
        case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
        case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
        case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
        case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
        case 't':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
                    case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
                    default: break;
                }
            }
            break;
        case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
        case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);

      //  default: return TOKEN_IDENTIFIER;
    }
    
    return TOKEN_IDENTIFIER;
}

static Token identifier() {
    while (isAlpha(peek()) || isDigit(peek())) advance();
    return makeToken(identifierType());
}

static Token number() {
    while (isDigit(peek())) advance();

    if (peek() == '.' && isDigit(peekNext())) {
        advance();

        while (isDigit(peek())) advance();
    }
    
    return makeToken(TOKEN_NUM);
}

static Token string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') scanner.line++;
        advance();
    }

    if (isAtEnd()) return errorToken("Unterminated string.");

    advance();
    return makeToken(TOKEN_STR);
}

Token scanToken() {
    skipWhitespace();
    scanner.start = scanner.current;

    if (isAtEnd()) return makeToken(TOKEN_EOF);

    char c = advance();
    if (isAlpha(c)) return identifier();
    if (isDigit(c)) return number();

    switch (c) {
        case '(': return makeToken(TOKEN_LPAREN);
        case ')': return makeToken(TOKEN_RPAREN);
        case '{': return makeToken(TOKEN_LBRACE);
        case '}': return makeToken(TOKEN_RBRACE);
        case '[': return makeToken(TOKEN_LBRACK);
        case ']': return makeToken(TOKEN_RBRACK);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case '-': return makeToken(TOKEN_MINUS);
        case '+': return makeToken(TOKEN_PLUS);
        case ';': return makeToken(TOKEN_SEMI);
        case '/': return makeToken(TOKEN_SLASH);
        case '*': return makeToken(TOKEN_STAR);
        
        case '!':
            return makeToken(
                match('=') ? TOKEN_EXCLAM_EQ : TOKEN_EXCLAM
            );

        case '=':
            return makeToken(
                match('=') ? TOKEN_EQ_EQ : TOKEN_EQ
            );
        case '>':
            return makeToken(
                match('=') ? TOKEN_GREATER_EQ : (
                    match('>') ? TOKEN_RSHIFT : TOKEN_GREATER
                )
            );
        case '<':
            return makeToken(
                match('=') ? TOKEN_LESS_EQ : (
                    match('<') ? TOKEN_LSHIFT : TOKEN_LESS
                )
            );
        case '"': return string();

        default:
            return errorToken("Unexpected character.");
    }
}
