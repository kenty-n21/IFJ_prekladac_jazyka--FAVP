#include <stdio.h>
#include "scanner.h"
#include "parser.h" // Odkomentované, aby main videl parser

// #define TEST_SCANNER    // ZAKOMENTOVANÉ: Prepína projekt z testovania scannera na reálny parser


#ifdef TEST_SCANNER
// Pomocna funkcia na vypis tokenu (vypisujeme na stderr, aby sme nespini stdout)
static void print_token(Token t) {
    fprintf(stderr, "Token: %d", t.type);
    if (t.attribute[0] != '\0') {
        fprintf(stderr, " | Text: '%s'", t.attribute);
    }
    fprintf(stderr, " | Indent: %d\n", t.indent);
}
#endif

/**
 * Hlavny vstupny bod prekladaca IFJ26.
 * [Program cita zo stdin, generuje IFJcode26 na stdout a vracia chybovy kod]
 */
int main(void) {
    scanner_init();

#ifdef TEST_SCANNER
    // (docasny rezim pre lokane testovanie samotneho scannera)
    Token token;
    do {
        token = get_next_token();
        print_token(token);
    } while (token.type != TOKEN_EOF && token.type != TOKEN_ERROR);

    if (token.type == TOKEN_ERROR) {
        fprintf(stderr, "Lexikalna chyba (kod 1).\n");
        return 1; // [chyba 1 = chyba v ramci lexikalnej analyzy]
    }
    return 0;
#else
    // (reálny režim pre preklad a syntaktickú analýzu)
    // Predpokladáme, že vaša hlavná funkcia v parser.c sa volá parser_program() 
    // a vrací 0 pri úspechu alebo chybový kód.
    int result = parser_program(); 
    
    if (result != 0) {
        fprintf(stderr, "Preklad skoncil s chybou: %d\n", result);
    } else {
        fprintf(stderr, "Preklad prebehol úspešne!\n");
    }
    return result; // [vracia 0 pri uspechu alebo kod chyby]
#endif
}