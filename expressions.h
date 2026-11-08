/**
 * Autor: Veronika Kentošová(xkentov00)
 * hlavičkový súbor pre precedenčnú analýzu
 */

#ifndef EXPRESSIONS_H
#define EXPRESSIONS_H
#include <stdbool.h>
#include "error.h"
#include "stack.h"
#include "scanner.h"   // tokeny
#include "symtable.h"  // tabuľka symbolov
// #include "parser.h" ale pozor na cyklickú závislosť


/* Hodnoty buniek tabuľky. */
typedef enum {
    PREC_SHIFT,   // <
    PREC_REDUCE,  // >
    PREC_EQUAL,   // =
    PREC_ERROR    // prazdna bunka
} PrecAction;

/* Uzol AST výrazu. */
typedef enum { NODE_LITERAL, NODE_VAR, NODE_UNARY, NODE_BINARY } NodeKind;


typedef struct ExprNode {
    NodeKind kind;
    DataType type;           /* výsledný typ po sémantickej kontrole */
    /* operátor: token alebo PrecSymbol, podľa toho, čo ti vyhovuje */
    PrecSymbol op;
    struct ExprNode *left;   /* pre unárny operátor len left */
    struct ExprNode *right;
    /* pre literál alebo premennú: hodnota resp. odkaz do tabuľky symbolov */
    // union { int i; double d; char *s; Symbol *sym; } value;
} ExprNode;

/* Výsledok analýzy výrazu. */
typedef struct {
    DataType type;
    ExprNode *root;
} ExprResult;


/*
 * Spracuje výraz od aktuálneho tokenu.
 * Po návrate je aktuálny token prvý, ktorý už k výrazu nepatrí.
 * Vracia 0 alebo chybový kód (1, 2, 3, 6 ...).
 */
int parse_expression(/* Parser *p, */ ExprResult *out);

void expr_free(ExprNode *root);

#endif