/**
 * Projekt: IFJ26 kompilátor
 * Súbor: test_integration.c
 * Popis: Komplexná testovacia sada overujúca prepojenie precedenčnej analýzy výrazov
 *        s tabuľkou symbolov (symtable) a sémantikou parseru.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "scanner.h"
#include "expressions.h"
#include "symtable.h"
#include "error.h"

// Globálny mock scanner pre unit-testy výrazov s tabuľkou
static Token mock_tokens[100];
static int mock_token_count = 0;
static int mock_token_idx = 0;

static void set_mock_tokens(Token *tokens, int count) {
    mock_token_count = count;
    mock_token_idx = 0;
    for (int i = 0; i < count; i++) {
        mock_tokens[i] = tokens[i];
    }
}

Token get_next_token(void) {
    if (mock_token_idx < mock_token_count) {
        return mock_tokens[mock_token_idx++];
    }
    Token eof;
    eof.type = TOKEN_EOF;
    eof.attribute[0] = '\0';
    return eof;
}

// Pomocné funkcie na vytváranie tokenov
static Token tok(TokenType type, const char *attr) {
    Token t;
    t.type = type;
    if (attr) {
        strncpy(t.attribute, attr, MAX_TOKEN_LEN);
        t.attribute[MAX_TOKEN_LEN] = '\0';
    } else {
        t.attribute[0] = '\0';
    }
    return t;
}

// Spustí parse_expression nad mock tokenmi a danou tabuľkou symbolov
static int run_expr(Token *tokens, int count, symtable *table, int scope_level, Datatype *out_type, ExprNode **out_root) {
    set_mock_tokens(tokens, count);
    Token curr = get_next_token();
    ExprNode *root = NULL;
    Datatype type = TYPE_ANY;

    int err = parse_expression(&curr, &root, &type, table, scope_level);
    if (err == NO_ERROR) {
        if (out_type) *out_type = type;
        if (out_root) {
            *out_root = root;
        } else {
            expr_free(root);
        }
    } else {
        if (root) expr_free(root);
    }
    return err;
}

// =========================================================================
// TESTOVACIE SADY
// =========================================================================

void test_suite_1_variable_types(symtable *table) {
    printf("[1/10] Zistenie typu premennej zo symtable...");

    Datatype type;

    // x (Int) -> TYPE_INT
    Token t1[] = { tok(TOKEN_IDENTIFIER, "x_int"), tok(TOKEN_EOL, NULL) };
    int err = run_expr(t1, 2, table, 0, &type, NULL);
    assert(err == NO_ERROR);
    assert(type == TYPE_INT);

    // y (Double) -> TYPE_DOUBLE
    Token t2[] = { tok(TOKEN_IDENTIFIER, "y_double"), tok(TOKEN_EOL, NULL) };
    err = run_expr(t2, 2, table, 0, &type, NULL);
    assert(err == NO_ERROR);
    assert(type == TYPE_DOUBLE);

    // s (String) -> TYPE_STRING
    Token t3[] = { tok(TOKEN_IDENTIFIER, "s_str"), tok(TOKEN_EOL, NULL) };
    err = run_expr(t3, 2, table, 0, &type, NULL);
    assert(err == NO_ERROR);
    assert(type == TYPE_STRING);

    printf(" PASSED\n");
}

void test_suite_2_implicit_conversion_with_vars(symtable *table) {
    printf("[2/10] Implicitná konverzia Int -> Double s premennými...");

    Datatype type;

    // x_int + y_double -> Double
    Token t1[] = {
        tok(TOKEN_IDENTIFIER, "x_int"),
        tok(TOKEN_PLUS, NULL),
        tok(TOKEN_IDENTIFIER, "y_double"),
        tok(TOKEN_EOL, NULL)
    };
    int err = run_expr(t1, 4, table, 0, &type, NULL);
    assert(err == NO_ERROR);
    assert(type == TYPE_DOUBLE);

    // y_double * 5 (Double * Int) -> Double
    Token t2[] = {
        tok(TOKEN_IDENTIFIER, "y_double"),
        tok(TOKEN_MUL, NULL),
        tok(TOKEN_INTEGER, "5"),
        tok(TOKEN_EOL, NULL)
    };
    err = run_expr(t2, 4, table, 0, &type, NULL);
    assert(err == NO_ERROR);
    assert(type == TYPE_DOUBLE);

    // 10 / x_int (Int / Int) -> Int
    Token t3[] = {
        tok(TOKEN_INTEGER, "10"),
        tok(TOKEN_DIV, NULL),
        tok(TOKEN_IDENTIFIER, "x_int"),
        tok(TOKEN_EOL, NULL)
    };
    err = run_expr(t3, 4, table, 0, &type, NULL);
    assert(err == NO_ERROR);
    assert(type == TYPE_INT);

    printf(" PASSED\n");
}

void test_suite_3_scope_shadowing(symtable *table) {
    printf("[3/10] Rešpektovanie zanorenia a tieňovanie premenných...");

    Datatype type;

    // Na úrovni 0: shadowed_var je TYPE_INT
    Token t1[] = { tok(TOKEN_IDENTIFIER, "shadowed_var"), tok(TOKEN_EOL, NULL) };
    int err = run_expr(t1, 2, table, 0, &type, NULL);
    assert(err == NO_ERROR);
    assert(type == TYPE_INT);

    // Na úrovni 1: shadowed_var je prekrytá na TYPE_DOUBLE
    err = run_expr(t1, 2, table, 1, &type, NULL);
    assert(err == NO_ERROR);
    assert(type == TYPE_DOUBLE);

    // Na úrovni 1: shadowed_var + 2 -> Double + Int -> Double
    Token t2[] = {
        tok(TOKEN_IDENTIFIER, "shadowed_var"),
        tok(TOKEN_PLUS, NULL),
        tok(TOKEN_INTEGER, "2"),
        tok(TOKEN_EOL, NULL)
    };
    err = run_expr(t2, 4, table, 1, &type, NULL);
    assert(err == NO_ERROR);
    assert(type == TYPE_DOUBLE);

    printf(" PASSED\n");
}

void test_suite_4_exit_scope_visibility(symtable *table) {
    (void)table; // nepouzity parameter, pouzivame lokalnu tabulku
    printf("[4/10] Viditeľnosť premenných po exit_scope...");

    // Vytvoríme lokálnu tabuľku pre tento test
    symtable *local_tab = init_hash_table();
    assert(local_tab != NULL);

    // Scope 0: globálna premenná g
    insert_var(local_tab, "g", TYPE_INT, 0);

    // Scope 1: lokálna premenná local_var
    insert_var(local_tab, "local_var", TYPE_DOUBLE, 1);

    Datatype type;
    Token t_local[] = { tok(TOKEN_IDENTIFIER, "local_var"), tok(TOKEN_EOL, NULL) };

    // V scope 1 je viditeľná
    int err = run_expr(t_local, 2, local_tab, 1, &type, NULL);
    assert(err == NO_ERROR);
    assert(type == TYPE_DOUBLE);

    // Opustíme scope 1
    exit_scope(1, local_tab);

    // V scope 0 už NESMIE byť viditeľná -> chyba 3!
    err = run_expr(t_local, 2, local_tab, 0, &type, NULL);
    assert(err == ERROR_SEM_UNDEF);

    // Premenná g musí zostať viditeľná
    Token t_g[] = { tok(TOKEN_IDENTIFIER, "g"), tok(TOKEN_EOL, NULL) };
    err = run_expr(t_g, 2, local_tab, 0, &type, NULL);
    assert(err == NO_ERROR);
    assert(type == TYPE_INT);

    delete_hash_tabulka(local_tab);
    printf(" PASSED\n");
}

void test_suite_5_undefined_variables(symtable *table) {
    printf("[5/10] Detekcia nedeklarovaných premenných (Chyba 3)...");

    Datatype type;

    // Samostatná nedeklarovaná premenná
    Token t1[] = { tok(TOKEN_IDENTIFIER, "neznama_premenna"), tok(TOKEN_EOL, NULL) };
    int err = run_expr(t1, 2, table, 0, &type, NULL);
    assert(err == ERROR_SEM_UNDEF);

    // Nedeklarovaná premenná v zložitom výraze: x_int + (unknown * 2)
    Token t2[] = {
        tok(TOKEN_IDENTIFIER, "x_int"),
        tok(TOKEN_PLUS, NULL),
        tok(TOKEN_LPAREN, NULL),
        tok(TOKEN_IDENTIFIER, "unknown"),
        tok(TOKEN_MUL, NULL),
        tok(TOKEN_INTEGER, "2"),
        tok(TOKEN_RPAREN, NULL),
        tok(TOKEN_EOL, NULL)
    };
    err = run_expr(t2, 8, table, 0, &type, NULL);
    assert(err == ERROR_SEM_UNDEF);

    printf(" PASSED\n");
}

void test_suite_6_semantic_type_errors(symtable *table) {
    printf("[6/10] Prísne typové chyby s premennými (Chyba 6)...");

    Datatype type;

    // x_int + s_str (Int + String -> Chyba 6)
    Token t1[] = {
        tok(TOKEN_IDENTIFIER, "x_int"),
        tok(TOKEN_PLUS, NULL),
        tok(TOKEN_IDENTIFIER, "s_str"),
        tok(TOKEN_EOL, NULL)
    };
    int err = run_expr(t1, 4, table, 0, &type, NULL);
    assert(err == ERROR_SEM_TYPE);

    // s_str * 5 (String * Int -> Chyba 6)
    Token t2[] = {
        tok(TOKEN_IDENTIFIER, "s_str"),
        tok(TOKEN_MUL, NULL),
        tok(TOKEN_INTEGER, "5"),
        tok(TOKEN_EOL, NULL)
    };
    err = run_expr(t2, 4, table, 0, &type, NULL);
    assert(err == ERROR_SEM_TYPE);

    // Unárne mínus na reťazec: -s_str -> Chyba 6
    Token t3[] = {
        tok(TOKEN_MINUS, NULL),
        tok(TOKEN_IDENTIFIER, "s_str"),
        tok(TOKEN_EOL, NULL)
    };
    err = run_expr(t3, 3, table, 0, &type, NULL);
    assert(err == ERROR_SEM_TYPE);

    printf(" PASSED\n");
}

void test_suite_7_string_operations(symtable *table) {
    printf("[7/10] Operácie s reťazcovými premennými...");

    Datatype type;

    // s_str + s_str2 (String + String -> String)
    Token t1[] = {
        tok(TOKEN_IDENTIFIER, "s_str"),
        tok(TOKEN_PLUS, NULL),
        tok(TOKEN_IDENTIFIER, "s_str2"),
        tok(TOKEN_EOL, NULL)
    };
    int err = run_expr(t1, 4, table, 0, &type, NULL);
    assert(err == NO_ERROR);
    assert(type == TYPE_STRING);

    // s_str + " svet" (String + String literál -> String)
    Token t2[] = {
        tok(TOKEN_IDENTIFIER, "s_str"),
        tok(TOKEN_PLUS, NULL),
        tok(TOKEN_STRING, " svet"),
        tok(TOKEN_EOL, NULL)
    };
    err = run_expr(t2, 4, table, 0, &type, NULL);
    assert(err == NO_ERROR);
    assert(type == TYPE_STRING);

    printf(" PASSED\n");
}

void test_suite_8_relational_expressions(symtable *table) {
    printf("[8/10] Relačné a porovnávacie výrazy premenných...");

    Datatype type;

    // x_int < y_double -> TYPE_BOOL
    Token t1[] = {
        tok(TOKEN_IDENTIFIER, "x_int"),
        tok(TOKEN_LT, NULL),
        tok(TOKEN_IDENTIFIER, "y_double"),
        tok(TOKEN_EOL, NULL)
    };
    int err = run_expr(t1, 4, table, 0, &type, NULL);
    assert(err == NO_ERROR);
    assert(type == TYPE_BOOL);

    // s_str == s_str2 -> TYPE_BOOL
    Token t2[] = {
        tok(TOKEN_IDENTIFIER, "s_str"),
        tok(TOKEN_EQ, NULL),
        tok(TOKEN_IDENTIFIER, "s_str2"),
        tok(TOKEN_EOL, NULL)
    };
    err = run_expr(t2, 4, table, 0, &type, NULL);
    assert(err == NO_ERROR);
    assert(type == TYPE_BOOL);

    // x_int == s_str (Int == String -> Chyba 6)
    Token t3[] = {
        tok(TOKEN_IDENTIFIER, "x_int"),
        tok(TOKEN_EQ, NULL),
        tok(TOKEN_IDENTIFIER, "s_str"),
        tok(TOKEN_EOL, NULL)
    };
    err = run_expr(t3, 4, table, 0, &type, NULL);
    assert(err == ERROR_SEM_TYPE);

    printf(" PASSED\n");
}

void test_suite_9_complex_expressions(symtable *table) {
    printf("[9/10] Komplexné zátvorkové výrazy s premennými...");

    Datatype type;

    // -(x_int + 5) * (y_double / 2.0) -> Double
    Token t1[] = {
        tok(TOKEN_MINUS, NULL),
        tok(TOKEN_LPAREN, NULL),
        tok(TOKEN_IDENTIFIER, "x_int"),
        tok(TOKEN_PLUS, NULL),
        tok(TOKEN_INTEGER, "5"),
        tok(TOKEN_RPAREN, NULL),
        tok(TOKEN_MUL, NULL),
        tok(TOKEN_LPAREN, NULL),
        tok(TOKEN_IDENTIFIER, "y_double"),
        tok(TOKEN_DIV, NULL),
        tok(TOKEN_DOUBLE, "2.0"),
        tok(TOKEN_RPAREN, NULL),
        tok(TOKEN_EOL, NULL)
    };
    int err = run_expr(t1, 13, table, 0, &type, NULL);
    assert(err == NO_ERROR);
    assert(type == TYPE_DOUBLE);

    // x_int + y_double > 100.5 -> TYPE_BOOL
    Token t2[] = {
        tok(TOKEN_IDENTIFIER, "x_int"),
        tok(TOKEN_PLUS, NULL),
        tok(TOKEN_IDENTIFIER, "y_double"),
        tok(TOKEN_GT, NULL),
        tok(TOKEN_DOUBLE, "100.5"),
        tok(TOKEN_EOL, NULL)
    };
    err = run_expr(t2, 6, table, 0, &type, NULL);
    assert(err == NO_ERROR);
    assert(type == TYPE_BOOL);

    printf(" PASSED\n");
}

void test_suite_10_ast_structure_validation(symtable *table) {
    printf("[10/10] Validácia generovaného AST stromu pre premenné...");

    Datatype type;
    ExprNode *root = NULL;

    // Výraz: x_int + 42
    Token t1[] = {
        tok(TOKEN_IDENTIFIER, "x_int"),
        tok(TOKEN_PLUS, NULL),
        tok(TOKEN_INTEGER, "42"),
        tok(TOKEN_EOL, NULL)
    };
    int err = run_expr(t1, 4, table, 0, &type, &root);
    assert(err == NO_ERROR);
    assert(root != NULL);

    // Koreň musí byť binárny operátor PLUS s typom TYPE_INT
    assert(root->nodetype == NODE_BINARY);
    assert(root->symbol == SYM_PLUS);
    assert(root->datatype == TYPE_INT);

    // Ľavý syn musí byť NODE_VAR pre "x_int" s typom TYPE_INT
    assert(root->left != NULL);
    assert(root->left->nodetype == NODE_VAR);
    assert(strcmp(root->left->value, "x_int") == 0);
    assert(root->left->datatype == TYPE_INT);

    // Pravý syn musí byť NODE_LITERAL pre "42" s typom TYPE_INT
    assert(root->right != NULL);
    assert(root->right->nodetype == NODE_LITERAL);
    assert(strcmp(root->right->value, "42") == 0);
    assert(root->right->datatype == TYPE_INT);

    expr_free(root);
    printf(" PASSED\n");
}

// =========================================================================
// MAIN
// =========================================================================

int main(void) {
    printf("=================================================================\n");
    printf("   TEST PREPOJENIA: PARSER + SYMTABLE + PRECEDENCNA ANALYZA      \n");
    printf("=================================================================\n");

    // Inicializácia testovacej tabuľky symbolov
    symtable *table = init_hash_table();
    assert(table != NULL);

    // Vloženie testovacích premenných na úrovni 0 (globálne)
    bool ok1 = insert_var(table, "x_int", TYPE_INT, 0);
    bool ok2 = insert_var(table, "y_double", TYPE_DOUBLE, 0);
    bool ok3 = insert_var(table, "s_str", TYPE_STRING, 0);
    bool ok4 = insert_var(table, "s_str2", TYPE_STRING, 0);
    bool ok5 = insert_var(table, "shadowed_var", TYPE_INT, 0);
    assert(ok1 && ok2 && ok3 && ok4 && ok5);

    // Vloženie tieňovanej premennej na úrovni 1 (lokálna)
    bool ok6 = insert_var(table, "shadowed_var", TYPE_DOUBLE, 1);
    assert(ok6);

    // Spustenie všetkých 10 sád
    test_suite_1_variable_types(table);
    test_suite_2_implicit_conversion_with_vars(table);
    test_suite_3_scope_shadowing(table);
    test_suite_4_exit_scope_visibility(table);
    test_suite_5_undefined_variables(table);
    test_suite_6_semantic_type_errors(table);
    test_suite_7_string_operations(table);
    test_suite_8_relational_expressions(table);
    test_suite_9_complex_expressions(table);
    test_suite_10_ast_structure_validation(table);

    // Vyčistenie tabuľky
    delete_hash_tabulka(table);

    printf("=================================================================\n");
    printf(" >>> GRATULUJEM! VSETKYCH 10 TESTOV PREPOJENIA BOLO USPESNYCH! <<<\n");
    printf("=================================================================\n");

    return 0;
}
