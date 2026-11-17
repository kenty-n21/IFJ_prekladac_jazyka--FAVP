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

//generuje aby sa v lokalnom ramci deklarovala premenna s nazvom name
void gen_var_declare(const char *name);

void gen_flush_defvars(void);

//generuje priradenie hodnoty premennej z vrcholu zasobnika
void gen_var_assign(const char *name);

void gen_if_start(int label_id);

void gen_if_else(int label_id);

void gen_if_end(int label_id);

void gen_while_start(int label_id, ExprNode *node);

void gen_while_end(int label_id);

//generuje kod z AST stromu
//zatial pouziva strukturu uzla AST ktoru vyuziva expressions.c/.h
void gen_expr(ExprNode *node);

//generuje kam ma skocit kod aby mohol vzkonat funkciu a do docasneho ramca vlozi argumenty
void gen_func_start(const char *name);

void gen_func_end(void);

void gen_call_start(void);

void gen_call_argument(int index);

void gen_call(const char *name);

//prejde cely AST a vygeneruje pomocou pomocnych funkcii kod
void generate_program(ExprNode *node);
#endif