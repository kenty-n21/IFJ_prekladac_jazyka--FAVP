/*
 * Projekt: Implementacia prekladaca imperativneho jazyka IFJ26
 * Subor: stack.c
 * Autor: Veronika Kentosova(xkentov00)
 */
#include <stdio.h>      
#include <stdbool.h>    
#include "stack.h"
#include "symtable.h"
#include "expressions.h"

//inicializacia zasobnika
void init_stack(TStack *s){
    s->top = NULL;
}

//alokuje miesto v pamati pre novy element v pripade uspesnej alokacie vloz prvok na vrch zasobnika
bool push_stack(TStack *s, TSData data){
    TSElem* NewElemPtr = (TSElem *)malloc(sizeof(struct TSElem));
    if (NewElemPtr == NULL)
        return false;
    NewElemPtr->data = data;
    NewElemPtr->next = s->top;
    s->top = NewElemPtr;
    return true;
}

//vlozi zarazku nad najvyssi terminal
bool insert_terminal_stop_stack(TStack *s){
    TSElem* NewElemPtr = (TSElem *)malloc(sizeof(struct TSElem));
    if (NewElemPtr == NULL)
        return false;
    NewElemPtr->data.symbol = SYM_STOP;
    NewElemPtr->data.type = TYPE_ANY;
    NewElemPtr->data.node = NULL;

    TSElem *PrevElem, *CurrElem;
    CurrElem = s->top;
    PrevElem = NULL;

    while(CurrElem != NULL && CurrElem->data.symbol == SYM_NONTERM){
        PrevElem = CurrElem;
        CurrElem = CurrElem->next;
    }

    if(PrevElem == NULL){
        s->top = NewElemPtr;
        NewElemPtr->next = CurrElem;
        return true;
    }

    NewElemPtr->next = CurrElem;
    PrevElem->next = NewElemPtr;

    return true;
}

//zrusi prvok na vrchu zasobnika
void pop_stack(TStack *s){
    TSElem *ElemPtr;
    if(s->top != NULL){
        ElemPtr = s->top;
        s->top = s->top->next;
        free(ElemPtr);
    }
}


//ziska najvyssie ulozeny terminal
TSElem* get_topterminal_stack(TStack *s){
    TSElem *CurrentElem;
    CurrentElem = s->top;
    while(CurrentElem != NULL && (CurrentElem->data.symbol == SYM_NONTERM || CurrentElem->data.symbol == SYM_STOP)){
        CurrentElem = CurrentElem->next;
    }
    return CurrentElem;
}

//vrati ukazovatel na vrchny prvok zasobnika
TSElem* top_stack(TStack *s){
    if(is_empty_stack(s) == true){
        fprintf(stderr,"Zásobník je prázdny ! \n");
        return NULL;
    }
    return (s->top);
}

//zisti ci je zasobnik prazdny
bool is_empty_stack(TStack *s){
    return (s->top == NULL);
}

//uvolni cely zasobnik
void free_stack(TStack *s){
    while(!is_empty_stack(s)){
        if(s->top->data.node != NULL){
            expr_free(s->top->data.node);
        }
        pop_stack(s);
    }
}