/*
 * Projekt: Implementacia prekladaca imperativneho jazyka IFJ26
 * Subor: main.c
 * Autor: Andrej Beles (xbelesa00)
 *
 * AI Prompt: "VYGENERUJ MI: Predpripraveny main.c pre IFJ26 s prepinacom
 * medzi testovanim scannera a finalnym volanim parsera."
 *
 * AI Prompt (uprava): "Uprav main.c tak, aby sa zatial dal zostavit bez parsera
 * (parser.h sa includuje az po vypnuti TEST_SCANNER) a volal parser_program()."
 */

#include <stdio.h>
#include "scanner.h"

// Rezim prekladu: kym nie je hotovy parser, bezi len test scannera.
// Ked bude parser hotovy: zakomentujte nasledujuci riadok a pridajte parser.c
// do SRCS v Makefile.
#define TEST_SCANNER

#ifndef TEST_SCANNER
#include "parser.h"
#endif

#ifdef TEST_SCANNER
// Pomocna funkcia na vypis tokenu (vypisujeme na stderr, aby sme nespinili stdout)
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
 * Cita zdrojovy kod zo stdin, generuje IFJcode26 na stdout a vracia chybovy kod.
 * Chybove a ladiace vypisy ide na stderr.
 *
 * @return 0 pri uspechu, inak kod chyby (1-7, 10, 99) podla zadania.
 */
int main(void) {
    scanner_init();

#ifdef TEST_SCANNER
    // Docasny rezim pre lokalne testovanie samotneho scannera
    Token token;
    do {
        token = get_next_token();
        print_token(token);
    } while (token.type != TOKEN_EOF && token.type != TOKEN_ERROR);

    if (token.type == TOKEN_ERROR) {
        fprintf(stderr, "Lexikalna chyba (kod 1).\n");
        return 1; // chyba 1 = chyba v ramci lexikalnej analyzy
    }
    return 0;
#else
    // Finalny rezim: syntakticka analyza a preklad
    int result = parser_program();
    if (result != 0) {
        fprintf(stderr, "Preklad skoncil s chybou: %d\n", result);
    }
    return result; // 0 pri uspechu alebo kod chyby 1..10, 99
#endif
}
