/**
 * Autor: Veronika Kentošová(xkentov00)
 * hlavičkový súbor pre precedenčnú analýzu
 */

#ifndef EXPRESSIONS_H
#define EXPRESSIONS_H
#include "stack.h"
#include "error.h"
#include "scanner.h"   // tokeny
#include "symtable.h"  // tabuľka symbolov
// #include "parser.h" ale pozor na cyklickú závislosť


// Hodnoty buniek tabuľky.
typedef enum {
    PREC_SHIFT,   // <
    PREC_REDUCE,  // >
    PREC_EQUAL,   // =
    PREC_ERROR    // prazdna bunka
} PrecAction;

//Abstraktný syntaktický strom - AST
typedef enum{
    NODE_LITERAL,       // literal
    NODE_VAR,           // identifikator premennej
    NODE_UNARY,         // unarny operator
    NODE_BINARY         // binarny operator
}NodeType;

typedef struct ExprNode{
    NodeType nodetype;
    Datatype datatype;
    PrecSymbol symbol;
    char value[1024];
    
    struct ExprNode *left;
    struct ExprNode *right;
}ExprNode;

/*
 * Spracuje výraz od aktuálneho tokenu.
 * Po návrate je aktuálny token prvý, ktorý už k výrazu nepatrí.
 * Vracia 0 alebo chybový kód (1, 2, 3, 6 ...).
 */
int parse_expression(Token* current_token, ExprNode **root, Datatype *type, symtable *symtable, int scope_level);

void expr_free(ExprNode *root);

#endif