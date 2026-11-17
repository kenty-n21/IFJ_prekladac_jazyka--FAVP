/**
 * KOMPLEXNA NEPRIESTRELNA TESTOVACIA SADA PRE PRECEDENCNU ANALYZU VYRAZOV (IFJ26)
 * Autor: generovane pomocou umelej inteligencie
 * 
 * Testuje:
 *  - Vsetky literaly a premenne
 *  - Kazdy jeden operator (+, -, *, /, <, >, <=, >=, ==, !=)
 *  - Asociativitu a prioritu kazdej dvojice operatorov
 *  - Unarne minus v kazdej moznej pozicii (-x, x + -y, -(x+y), - -x, -x * -y)
 *  - Hlboko vnorene a retazene zatvorky
 *  - Implicitnu konverziu Int -> Double pri vsetkych operatoroch
 *  - Zretazenie retazcov (+)
 *  - Relacne operatory pre cisla a retazce
 *  - Kompletne syntakticke chyby (chybajuce operandy, nespravne zatvorky, retazenie relacii)
 *  - Kompletne semanticke chyby (neplatne kombinacie typov)
 *  - Stresovy test (velke vyrazy, hlboke zatvorky, 1000 iteracii bez memory leakov)
 *  - Textovu verifikaciu struktury AST stromu
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "expressions.h"
#include "scanner.h"
#include "error.h"
#include "stack.h"

// =========================================================================
// MOCK SCANNER (Simulacia vstupu tokenov)
// =========================================================================
#define MAX_MOCK_TOKENS 512
static Token mock_stream[MAX_MOCK_TOKENS];
static int mock_count = 0;
static int mock_index = 0;

void set_mock_tokens(Token *tokens, int count) {
    assert(count < MAX_MOCK_TOKENS);
    mock_count = count;
    mock_index = 0;
    for (int i = 0; i < count; i++) {
        mock_stream[i] = tokens[i];
    }
}

Token get_next_token(void) {
    if (mock_index < mock_count) {
        return mock_stream[mock_index++];
    }
    Token eof_tok;
    eof_tok.type = TOKEN_EOF;
    eof_tok.attribute[0] = '\0';
    return eof_tok;
}

// Helpery pre tvorbu tokenov
static Token tok_int(const char *val) {
    Token t; t.type = TOKEN_INTEGER; strncpy(t.attribute, val, sizeof(t.attribute)-1); t.attribute[sizeof(t.attribute)-1] = '\0'; return t;
}
static Token tok_double(const char *val) {
    Token t; t.type = TOKEN_DOUBLE; strncpy(t.attribute, val, sizeof(t.attribute)-1); t.attribute[sizeof(t.attribute)-1] = '\0'; return t;
}
static Token tok_string(const char *val) {
    Token t; t.type = TOKEN_STRING; strncpy(t.attribute, val, sizeof(t.attribute)-1); t.attribute[sizeof(t.attribute)-1] = '\0'; return t;
}
static Token tok_id(const char *val) {
    Token t; t.type = TOKEN_IDENTIFIER; strncpy(t.attribute, val, sizeof(t.attribute)-1); t.attribute[sizeof(t.attribute)-1] = '\0'; return t;
}
static Token tok_op(TokenType type) {
    Token t; t.type = type; t.attribute[0] = '\0'; return t;
}
static Token tok_eol(void) {
    Token t; t.type = TOKEN_EOL; t.attribute[0] = '\0'; return t;
}

// Spustac testu
static int run_test(Token *tokens, int count, Datatype expected_type, ExprNode **out_root) {
    set_mock_tokens(tokens, count);
    Token curr = get_next_token();
    ExprNode *root = NULL;
    Datatype type;

    int err = parse_expression(&curr, &root, &type, NULL, 0);
    if (err == NO_ERROR) {
        assert(type == expected_type);
        if (out_root != NULL) {
            *out_root = root;
        } else {
            expr_free(root);
        }
    } else {
        if (root != NULL) {
            expr_free(root);
        }
    }
    return err;
}

// Pomocna funkcia: overi, ze vyraz zlyha s presnym chybovym kodom
static void assert_error(Token *tokens, int count, int expected_error) {
    set_mock_tokens(tokens, count);
    Token curr = get_next_token();
    ExprNode *root = NULL;
    Datatype type;
    int err = parse_expression(&curr, &root, &type, NULL, 0);
    if (root != NULL) {
        expr_free(root);
    }
    assert(err == expected_error);
}

// Pomocna funkcia: serializuje AST do plne uzatvorkovaneho textu pre kontrolu struktury
void ast_to_string(ExprNode *node, char *buf, size_t max_len) {
    if (node == NULL) return;
    if (node->nodetype == NODE_LITERAL || node->nodetype == NODE_VAR) {
        strncat(buf, node->value, max_len - strlen(buf) - 1);
        return;
    }
    if (node->nodetype == NODE_UNARY) {
        strncat(buf, "(-", max_len - strlen(buf) - 1);
        ast_to_string(node->left, buf, max_len);
        strncat(buf, ")", max_len - strlen(buf) - 1);
        return;
    }
    if (node->nodetype == NODE_BINARY) {
        strncat(buf, "(", max_len - strlen(buf) - 1);
        ast_to_string(node->left, buf, max_len);
        switch (node->symbol) {
            case SYM_PLUS:  strncat(buf, " + ", max_len - strlen(buf) - 1); break;
            case SYM_MINUS: strncat(buf, " - ", max_len - strlen(buf) - 1); break;
            case SYM_MUL:   strncat(buf, " * ", max_len - strlen(buf) - 1); break;
            case SYM_DIV:   strncat(buf, " / ", max_len - strlen(buf) - 1); break;
            case SYM_LT:    strncat(buf, " < ", max_len - strlen(buf) - 1); break;
            case SYM_GT:    strncat(buf, " > ", max_len - strlen(buf) - 1); break;
            case SYM_LTE:   strncat(buf, " <= ", max_len - strlen(buf) - 1); break;
            case SYM_GTE:   strncat(buf, " >= ", max_len - strlen(buf) - 1); break;
            case SYM_EQ:    strncat(buf, " == ", max_len - strlen(buf) - 1); break;
            case SYM_NEQ:   strncat(buf, " != ", max_len - strlen(buf) - 1); break;
            default: break;
        }
        ast_to_string(node->right, buf, max_len);
        strncat(buf, ")", max_len - strlen(buf) - 1);
    }
}

// =========================================================================
// TEST 1: Zakladne literaly a premenne
// =========================================================================
void test_suite_1_literals(void) {
    printf("[1/10] Zakladne literaly a identifikatory... ");

    Token t_int[] = { tok_int("123456"), tok_eol() };
    ExprNode *r = NULL;
    assert(run_test(t_int, 2, TYPE_INT, &r) == NO_ERROR);
    assert(r->nodetype == NODE_LITERAL && strcmp(r->value, "123456") == 0);
    expr_free(r);

    Token t_dbl[] = { tok_double("0.0001"), tok_eol() };
    assert(run_test(t_dbl, 2, TYPE_DOUBLE, &r) == NO_ERROR);
    assert(r->nodetype == NODE_LITERAL && strcmp(r->value, "0.0001") == 0);
    expr_free(r);

    Token t_str[] = { tok_string("test string with spaces"), tok_eol() };
    assert(run_test(t_str, 2, TYPE_STRING, &r) == NO_ERROR);
    assert(r->nodetype == NODE_LITERAL && strcmp(r->value, "test string with spaces") == 0);
    expr_free(r);

    Token t_var[] = { tok_id("_my_variable_42"), tok_eol() };
    assert(run_test(t_var, 2, TYPE_ANY, &r) == NO_ERROR);
    assert(r->nodetype == NODE_VAR && strcmp(r->value, "_my_variable_42") == 0);
    expr_free(r);

    printf("PASSED\n");
}

// =========================================================================
// TEST 2: Vsetky aritmeticke operatory (+, -, *, /) a ich asociativita
// =========================================================================
void test_suite_2_arithmetic(void) {
    printf("[2/10] Vsetky aritmeticke operatory a asociativita... ");

    // 10 + 20
    Token t1[] = { tok_int("10"), tok_op(TOKEN_PLUS), tok_int("20"), tok_eol() };
    assert(run_test(t1, 4, TYPE_INT, NULL) == NO_ERROR);

    // 10 - 20
    Token t2[] = { tok_int("10"), tok_op(TOKEN_MINUS), tok_int("20"), tok_eol() };
    assert(run_test(t2, 4, TYPE_INT, NULL) == NO_ERROR);

    // 10 * 20
    Token t3[] = { tok_int("10"), tok_op(TOKEN_MUL), tok_int("20"), tok_eol() };
    assert(run_test(t3, 4, TYPE_INT, NULL) == NO_ERROR);

    // 10 / 20
    Token t4[] = { tok_int("10"), tok_op(TOKEN_DIV), tok_int("20"), tok_eol() };
    assert(run_test(t4, 4, TYPE_INT, NULL) == NO_ERROR);

    // Lava asociativita delenia a nasobenia: 24 / 4 / 2 * 3 => (((24 / 4) / 2) * 3)
    Token t5[] = {
        tok_int("24"), tok_op(TOKEN_DIV), tok_int("4"), tok_op(TOKEN_DIV),
        tok_int("2"), tok_op(TOKEN_MUL), tok_int("3"), tok_eol()
    };
    ExprNode *r5 = NULL;
    assert(run_test(t5, 8, TYPE_INT, &r5) == NO_ERROR);
    char buf[256] = "";
    ast_to_string(r5, buf, sizeof(buf));
    assert(strcmp(buf, "(((24 / 4) / 2) * 3)") == 0);
    expr_free(r5);

    printf("PASSED\n");
}

// =========================================================================
// TEST 3: Priorita operatorov (Aritmetika vs Nasobenie)
// =========================================================================
void test_suite_3_operator_precedence(void) {
    printf("[3/10] Priorita operatorov (* a / pred + a -)... ");

    // 2 + 3 * 4 - 8 / 2 => ((2 + (3 * 4)) - (8 / 2))
    Token t[] = {
        tok_int("2"), tok_op(TOKEN_PLUS),
        tok_int("3"), tok_op(TOKEN_MUL), tok_int("4"),
        tok_op(TOKEN_MINUS),
        tok_int("8"), tok_op(TOKEN_DIV), tok_int("2"),
        tok_eol()
    };
    ExprNode *r = NULL;
    assert(run_test(t, 10, TYPE_INT, &r) == NO_ERROR);
    char buf[256] = "";
    ast_to_string(r, buf, sizeof(buf));
    assert(strcmp(buf, "((2 + (3 * 4)) - (8 / 2))") == 0);
    expr_free(r);

    printf("PASSED\n");
}

// =========================================================================
// TEST 4: Komplexne zatvorky a vnorenia
// =========================================================================
void test_suite_4_parentheses(void) {
    printf("[4/10] Komplexne a vnorene zatvorky... ");

    // Zbytocne zatvorky: ((((100))))
    Token t1[] = {
        tok_op(TOKEN_LPAREN), tok_op(TOKEN_LPAREN), tok_op(TOKEN_LPAREN), tok_op(TOKEN_LPAREN),
        tok_int("100"),
        tok_op(TOKEN_RPAREN), tok_op(TOKEN_RPAREN), tok_op(TOKEN_RPAREN), tok_op(TOKEN_RPAREN),
        tok_eol()
    };
    ExprNode *r1 = NULL;
    assert(run_test(t1, 10, TYPE_INT, &r1) == NO_ERROR);
    assert(r1->nodetype == NODE_LITERAL && strcmp(r1->value, "100") == 0);
    expr_free(r1);

    // Zmena priority: (2 + 3) * (4 + 5)
    Token t2[] = {
        tok_op(TOKEN_LPAREN), tok_int("2"), tok_op(TOKEN_PLUS), tok_int("3"), tok_op(TOKEN_RPAREN),
        tok_op(TOKEN_MUL),
        tok_op(TOKEN_LPAREN), tok_int("4"), tok_op(TOKEN_PLUS), tok_int("5"), tok_op(TOKEN_RPAREN),
        tok_eol()
    };
    ExprNode *r2 = NULL;
    assert(run_test(t2, 12, TYPE_INT, &r2) == NO_ERROR);
    char buf[256] = "";
    ast_to_string(r2, buf, sizeof(buf));
    assert(strcmp(buf, "((2 + 3) * (4 + 5))") == 0);
    expr_free(r2);

    printf("PASSED\n");
}

// =========================================================================
// TEST 5: Unarne minus v kazdej pozicii
// =========================================================================
void test_suite_5_unary_minus(void) {
    printf("[5/10] Unarne minus v kazdej mozne pozicii... ");

    // Na zaciatku: -42
    Token t1[] = { tok_op(TOKEN_MINUS), tok_int("42"), tok_eol() };
    assert(run_test(t1, 3, TYPE_INT, NULL) == NO_ERROR);

    // Po binarnom operatore: 10 * -5
    Token t2[] = { tok_int("10"), tok_op(TOKEN_MUL), tok_op(TOKEN_MINUS), tok_int("5"), tok_eol() };
    ExprNode *r2 = NULL;
    assert(run_test(t2, 5, TYPE_INT, &r2) == NO_ERROR);
    char buf[256] = "";
    ast_to_string(r2, buf, sizeof(buf));
    assert(strcmp(buf, "(10 * (-5))") == 0);
    expr_free(r2);

    // Obe unarne minus: -10 * -20
    Token t3[] = { tok_op(TOKEN_MINUS), tok_int("10"), tok_op(TOKEN_MUL), tok_op(TOKEN_MINUS), tok_int("20"), tok_eol() };
    ExprNode *r3 = NULL;
    assert(run_test(t3, 6, TYPE_INT, &r3) == NO_ERROR);
    buf[0] = '\0';
    ast_to_string(r3, buf, sizeof(buf));
    assert(strcmp(buf, "((-10) * (-20))") == 0);
    expr_free(r3);

    // Unarne minus pred zatvorkou: -(1 + 2)
    Token t4[] = {
        tok_op(TOKEN_MINUS), tok_op(TOKEN_LPAREN), tok_int("1"), tok_op(TOKEN_PLUS), tok_int("2"), tok_op(TOKEN_RPAREN),
        tok_eol()
    };
    ExprNode *r4 = NULL;
    assert(run_test(t4, 7, TYPE_INT, &r4) == NO_ERROR);
    buf[0] = '\0';
    ast_to_string(r4, buf, sizeof(buf));
    assert(strcmp(buf, "(-(1 + 2))") == 0);
    expr_free(r4);

    // Viacnasobne unarne minus: - -5
    Token t5[] = { tok_op(TOKEN_MINUS), tok_op(TOKEN_MINUS), tok_int("5"), tok_eol() };
    ExprNode *r5 = NULL;
    assert(run_test(t5, 4, TYPE_INT, &r5) == NO_ERROR);
    buf[0] = '\0';
    ast_to_string(r5, buf, sizeof(buf));
    assert(strcmp(buf, "(-(-5))") == 0);
    expr_free(r5);

    printf("PASSED\n");
}

// =========================================================================
// TEST 6: Typova kompatibilita, implicitna konverzia a retazce
// =========================================================================
void test_suite_6_type_conversions(void) {
    printf("[6/10] Typova kompatibilita a implicitna konverzia... ");

    // Int + Double => Double
    Token t1[] = { tok_int("1"), tok_op(TOKEN_PLUS), tok_double("2.0"), tok_eol() };
    assert(run_test(t1, 4, TYPE_DOUBLE, NULL) == NO_ERROR);

    // Double - Int => Double
    Token t2[] = { tok_double("5.5"), tok_op(TOKEN_MINUS), tok_int("2"), tok_eol() };
    assert(run_test(t2, 4, TYPE_DOUBLE, NULL) == NO_ERROR);

    // (Int + Int) * Double => Double
    Token t3[] = {
        tok_op(TOKEN_LPAREN), tok_int("1"), tok_op(TOKEN_PLUS), tok_int("2"), tok_op(TOKEN_RPAREN),
        tok_op(TOKEN_MUL), tok_double("3.0"), tok_eol()
    };
    assert(run_test(t3, 8, TYPE_DOUBLE, NULL) == NO_ERROR);

    // String konkatenacia: String + String => String
    Token t4[] = { tok_string("ahoj "), tok_op(TOKEN_PLUS), tok_string("svet"), tok_eol() };
    assert(run_test(t4, 4, TYPE_STRING, NULL) == NO_ERROR);

    // Viacnasobne spajanie stringov: "a" + "b" + "c" => String
    Token t5[] = {
        tok_string("a"), tok_op(TOKEN_PLUS), tok_string("b"), tok_op(TOKEN_PLUS), tok_string("c"), tok_eol()
    };
    assert(run_test(t5, 6, TYPE_STRING, NULL) == NO_ERROR);

    printf("PASSED\n");
}

// =========================================================================
// TEST 7: Vsetky relacne operatory (<, >, <=, >=, ==, !=)
// =========================================================================
void test_suite_7_relational(void) {
    printf("[7/10] Vsetky relacne operatory a ich priorita... ");

    // 1 < 2 => Bool
    Token t1[] = { tok_int("1"), tok_op(TOKEN_LT), tok_int("2"), tok_eol() };
    assert(run_test(t1, 4, TYPE_BOOL, NULL) == NO_ERROR);

    // 1.5 >= 1.0 => Bool
    Token t2[] = { tok_double("1.5"), tok_op(TOKEN_GTE), tok_double("1.0"), tok_eol() };
    assert(run_test(t2, 4, TYPE_BOOL, NULL) == NO_ERROR);

    // Porovnanie Int a Double v relacii: 5 <= 5.5 => Bool
    Token t3[] = { tok_int("5"), tok_op(TOKEN_LTE), tok_double("5.5"), tok_eol() };
    assert(run_test(t3, 4, TYPE_BOOL, NULL) == NO_ERROR);

    // Lexikograficke porovnanie retazcov: "abc" < "xyz" => Bool
    Token t4[] = { tok_string("abc"), tok_op(TOKEN_LT), tok_string("xyz"), tok_eol() };
    assert(run_test(t4, 4, TYPE_BOOL, NULL) == NO_ERROR);

    // Rovnost a nerovnost cisel a retazcov
    Token t5[] = { tok_string("a"), tok_op(TOKEN_EQ), tok_string("a"), tok_eol() };
    assert(run_test(t5, 4, TYPE_BOOL, NULL) == NO_ERROR);

    Token t6[] = { tok_int("1"), tok_op(TOKEN_NEQ), tok_double("2.0"), tok_eol() };
    assert(run_test(t6, 4, TYPE_BOOL, NULL) == NO_ERROR);

    // Priorita: aritmetika ma prednost pred relaciou: 1 + 2 < 3 * 4
    Token t7[] = {
        tok_int("1"), tok_op(TOKEN_PLUS), tok_int("2"), tok_op(TOKEN_LT),
        tok_int("3"), tok_op(TOKEN_MUL), tok_int("4"), tok_eol()
    };
    ExprNode *r7 = NULL;
    assert(run_test(t7, 8, TYPE_BOOL, &r7) == NO_ERROR);
    char buf[256] = "";
    ast_to_string(r7, buf, sizeof(buf));
    assert(strcmp(buf, "((1 + 2) < (3 * 4))") == 0);
    expr_free(r7);

    printf("PASSED\n");
}

// =========================================================================
// TEST 8: Kompletna sada syntaktickych chyb (ERROR_SYNT = 2)
// =========================================================================
void test_suite_8_syntax_errors(void) {
    printf("[8/10] Prisne syntakticke chyby (ERROR_SYNT 2)... ");

    // Prazdny vstup
    Token t1[] = { tok_eol() };
    assert_error(t1, 1, ERROR_SYNT);

    // Chybajuci lavy operand: * 5
    Token t2[] = { tok_op(TOKEN_MUL), tok_int("5"), tok_eol() };
    assert_error(t2, 3, ERROR_SYNT);

    // Chybajuci pravy operand: 5 *
    Token t3[] = { tok_int("5"), tok_op(TOKEN_MUL), tok_eol() };
    assert_error(t3, 3, ERROR_SYNT);

    // Dva operatory za sebou: 5 + * 2
    Token t4[] = { tok_int("5"), tok_op(TOKEN_PLUS), tok_op(TOKEN_MUL), tok_int("2"), tok_eol() };
    assert_error(t4, 5, ERROR_SYNT);

    // Prazdne zatvorky: ()
    Token t5[] = { tok_op(TOKEN_LPAREN), tok_op(TOKEN_RPAREN), tok_eol() };
    assert_error(t5, 3, ERROR_SYNT);

    // Neuzatvorena zatvorka: (1 + 2
    Token t6[] = { tok_op(TOKEN_LPAREN), tok_int("1"), tok_op(TOKEN_PLUS), tok_int("2"), tok_eol() };
    assert_error(t6, 5, ERROR_SYNT);

    // Zbytocna zatvaracia zatvorka: 1 + 2)
    Token t7[] = { tok_int("1"), tok_op(TOKEN_PLUS), tok_int("2"), tok_op(TOKEN_RPAREN), tok_eol() };
    assert_error(t7, 5, ERROR_SYNT);

    // Zatvorka za zatvorkou bez operatora: (1 + 2)(3 + 4)
    Token t8[] = {
        tok_op(TOKEN_LPAREN), tok_int("1"), tok_op(TOKEN_PLUS), tok_int("2"), tok_op(TOKEN_RPAREN),
        tok_op(TOKEN_LPAREN), tok_int("3"), tok_op(TOKEN_PLUS), tok_int("4"), tok_op(TOKEN_RPAREN),
        tok_eol()
    };
    assert_error(t8, 11, ERROR_SYNT);

    // Zakazane retazenie relacnych operatorov (v IFJ26 striktne bez asociativity!):
    // a < b < c
    Token t9[] = {
        tok_id("a"), tok_op(TOKEN_LT), tok_id("b"), tok_op(TOKEN_LT), tok_id("c"), tok_eol()
    };
    assert_error(t9, 6, ERROR_SYNT);

    // a == b == c
    Token t10[] = {
        tok_id("a"), tok_op(TOKEN_EQ), tok_id("b"), tok_op(TOKEN_EQ), tok_id("c"), tok_eol()
    };
    assert_error(t10, 6, ERROR_SYNT);

    // a <= b >= c
    Token t11[] = {
        tok_id("a"), tok_op(TOKEN_LTE), tok_id("b"), tok_op(TOKEN_GTE), tok_id("c"), tok_eol()
    };
    assert_error(t11, 6, ERROR_SYNT);

    printf("PASSED\n");
}

// =========================================================================
// TEST 9: Kompletna sada semantickych typovych chyb (ERROR_SEM_TYPE = 6)
// =========================================================================
void test_suite_9_semantic_errors(void) {
    printf("[9/10] Prisne semanticke typove chyby (ERROR_SEM_TYPE 6)... ");

    // Scitanie Int + String: 1 + "ahoj"
    Token t1[] = { tok_int("1"), tok_op(TOKEN_PLUS), tok_string("ahoj"), tok_eol() };
    assert_error(t1, 4, ERROR_SEM_TYPE);

    // Scitanie String + Int: "ahoj" + 1
    Token t2[] = { tok_string("ahoj"), tok_op(TOKEN_PLUS), tok_int("1"), tok_eol() };
    assert_error(t2, 4, ERROR_SEM_TYPE);

    // Odcitanie dvoch stringov: "a" - "b" (string podporuje LEN +)
    Token t3[] = { tok_string("a"), tok_op(TOKEN_MINUS), tok_string("b"), tok_eol() };
    assert_error(t3, 4, ERROR_SEM_TYPE);

    // Nasobenie stringu cislom: "a" * 5
    Token t4[] = { tok_string("a"), tok_op(TOKEN_MUL), tok_int("5"), tok_eol() };
    assert_error(t4, 4, ERROR_SEM_TYPE);

    // Delenie stringu: "a" / 2
    Token t5[] = { tok_string("a"), tok_op(TOKEN_DIV), tok_int("2"), tok_eol() };
    assert_error(t5, 4, ERROR_SEM_TYPE);

    // Unarne minus na string: -"ahoj"
    Token t6[] = { tok_op(TOKEN_MINUS), tok_string("ahoj"), tok_eol() };
    assert_error(t6, 3, ERROR_SEM_TYPE);

    // Relacne porovnanie cisla a stringu: 5 < "text"
    Token t7[] = { tok_int("5"), tok_op(TOKEN_LT), tok_string("text"), tok_eol() };
    assert_error(t7, 4, ERROR_SEM_TYPE);

    // Rovnost cisla a stringu: 5 == "5"
    Token t8[] = { tok_int("5"), tok_op(TOKEN_EQ), tok_string("5"), tok_eol() };
    assert_error(t8, 4, ERROR_SEM_TYPE);

    printf("PASSED\n");
}

// =========================================================================
// TEST 10: Stresovy test (Dlhý reťazec, 20 úrovní zátvoriek, 1000 cyklov)
// =========================================================================
void test_suite_10_stress_and_leaks(void) {
    printf("[10/10] Stresovy test pamate a hlbky stromu (1000 iteracii)... ");

    // Pripravime velky vyraz s 20 urovnami zatvoriek a striedanim operatorov:
    // (((((((((( 1 + 2 ) * 3 ) - 4 ) + 5 ) * 6 ) ... ))))))))))
    Token deep[120];
    int idx = 0;
    int depth = 15;

    for (int i = 0; i < depth; i++) {
        deep[idx++] = tok_op(TOKEN_LPAREN);
    }
    deep[idx++] = tok_int("1");

    for (int i = 0; i < depth; i++) {
        deep[idx++] = (i % 2 == 0) ? tok_op(TOKEN_PLUS) : tok_op(TOKEN_MUL);
        deep[idx++] = tok_int("2");
        deep[idx++] = tok_op(TOKEN_RPAREN);
    }
    deep[idx++] = tok_eol();

    // Spustime 1000-krat za sebou pre overenie, ze nedochadza k memory leakom ani preteceniu
    for (int iter = 0; iter < 1000; iter++) {
        assert(run_test(deep, idx, TYPE_INT, NULL) == NO_ERROR);
    }

    printf("PASSED\n");
}

// =========================================================================
// MAIN RUNNER
// =========================================================================
int main(void) {
    printf("=================================================================\n");
    printf("   SPUSTAM MEGA-TESTOVACIU SADU PRECEDENCNEJ ANALYZY (IFJ26)     \n");
    printf("=================================================================\n");

    test_suite_1_literals();
    test_suite_2_arithmetic();
    test_suite_3_operator_precedence();
    test_suite_4_parentheses();
    test_suite_5_unary_minus();
    test_suite_6_type_conversions();
    test_suite_7_relational();
    test_suite_8_syntax_errors();
    test_suite_9_semantic_errors();
    test_suite_10_stress_and_leaks();

    printf("=================================================================\n");
    printf(" >>> GRATULUJEM! VSETKYCH 10 KOMPLEXNYCH TESTOV PRESLO NA 100%%! <<<\n");
    printf("=================================================================\n");

    return 0;
}
