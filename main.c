/*
 * Autor: Andrej Beleš(xbelesa00)
 * VYGENERUJ MI: Aktualizovany main.c pre testovanie kompletneho scannera.
 * Program v cykle nacitava tokeny a vypisuje ich typ a atribut (text),
 * az kym nenarazi na TOKEN_EOF alebo lexikalnu chybu.
 */

#include <stdio.h>
#include "scanner.h"

// Pomocna funkcia, ktora do konzoly vypise, co scanner nasiel
void print_token(Token t) {
    printf("\n");
    printf("Token: ");
    
    // Vypiseme ludsky nazov typu tokenu
    switch(t.type) {
        case TOKEN_EOF: printf("EOF"); break;
        case TOKEN_EOL: printf("EOL (Koniec riadku)"); break;
        case TOKEN_IDENTIFIER: printf("IDENTIFIER"); break;
        case TOKEN_INTEGER: printf("INTEGER"); break;
        case TOKEN_DOUBLE: printf("DOUBLE"); break;
        case TOKEN_STRING: printf("STRING"); break;
        case TOKEN_KW_DEF: printf("KW_DEF"); break;
        case TOKEN_KW_VAL: printf("KW_VAL"); break;
        case TOKEN_KW_INT: printf("KW_INT"); break;
        case TOKEN_ASSIGN: printf("ASSIGN (=)"); break;
        case TOKEN_PLUS: printf("PLUS (+)"); break;
        case TOKEN_LPAREN: printf("LPAREN ("); break;
        case TOKEN_RPAREN: printf("RPAREN )"); break;
        case TOKEN_AT: printf("AT (@)"); break;
        case TOKEN_ERROR: printf("ERROR"); break;
        // Ak nie je zozname, vypiseme aspon jeho ciselny kod
        default: printf("Iny operator alebo klucove slovo (kod %d)", t.type); break;
    }
    
    // Ak si Scanner zapamatal aj nejaky text (atribut), vypiseme ho
    if (t.attribute[0] != '\0') {
        printf(" | Text: '%s'", t.attribute);
    }
}

int main() {
    printf("Prekladac IFJ - Test Scannera\n");
    printf("Napis kod IFJ26, stlac Enter. Pre ukoncenie stlac Ctrl+D:\n");
    printf("--------------------------------------------------------\n");

    Token token;
    
    // Ziskavaj a vypisuj tokeny, kym nenarazis na koniec alebo chybu
    do {
        token = get_next_token();
        print_token(token);
    } while (token.type != TOKEN_EOF && token.type != TOKEN_ERROR);

    // Spracovanie chyboveho ukoncenia [navratovy kod 1 = lexikalna chyba]
    if (token.type == TOKEN_ERROR) {
        fprintf(stderr, "\nPreklad ukonceny: Lexikalna chyba (nepovoleny znak).\n");
        return 1; 
    }

    // Uspesne ukoncenie [navratovy kod 0 = uspesny preklad]
    printf("\nKoniec suboru (EOF). Vsetko OK.\n");
    return 0; 
}