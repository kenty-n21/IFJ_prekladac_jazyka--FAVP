#include "symtable.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// =============================================================================
// 1. TESTY PRE: init_hash_table
// =============================================================================

// Test 1.1: Štandardná alokácia a kontrola počiatokého stavu
void test_init_hash_table_1_basic(void) {
    symtable *t = init_hash_table();
    assert(t != NULL);
    assert(t->hash_table != NULL);
    assert(t->pocet_synbolov == 0);
    assert(t->velkost_tabulky > 0);
    delete_hash_tabulka(t);
    printf("[PASS] test_init_hash_table_1_basic\n");
}

// Test 1.2: Vytvorenie viacerých nezávislých tabuliek
void test_init_hash_table_2_multiple(void) {
    symtable *t1 = init_hash_table();
    symtable *t2 = init_hash_table();
    assert(t1 != NULL && t2 != NULL);
    assert(t1 != t2);
    assert(t1->hash_table != t2->hash_table);
    delete_hash_tabulka(t1);
    delete_hash_tabulka(t2);
    printf("[PASS] test_init_hash_table_2_multiple\n");
}

// Test 1.3: Hraničný stav - okamžité zrušenie bez vloženia prvkov
void test_init_hash_table_3_immediate_cleanup(void) {
    symtable *t = init_hash_table();
    assert(t != NULL);
    delete_hash_tabulka(t);
    printf("[PASS] test_init_hash_table_3_immediate_cleanup\n");
}

// =============================================================================
// 2. TESTY PRE: insert_var
// =============================================================================

// Test 2.1: Vloženie jednej premennej a kontrola atribútov
void test_insert_var_1_basic(void) {
    symtable *t = init_hash_table();
    bool res = insert_var(t, "x", type_int, 0);
    assert(res == true);

    Symboldata *s = lookup_var(0, "x", t);
    assert(s != NULL);
    assert(strcmp(s->meno, "0$x") == 0);
    assert(s->typ == type_int);
    assert(s->scope_level == 0);
    assert(s->is_function == false);

    delete_hash_tabulka(t);
    printf("[PASS] test_insert_var_1_basic\n");
}

// Test 2.2: Shadowing - rovnaké meno v rôznych úrovniach zánorenia (scope)
void test_insert_var_2_shadowing_scopes(void) {
    symtable *t = init_hash_table();
    assert(insert_var(t, "var", type_int, 0) == true);
    assert(insert_var(t, "var", type_string, 1) == true);
    assert(insert_var(t, "var", type_double, 2) == true);

    Symboldata *s2 = lookup_var(2, "var", t);
    assert(s2 != NULL && s2->typ == type_double && s2->scope_level == 2);

    Symboldata *s1 = lookup_var(1, "var", t);
    assert(s1 != NULL && s1->typ == type_string && s1->scope_level == 1);

    Symboldata *s0 = lookup_var(0, "var", t);
    assert(s0 != NULL && s0->typ == type_int && s0->scope_level == 0);

    delete_hash_tabulka(t);
    printf("[PASS] test_insert_var_2_shadowing_scopes\n");
}

// Test 2.3: Hraničný stav - duplicitné vloženie premennej v rovnakom scope
void test_insert_var_3_duplicate_in_same_scope(void) {
    symtable *t = init_hash_table();
    assert(insert_var(t, "a", type_bool, 0) == true);
    // Vloženie druhej premennej s rovnakým menom v rovnakom scope nesmie zničiť pôvodný uzol
    bool res_dup = insert_var(t, "a", type_double, 0);
    assert(res_dup == false || lookup_var(0, "a", t) != NULL);

    delete_hash_tabulka(t);
    printf("[PASS] test_insert_var_3_duplicate_in_same_scope\n");
}

// =============================================================================
// 3. TESTY PRE: insert_func
// =============================================================================

// Test 3.1: Vloženie funkcie s dynamicky alokovaným poľom parametrov
void test_insert_func_1_with_params(void) {
    symtable *t = init_hash_table();
    Datatype *params = malloc(2 * sizeof(Datatype));
    params[0] = type_int;
    params[1] = type_string;

    bool res = insert_func(t, "foo", type_double, 2, params);
    assert(res == true);

    Symboldata *s = lookup_var(0, "foo", t);
    assert(s != NULL);
    assert(s->is_function == true);
    assert(s->param_count == 2);
    assert(s->typ == type_double);
    assert(s->param_type != NULL);
    assert(s->param_type[0] == type_int);
    assert(s->param_type[1] == type_string);

    delete_hash_tabulka(t);
    printf("[PASS] test_insert_func_1_with_params\n");
}

// Test 3.2: Hraničný stav - vloženie funkcie bez parametrov (param_count = 0, NULL)
void test_insert_func_2_zero_params(void) {
    symtable *t = init_hash_table();
    bool res = insert_func(t, "bar", type_unit, 0, NULL);
    assert(res == true);

    Symboldata *s = lookup_var(0, "bar", t);
    assert(s != NULL);
    assert(s->is_function == true);
    assert(s->param_count == 0);

    delete_hash_tabulka(t);
    printf("[PASS] test_insert_func_2_zero_params\n");
}

// Test 3.3: Vloženie viacerých funkcií s odlišnými typmi a počtami parametrov
void test_insert_func_3_multiple_funcs(void) {
    symtable *t = init_hash_table();

    Datatype *p1 = malloc(sizeof(Datatype) * 1);
    p1[0] = type_int;

    Datatype *p2 = malloc(sizeof(Datatype) * 2);
    p2[0] = type_bool;
    p2[1] = type_double;

    assert(insert_func(t, "func1", type_int, 1, p1) == true);
    assert(insert_func(t, "func2", type_string, 2, p2) == true);

    delete_hash_tabulka(t); // Úspešne uvoľní p1 aj p2 z haldy
    printf("[PASS] test_insert_func_3_multiple_funcs\n");
}

// =============================================================================
// 4. TESTY PRE: insert_vstavane_funkcie
// =============================================================================

// Test 4.1: Základné vloženie vstavaných funkcií do prázdnej tabuľky
void test_insert_vstavane_funkcie_1_basic(void) {
    symtable *t = init_hash_table();
    insert_vstavane_funkcie(t);
    assert(t->pocet_synbolov > 0);
    delete_hash_tabulka(t);
    printf("[PASS] test_insert_vstavane_funkcie_1_basic\n");
}

// Test 4.2: Overenie integrity vstavaných funkcií v hash tabuľke
void test_insert_vstavane_funkcie_2_verify_signatures(void) {
    symtable *t = init_hash_table();
    insert_vstavane_funkcie(t);

    for (int i = 0; i < t->velkost_tabulky; i++) {
        if (t->hash_table[i] != NULL && t->hash_table[i] != delete_item) {
            assert(t->hash_table[i]->meno != NULL);
            assert(t->hash_table[i]->is_function == true);
        }
    }

    delete_hash_tabulka(t);
    printf("[PASS] test_insert_vstavane_funkcie_2_verify_signatures\n");
}

// Test 4.3: Hraničný stav - vloženie vstavaných funkcií do tabuľky s premennými
void test_insert_vstavane_funkcie_3_preexisting_table(void) {
    symtable *t = init_hash_table();
    insert_var(t, "user_var", type_int, 0);

    insert_vstavane_funkcie(t);

    Symboldata *s = lookup_var(0, "user_var", t);
    assert(s != NULL && strcmp(s->meno, "0$user_var") == 0);

    delete_hash_tabulka(t);
    printf("[PASS] test_insert_vstavane_funkcie_3_preexisting_table\n");
}

// =============================================================================
// 5. TESTY PRE: lookup_var
// =============================================================================

// Test 5.1: Vyhľadanie existujúcej premennej
void test_lookup_var_1_exist(void) {
    symtable *t = init_hash_table();
    insert_var(t, "target", type_double, 0);

    Symboldata *res = lookup_var(0, "target", t);
    assert(res != NULL);
    assert(strcmp(res->meno, "0$target") == 0);

    delete_hash_tabulka(t);
    printf("[PASS] test_lookup_var_1_exist\n");
}

// Test 5.2: Hierarchické vyhľadávanie premennej z vyššieho scope
void test_lookup_var_2_hierarchical_scope(void) {
    symtable *t = init_hash_table();
    insert_var(t, "global_var", type_int, 0);
    insert_var(t, "local_var", type_string, 2);

    // Hľadanie z vnútornejšieho scope (2) nájde aj globálnu premennú (0)
    Symboldata *g = lookup_var(2, "global_var", t);
    assert(g != NULL);
    assert(strcmp(g->meno, "0$global_var") == 0);

    Symboldata *l = lookup_var(2, "local_var", t);
    assert(l != NULL);

    delete_hash_tabulka(t);
    printf("[PASS] test_lookup_var_2_hierarchical_scope\n");
}

// Test 5.3: Hraničný stav - vyhľadanie neexistujúcej premennej alebo nesprávneho scope
void test_lookup_var_3_non_existent(void) {
    symtable *t = init_hash_table();
    insert_var(t, "a", type_int, 0);

    // Neexistujúce meno
    assert(lookup_var(0, "non_existent", t) == NULL);

    // Premenná existuje v scope 3, ale hľadáme iba do scope 1
    insert_var(t, "deep_var", type_int, 3);
    assert(lookup_var(1, "deep_var", t) == NULL);

    delete_hash_tabulka(t);
    printf("[PASS] test_lookup_var_3_non_existent\n");
}

// =============================================================================
// 6. TESTY PRE: exit_scope
// =============================================================================

// Test 6.1: Odstránenie premenných len zo zadaného scope
void test_exit_scope_1_basic(void) {
    symtable *t = init_hash_table();
    insert_var(t, "v0", type_int, 0);
    insert_var(t, "v1", type_string, 1);
    insert_var(t, "v2", type_double, 2);

    exit_scope(2, t);

    assert(lookup_var(2, "v2", t) == NULL);
    assert(lookup_var(1, "v1", t) != NULL);
    assert(lookup_var(0, "v0", t) != NULL);

    delete_hash_tabulka(t);
    printf("[PASS] test_exit_scope_1_basic\n");
}

// Test 6.2: Hraničný stav - exit_scope pre prázdny/neexistujúci scope
void test_exit_scope_2_empty_scope(void) {
    symtable *t = init_hash_table();
    insert_var(t, "v0", type_int, 0);

    exit_scope(99, t); // Žiadne premenné v scope 99
    assert(lookup_var(0, "v0", t) != NULL);

    delete_hash_tabulka(t);
    printf("[PASS] test_exit_scope_2_empty_scope\n");
}

// Test 6.3: Hromadné uvoľnenie viacerých položiek z jedného scope
void test_exit_scope_3_deep_clean(void) {
    symtable *t = init_hash_table();
    insert_var(t, "a", type_int, 1);
    insert_var(t, "b", type_double, 1);
    insert_var(t, "c", type_string, 1);

    exit_scope(1, t);

    assert(lookup_var(1, "a", t) == NULL);
    assert(lookup_var(1, "b", t) == NULL);
    assert(lookup_var(1, "c", t) == NULL);

    delete_hash_tabulka(t);
    printf("[PASS] test_exit_scope_3_deep_clean\n");
}

// =============================================================================
// 7. TESTY PRE: hash_table_delete
// =============================================================================

// Test 7.1: Odstránenie jednej konkrétnej položky podľa kľúča
void test_hash_table_delete_1_single(void) {
    symtable *t = init_hash_table();
    insert_var(t, "item", type_int, 0); // Vloží sa pod kľúčom "0$item"

    // 1. Pri mazaní z low-level hash tabuľky musíte použiť skutočný kľúč "0$item"
    Symboldata *del = hash_table_delete("0$item", t);
    if (del != NULL && del != delete_item) {
        if (del->meno)
            free(del->meno);
        if (del->param_type)
            free(del->param_type);
        if (del->local_table)
            delete_hash_tabulka(del->local_table);
        free(del);
    }

    // 2. Pri overení cez lookup_var odovzdávajte čistý názov premennej "item"
    assert(lookup_var(0, "item", t) == NULL);

    delete_hash_tabulka(t);
    printf("[PASS] test_hash_table_delete_1_single\n");
}

// Test 7.2: Hraničný stav - pokus o zmazanie kľúča, ktorý v tabuľke nie je
void test_hash_table_delete_2_non_existent(void) {
    symtable *t = init_hash_table();
    insert_var(t, "x", type_int, 0);

    Symboldata *del = hash_table_delete("ghost", t);
    assert(del == NULL || del == delete_item);

    delete_hash_tabulka(t);
    printf("[PASS] test_hash_table_delete_2_non_existent\n");
}

// Test 7.3: Opätovné vloženie prvku po zmazaní (test tombstone / kolízií)
void test_hash_table_delete_3_reinsert_after_delete(void) {
    symtable *t = init_hash_table();
    insert_var(t, "key", type_int, 0); // Vloží sa pod kľúčom "0$key"

    // 1. Zmažeme položku pomocou celého kľúča "0$key"
    Symboldata *del = hash_table_delete("0$key", t);
    if (del != NULL && del != delete_item) {
        if (del->meno)
            free(del->meno);
        if (del->param_type)
            free(del->param_type);
        if (del->local_table)
            delete_hash_tabulka(del->local_table);
        free(del);
    }

    // 2. Opätovné vloženie premennej s rovnakým menom v rovnakej úrovni scope teraz prejde
    assert(insert_var(t, "key", type_string, 0) == true);

    Symboldata *s = lookup_var(0, "key", t);
    assert(s != NULL && s->typ == type_string);

    delete_hash_tabulka(t);
    printf("[PASS] test_hash_table_delete_3_reinsert_after_delete\n");
}

// =============================================================================
// 8. TESTY PRE: delete_hash_tabulka
// =============================================================================

// Test 8.1: Uvoľnenie úplne prázdnej tabuľky
void test_delete_hash_tabulka_1_empty(void) {
    symtable *t = init_hash_table();
    delete_hash_tabulka(t);
    printf("[PASS] test_delete_hash_tabulka_1_empty\n");
}

// Test 8.2: Rekurzívne uvoľnenie komplexnej štruktúry (premenné + funkcie + local_table)
void test_delete_hash_tabulka_2_complex_data(void) {
    symtable *t = init_hash_table();

    insert_var(t, "g_var", type_int, 0);

    Datatype *p = malloc(sizeof(Datatype));
    p[0] = type_int;
    insert_func(t, "fn", type_unit, 1, p);

    Symboldata *fn_node = lookup_var(0, "fn", t);
    if (fn_node != NULL) {
        fn_node->local_table = init_hash_table();
        insert_var(fn_node->local_table, "local_param", type_int, 1);
    }

    delete_hash_tabulka(t);
    printf("[PASS] test_delete_hash_tabulka_2_complex_data\n");
}

// Test 8.3: Uvoľnenie tabuľky naplnenej vstavanými funkciami
void test_delete_hash_tabulka_3_builtins(void) {
    symtable *t = init_hash_table();
    insert_vstavane_funkcie(t);
    delete_hash_tabulka(t);
    printf("[PASS] test_delete_hash_tabulka_3_builtins\n");
}

// =============================================================================
// MAIN SPÚŠŤAČ TESTOV
// =============================================================================

int main(void) {
    printf("=== RUNNING SYMTABLE UNIT TESTS ===\n\n");

    // 1. init_hash_table
    test_init_hash_table_1_basic();
    test_init_hash_table_2_multiple();
    test_init_hash_table_3_immediate_cleanup();

    // 2. insert_var
    test_insert_var_1_basic();
    test_insert_var_2_shadowing_scopes();
    test_insert_var_3_duplicate_in_same_scope();

    // 3. insert_func
    test_insert_func_1_with_params();
    test_insert_func_2_zero_params();
    test_insert_func_3_multiple_funcs();

    // 4. insert_vstavane_funkcie
    test_insert_vstavane_funkcie_1_basic();
    test_insert_vstavane_funkcie_2_verify_signatures();
    test_insert_vstavane_funkcie_3_preexisting_table();

    // 5. lookup_var
    test_lookup_var_1_exist();
    test_lookup_var_2_hierarchical_scope();
    test_lookup_var_3_non_existent();

    // 6. exit_scope
    test_exit_scope_1_basic();
    test_exit_scope_2_empty_scope();
    test_exit_scope_3_deep_clean();

    // 7. hash_table_delete
    test_hash_table_delete_1_single();
    test_hash_table_delete_2_non_existent();
    test_hash_table_delete_3_reinsert_after_delete();

    // 8. delete_hash_tabulka
    test_delete_hash_tabulka_1_empty();
    test_delete_hash_tabulka_2_complex_data();
    test_delete_hash_tabulka_3_builtins();

    printf("\n=== ALL 24 TESTS PASSED SUCCESSFULLY! ===\n");
    return 0;
}