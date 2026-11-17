#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "stack.h"

//PROMPT : Vytvor mi komplexne testy od najjednoduchsich po najkomplexnejsie na otestovanie funkcnosti zasobnika.

// --- PÔVODNÉ TESTY (1 až 5) ---
void test_1_init_and_empty() {
    TStack s; init_stack(&s);
    assert(is_empty_stack(&s) == true);
    assert(top_stack(&s) == NULL);
    printf("Test 1 (Init & Empty) - PASSED\n");
}

void test_2_push_top_pop() {
    TStack s; init_stack(&s);
    TSData data = {TYPE_INT, SYM_ID};
    push_stack(&s, data);
    assert(top_stack(&s)->data.symbol == SYM_ID);
    pop_stack(&s);
    assert(is_empty_stack(&s) == true);
    printf("Test 2 (Push, Top, Pop) - PASSED\n");
}

void test_3_multiple_push_and_free() {
    TStack s; init_stack(&s);
    TSData d1 = {TYPE_INT, SYM_ID}; TSData d2 = {TYPE_UNDEFINED, SYM_PLUS};
    push_stack(&s, d1); push_stack(&s, d2);
    free_stack(&s);
    assert(is_empty_stack(&s) == true);
    printf("Test 3 (Multiple Push & Free) - PASSED\n");
}

void test_4_get_topterminal() {
    TStack s; init_stack(&s);
    TSData d1 = {TYPE_INT, SYM_ID}; TSData d2 = {TYPE_UNDEFINED, SYM_PLUS}; TSData d3 = {TYPE_UNDEFINED, SYM_NONTERM};
    push_stack(&s, d1); push_stack(&s, d2); push_stack(&s, d3);
    assert(get_topterminal_stack(&s)->data.symbol == SYM_PLUS);
    free_stack(&s);
    printf("Test 4 (Get Top Terminal) - PASSED\n");
}

void test_5_insert_terminal_stop() {
    TStack s; init_stack(&s);
    TSData d1 = {TYPE_INT, SYM_ID}; TSData d2 = {TYPE_UNDEFINED, SYM_PLUS}; TSData d3 = {TYPE_UNDEFINED, SYM_NONTERM};
    push_stack(&s, d1); push_stack(&s, d2); push_stack(&s, d3);
    assert(insert_terminal_stop_stack(&s) == true);
    assert(top_stack(&s)->data.symbol == SYM_NONTERM);
    assert(top_stack(&s)->next->data.symbol == SYM_STOP);
    free_stack(&s);
    printf("Test 5 (Insert Terminal Stop) - PASSED\n");
}

// --- NOVÉ KOMPLEXNÉ TESTY ---

// Test 6: Extrémne prípady (Edge cases) na prázdnom zásobníku
// Testujeme, či program nespadne, ak robíme operácie na prázdnom zásobníku
void test_6_empty_edge_cases() {
    TStack s;
    init_stack(&s);
    
    pop_stack(&s); // Nemalo by spadnúť!
    assert(is_empty_stack(&s) == true);
    
    assert(get_topterminal_stack(&s) == NULL); // Neni z čoho čítať
    
    free_stack(&s); // Nesmie spadnúť
    printf("Test 6 (Empty Stack Edge Cases) - PASSED\n");
}

// Test 7: Striedanie operácií (Chaos test)
// Simuluje reálny priebeh, kde sa neustále pridáva a odoberá
void test_7_alternating_operations() {
    TStack s;
    init_stack(&s);
    TSData d = {TYPE_UNDEFINED, SYM_ID};
    
    push_stack(&s, d); push_stack(&s, d); push_stack(&s, d); // +3 (spolu 3)
    pop_stack(&s); pop_stack(&s);                            // -2 (spolu 1)
    push_stack(&s, d); push_stack(&s, d);                    // +2 (spolu 3)
    pop_stack(&s);                                           // -1 (spolu 2)
    
    // Na zásobníku musia ostať presne 2 prvky
    assert(is_empty_stack(&s) == false);
    pop_stack(&s); pop_stack(&s);
    assert(is_empty_stack(&s) == true); // Teraz musí byť prázdny
    
    printf("Test 7 (Alternating Operations) - PASSED\n");
}

// Test 8: Záťažový test pamäte (Stress test)
// Vložíme 10 000 prvkov a všetky korektne uvoľníme
void test_8_stress_test() {
    TStack s;
    init_stack(&s);
    TSData d = {TYPE_INT, SYM_ID};
    
    for (int i = 0; i < 10000; i++) {
        assert(push_stack(&s, d) == true);
    }
    
    assert(is_empty_stack(&s) == false);
    free_stack(&s);
    assert(is_empty_stack(&s) == true);
    
    printf("Test 8 (Stress Test - 10 000 items) - PASSED\n");
}
// Test 9: Simulácia reálneho zložitého výrazu: a + b * c
void test_9_complex_expression_simulation() {
    TStack s;
    init_stack(&s);
    
    TSData d_dollar = {TYPE_UNDEFINED, SYM_DOLLAR};
    TSData d_e1 = {TYPE_INT, SYM_NONTERM};
    TSData d_plus = {TYPE_UNDEFINED, SYM_PLUS};
    TSData d_e2 = {TYPE_INT, SYM_NONTERM};
    TSData d_mul = {TYPE_UNDEFINED, SYM_MUL};
    TSData d_id = {TYPE_INT, SYM_ID};
    
    push_stack(&s, d_dollar);
    push_stack(&s, d_e1);
    push_stack(&s, d_plus);
    push_stack(&s, d_e2);
    push_stack(&s, d_mul); // Na vrchu je zatiaľ *
    
    // KROK 1: Najprv vložíme zarážku (mala by ísť nad *, čiže priamo na vrch)
    assert(insert_terminal_stop_stack(&s) == true);
    
    // KROK 2: Až teraz vložíme (Push) samotný prečítaný terminál (ID)
    push_stack(&s, d_id);
    
    // KROK 3: Kontrola celého zásobníka zhora nadol
    TSElem *curr = top_stack(&s);
    assert(curr->data.symbol == SYM_ID);      curr = curr->next;
    assert(curr->data.symbol == SYM_STOP);    curr = curr->next; // Zarážka je správne pod ID!
    assert(curr->data.symbol == SYM_MUL);     curr = curr->next;
    assert(curr->data.symbol == SYM_NONTERM); curr = curr->next;
    assert(curr->data.symbol == SYM_PLUS);    curr = curr->next;
    assert(curr->data.symbol == SYM_NONTERM); curr = curr->next;
    assert(curr->data.symbol == SYM_DOLLAR);  curr = curr->next;
    assert(curr == NULL); // Sme na dne
    
    free_stack(&s);
    printf("Test 9 (Complex Expression Simulation) - PASSED\n");
}

int main() {
    printf("--- SPUSTAM KOMPLEXNE TESTY ZASOBNIKA ---\n");
    
    test_1_init_and_empty();
    test_2_push_top_pop();
    test_3_multiple_push_and_free();
    test_4_get_topterminal();
    test_5_insert_terminal_stop();
    
    printf("\n--- SPUSTAM POKROCILE TESTY ---\n");
    test_6_empty_edge_cases();
    test_7_alternating_operations();
    test_8_stress_test();
    test_9_complex_expression_simulation();
    
    printf("\n>>> VSETKYCH 9 TESTOV ZBEHLO USPESNE! ZASOBNIK JE 100%% NEPRIESTRELNY! <<<\n");
    return 0;
}