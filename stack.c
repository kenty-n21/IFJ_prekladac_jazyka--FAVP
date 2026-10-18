/**
 * Autor: Veronika Kentošová(xkentov00)
 * implementácia zásobníku
 */


 /**
  * init_stack - hotova
  * !insert_terminal_stop_stack 
  * pop_stack - hotova
  * !get_topterminal_stack 
  * top_stack - hotova
  * is_empty_stack - hotova
  * !free_stack 
  */
#include <stdio.h>      // pre mozne chybove hlasnia
#include <stdlib.h>     //na alokaciu
#include <stdbool.h>    //True a False
#include "stack.h"

//inicializácia zásobníka
void init_stack(TStack *s){
    s->top = NULL;
}

//alokuj miesto v pamäti pre nový element, v prípade úspešnej alokácie vlož prvok na vrch zásobníka
bool push_stack(TStack *s, TSData data){
    TSElem* NewElemPtr = (TSElem *)malloc(sizeof(struct TSElem));
    if (NewElemPtr == NULL)
        return false;
    NewElemPtr->data.symbol = data.symbol;
    NewElemPtr->data.type = data.type;
    NewElemPtr->next = s->top;
    s->top = NewElemPtr;
    return true;
}

//vlož zarážku nad najvyšší terminál
int insert_terminal_stop_stack(TStack *s);

//zruší prvok na vrchu zásobníka
void pop_stack(TStack *s){
    TSElem *ElemPtr;
    if(s->top != NULL){
        ElemPtr = s->top;
        s->top = s->top->next;
        free(ElemPtr);
    }
}

//získaj najvyššie uložený terminál
TSElem* get_topterminal_stack(TStack *s);

//vráti ukazovateľ na vrchný prvok zásobníka
TSElem* top_stack(TStack *s){
    if(is_empty_stack(s) == true){
        fprintf(stderr,"Zásobník je prázdny ! \n");
        retunr NULL;
    }
    return (s->top);
}

//zistí či je zásobník prázdny
bool is_empty_stack(TStack *s){
    return (s->top == NULL);
}

//uvoľni celý zásobník
void free_stack(TStack *s);