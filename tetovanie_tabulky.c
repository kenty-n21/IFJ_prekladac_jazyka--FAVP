// ============================================================================
// TESTOVACÍ FRAMEWORK A POMOCNÉ FUNKCIE
// ============================================================================
#include "symtable.c"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tests_run = 0;
int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) static void test_##name(void)
#define RUN_TEST(name)                             \
    do {                                           \
        printf("Running %-35s ... ", #name);       \
        int prev_failed = tests_failed;            \
        test_##name();                             \
        tests_run++;                               \
        if (tests_failed == prev_failed) {         \
            tests_passed++;                        \
            printf("\033[0;32m[PASSED]\033[0m\n"); \
        } else {                                   \
            printf("\033[0;31m[FAILED]\033[0m\n"); \
        }                                          \
    } while (0)

#define ASSERT_TRUE(condition)                                                       \
    do {                                                                             \
        if (!(condition)) {                                                          \
            printf("\n  ASSERT_TRUE failed: (%s) at line %d", #condition, __LINE__); \
            tests_failed++;                                                          \
            return;                                                                  \
        }                                                                            \
    } while (0)

#define ASSERT_FALSE(condition)                                                       \
    do {                                                                              \
        if ((condition)) {                                                            \
            printf("\n  ASSERT_FALSE failed: (%s) at line %d", #condition, __LINE__); \
            tests_failed++;                                                           \
            return;                                                                   \
        }                                                                             \
    } while (0)

#define ASSERT_NOT_NULL(ptr)                                                               \
    do {                                                                                   \
        if ((ptr) == NULL) {                                                               \
            printf("\n  ASSERT_NOT_NULL failed: (%s == NULL) at line %d", #ptr, __LINE__); \
            tests_failed++;                                                                \
            return;                                                                        \
        }                                                                                  \
    } while (0)

#define ASSERT_NULL(ptr)                                                               \
    do {                                                                               \
        if ((ptr) != NULL) {                                                           \
            printf("\n  ASSERT_NULL failed: (%s != NULL) at line %d", #ptr, __LINE__); \
            tests_failed++;                                                            \
            return;                                                                    \
        }                                                                              \
    } while (0)

#define ASSERT_EQ_INT(expected, actual)                                                               \
    do {                                                                                              \
        int _exp = (expected);                                                                        \
        int _act = (actual);                                                                          \
        if (_exp != _act) {                                                                           \
            printf("\n  ASSERT_EQ_INT failed: expected %d, got %d at line %d", _exp, _act, __LINE__); \
            tests_failed++;                                                                           \
            return;                                                                                   \
        }                                                                                             \
    } while (0)

#define ASSERT_EQ_STR(expected, actual)                                                                                                       \
    do {                                                                                                                                      \
        const char *_exp = (expected);                                                                                                        \
        const char *_act = (actual);                                                                                                          \
        if (_exp == NULL || _act == NULL || strcmp(_exp, _act) != 0) {                                                                        \
            printf("\n  ASSERT_EQ_STR failed: expected \"%s\", got \"%s\" at line %d", _exp ? _exp : "NULL", _act ? _act : "NULL", __LINE__); \
            tests_failed++;                                                                                                                   \
            return;                                                                                                                           \
        }                                                                                                                                     \
    } while (0)

// Helper pre vytvorenie Symboldata štruktúry:
// Ak ide o premennú (is_fn == false) a meno nemá zatiaľ $, vytvorí sa kľúč "0$meno"
static Symboldata *create_dummy_symbol(const char *name, Datatype type, bool is_fn) {
    Symboldata *s = malloc(sizeof(Symboldata));
    if (!s)
        return NULL;
    if (is_fn || strchr(name, '$') != NULL) {
        s->meno = strdup(name);
    } else {
        s->meno = make_compose_key(0, (char *)name);
    }
    s->typ = type;
    s->is_function = is_fn;
    s->param_count = 0;
    s->param_type = NULL;
    s->local_table = NULL;
    s->define = true;
    return s;
}

static void free_dummy_symbol(Symboldata *s) {
    if (!s || s == delete_item)
        return;
    free(s->meno);
    if (s->local_table)
        delete_hash_tabulka(s->local_table);
    free(s);
}

// ============================================================================
// 1. TESTY PRE init_hash_table
// ============================================================================

TEST(init_hash_table_basic) {
    symtable *t = init_hash_table();
    ASSERT_NOT_NULL(t);
    ASSERT_EQ_INT(10, t->velkost_tabulky);
    ASSERT_EQ_INT(0, t->pocet_synbolov);
    ASSERT_NOT_NULL(t->hash_table);
    delete_hash_tabulka(t);
}

TEST(init_hash_table_slots_null) {
    symtable *t = init_hash_table();
    ASSERT_NOT_NULL(t);
    for (int i = 0; i < t->velkost_tabulky; i++) {
        ASSERT_NULL(t->hash_table[i]);
    }
    delete_hash_tabulka(t);
}

TEST(init_hash_table_multiple_instances) {
    symtable *t1 = init_hash_table();
    symtable *t2 = init_hash_table();
    ASSERT_NOT_NULL(t1);
    ASSERT_NOT_NULL(t2);
    ASSERT_TRUE(t1 != t2);
    ASSERT_TRUE(t1->hash_table != t2->hash_table);
    delete_hash_tabulka(t1);
    delete_hash_tabulka(t2);
}

// ============================================================================
// 2. TESTY PRE hash_funkcia
// ============================================================================

TEST(hash_funkcia_deterministic) {
    unsigned int h1 = hash_funkcia("0$variable_x");
    unsigned int h2 = hash_funkcia("0$variable_x");
    ASSERT_EQ_INT(h1, h2);
}

TEST(hash_funkcia_different_strings) {
    unsigned int h1 = hash_funkcia("0$foo");
    unsigned int h2 = hash_funkcia("0$bar");
    ASSERT_TRUE(h1 != h2);
}

TEST(hash_funkcia_edge_cases) {
    unsigned int h_empty = hash_funkcia("");
    ASSERT_EQ_INT(0, h_empty);

    unsigned int h_scope = hash_funkcia("0$my_var_name_with_long_length_12345");
    ASSERT_TRUE(h_scope > 0);
}

// ============================================================================
// 3. TESTY PRE make_compose_key
// ============================================================================

TEST(make_compose_key_basic) {
    char *key = make_compose_key(0, "counter");
    ASSERT_NOT_NULL(key);
    ASSERT_EQ_STR("0$counter", key);
    free(key);
}

TEST(make_compose_key_nested_scopes) {
    char *key = make_compose_key(105, "myVar");
    ASSERT_NOT_NULL(key);
    ASSERT_EQ_STR("105$myVar", key);
    free(key);
}

TEST(make_compose_key_special_names) {
    char *key = make_compose_key(3, "x");
    ASSERT_NOT_NULL(key);
    ASSERT_EQ_STR("3$x", key);
    free(key);
}

// ============================================================================
// 4. TESTY PRE hash_table_insert A resize_hash_table
// ============================================================================

TEST(hash_table_insert_basic) {
    symtable *t = init_hash_table();
    Symboldata *s = create_dummy_symbol("elem1", type_int, false); // vytvorí "0$elem1"

    ASSERT_TRUE(hash_table_insert(s, t));
    ASSERT_EQ_INT(1, t->pocet_synbolov);

    delete_hash_tabulka(t);
}

TEST(hash_table_insert_null_params) {
    symtable *t = init_hash_table();
    Symboldata *s = create_dummy_symbol("elem", type_int, false);

    ASSERT_FALSE(hash_table_insert(NULL, t));
    ASSERT_FALSE(hash_table_insert(s, NULL));
    ASSERT_FALSE(hash_table_insert(NULL, NULL));

    free_dummy_symbol(s);
    delete_hash_tabulka(t);
}

TEST(hash_table_insert_trigger_resize) {
    symtable *t = init_hash_table();
    ASSERT_EQ_INT(10, t->velkost_tabulky);

    char buf[32];
    for (int i = 0; i < 15; i++) {
        snprintf(buf, sizeof(buf), "var_%d", i);
        Symboldata *s = create_dummy_symbol(buf, type_int, false); // vytvorí "0$var_i"
        ASSERT_TRUE(hash_table_insert(s, t));
    }

    ASSERT_EQ_INT(15, t->pocet_synbolov);
    ASSERT_TRUE(t->velkost_tabulky > 10);

    for (int i = 0; i < 15; i++) {
        snprintf(buf, sizeof(buf), "0$var_%d", i);
        ASSERT_NOT_NULL(hash_table_lookup(buf, t));
    }

    delete_hash_tabulka(t);
}

// ============================================================================
// 5. TESTY PRE hash_table_lookup
// ============================================================================

TEST(hash_table_lookup_existing) {
    symtable *t = init_hash_table();
    Symboldata *s = create_dummy_symbol("target", type_float, false); // "0$target"
    hash_table_insert(s, t);

    Symboldata *found = hash_table_lookup("0$target", t);
    ASSERT_NOT_NULL(found);
    ASSERT_EQ_STR("0$target", found->meno);
    ASSERT_TRUE(found->typ == type_float);

    delete_hash_tabulka(t);
}

TEST(hash_table_lookup_non_existing) {
    symtable *t = init_hash_table();
    Symboldata *s = create_dummy_symbol("existing", type_int, false);
    hash_table_insert(s, t);

    ASSERT_NULL(hash_table_lookup("0$not_here", t));

    delete_hash_tabulka(t);
}

TEST(hash_table_lookup_null_and_deleted) {
    symtable *t = init_hash_table();
    ASSERT_NULL(hash_table_lookup(NULL, t));
    ASSERT_NULL(hash_table_lookup("0$key", NULL));

    Symboldata *s1 = create_dummy_symbol("key1", type_int, false); // "0$key1"
    Symboldata *s2 = create_dummy_symbol("key2", type_int, false); // "0$key2"
    hash_table_insert(s1, t);
    hash_table_insert(s2, t);

    hash_table_delete("0$key1", t);
    free_dummy_symbol(s1);

    Symboldata *found = hash_table_lookup("0$key2", t);
    ASSERT_NOT_NULL(found);
    ASSERT_EQ_STR("0$key2", found->meno);

    delete_hash_tabulka(t);
}

// ============================================================================
// 6. TESTY PRE insert_var
// ============================================================================

TEST(insert_var_basic) {
    symtable *t = init_hash_table();
    ASSERT_TRUE(insert_var(t, "x", type_int, 0));

    Symboldata *res = hash_table_lookup("0$x", t);
    ASSERT_NOT_NULL(res);
    ASSERT_FALSE(res->is_function);
    ASSERT_TRUE(res->typ == type_int);

    delete_hash_tabulka(t);
}

TEST(insert_var_duplicate_same_scope) {
    symtable *t = init_hash_table();
    ASSERT_TRUE(insert_var(t, "counter", type_int, 1));
    ASSERT_FALSE(insert_var(t, "counter", type_float, 1));
    ASSERT_EQ_INT(1, t->pocet_synbolov);

    delete_hash_tabulka(t);
}

TEST(insert_var_same_name_different_scopes) {
    symtable *t = init_hash_table();
    ASSERT_TRUE(insert_var(t, "val", type_int, 0));
    ASSERT_TRUE(insert_var(t, "val", type_string, 1));
    ASSERT_TRUE(insert_var(t, "val", type_bool, 2));
    ASSERT_EQ_INT(3, t->pocet_synbolov);

    delete_hash_tabulka(t);
}

// ============================================================================
// 7. TESTY PRE insert_func
// ============================================================================

TEST(insert_func_basic) {
    symtable *t = init_hash_table();
    ASSERT_TRUE(insert_func(t, "foo", type_void, 0, NULL));

    Symboldata *fn = hash_table_lookup("foo", t);
    ASSERT_NOT_NULL(fn);
    ASSERT_TRUE(fn->is_function);
    ASSERT_TRUE(fn->typ == type_void);
    ASSERT_EQ_INT(0, fn->param_count);
    ASSERT_NOT_NULL(fn->local_table);

    delete_hash_tabulka(t);
}

TEST(insert_func_with_params) {
    symtable *t = init_hash_table();
    Datatype params[] = {type_int, type_float};

    ASSERT_TRUE(insert_func(t, "calculate", type_float, 2, params));

    Symboldata *fn = hash_table_lookup("calculate", t);
    ASSERT_NOT_NULL(fn);
    ASSERT_EQ_INT(2, fn->param_count);
    ASSERT_TRUE(fn->param_type[0] == type_int);
    ASSERT_TRUE(fn->param_type[1] == type_float);

    delete_hash_tabulka(t);
}

TEST(insert_func_duplicate_and_long_name) {
    symtable *t = init_hash_table();

    char *long_fn_name = "very_long_function_name_that_exceeds_pointer_size";
    ASSERT_TRUE(insert_func(t, long_fn_name, type_int, 0, NULL));
    ASSERT_FALSE(insert_func(t, long_fn_name, type_int, 0, NULL));

    Symboldata *fn = hash_table_lookup(long_fn_name, t);
    ASSERT_NOT_NULL(fn);
    ASSERT_EQ_STR(long_fn_name, fn->meno);

    delete_hash_tabulka(t);
}

// ============================================================================
// 8. TESTY PRE lookup_var
// ============================================================================

TEST(lookup_var_current_scope) {
    symtable *t = init_hash_table();
    insert_var(t, "a", type_int, 2);

    Symboldata *var = lookup_var(2, "a", t);
    ASSERT_NOT_NULL(var);
    ASSERT_EQ_STR("2$a", var->meno);

    delete_hash_tabulka(t);
}

TEST(lookup_var_shadowing_fallback) {
    symtable *t = init_hash_table();
    insert_var(t, "global_var", type_int, 0);

    Symboldata *var = lookup_var(2, "global_var", t);
    ASSERT_NOT_NULL(var);
    ASSERT_EQ_STR("0$global_var", var->meno);

    insert_var(t, "global_var", type_float, 1);
    Symboldata *shadowed = lookup_var(2, "global_var", t);
    ASSERT_NOT_NULL(shadowed);
    ASSERT_EQ_STR("1$global_var", shadowed->meno);
    ASSERT_TRUE(shadowed->typ == type_float);

    delete_hash_tabulka(t);
}

TEST(lookup_var_not_found_and_null) {
    symtable *t = init_hash_table();
    insert_var(t, "x", type_int, 0);

    ASSERT_NULL(lookup_var(0, "non_existing", t));
    ASSERT_NULL(lookup_var(5, "non_existing", t));
    ASSERT_NULL(lookup_var(0, NULL, t));
    ASSERT_NULL(lookup_var(0, "x", NULL));

    delete_hash_tabulka(t);
}

// ============================================================================
// 9. TESTY PRE exit_scope
// ============================================================================

TEST(exit_scope_basic) {
    symtable *t = init_hash_table();
    insert_var(t, "var0", type_int, 0);
    insert_var(t, "var1", type_int, 1);
    insert_var(t, "var2", type_int, 1);

    ASSERT_EQ_INT(3, t->pocet_synbolov);

    exit_scope(1, t);

    ASSERT_EQ_INT(1, t->pocet_synbolov);
    ASSERT_NULL(lookup_var(1, "var1", t));
    ASSERT_NULL(lookup_var(1, "var2", t));
    ASSERT_NOT_NULL(lookup_var(0, "var0", t));

    delete_hash_tabulka(t);
}

TEST(exit_scope_preserve_functions) {
    symtable *t = init_hash_table();
    insert_func(t, "my_func", type_void, 0, NULL);
    insert_var(t, "temp_var", type_int, 0);

    exit_scope(0, t);

    ASSERT_NOT_NULL(hash_table_lookup("my_func", t));
    ASSERT_NULL(hash_table_lookup("0$temp_var", t));

    delete_hash_tabulka(t);
}

TEST(exit_scope_empty_or_invalid) {
    symtable *t = init_hash_table();
    insert_var(t, "x", type_int, 0);

    exit_scope(5, t);
    ASSERT_EQ_INT(1, t->pocet_synbolov);

    exit_scope(0, NULL);

    delete_hash_tabulka(t);
}

// ============================================================================
// 10. TESTY PRE hash_table_delete
// ============================================================================

TEST(hash_table_delete_basic) {
    symtable *t = init_hash_table();
    Symboldata *s = create_dummy_symbol("to_delete", type_int, false); // "0$to_delete"
    hash_table_insert(s, t);

    ASSERT_EQ_INT(1, t->pocet_synbolov);

    Symboldata *deleted = hash_table_delete("0$to_delete", t);
    ASSERT_NOT_NULL(deleted);
    ASSERT_TRUE(deleted == s);
    ASSERT_EQ_INT(0, t->pocet_synbolov);

    free_dummy_symbol(deleted);
    delete_hash_tabulka(t);
}

TEST(hash_table_delete_non_existent_and_null) {
    symtable *t = init_hash_table();
    Symboldata *s = create_dummy_symbol("item", type_int, false); // "0$item"
    hash_table_insert(s, t);

    ASSERT_NULL(hash_table_delete("0$ghost", t));
    ASSERT_NULL(hash_table_delete(NULL, t));
    ASSERT_NULL(hash_table_delete("0$item", NULL));
    ASSERT_EQ_INT(1, t->pocet_synbolov);

    delete_hash_tabulka(t);
}

TEST(hash_table_delete_linear_probing_chain) {
    symtable *t = init_hash_table();

    Symboldata *s1 = create_dummy_symbol("alpha", type_int, false); // "0$alpha"
    Symboldata *s2 = create_dummy_symbol("beta", type_int, false);  // "0$beta"
    Symboldata *s3 = create_dummy_symbol("gamma", type_int, false); // "0$gamma"

    hash_table_insert(s1, t);
    hash_table_insert(s2, t);
    hash_table_insert(s3, t);

    Symboldata *d = hash_table_delete("0$beta", t);
    free_dummy_symbol(d);

    ASSERT_NOT_NULL(hash_table_lookup("0$gamma", t));

    delete_hash_tabulka(t);
}

// ============================================================================
// 11. TESTY PRE delete_hash_tabulka
// ============================================================================

TEST(delete_hash_tabulka_empty_and_null) {
    delete_hash_tabulka(NULL);

    symtable *t = init_hash_table();
    delete_hash_tabulka(t);
}

TEST(delete_hash_tabulka_complex_nested) {
    symtable *t = init_hash_table();

    insert_var(t, "g1", type_int, 0);
    insert_var(t, "g2", type_float, 0);

    insert_func(t, "my_func", type_void, 0, NULL);
    Symboldata *fn = hash_table_lookup("my_func", t);
    ASSERT_NOT_NULL(fn);
    ASSERT_NOT_NULL(fn->local_table);

    insert_var(fn->local_table, "local_var", type_int, 0);

    delete_hash_tabulka(t);
}

TEST(delete_hash_tabulka_with_tombstones) {
    symtable *t = init_hash_table();

    insert_var(t, "a", type_int, 1);
    insert_var(t, "b", type_int, 1);
    insert_var(t, "c", type_int, 0);

    exit_scope(1, t);

    delete_hash_tabulka(t);
}

// ============================================================================
// MAIN RUNNER
// ============================================================================

int main(void) {
    printf("=====================================================\n");
    printf("     RUNNING SYMTABLE STRICT COMPREHENSIVE SUITE     \n");
    printf("=====================================================\n\n");

    RUN_TEST(init_hash_table_basic);
    RUN_TEST(init_hash_table_slots_null);
    RUN_TEST(init_hash_table_multiple_instances);

    RUN_TEST(hash_funkcia_deterministic);
    RUN_TEST(hash_funkcia_different_strings);
    RUN_TEST(hash_funkcia_edge_cases);

    RUN_TEST(make_compose_key_basic);
    RUN_TEST(make_compose_key_nested_scopes);
    RUN_TEST(make_compose_key_special_names);

    RUN_TEST(hash_table_insert_basic);
    RUN_TEST(hash_table_insert_null_params);
    RUN_TEST(hash_table_insert_trigger_resize);

    RUN_TEST(hash_table_lookup_existing);
    RUN_TEST(hash_table_lookup_non_existing);
    RUN_TEST(hash_table_lookup_null_and_deleted);

    RUN_TEST(insert_var_basic);
    RUN_TEST(insert_var_duplicate_same_scope);
    RUN_TEST(insert_var_same_name_different_scopes);

    RUN_TEST(insert_func_basic);
    RUN_TEST(insert_func_with_params);
    RUN_TEST(insert_func_duplicate_and_long_name);

    RUN_TEST(lookup_var_current_scope);
    RUN_TEST(lookup_var_shadowing_fallback);
    RUN_TEST(lookup_var_not_found_and_null);

    RUN_TEST(exit_scope_basic);
    RUN_TEST(exit_scope_preserve_functions);
    RUN_TEST(exit_scope_empty_or_invalid);

    RUN_TEST(hash_table_delete_basic);
    RUN_TEST(hash_table_delete_non_existent_and_null);
    RUN_TEST(hash_table_delete_linear_probing_chain);

    RUN_TEST(delete_hash_tabulka_empty_and_null);
    RUN_TEST(delete_hash_tabulka_complex_nested);
    RUN_TEST(delete_hash_tabulka_with_tombstones);

    printf("\n=====================================================\n");
    printf("TEST RESULTS: %d Passed, %d Failed (Total %d)\n", tests_passed, tests_failed, tests_run);
    printf("=====================================================\n");

    return tests_failed == 0 ? 0 : 1;
}