/*
 * Autor: Veronika Kentošová(xkentov00)
 * hlavičkový súbor pre zásobník
 */

#ifndef STACK_H
#define STACK_H
#include <stdbool.h>

//Prompt : "Aké typy dát je vhodné definovať pre precedenčnú analýzu"

//Symboly, s ktorými pracuje precedenčná tabuľka
typedef enum{
    SYM_PLUS,
    SYM_MINUS,
    SYM_MUL,
    SYM_DIV,
    SYM_RPAR,
    SYM_LPAR,
    SYM_ID,
    SYM_REL_OP,
    SYM_STOP,
    SYM_DOLLAR,
    SYM_NONTERM
}PrecSymbol;

//dátové typy pre sémantickú kontrolu
typedef enum{
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_UNIT,
    TYPE_UNDEFINED
}Datatype;



typedef struct TSData{
    Datatype type;
    PrecSymbol symbol;
}TSData;

//štrúktúra prvku v zásobníku
typedef struct TSElem{
    TSData data;
    struct TSElem *next;
}TSElem;

//štruktúra zásobníku
typedef struct tstack{
    TSElem *top;
}TStack;



//inicializáacia zásobníka
void init_stack(TStack *s);

//alokuj miesto v pamäti pre nový element, v prípade úspešnej alokácie vlož prvok na vrch zásobníka
bool push_stack(TStack *s, TSData data);

//vlož zarážku nad najvyšší terminál
bool insert_terminal_stop_stack(TStack *s);

//zruší prvok na vrchu zásobníka
void pop_stack(TStack *s);

//získaj najvyššie uložený terminál
TSElem* get_topterminal_stack(TStack *s);

//vráti ukazovateľ na vrchný prvok zásobníka
TSElem* top_stack(TStack *s);

//zistí či je zásobník prázdny
bool is_empty_stack(TStack *s);

//uvoľni celý zásobník
void free_stack(TStack *s);


#endif
