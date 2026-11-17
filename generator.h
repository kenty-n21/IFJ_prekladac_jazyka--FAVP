/**
 * Autor: Veronika Kentošová(xkentov00)
 * hlavičkový súbor ku generátoru kód
 */
#ifndef GENERATOR_H
#define GENERATOR_H
#include "expressions.h" //dolezite pre strukturu uzla teda AST
 
// funkcia vypise hlavicku .IFJcode26, docasne premenne a skok na main
void gen_init(void);   

//funckia vrati unikatne cislo pre navestia, zabezpecuje ze dve navestia nebudu mat rovnake meno
int gen_new_label(void);
#endif