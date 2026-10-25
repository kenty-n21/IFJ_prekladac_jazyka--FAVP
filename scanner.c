/*
 * Autor: Andrej Beleš(xbelesa00)
 * VYGENERUJ MI: Kompletny scanner.c pre IFJ26 s osetrenim escape sekvencii, 
 * viacriadkovych retazcov a prisnou kontrolou cisel.
 * (+oprava chýb).
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "scanner.h"

// Zisti ci slovo nie je klucove slovo [rozlisenie rezervovanych klucovych slov od beznych identifikatorov][cite: 13]
TokenType check_keyword(const char* word) {
    if (strcmp(word, "def") == 0) return TOKEN_KW_DEF;
    if (strcmp(word, "do") == 0) return TOKEN_KW_DO;
    if (strcmp(word, "else") == 0) return TOKEN_KW_ELSE;
    if (strcmp(word, "if") == 0) return TOKEN_KW_IF;
    if (strcmp(word, "return") == 0) return TOKEN_KW_RETURN;
    if (strcmp(word, "then") == 0) return TOKEN_KW_THEN;
    if (strcmp(word, "while") == 0) return TOKEN_KW_WHILE;
    if (strcmp(word, "val") == 0) return TOKEN_KW_VAL;
    if (strcmp(word, "var") == 0) return TOKEN_KW_VAR;
    if (strcmp(word, "Int") == 0) return TOKEN_KW_INT;
    if (strcmp(word, "Double") == 0) return TOKEN_KW_DOUBLE_TYPE;
    if (strcmp(word, "String") == 0) return TOKEN_KW_STRING_TYPE;
    if (strcmp(word, "Unit") == 0) return TOKEN_KW_UNIT;
    return TOKEN_IDENTIFIER;
}

Token get_next_token() {
    Token token;
    token.attribute[0] = '\0';
    int attr_len = 0;
    int c;

    // Bezpecne makro pre pridanie znaku
    #define ADD_CHAR(ch) do { if (attr_len < 1023) { token.attribute[attr_len++] = (ch); } } while(0)

    while (1) {
        c = getchar();

        // Preskoc medzery
        if (c == ' ' || c == '\r') continue;
        
        // Tabulator je zakazany [pouzitie tabulatora na zaciatku riadku je lexikalna chyba 1]
        if (c == '\t') { token.type = TOKEN_ERROR; return token; }

        if (c == '\n') { token.type = TOKEN_EOL; return token; }
        if (c == EOF) { token.type = TOKEN_EOF; return token; }

        // Komentare a delenie [ignorovanie riadkovych a spravne vnorenie blokovych komentarov]
        if (c == '/') {
            int next_c = getchar();
            if (next_c == '/') {
                while ((c = getchar()) != '\n' && c != EOF);
                ungetc(c, stdin);
                continue;
            } else if (next_c == '*') {
                int depth = 1;
                while (depth > 0) {
                    c = getchar();
                    if (c == EOF) { token.type = TOKEN_ERROR; return token; } // [neukonceny komentar vedie na lexikalnu chybu]
                    if (c == '/') {
                        int temp = getchar();
                        if (temp == '*') depth++;
                        else if (temp != EOF) ungetc(temp, stdin);
                    } else if (c == '*') {
                        int temp = getchar();
                        if (temp == '/') depth--;
                        else if (temp != EOF) ungetc(temp, stdin);
                    }
                }
                continue;
            } else {
                ungetc(next_c, stdin);
                token.type = TOKEN_DIV;
                return token;
            }
        }

        // Identifikatory a klucove slova [identifikator zacina pismenom alebo podtrznikom]
        if (isalpha(c) || c == '_') {
            while (isalnum(c) || c == '_') {
                ADD_CHAR(c);
                c = getchar();
            }
            ungetc(c, stdin);
            token.attribute[attr_len] = '\0';
            token.type = check_keyword(token.attribute);
            return token;
        }

        // Znak @ pre @main [vstupna funkcia musi byt oznacena zavinacom]
        if (c == '@') {
            token.type = TOKEN_AT;
            return token;
        }

        // Cisla (Int a Double formaty) [dodrzanie prisneho formatu pre celociselne a desatinne literaly vratane exponentu]
        if (isdigit(c)) {
            int is_double = 0;
            ADD_CHAR(c);
            
            // Cela cast
            while ((c = getchar()) && (isdigit(c) || c == '_')) {
                if (c != '_') ADD_CHAR(c); // [podtrzniky v cislach su povolene ale ignoruju sa]
            }
            
            // Desatinna cast
            if (c == '.') {
                is_double = 1;
                ADD_CHAR(c);
                c = getchar();
                if (!isdigit(c)) { token.type = TOKEN_ERROR; return token; }
                while (isdigit(c) || c == '_') {
                    if (c != '_') ADD_CHAR(c);
                    c = getchar();
                }
            }
            
            // Exponent
            if (c == 'e' || c == 'E') {
                is_double = 1;
                ADD_CHAR(c);
                c = getchar();
                if (c == '+' || c == '-') {
                    ADD_CHAR(c);
                    c = getchar();
                }
                if (!isdigit(c)) { token.type = TOKEN_ERROR; return token; }
                while (isdigit(c) || c == '_') {
                    if (c != '_') ADD_CHAR(c);
                    c = getchar();
                }
            }
            
            ungetc(c, stdin);
            token.attribute[attr_len] = '\0';
            token.type = is_double ? TOKEN_DOUBLE : TOKEN_INTEGER;
            return token;
        }

        // Retazce (String) vcetne escape sekvencii [spracovanie obycajnych a viacriadkovych retazcov]
        if (c == '"') {
            int next1 = getchar();
            int next2 = getchar();
            
            // Skontrolujeme ci ide o viacriadkovy retazec """ [viacriadkovy retazec musi byt ohraniceny tromi uvodzovkami]
            if (next1 == '"' && next2 == '"') {
                int quotes = 0;
                while (1) {
                    c = getchar();
                    if (c == EOF) { token.type = TOKEN_ERROR; return token; }
                    
                    if (c == '"') {
                        quotes++;
                        if (quotes == 3) {
                            attr_len -= 2; // Zmazeme prve dve uvodzovky z textu
                            break;
                        }
                    } else {
                        quotes = 0;
                    }
                    ADD_CHAR(c);
                }
            } else {
                // Obycajny jedoriadkovy retazec
                ungetc(next2, stdin);
                c = next1;
                while (c != '"') {
                    if (c == '\n' || c == EOF) { token.type = TOKEN_ERROR; return token; } // [obycajny retazec nesmie obsahovat skutocny novy riadok]
                    
                    // Escape sekvencie [povinne spracovanie lomeno n, t, r, uvodzovky a spatneho lomitka]
                    if (c == '\\') {
                        c = getchar();
                        if (c == 'n') ADD_CHAR('\n');
                        else if (c == 't') ADD_CHAR('\t');
                        else if (c == 'r') ADD_CHAR('\r');
                        else if (c == '"') ADD_CHAR('"');
                        else if (c == '\\') ADD_CHAR('\\');
                        else { token.type = TOKEN_ERROR; return token; }
                    } else {
                        ADD_CHAR(c);
                    }
                    c = getchar();
                }
            }
            token.attribute[attr_len] = '\0';
            token.type = TOKEN_STRING;
            return token;
        }

        // Operatory [rozlisenie zlozenych operatorov pre spravne porovnavanie]
        if (c == '=') {
            if ((c = getchar()) == '=') { token.type = TOKEN_EQ; return token; }
            ungetc(c, stdin); token.type = TOKEN_ASSIGN; return token;
        }
        if (c == '!') {
            if ((c = getchar()) == '=') { token.type = TOKEN_NEQ; return token; }
            token.type = TOKEN_ERROR; return token;
        }
        if (c == '<') {
            if ((c = getchar()) == '=') { token.type = TOKEN_LTE; return token; }
            ungetc(c, stdin); token.type = TOKEN_LT; return token;
        }
        if (c == '>') {
            if ((c = getchar()) == '=') { token.type = TOKEN_GTE; return token; }
            ungetc(c, stdin); token.type = TOKEN_GT; return token;
        }

        // Jednoznakove operatory
        switch (c) {
            case '+': token.type = TOKEN_PLUS; return token;
            case '-': token.type = TOKEN_MINUS; return token;
            case '*': token.type = TOKEN_MUL; return token;
            case '(': token.type = TOKEN_LPAREN; return token;
            case ')': token.type = TOKEN_RPAREN; return token;
            case ':': token.type = TOKEN_COLON; return token;
            case ',': token.type = TOKEN_COMMA; return token;
        }

        // Neznamy znak je chyba [akykolvek iny znak okrem povolenych vedie na lexikalnu chybu]
        token.type = TOKEN_ERROR;
        return token;
    }
}