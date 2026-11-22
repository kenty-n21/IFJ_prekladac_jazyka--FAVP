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

typedef enum{
    AST_VARDEF,
    AST_ASSIGN,
    AST_EXPR,
    AST_IF,
    AST_WHILE,
    AST_RETURN
}AstStmtType;


typedef struct
{
    char *name;
    ExprNode **args;
    int arg_count;
    Datatype ret_type;
}AstCall;


typedef struct AstStmt
{
    AstStmtType kind;
    char *name;
    int unique_id;
    bool is_mutable;
    Datatype datatype;
    ExprNode *expr;
    AstCall *call;
    struct AstStmt *then_block;
    struct AstStmt *else_block;
    struct AstStmt *next;
}AstStmt;

typedef struct AstFunc
{
    char *name;
    Datatype return_type;
    bool is_main;
    int param_count;
    char **param_names;
    Datatype *param_types;
    int *param_ids;
    AstStmt *body;
    struct AstFunc *next;

}AstFunc;


typedef struct 
{
    AstFunc *first_func;
    AstFunc *last_func;
}AstProgram;





int parser_program(void);
AstProgram *parser_get_program(void);

#endif