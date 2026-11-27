/*
 * Projekt: Implementacia prekladaca imperativneho jazyka IFJ26
 * Subor: expressions.h
 * Autor: Veronika Kentosova (xkentov00)
 */
#ifndef EXPRESSIONS_H
#define EXPRESSIONS_H
#include "stack.h"
#include "error.h"
#include "scanner.h"   // tokeny
#include "symtable.h"  // tabulka symbolov


// Hodnoty buniek precedencnej tabulky
typedef enum {
    PREC_SHIFT,   // <
    PREC_REDUCE,  // >
    PREC_EQUAL,   // =
    PREC_ERROR    // prazdna bunka - nepovolena kombinacia
} PrecAction;

/**
 * Abstraktný syntaktický strom - AST
 * vyuzitie pri vnutornej reprezentacii kodu
 */
typedef enum{
    NODE_LITERAL,       // literal
    NODE_VAR,           // identifikator premennej
    NODE_UNARY,         // unarny operator
    NODE_BINARY,        // binarny operator
    NODE_MEM,           // pre VALMEM rozsirenie (.length, .toInt, .floor, .toString)
    NODE_INTERP         // pre rozšírenie INTERPOLATED
}NodeType;

/**
 * ADT - uzol AST
 * vyuzity na vnutornu reprezentaciu
 * kazdy uzol nesie svoj typ uzla (definovane vyssie), datovy typ, 
 * symbol (pri operatoroch definuje konkretny typ alebo id)
 * hodnotu, ukazovatele na podstromy
 */
typedef struct ExprNode{
    NodeType nodetype;
    Datatype datatype;
    PrecSymbol symbol;
    char value[1024];
    
    struct ExprNode *left;
    struct ExprNode *right;
}ExprNode;


/**
 * spracuje vyraz od aktualneho tokenu
 * po navrate je aktualny token prvy, ktory uz k vyrazu nepatri
 * vrati 0 alebo chybovy kod
 */
int parse_expression(Token* current_token, ExprNode **root, Datatype *type, symtable *symtable, int scope_level);

//funkcia, ktora rekurzivne (post-order) uvolni strom
void expr_free(ExprNode *root);

#endif