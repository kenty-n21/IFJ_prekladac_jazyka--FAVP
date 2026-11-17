/**
 * Autor: Veronika Kentošová(xkentov00)
 * generátor kódu
 * 
 * Zvoleny pristup generovania kodu z AST, ktory sme vyuzili na vnutornu reprezantaciu naseho kodu
 */
#include <stdio.h>
#include "generator.h"

static int label_counter = 0;   //pocitadlo navesti/skokov

int gen_new_label(void){
    return label_counter++;
}

void gen_init(void){
    printf(".IFJcode26\n");
    printf("DEFVAR GF@$tmp1\n");
    printf("DEFVAR GF@$tmp2\n");
    printf("JUMP $$main\n");
}  