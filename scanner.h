/*
 * Projekt: Implementacia prekladaca imperativneho jazyka IFJ26
 * Subor: scanner.h
 * Autor: Andrej Beles (xbelesa00)
 *
 * AI Prompt: "VYGENERUJ MI: Predpripraveny hlavickovy subor scanner.h pre jazyk
 * IFJ26 vratane vsetkych klucovych slov, operatorov a podpory pre odsadenie. + Oprav chyby"
 */

#ifndef SCANNER_H
#define SCANNER_H

// Maximalna dlzka atributu tokenu (v znakoch bez ukoncovacej nuly)
#define MAX_TOKEN_LEN 4095

// Typy lexikalnych jednotiek (tokenov) [podla sekcie 1.1 a 1.2 specifikacie]
typedef enum {
    // Specialne tokeny pre riadenie prekladu a odsadenia
    TOKEN_EOF,              // Koniec suboru
    TOKEN_EOL,              // Koniec logickeho riadku (\n)
    TOKEN_INDENT,           // Zvacsenie urovne odsadenia (zaciatok bloku)
    TOKEN_DEDENT,           // Zmensenie urovne odsadenia (koniec bloku)
    TOKEN_ERROR,            // Lexikalna chyba [navratovy kod 1]

    // Identifikator a literaly
    TOKEN_IDENTIFIER,       // Bezne meno premennej alebo funkcie
    TOKEN_UNDERSCORE,       // Samostatny znak '_' [nie je identifikator]
    TOKEN_INTEGER,          // Celociselny literal (Int)
    TOKEN_DOUBLE,           // Desatinny literal (Double)
    TOKEN_STRING,           // Retazcovy literal (String)
    TOKEN_INTERP_STRING,    // Interpolovany retazec s"..." (pre rozsirenie INTERPOLATEDS)

    // Zakladne klucove slova [sekcia 1.1]
    TOKEN_KW_DEF,           // def
    TOKEN_KW_DO,            // do
    TOKEN_KW_ELSE,          // else
    TOKEN_KW_IF,            // if
    TOKEN_KW_IMPORT,        // import [povinne pre prolog import ifj26.*]
    TOKEN_KW_RETURN,        // return
    TOKEN_KW_THEN,          // then
    TOKEN_KW_WHILE,         // while
    TOKEN_KW_VAL,           // val
    TOKEN_KW_VAR,           // var
    TOKEN_KW_INT,           // Int
    TOKEN_KW_DOUBLE_TYPE,   // Double
    TOKEN_KW_STRING_TYPE,   // String
    TOKEN_KW_UNIT,          // Unit

    // Klucove slova pre rozsirenia [sekcia 1.1 pod ciarou a sekcia 5]
    TOKEN_KW_BOOLEAN,       // Boolean
    TOKEN_KW_TRUE,          // true
    TOKEN_KW_FALSE,         // false
    TOKEN_KW_CASE,          // case
    TOKEN_KW_CLASS,         // class
    TOKEN_KW_EXTENSION,     // extension
    TOKEN_KW_FOR,           // for
    TOKEN_KW_LIST,          // List
    TOKEN_KW_MATCH,         // match
    TOKEN_KW_TYPE,          // type
    // Pozn.: 'to', 'until' a 'by' nie su klucove slova (nie su v zozname v sekcii 1.1),
    // scanner ich vracia ako TOKEN_IDENTIFIER a parser ich rozpozna podla atributu.

    // Operatory a oddelovace [sekcia 1.2, 2 a 3]
    TOKEN_ASSIGN,           // =
    TOKEN_PLUS,             // +
    TOKEN_CONCAT,           // ++ (pre rozsirenie LISTS)
    TOKEN_MINUS,            // -
    TOKEN_MUL,              // *
    TOKEN_DIV,              // /
    TOKEN_EQ,               // ==
    TOKEN_NEQ,              // !=
    TOKEN_LT,               // <
    TOKEN_LTE,              // <=
    TOKEN_GT,               // >
    TOKEN_GTE,              // >=
    TOKEN_NOT,              // !  (pre rozsirenie BOOLTHEN)
    TOKEN_AND,              // && (pre rozsirenie BOOLTHEN)
    TOKEN_OR,               // || (pre rozsirenie BOOLTHEN)
    TOKEN_PIPE,             // |  (pre rozsirenie ALGEBDT)
    TOKEN_ARROW,            // => (pre rozsirenia ALGEBDT a LAMBDAS)
    TOKEN_LEFT_ARROW,       // <- (pre rozsirenie CONTROL)
    TOKEN_LPAREN,           // (
    TOKEN_RPAREN,           // )
    TOKEN_LBRACKET,         // [  (pre rozsirenie LISTS)
    TOKEN_RBRACKET,         // ]  (pre rozsirenie LISTS)
    TOKEN_COLON,            // :
    TOKEN_COMMA,            // ,
    TOKEN_SEMICOLON,        // ;  [oddelovac prikazov na riadku]
    TOKEN_DOT,              // .  [pre import ifj26.* a clenske metody]
    TOKEN_AT                // @  [pre anotaciu @main]
} TokenType;

// Struktura reprezentujuca jeden nacitany token
typedef struct {
    TokenType type;                     // Typ tokenu
    char attribute[MAX_TOKEN_LEN + 1];  // Textova hodnota (pre id, cisla, stringy)
    int indent;                         // Pocet medzier na zaciatku aktualneho logickeho riadku
    int had_newline_before;             // 1 ak pred tokenom bol fyzicky koniec riadku (pre koncovu ciarku)
} Token;

/**
 * @brief Resetuje vnutornu pamat a zasobnik odsadenia lexikalneho analyzatora.
 */
void scanner_init(void);

/**
 * @brief Nacita a vrati dalsiu lexikalnu jednotku (token) zo standardneho vstupu.
 * Na konci vstupu vrati najprv TOKEN_EOL (ak posledny riadok nekonci LF), potom
 * zodpovedajuci pocet TOKEN_DEDENT a nakoniec TOKEN_EOF. Pri lexikalnej chybe
 * (vratane atributu dlhsieho ako MAX_TOKEN_LEN) vrati TOKEN_ERROR.
 * @return Struktura Token obsahujuca typ, atribut a informacie o odsadeni.
 */
Token get_next_token(void);

/** @return 1 ak bol pred naposledy nacitanym tokenom biely znak. */
int scanner_space_before_token(void);

#endif // SCANNER_H
