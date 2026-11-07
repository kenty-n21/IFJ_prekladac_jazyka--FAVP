#include "symtable.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// generovane AI

// ============================================================================
// FAREBNÝ TESTOVACÍ RÁMEC (ANSI escape codes)
// ============================================================================
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[1;31m"
#define COLOR_GREEN "\033[1;32m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_CYAN "\033[1;36m"
#define COLOR_MAGENTA "\033[1;35m"
#define COLOR_BOLD "\033[1m"

static int total_assertions = 0;
static int passed_assertions = 0;
static int failed_assertions = 0;

static int current_test_failed = 0;

#define ASSERT(condition, message)                                                                  \
    do {                                                                                            \
        total_assertions++;                                                                         \
        if (condition) {                                                                            \
            passed_assertions++;                                                                    \
        } else {                                                                                    \
            failed_assertions++;                                                                    \
            current_test_failed = 1;                                                                \
            printf(COLOR_RED "    [FAIL] %s:%d: %s" COLOR_RESET "\n", __FILE__, __LINE__, message); \
        }                                                                                           \
    } while (0)

#define ASSERT_TRUE(cond) ASSERT((cond), "Očakávalo sa TRUE (platnosť)")
#define ASSERT_FALSE(cond) ASSERT(!(cond), "Očakávalo sa FALSE (neplatnosť)")
#define ASSERT_NOT_NULL(ptr) ASSERT((ptr) != NULL, "Ukazovateľ nesmie byť NULL")
#define ASSERT_NULL(ptr) ASSERT((ptr) == NULL, "Ukazovateľ musí byť NULL")
#define ASSERT_EQ_INT(a, b) ASSERT((a) == (b), "Hodnoty int sa nezhodujú")
#define ASSERT_EQ_STR(a, b) ASSERT((a) != NULL && (b) != NULL && strcmp((a), (b)) == 0, "Reťazce sa nezhodujú")

#define TEST_SECTION(name)                                                                            \
    printf("\n" COLOR_MAGENTA "==================================================" COLOR_RESET "\n"); \
    printf(COLOR_BOLD "%s" COLOR_RESET "\n", name);                                                   \
    printf(COLOR_MAGENTA "==================================================" COLOR_RESET "\n")

#define RUN_TEST(test_func)                                                     \
    do {                                                                        \
        current_test_failed = 0;                                                \
        printf(COLOR_CYAN "[ RUN      ] " COLOR_RESET "%s\n", #test_func);      \
        test_func();                                                            \
        if (!current_test_failed) {                                             \
            printf(COLOR_GREEN "[       OK ] " COLOR_RESET "%s\n", #test_func); \
        } else {                                                                \
            printf(COLOR_RED "[  FAILED  ] " COLOR_RESET "%s\n", #test_func);   \
        }                                                                       \
    } while (0)

// ============================================================================
// INDIVIDUÁLNE TESTOVACIE PRÍPADY
// ============================================================================

// 1. Test inicializácie a korektného uvoľnenia
void test_init_and_cleanup(void) {
    symtable *t = init_hash_table();
    ASSERT_NOT_NULL(t);
    if (t) {
        ASSERT_EQ_INT(t->pocet_synbolov, 0);
        ASSERT_NOT_NULL(t->hash_table);
    }
    delete_hash_tabulka(t);
}

// 2. Vkladanie a vyhľadávanie jednoduchých premenných
void test_insert_and_lookup_var(void) {
    symtable *t = init_hash_table();
    ASSERT_NOT_NULL(t);

    bool res1 = insert_var(t, "counter", type_int, 0);
    ASSERT_TRUE(res1);

    bool res2 = insert_var(t, "price", type_double, 0);
    ASSERT_TRUE(res2);

    bool res3 = insert_var(t, "is_valid", type_bool, 0);
    ASSERT_TRUE(res3);

    // Lookup premennej s prefixom úrovne (podľa dokumentácie v hlavičkovom súbore: "0$meno")
    Symboldata *s1 = lookup_var(0, "0$counter", t);
    ASSERT_NOT_NULL(s1);
    if (s1) {
        ASSERT_EQ_INT(s1->typ, type_int);
        ASSERT_FALSE(s1->is_function);
        ASSERT_EQ_INT(s1->scope_level, 0);
    }

    Symboldata *s2 = lookup_var(0, "0$price", t);
    ASSERT_NOT_NULL(s2);
    if (s2) {
        ASSERT_EQ_INT(s2->typ, type_double);
    }

    // Vyhľadávanie neexistujúcej premennej
    Symboldata *s_null = lookup_var(0, "0$nonexistent", t);
    ASSERT_NULL(s_null);

    delete_hash_tabulka(t);
}

// 3. Vkladanie a vyhľadávanie funkcií a ich parametrov
void test_insert_and_lookup_func(void) {
    symtable *t = init_hash_table();
    ASSERT_NOT_NULL(t);

    // Funkcia foo(int, string) -> double
    Datatype *params = malloc(sizeof(Datatype) * 2);
    params[0] = type_int;
    params[1] = type_string;

    bool res = insert_func(t, "foo", type_double, 2, params);
    ASSERT_TRUE(res);

    // Vyhľadanie funkcie (bez prefixu $)
    Symboldata *f = lookup_var(0, "foo", t);
    ASSERT_NOT_NULL(f);
    if (f) {
        ASSERT_TRUE(f->is_function);
        ASSERT_EQ_INT(f->typ, type_double);
        ASSERT_EQ_INT(f->param_count, 2);
        ASSERT_NOT_NULL(f->param_type);
        if (f->param_type) {
            ASSERT_EQ_INT(f->param_type[0], type_int);
            ASSERT_EQ_INT(f->param_type[1], type_string);
        }
    }

    delete_hash_tabulka(t);
}

// 4. Test vstavaných funkcií
void test_builtin_functions(void) {
    symtable *t = init_hash_table();
    ASSERT_NOT_NULL(t);

    insert_vstavane_funkcie(t);
    ASSERT(t->pocet_synbolov > 0, "Tabuľka by mala obsahovať vstavané funkcie");

    delete_hash_tabulka(t);
}

// 5. Test zanorenia (scope) a opustenia rozsahov (exit_scope)
void test_scope_management(void) {
    symtable *t = init_hash_table();
    ASSERT_NOT_NULL(t);

    // Vloženie premenných na rôznych úrovniach zanorenia
    insert_var(t, "global_var", type_int, 0);    // 0$global_var
    insert_var(t, "local_var1", type_string, 1); // 1$local_var1
    insert_var(t, "local_var2", type_double, 1); // 1$local_var2
    insert_var(t, "nested_var", type_bool, 2);   // 2$nested_var

    // Skontrolujeme, že existujú
    ASSERT_NOT_NULL(lookup_var(0, "0$global_var", t));
    ASSERT_NOT_NULL(lookup_var(1, "1$local_var1", t));
    ASSERT_NOT_NULL(lookup_var(1, "1$local_var2", t));
    ASSERT_NOT_NULL(lookup_var(2, "2$nested_var", t));

    // Opustíme scope level 2
    exit_scope(2, t);
    ASSERT_NULL(lookup_var(2, "2$nested_var", t));
    ASSERT_NOT_NULL(lookup_var(1, "1$local_var1", t)); // Level 1 stále žije

    // Opustíme scope level 1
    exit_scope(1, t);
    ASSERT_NULL(lookup_var(1, "1$local_var1", t));
    ASSERT_NULL(lookup_var(1, "1$local_var2", t));
    ASSERT_NOT_NULL(lookup_var(0, "0$global_var", t)); // Level 0 stále žije

    delete_hash_tabulka(t);
}

// 6. Test manuálneho mazania prvkov (hash_table_delete)
void test_explicit_deletion(void) {
    symtable *t = init_hash_table();
    ASSERT_NOT_NULL(t);

    insert_var(t, "temp_var", type_int, 0);
    ASSERT_NOT_NULL(lookup_var(0, "0$temp_var", t));

    // Vymazanie z tabuľky
    bool deleted = hash_table_delete("0$temp_var", t);
    ASSERT_TRUE(deleted);

    // Po zmazaní by mal byť výsledok lookup NULL
    ASSERT_NULL(lookup_var(0, "0$temp_var", t));

    // Vymazanie neexistujúceho prvku
    bool deleted_nonexistent = hash_table_delete("0$ghost", t);
    ASSERT_FALSE(deleted_nonexistent);

    delete_hash_tabulka(t);
}

// 7. HRANIČNÝ TEST: Shadowing premenných (rovnaké meno v rôznych úrovniach)
void test_variable_shadowing(void) {
    symtable *t = init_hash_table();
    ASSERT_NOT_NULL(t);

    // Rovnaký identifikátor "x" na úrovni 0 aj 1
    insert_var(t, "x", type_int, 0);    // 0$x
    insert_var(t, "x", type_string, 1); // 1$x

    Symboldata *x0 = lookup_var(0, "0$x", t);
    Symboldata *x1 = lookup_var(1, "1$x", t);

    ASSERT_NOT_NULL(x0);
    ASSERT_NOT_NULL(x1);

    if (x0 && x1) {
        ASSERT_EQ_INT(x0->typ, type_int);
        ASSERT_EQ_INT(x1->typ, type_string);
    }

    // Odstránenie len zanoreného x
    exit_scope(1, t);
    ASSERT_NULL(lookup_var(1, "1$x", t));
    ASSERT_NOT_NULL(lookup_var(0, "0$x", t));

    delete_hash_tabulka(t);
}

// 8. HRANIČNÝ TEST: Masívne vkladanie (zťažkávací test na kolízie a pretečenie)
void test_stress_and_collisions(void) {
    symtable *t = init_hash_table();
    ASSERT_NOT_NULL(t);

    const int ITEM_COUNT = 150;
    char buffer[64];

    // Vloženie 150 prvkov
    for (int i = 0; i < ITEM_COUNT; i++) {
        snprintf(buffer, sizeof(buffer), "var_%d", i);
        bool inserted = insert_var(t, buffer, type_int, 0);
        ASSERT_TRUE(inserted);
    }

    // Overenie všetkých 150 prvkov
    for (int i = 0; i < ITEM_COUNT; i++) {
        snprintf(buffer, sizeof(buffer), "0$var_%d", i);
        Symboldata *s = lookup_var(0, buffer, t);
        ASSERT_NOT_NULL(s);
    }

    delete_hash_tabulka(t);
}

// 9. HRANIČNÝ TEST: Opätovné vloženie po zmazaní (overenie funkčnosti tombstones `delete_item`)
void test_tombstone_reuse(void) {
    symtable *t = init_hash_table();
    ASSERT_NOT_NULL(t);

    insert_var(t, "alpha", type_int, 0);
    insert_var(t, "beta", type_double, 0);

    // Zmažeme "alpha" (vytvorí sa tombstone delete_item)
    hash_table_delete("0$alpha", t);
    ASSERT_NULL(lookup_var(0, "0$alpha", t));

    // "beta" musí byť stále nájditeľná aj cez vymazané miesto
    ASSERT_NOT_NULL(lookup_var(0, "0$beta", t));

    // Re-insert "alpha"
    bool reinserted = insert_var(t, "alpha", type_bool, 0);
    ASSERT_TRUE(reinserted);

    Symboldata *re_s = lookup_var(0, "0$alpha", t);
    ASSERT_NOT_NULL(re_s);
    if (re_s) {
        ASSERT_EQ_INT(re_s->typ, type_bool);
    }

    delete_hash_tabulka(t);
}

// 10. HRANIČNÝ TEST: Prázdne a špeciálne vstupy
void test_edge_cases(void) {
    symtable *t = init_hash_table();
    ASSERT_NOT_NULL(t);

    // Hľadanie v prázdnej tabuľke
    Symboldata *empty_look = lookup_var(0, "0$anything", t);
    ASSERT_NULL(empty_look);

    // Mazanie z prázdnej tabuľky
    bool del_empty = hash_table_delete("0$anything", t);
    ASSERT_FALSE(del_empty);

    // Opustenie rozsahu na prázdnej tabuľke (nesmie spadnúť)
    exit_scope(5, t);

    delete_hash_tabulka(t);
}

// ============================================================================
// HLAVNÁ FUNKCIA (MAIN)
// ============================================================================
int main(void) {
    printf(COLOR_BOLD COLOR_YELLOW "\n=== SPUSTAM TESTOVACIU SADU SYMBOLICKEJ TABULKY ===" COLOR_RESET "\n");

    TEST_SECTION("1. ZAKLADNE OPERACIE & PAMAT");
    RUN_TEST(test_init_and_cleanup);
    RUN_TEST(test_insert_and_lookup_var);
    RUN_TEST(test_insert_and_lookup_func);
    RUN_TEST(test_builtin_functions);

    TEST_SECTION("2. RIADENIE SCOPE & MAZANIE");
    RUN_TEST(test_scope_management);
    RUN_TEST(test_explicit_deletion);

    TEST_SECTION("3. HRANICNE STAVY & STRESS TESTY");
    RUN_TEST(test_variable_shadowing);
    RUN_TEST(test_stress_and_collisions);
    RUN_TEST(test_tombstone_reuse);
    RUN_TEST(test_edge_cases);

    // Záverečné vyhodnotenie
    printf("\n" COLOR_MAGENTA "==================================================" COLOR_RESET "\n");
    printf(COLOR_BOLD "ZHRNUTIE TESTOV:" COLOR_RESET "\n");
    printf(" Celkovo testovacích tvrdení (assertions): %d\n", total_assertions);
    printf(COLOR_GREEN " USPESNE:  %d" COLOR_RESET "\n", passed_assertions);
    if (failed_assertions > 0) {
        printf(COLOR_RED " NEUSPESNE: %d" COLOR_RESET "\n", failed_assertions);
        printf(COLOR_RED COLOR_BOLD "\n[ RESULT ] NIEKTORE TESTY ZLYHALI!" COLOR_RESET "\n\n");
        return 1;
    } else {
        printf(COLOR_GREEN " NEUSPESNE: 0\n");
        printf(COLOR_GREEN COLOR_BOLD "\n[ RESULT ] VSETKY TESTY PRESHLI UPESNE!" COLOR_RESET "\n\n");
        return 0;
    }
}