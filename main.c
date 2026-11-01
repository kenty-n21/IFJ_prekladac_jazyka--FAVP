/*
 * Projekt: Implementacia prekladaca imperativneho jazyka IFJ26
 * Subor: main.c
 * Autor: Andrej Beles (xbelesa00)
 *
 * AI Prompt: "VYGENERUJ MI: Predpripraveny main.c pre IFJ26 s prepinacom
 * medzi testovanim scannera a finalnym volanim parsera."
 */

#include <stdio.h>
#include "scanner.h"
// #include "parser.h" (odkomentujeme, ked bude hotovy parser.h)

#define TEST_SCANNER    // (!zakomentujeme tento riadok, ked budete spajat projekt s Parserom)


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
    // (finalny rezim pre odovzdanie - spusti syntakticku analyzu a preklad)
    int result = parse(); 
    if (result != 0) {
        fprintf(stderr, "Preklad skoncil s chybou: %d\n", result);
    }
    return result; // [vracia 0 pri uspechu alebo kod chyby 1..10, 99]
#endif
}