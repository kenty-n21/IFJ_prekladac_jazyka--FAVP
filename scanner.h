/*
 * Autor: Andrej Beleš(xbelesa00)
 * VYGENERUJ MI: Kompletny hlavickovy subor scanner.h pre IFJ26.
 * Doplnene o tokeny pre @main. 
 */

#ifndef SCANNER_H
#define SCANNER_H

// Zoznam vsetkych tokenov podla specifikacie IFJ26
typedef enum {
    TOKEN_EOF,          // Koniec suboru
    TOKEN_EOL,          // Koniec riadku
    
    TOKEN_IDENTIFIER,   // Nazvy premennych
    TOKEN_INTEGER,      // Cele cisla (Int)
    TOKEN_DOUBLE,       // Desatinne cisla (Double)
    TOKEN_STRING,       // Text (String)

    // Klucove slova
    TOKEN_KW_DEF, TOKEN_KW_DO, TOKEN_KW_ELSE, TOKEN_KW_IF, 
    TOKEN_KW_RETURN, TOKEN_KW_THEN, TOKEN_KW_WHILE, 
    TOKEN_KW_VAL, TOKEN_KW_VAR, TOKEN_KW_INT, 
    TOKEN_KW_DOUBLE_TYPE, TOKEN_KW_STRING_TYPE, TOKEN_KW_UNIT,

    // Operatory a interpunkcia
    TOKEN_ASSIGN,       // =
    TOKEN_EQ,           // ==
    TOKEN_NEQ,          // !=
    TOKEN_LT,           // <
    TOKEN_LTE,          // <=
    TOKEN_GT,           // >
    TOKEN_GTE,          // >=
    TOKEN_PLUS,         // +
    TOKEN_MINUS,        // -
    TOKEN_MUL,          // *
    TOKEN_DIV,          // /
    
    TOKEN_LPAREN,       // (
    TOKEN_RPAREN,       // )
    TOKEN_COLON,        // :
    TOKEN_COMMA,        // ,
    TOKEN_AT,           // @ (pre @main)

    TOKEN_ERROR         // Lexikalna chyba (kod 1)
} TokenType;

typedef struct {
    TokenType type;
    char attribute[1024]; // Sem ulozime text tokenu
} Token;

Token get_next_token();

#endif