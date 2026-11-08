/*
 * Projekt: Implementacia prekladaca imperativneho jazyka IFJ26
 * Subor: scanner.c
 * Autor: Andrej Beles (xbelesa00)
 *
 * AI Prompt: "VYGENERUJ MI: Kompletny scanner.c pre IFJ26 s osetrenim escape
 * sekvencii, viacriadkovych retazcov, prisnou kontrolou cisel, komentarov
 * a sledovanim odsadenia (INDENT/DEDENT) podla specifikacie IFJ26."
 *
 * AI Prompt (uprava): "Uprav scanner: vydaj TOKEN_EOL pred EOF ak posledny
 * riadok nekonci LF; 'to', 'until', 'by' nech su bezne identifikatory; pri
 * prilis dlhom atribute vrat chybu namiesto ticheho orezania; nahrad ungetc
 * vlastnym zasobnikom spatne vratenych znakov (ungetc zaruci len 1 znak)."
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "scanner.h"

// Zasobnik pre sledovanie urovni odsadenia blokov [sekcia 1.2]
#define MAX_INDENT_LEVELS 256
static int indent_stack[MAX_INDENT_LEVELS] = {0};
static int indent_top = 0;
static int pending_dedents = 0;

//Stavove premenne pre spracovanie logickych a fyzickych riadkov
static int at_line_start = 1;       // Sme na zaciatku fyzickeho riadku (pocitame medzery)
static int paren_depth = 0;         // Hlbka vnorenia okruhlych zatvoriek ()
static int seen_newline = 0;        // Bol od posledneho tokenu fyzicky koniec riadku \n
static int line_has_tokens = 0;     // Boli uz na aktualnom logickom riadku vydane tokeny

// Priznak, ze atribut aktualneho tokenu je dlhsi ako MAX_TOKEN_LEN
static int attr_overflow = 0;

// Bezpecne makro na pridanie znaku do atributu tokenu (pri preteceni nastavi priznak)
#define ADD_CHAR(token, attr_len, ch) \
    do { \
        if ((attr_len) < MAX_TOKEN_LEN) { \
            (token).attribute[(attr_len)++] = (char)(ch); \
        } else { \
            attr_overflow = 1; \
        } \
    } while (0)

// Vlastny zasobnik spatne vratenych znakov (ungetc garantuje iba jeden znak)
#define PUSHBACK_SIZE 8
static int pushback[PUSHBACK_SIZE];
static int pushback_count = 0;

/**
 * @brief Nacita dalsi znak - najprv zo zasobnika vratenych znakov, inak zo stdin.
 */
static int next_char(void) {
    if (pushback_count > 0) {
        return pushback[--pushback_count];
    }
    return getchar();
}

/**
 * @brief Vrati znak spat na vstup (EOF sa ignoruje, dalsi next_char ho vrati znova).
 */
static void unget_char(int c) {
    if (c != EOF && pushback_count < PUSHBACK_SIZE) {
        pushback[pushback_count++] = c;
    }
}

/**
 * @brief Resetuje stav lexikalneho analyzatora do pociatocneho stavu.
 */
void scanner_init(void) {
    indent_stack[0] = 0;
    indent_top = 0;
    pending_dedents = 0;
    at_line_start = 1;
    paren_depth = 0;
    seen_newline = 0;
    line_has_tokens = 0;
    pushback_count = 0;
    attr_overflow = 0;
}

/**
 * @brief Rozlisi rezervovane klucove slova a specialny znak '_' od identifikatorov.
 * @param word Nacitany retazec.
 * @return Zodpovedajuci TokenType.
 */
static TokenType check_keyword(const char *word) {
    // Samotne podtrzitko nie je platny identifikator [sekcia 1.1]
    if (strcmp(word, "_") == 0) return TOKEN_UNDERSCORE;

    // Zakladne klucove slova [sekcia 1.1]
    if (strcmp(word, "def") == 0) return TOKEN_KW_DEF;
    if (strcmp(word, "do") == 0) return TOKEN_KW_DO;
    if (strcmp(word, "else") == 0) return TOKEN_KW_ELSE;
    if (strcmp(word, "if") == 0) return TOKEN_KW_IF;
    if (strcmp(word, "import") == 0) return TOKEN_KW_IMPORT;
    if (strcmp(word, "return") == 0) return TOKEN_KW_RETURN;
    if (strcmp(word, "then") == 0) return TOKEN_KW_THEN;
    if (strcmp(word, "while") == 0) return TOKEN_KW_WHILE;
    if (strcmp(word, "val") == 0) return TOKEN_KW_VAL;
    if (strcmp(word, "var") == 0) return TOKEN_KW_VAR;
    if (strcmp(word, "Int") == 0) return TOKEN_KW_INT;
    if (strcmp(word, "Double") == 0) return TOKEN_KW_DOUBLE_TYPE;
    if (strcmp(word, "String") == 0) return TOKEN_KW_STRING_TYPE;
    if (strcmp(word, "Unit") == 0) return TOKEN_KW_UNIT;

    // Klucove slova pre rozsirenia [sekcia 1.1 a 5]
    if (strcmp(word, "Boolean") == 0) return TOKEN_KW_BOOLEAN;
    if (strcmp(word, "true") == 0) return TOKEN_KW_TRUE;
    if (strcmp(word, "false") == 0) return TOKEN_KW_FALSE;
    if (strcmp(word, "case") == 0) return TOKEN_KW_CASE;
    if (strcmp(word, "class") == 0) return TOKEN_KW_CLASS;
    if (strcmp(word, "extension") == 0) return TOKEN_KW_EXTENSION;
    if (strcmp(word, "for") == 0) return TOKEN_KW_FOR;
    if (strcmp(word, "List") == 0) return TOKEN_KW_LIST;
    if (strcmp(word, "match") == 0) return TOKEN_KW_MATCH;
    if (strcmp(word, "type") == 0) return TOKEN_KW_TYPE;

    return TOKEN_IDENTIFIER;
}

/**
 * @brief Pomocna funkcia na nacitanie sekvencie cislic s povolenymi podtrzitkami.
 * [Podtrzitko smie byt vyhradne medzi dvoma cislicami - sekcia 1.1]
 * @return 1 ak je sekvencia platna, 0 pri lexikalnej chybe.
 */
static int read_digits_with_underscores(Token *token, int *attr_len) {
    int c = next_char();
    while (isdigit(c) || c == '_') {
        if (c == '_') {
            int next_c = next_char();
            // Za podtrzitkom musi okamzite nasledovat cislica [sekcia 1.1]
            if (!isdigit(next_c)) {
                return 0;
            }
            ADD_CHAR(*token, *attr_len, next_c);
        } else {
            ADD_CHAR(*token, *attr_len, c);
        }
        c = next_char();
    }
    if (c != EOF) {
        unget_char(c);
    }
    return 1;
}

/**
 * @brief Vlastny lexikalny analyzator - nacita a vrati dalsi token (bez kontroly pretecenia).
 */
static Token scan_token(void) {
    Token token;
    token.attribute[0] = '\0';
    token.indent = indent_stack[indent_top];
    token.had_newline_before = seen_newline;
    int attr_len = 0;

    // Ak mame nevybavene vynorenia z viacerych blokov naraz, vydavame TOKEN_DEDENT
    if (pending_dedents > 0) {
        pending_dedents--;
        token.type = TOKEN_DEDENT;
        token.indent = indent_stack[indent_top];
        return token;
    }

    int c;
    int current_spaces = 0;
    int has_leading_tab = 0;

    while (1) {
        // 1. Spracovanie zaciatku fyzickeho riadku a meranie odsadenia [sekcia 1.2]
        if (at_line_start) {
            current_spaces = 0;
            has_leading_tab = 0;

            while (1) {
                c = next_char();
                if (c == ' ') {
                    current_spaces++;
                } else if (c == '\t') {
                    // Tabulator na zaciatku neprazdneho riadku je chyba 1 [sekcia 1.2]
                    has_leading_tab = 1;
                } else if (c == '\r') {
                    continue;
                } else {
                    break;
                }
            }

            // Prazdny fyzicky riadok sa ignoruje a nemeni odsadenie [sekcia 1.2]
            if (c == '\n') {
                seen_newline = 1;
                continue;
            }

            // Na konci suboru uzatvorime vsetky otvorene odsadene bloky
            if (c == EOF) {
                if (paren_depth > 0) {
                    token.type = TOKEN_ERROR;
                    return token;
                }
                if (indent_top > 0) {
                    indent_top--;
                    pending_dedents = indent_top;
                    indent_top = 0;
                    token.type = TOKEN_DEDENT;
                    token.indent = 0;
                    return token;
                }
                token.type = TOKEN_EOF;
                return token;
            }

            // Kontrola komentarov zacinajucich priamo po odsadenych medzerach
            if (c == '/') {
                int next_c = next_char();
                if (next_c == '/') {
                    // Riadok tvoreny len bielymi znakmi a riadkovym komentarom sa ignoruje [sekcia 1.2]
                    while ((c = next_char()) != '\n' && c != EOF);
                    seen_newline = 1;
                    continue;
                } else if (next_c == '*') {
                    // Blokovy komentar na zaciatku riadku
                    int depth = 1;
                    int has_nl = 0;
                    while (depth > 0) {
                        c = next_char();
                        if (c == EOF) { token.type = TOKEN_ERROR; return token; }
                        if (c == '\n') has_nl = 1;
                        if (c == '/') {
                            int temp = next_char();
                            if (temp == '*') depth++;
                            else if (temp != EOF) unget_char(temp);
                        } else if (c == '*') {
                            int temp = next_char();
                            if (temp == '/') depth--;
                            else if (temp != EOF) unget_char(temp);
                        }
                    }
                    if (has_nl) {
                        // Viacriadkovy blokovy komentar: za nim nesmu byt na riadku dalsie tokeny [sekcia 1.1]
                        while ((c = next_char()) == ' ' || c == '\t' || c == '\r');
                        if (c == '/') {
                            int temp = next_char();
                            if (temp == '/') {
                                while ((c = next_char()) != '\n' && c != EOF);
                            } else {
                                token.type = TOKEN_ERROR;
                                return token;
                            }
                        } else if (c != '\n' && c != EOF) {
                            token.type = TOKEN_ERROR;
                            return token;
                        }
                        seen_newline = 1;
                        continue;
                    } else {
                        // Jednoriadkovy blokovy komentar na zaciatku riadku: skontrolujeme ci nasleduje koniec riadku
                        while ((c = next_char()) == ' ' || c == '\t' || c == '\r');
                        if (c == '\n') {
                            seen_newline = 1;
                            continue;
                        }
                        if (c == '/') {
                            int temp = next_char();
                            if (temp == '/') {
                                while ((c = next_char()) != '\n' && c != EOF);
                                seen_newline = 1;
                                continue;
                            }
                            if (temp != EOF) unget_char(temp);
                        }
                        // Ak za /* ... */ na tom istom riadku nasleduje kod, uplatnia sa pravidla pre riadok
                    }
                } else {
                    if (next_c != EOF) unget_char(next_c);
                }
            }

            // Riadok je neprazdny -> ak obsahoval tabulator v odsadeni, je to chyba 1 [sekcia 1.2]
            if (has_leading_tab) {
                token.type = TOKEN_ERROR;
                return token;
            }

            at_line_start = 0;

            if (paren_depth > 0) {
                // Vo vnutri zatvoriek musi byt novy fyzicky riadok odsadeny viac ako zaciatok logickeho riadku [sekcia 1.2]
                if (current_spaces <= indent_stack[indent_top]) {
                    token.type = TOKEN_ERROR;
                    return token;
                }
            } else {
                // Sme na zaciatku noveho logickeho riadku -> porovname odsadenie
                int prev_indent = indent_stack[indent_top];
                if (current_spaces > prev_indent) {
                    if (indent_top + 1 >= MAX_INDENT_LEVELS) {
                        token.type = TOKEN_ERROR;
                        return token;
                    }
                    indent_stack[++indent_top] = current_spaces;
                    unget_char(c);
                    token.type = TOKEN_INDENT;
                    token.indent = current_spaces;
                    token.had_newline_before = seen_newline;
                    return token;
                } else if (current_spaces < prev_indent) {
                    int dedent_count = 0;
                    while (indent_top > 0 && indent_stack[indent_top] > current_spaces) {
                        indent_top--;
                        dedent_count++;
                    }
                    // Ak sa odsadenie nezhoduje so ziadnou predchadzajucou urovnou, ide o chybu
                    if (indent_stack[indent_top] != current_spaces) {
                        token.type = TOKEN_ERROR;
                        return token;
                    }
                    unget_char(c);
                    pending_dedents = dedent_count - 1;
                    token.type = TOKEN_DEDENT;
                    token.indent = current_spaces;
                    token.had_newline_before = seen_newline;
                    return token;
                }
            }
            token.indent = indent_stack[indent_top];
        } else {
            c = next_char();
        }

        // 2. Bezne biele znaky uprostred riadku
        if (c == ' ' || c == '\t' || c == '\r') continue;

        // 3. Koniec fyzickeho riadku
        if (c == '\n') {
            at_line_start = 1;
            seen_newline = 1;
            // Vo vnutri zatvoriek sa \n chova len ako biely znak [sekcia 1.2]
            if (paren_depth > 0 || !line_has_tokens) {
                continue;
            }
            line_has_tokens = 0;
            token.type = TOKEN_EOL;
            return token;
        }

        // 4. Koniec suboru (EOF)
        if (c == EOF) {
            if (paren_depth > 0) {
                token.type = TOKEN_ERROR;
                return token;
            }
            // Posledny riadok bez koncoveho \n: najprv vydame EOL, EOF az potom
            if (line_has_tokens) {
                line_has_tokens = 0;
                token.type = TOKEN_EOL;
                return token;
            }
            if (indent_top > 0) {
                indent_top--;
                pending_dedents = indent_top;
                indent_top = 0;
                token.type = TOKEN_DEDENT;
                token.indent = 0;
                return token;
            }
            token.type = TOKEN_EOF;
            return token;
        }

        // 5. Komentare a operator delenia '/' [sekcia 1.1]
        if (c == '/') {
            int next_c = next_char();
            if (next_c == '/') {
                // Riadkovy komentar plati do konca riadku [sekcia 1.1]
                while ((c = next_char()) != '\n' && c != EOF);
                if (c != EOF) unget_char(c);
                continue;
            } else if (next_c == '*') {
                // Blokovy komentar s podporou vnorenia [sekcia 1.1]
                int depth = 1;
                int has_nl = 0;
                while (depth > 0) {
                    c = next_char();
                    if (c == EOF) { token.type = TOKEN_ERROR; return token; }
                    if (c == '\n') has_nl = 1;
                    if (c == '/') {
                        int temp = next_char();
                        if (temp == '*') depth++;
                        else if (temp != EOF) unget_char(temp);
                    } else if (c == '*') {
                        int temp = next_char();
                        if (temp == '/') depth--;
                        else if (temp != EOF) unget_char(temp);
                    }
                }
                if (has_nl) {
                    // Za viacriadkovym blokovym komentarom nesmu byt na tom istom riadku dalsie tokeny [sekcia 1.1]
                    while ((c = next_char()) == ' ' || c == '\t' || c == '\r');
                    if (c == '/') {
                        int temp = next_char();
                        if (temp == '/') {
                            while ((c = next_char()) != '\n' && c != EOF);
                        } else {
                            token.type = TOKEN_ERROR;
                            return token;
                        }
                    } else if (c != '\n' && c != EOF) {
                        token.type = TOKEN_ERROR;
                        return token;
                    }
                    if (c != EOF) unget_char(c);
                }
                continue;
            } else {
                if (next_c != EOF) unget_char(next_c);
                token.type = TOKEN_DIV;
                token.had_newline_before = seen_newline;
                seen_newline = 0;
                line_has_tokens = 1;
                return token;
            }
        }

        // Od tohto bodu vieme, ze ideme vydat skutocny token na aktualnom riadku
        token.had_newline_before = seen_newline;
        seen_newline = 0;
        line_has_tokens = 1;

        // 6. Identifikatory, klucove slova a '_' [sekcia 1.1]
        if (isalpha(c) || c == '_') {
            while (isalnum(c) || c == '_') {
                ADD_CHAR(token, attr_len, c);
                c = next_char();
            }
            if (c != EOF) unget_char(c);
            token.attribute[attr_len] = '\0';
            token.type = check_keyword(token.attribute);
            return token;
        }

        // 7. Anotacia @ pre @main [sekcia 2]
        if (c == '@') {
            token.type = TOKEN_AT;
            return token;
        }

        // 8. Bodka '.' alebo desatinny literal zacinajuci bodkou (napr. .5) [sekcia 1.1 a 2]
        if (c == '.') {
            int next_c = next_char();
            if (isdigit(next_c)) {
                // Desatinne cislo bez celej casti (napr. .25 -> ulozime ako 0.25) [sekcia 1.1]
                ADD_CHAR(token, attr_len, '0');
                ADD_CHAR(token, attr_len, '.');
                ADD_CHAR(token, attr_len, next_c);
                if (!read_digits_with_underscores(&token, &attr_len)) {
                    token.type = TOKEN_ERROR;
                    return token;
                }
                c = next_char();
                if (c == 'e' || c == 'E') {
                    ADD_CHAR(token, attr_len, c);
                    c = next_char();
                    if (c == '+' || c == '-') {
                        ADD_CHAR(token, attr_len, c);
                        c = next_char();
                    }
                    if (!isdigit(c)) { token.type = TOKEN_ERROR; return token; }
                    ADD_CHAR(token, attr_len, c);
                    if (!read_digits_with_underscores(&token, &attr_len)) {
                        token.type = TOKEN_ERROR;
                        return token;
                    }
                    c = next_char();
                }
                // Nepovinna pripona d alebo D [sekcia 1.1]
                if (c != 'd' && c != 'D' && c != EOF) {
                    unget_char(c);
                }
                // Kontrola, ci tesne za cislom nenasleduje pismeno alebo podtrzitko (napr. .5f)
                int peek = next_char();
                if (isalpha(peek) || peek == '_') {
                    token.type = TOKEN_ERROR;
                    return token;
                }
                if (peek != EOF) unget_char(peek);

                token.attribute[attr_len] = '\0';
                token.type = TOKEN_DOUBLE;
                return token;
            } else {
                if (next_c != EOF) unget_char(next_c);
                token.type = TOKEN_DOT;
                return token;
            }
        }

        // 9. Ciselne literaly (Int a Double) [sekcia 1.1]
        if (isdigit(c)) {
            int is_double = 0;
            int starts_with_zero = (c == '0');
            ADD_CHAR(token, attr_len, c);

            if (!read_digits_with_underscores(&token, &attr_len)) {
                token.type = TOKEN_ERROR;
                return token;
            }

            c = next_char();

            // Desatinna cast
            if (c == '.') {
                int next_c = next_char();
                if (isdigit(next_c)) {
                    is_double = 1;
                    ADD_CHAR(token, attr_len, '.');
                    ADD_CHAR(token, attr_len, next_c);
                    if (!read_digits_with_underscores(&token, &attr_len)) {
                        token.type = TOKEN_ERROR;
                        return token;
                    }
                    c = next_char();
                } else {
                    // Ak za bodkou nie je cislica (napr. 5.toString vo VALMEM), vratime bodku do vstupu
                    if (next_c != EOF) unget_char(next_c);
                }
            }

            // Exponent (e alebo E) [sekcia 1.1]
            if (c == 'e' || c == 'E') {
                is_double = 1;
                ADD_CHAR(token, attr_len, c);
                c = next_char();
                if (c == '+' || c == '-') {
                    ADD_CHAR(token, attr_len, c);
                    c = next_char();
                }
                if (!isdigit(c)) { token.type = TOKEN_ERROR; return token; }
                ADD_CHAR(token, attr_len, c);
                if (!read_digits_with_underscores(&token, &attr_len)) {
                    token.type = TOKEN_ERROR;
                    return token;
                }
                c = next_char();
            }

            // Nepovinna pripona d alebo D pre Double [sekcia 1.1]
            if (is_double && (c == 'd' || c == 'D')) {
                c = next_char();
            }

            // Nenulovy celociselny literal nesmie zacinat nulou (napr. 012 alebo 00) [sekcia 1.1]
            if (!is_double && starts_with_zero && attr_len > 1) {
                token.type = TOKEN_ERROR;
                return token;
            }

            // Za cislom nesmie bezprostredne nasledovat pismeno ani '_' (napr. 10L, 1.5f, 12abc)
            if (isalpha(c) || c == '_') {
                token.type = TOKEN_ERROR;
                return token;
            }

            if (c != EOF) unget_char(c);
            token.attribute[attr_len] = '\0';
            token.type = is_double ? TOKEN_DOUBLE : TOKEN_INTEGER;
            return token;
        }

        // 10. Retazcove literaly (String) - obycajne "..." aj viacriadkove """...""" [sekcia 1.1]
        if (c == '"') {
            int next1 = next_char();
            if (next1 == '"') {
                int next2 = next_char();
                if (next2 == '"') {
                    // Viacriadkovy retazec """...""" (escape sekvencie sa nevyhodnocuju) [sekcia 1.1]
                    int quotes = 0;
                    while (1) {
                        c = next_char();
                        if (c == EOF) { token.type = TOKEN_ERROR; return token; }
                        if (c == '\r') continue;
                        if (c == '"') {
                            quotes++;
                            if (quotes == 3) {
                                attr_len -= 2; // Odstranime predchadzajuce dve uvodzovky z konca
                                break;
                            }
                        } else {
                            quotes = 0;
                        }
                        ADD_CHAR(token, attr_len, c);
                    }
                    token.attribute[attr_len] = '\0';
                    token.type = TOKEN_STRING;
                    return token;
                } else {
                    // Prazdny jednoriadkovy retazec ""
                    if (next2 != EOF) unget_char(next2);
                    token.attribute[0] = '\0';
                    token.type = TOKEN_STRING;
                    return token;
                }
            } else {
                // Neprazdny jednoriadkovy retazec "..." [sekcia 1.1]
                c = next1;
                while (c != '"') {
                    if (c == '\n' || c == '\r' || c == EOF) {
                        token.type = TOKEN_ERROR;
                        return token;
                    }
                    if (c == '\\') {
                        c = next_char();
                        // Povolene su vyhradne escape sekvencie \\, \", \n, \r, \t [sekcia 1.1]
                        if (c == 'n') ADD_CHAR(token, attr_len, '\n');
                        else if (c == 't') ADD_CHAR(token, attr_len, '\t');
                        else if (c == 'r') ADD_CHAR(token, attr_len, '\r');
                        else if (c == '"') ADD_CHAR(token, attr_len, '"');
                        else if (c == '\\') ADD_CHAR(token, attr_len, '\\');
                        else { token.type = TOKEN_ERROR; return token; }
                    } else {
                        ADD_CHAR(token, attr_len, c);
                    }
                    c = next_char();
                }
                token.attribute[attr_len] = '\0';
                token.type = TOKEN_STRING;
                return token;
            }
        }

        // 11. Viacznakove a jednoznakove operatory [sekcia 3 a 5]
        if (c == '=') {
            int next_c = next_char();
            if (next_c == '=') { token.type = TOKEN_EQ; return token; }
            if (next_c == '>') { token.type = TOKEN_ARROW; return token; }
            if (next_c != EOF) unget_char(next_c);
            token.type = TOKEN_ASSIGN;
            return token;
        }
        if (c == '!') {
            int next_c = next_char();
            if (next_c == '=') { token.type = TOKEN_NEQ; return token; }
            if (next_c != EOF) unget_char(next_c);
            token.type = TOKEN_NOT;
            return token;
        }
        if (c == '<') {
            int next_c = next_char();
            if (next_c == '=') { token.type = TOKEN_LTE; return token; }
            if (next_c == '-') { token.type = TOKEN_LEFT_ARROW; return token; }
            if (next_c != EOF) unget_char(next_c);
            token.type = TOKEN_LT;
            return token;
        }
        if (c == '>') {
            int next_c = next_char();
            if (next_c == '=') { token.type = TOKEN_GTE; return token; }
            if (next_c != EOF) unget_char(next_c);
            token.type = TOKEN_GT;
            return token;
        }
        if (c == '+') {
            int next_c = next_char();
            if (next_c == '+') { token.type = TOKEN_CONCAT; return token; }
            if (next_c != EOF) unget_char(next_c);
            token.type = TOKEN_PLUS;
            return token;
        }
        if (c == '&') {
            int next_c = next_char();
            if (next_c == '&') { token.type = TOKEN_AND; return token; }
            if (next_c != EOF) unget_char(next_c);
            token.type = TOKEN_ERROR;
            return token;
        }
        if (c == '|') {
            int next_c = next_char();
            if (next_c == '|') { token.type = TOKEN_OR; return token; }
            if (next_c != EOF) unget_char(next_c);
            token.type = TOKEN_PIPE;
            return token;
        }

        // 12. Zvysne jednoznakove operatory a zatvorky
        switch (c) {
            case '-': token.type = TOKEN_MINUS; return token;
            case '*': token.type = TOKEN_MUL; return token;
            case '(':
                paren_depth++;
                token.type = TOKEN_LPAREN;
                return token;
            case ')':
                if (paren_depth > 0) paren_depth--;
                token.type = TOKEN_RPAREN;
                return token;
            case '[': token.type = TOKEN_LBRACKET; return token;
            case ']': token.type = TOKEN_RBRACKET; return token;
            case ':': token.type = TOKEN_COLON; return token;
            case ',': token.type = TOKEN_COMMA; return token;
            case ';': token.type = TOKEN_SEMICOLON; return token;
        }

        // Akykolvek iny neznamy znak vedie na lexikalnu chybu 1 [sekcia 1.1]
        token.type = TOKEN_ERROR;
        return token;
    }
}

/**
 * @brief Nacita a vrati dalsi token zo standardneho vstupu.
 * Ak sa atribut tokenu nezmestil do MAX_TOKEN_LEN, vrati TOKEN_ERROR.
 */
Token get_next_token(void) {
    attr_overflow = 0;
    Token token = scan_token();
    if (attr_overflow) {
        token.type = TOKEN_ERROR;
    }
    return token;
}
