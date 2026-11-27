/*
 * Projekt: Implementacia prekladaca imperativneho jazyka IFJ26
 * Subor: stack.h
 * Autor: Veronika Kentosova(xkentov00)
 */
#ifndef STACK_H
#define STACK_H
#include <stdbool.h>
#include "symtable.h"

//Prompt : "Ake typy dat je vhodne definovat pre precedencnu analyzu"

//symboly, s ktorymi pracuje precedencna tabulka
typedef enum{
    SYM_PLUS,       // +
    SYM_UNARY_MIN,  // -    (unarne minus ako znamienko nie operacia)
    SYM_MINUS,      // -
    SYM_MUL,        // *
    SYM_DIV,        // /
    SYM_LT,         // <
    SYM_GT,         // >
    SYM_LTE,        // <=
    SYM_GTE,        // >=
    SYM_EQ,         // ==
    SYM_NEQ,        // !=
    SYM_LPAR,       // (
    SYM_RPAR,       // )
    SYM_ID,         // id
    SYM_DOLLAR,     // $
    SYM_NONTERM,    // zredukovany vyraz
    SYM_STOP        // zarazka na redukciu
}PrecSymbol;

//struktura pre data ulozena v jednom prvku
typedef struct TSData{
    Datatype type;
    PrecSymbol symbol;
    struct ExprNode *node;          //podstrom pre dany symbol
}TSData;

//struktura prvku v zasobniku
typedef struct TSElem{
    TSData data;
    struct TSElem *next;
}TSElem;

//struktura zasobniku
typedef struct tstack{
    TSElem *top;
}TStack;



//inicializacia zasobnika
void init_stack(TStack *s);

//alokuje miesto v pamati pre novy element, v pripade uspesnej alokacie vloz prvok na vrch zasobnika
bool push_stack(TStack *s, TSData data);

//vlozi zarazku nad najvyssi terminal
bool insert_terminal_stop_stack(TStack *s);

//zrusi prvok na vrchu zasobnika
void pop_stack(TStack *s);

//ziska najvyssie ulozeny terminal
TSElem* get_topterminal_stack(TStack *s);

//vrati ukazatel na vrchny prvok zasobnika
TSElem* top_stack(TStack *s);

//zisti ci je zasobnik prazdny
bool is_empty_stack(TStack *s);

//uvolni cely zasobnik
void free_stack(TStack *s);


#endif
