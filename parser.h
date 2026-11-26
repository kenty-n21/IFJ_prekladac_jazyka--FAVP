/*
 * Projekt: Implementacia prekladaca imperativneho jazyka IFJ26
 * Subor: parser.h
 * Autor: Paulina Kostalova (xkostap00)
 */
#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include "symtable.h"
#include "expressions.h"

// Typy prikazov v abstraktnom syntaktickom strome (AST)
typedef enum{
    AST_VARDEF,
    AST_ASSIGN,
    AST_EXPR,
    AST_IF,
    AST_WHILE,
    AST_RETURN
}AstStmtType;


// Reprezentacia volania funkcie v AST
typedef struct
{
    char *name;
    ExprNode **args;
    int arg_count;
    Datatype ret_type;
}AstCall;


// Uzol prikazu v AST (zretazeny zoznam prikazov v bloku)
typedef struct AstStmt
{
    AstStmtType kind;           // Typ prikazu (deklaracia, priradenie, if, while, return, vyraz)
    char *name;                 // Meno cielovej premennej (pre AST_VARDEF a AST_ASSIGN)
    int unique_id;              // Unikatny identifikator premennej (pre rozlisenie pri shadowingu)
    bool is_mutable;            // true pre 'var', false pre 'val'
    Datatype datatype;          // Vysledny datovy typ prikazu alebo premennej
    ExprNode *expr;             // Strom vyrazu
    AstCall *call;              // Volanie funkcie 
    struct AstStmt *then_block; // Telo vetvy then (pri AST_IF) alebo telo cyklu (AST_WHILE)
    struct AstStmt *else_block; // Telo vetvy else (pri AST_IF)
    struct AstStmt *next;       // Nasledujuci prikaz v bloku
}AstStmt;

// Uzol definicie funkcie v AST
typedef struct AstFunc
{
    char *name;                 // Meno funkcie
    Datatype return_type;       // Navratovy typ funkcie (explicitny alebo odvodeny)
    bool is_main;               // true ak ma funkcia anotaciu @main
    int param_count;            // Pocet parametrov funkcie
    char **param_names;         // Pole mien parametrov
    Datatype *param_types;      // Pole datovych typov parametrov
    int *param_ids;             // Pole unikatnych ID parametrov
    AstStmt *body;              // Zoznam prikazov tela funkcie
    struct AstFunc *next;       // Nasledujuca funkcia v programe

}AstFunc;


// Korenovy uzol AST celeho programu (obsahuje zoznam funkcii)
typedef struct 
{
    AstFunc *first_func;
    AstFunc *last_func;
}AstProgram;





// Hlavny vstupny bod syntaktickej a semantickej analyzy programu
int parser_program(void);

// Vrati vytvoreny AST strom programu pre generator kodu
AstProgram *parser_get_program(void);

// Uvolni pamat alokovanu pre AST strom, tabulku symbolov a pomocne struktury
void parser_cleanup(void);

#endif