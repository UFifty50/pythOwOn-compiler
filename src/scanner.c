#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"
#include "scanner.h"
#include "compiler.h"

Scanner scanner;

void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
    scanner.currentChar = 0;
    scanner.currentString = NULL;
    scanner.currentStringLength = 0;
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

static char speek() {
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
    if (token.type == TOKEN_STR) {
        token.length = scanner.currentStringLength+1;
        token.start = scanner.currentString;
        scanner.current--;
    } else {
        token.length = (int)(scanner.current - scanner.start);
        token.start = scanner.start;
    }

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
        char c = speek();
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
                if (peekNext() == '|') {
                    while ((speek() != '|' || peekNext() != '#') && !isAtEnd()) advance();
                    advance();
                    advance();
                } else {
                    while (speek() != '\n' && !isAtEnd()) advance();
                }
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

        default: return TOKEN_IDENTIFIER;
    }
    
    return TOKEN_IDENTIFIER;
}

static Token identifier() {
    while (isAlpha(speek()) || isDigit(speek())) advance();
    return makeToken(identifierType());
}

static Token number() {
    while (isDigit(speek())) advance();

    if (speek() == '.' && isDigit(peekNext())) {
        advance();

        while (isDigit(speek())) advance();
    }
    
    return makeToken(TOKEN_NUM);
}

static char peekChar() {
    scanner.currentChar++;
    scanner.current++;
    return scanner.current[-1];
}

static void addStringChar(char c) {
    scanner.currentString = realloc(scanner.currentString, sizeof(char) * (scanner.currentStringLength + 2));
    scanner.currentString[scanner.currentStringLength] = c;
    scanner.currentStringLength++;
}

static char nextChar() {
    char c = peekChar();
    return c;
}

static Token string() {
    scanner.currentStringLength = 0;
    scanner.currentString = (char*)malloc(sizeof(char) * 1);
    for (;;) {
        char c = nextChar();
        if (c == '"') break;
        if (c != '"' && isAtEnd()) return errorToken("Unterminated string.");
        if (c == '\n') scanner.line++;
        if (c == '\\') {
            switch (peekChar()) {
                case '"':  addStringChar('"'); break;  // double quote
                case '\'': addStringChar('\''); break; // single quote
                case 'n':  addStringChar('\n'); break; // newline
                case 'r':  addStringChar('\r'); break; // carriage return
                case 't':  addStringChar('\t'); break; // horizontal tab
                case 'v':  addStringChar('\v'); break; // vertical tab
                case 'f':  addStringChar('\f'); break; // form feed
                case '\\': addStringChar('\\'); break; // backslash
                case '0':  addStringChar('\0'); break; // null-terminating zero
                case 'e':  addStringChar('\e'); break; // escape
                case 'a':  addStringChar('\a'); break; // beep/bell (why???)
                default: return errorToken("Unknown escape sequence");
            }
        } else {
            addStringChar(c);
        }
    }

  //  free(scanner.currentString);
    scanner.currentString[scanner.currentStringLength] = '\0';
    scanner.currentChar = 0;
    scanner.current++;
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
