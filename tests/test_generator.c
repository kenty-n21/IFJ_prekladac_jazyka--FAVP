/**
 * Komplexný test generátora kódu pre IFJ26
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "generator.h"
#include "parser.h"
#include "expressions.h"

// Pomocná funkcia na vytvorenie uzla literálu
static ExprNode *make_lit(Datatype type, const char *val) {
    ExprNode *n = calloc(1, sizeof(ExprNode));
    n->nodetype = NODE_LITERAL;
    n->datatype = type;
    strncpy(n->value, val, sizeof(n->value) - 1);
    return n;
}

// Pomocná funkcia na vytvorenie premennej
static ExprNode *make_var(Datatype type, const char *name) {
    ExprNode *n = calloc(1, sizeof(ExprNode));
    n->nodetype = NODE_VAR;
    n->datatype = type;
    strncpy(n->value, name, sizeof(n->value) - 1);
    return n;
}

// Pomocná funkcia na vytvorenie binárneho operátora
static ExprNode *make_bin(PrecSymbol sym, ExprNode *l, ExprNode *r, Datatype res_type) {
    ExprNode *n = calloc(1, sizeof(ExprNode));
    n->nodetype = NODE_BINARY;
    n->symbol = sym;
    n->datatype = res_type;
    n->left = l;
    n->right = r;
    return n;
}

// Pomocná funkcia na vytvorenie unárneho mínusu
static ExprNode *make_un(ExprNode *child, Datatype type) {
    ExprNode *n = calloc(1, sizeof(ExprNode));
    n->nodetype = NODE_UNARY;
    n->symbol = SYM_MINUS;
    n->datatype = type;
    n->left = child;
    return n;
}

int main(void) {
    // -------------------------------------------------------------
    // 1. Zostavenie funkcie "vypocet(a: Int, b: Double): Double"
    // -------------------------------------------------------------
    AstFunc *func1 = calloc(1, sizeof(AstFunc));
    func1->name = "vypocet";
    func1->is_main = false;
    func1->param_count = 2;
    func1->param_names = malloc(sizeof(char*) * 2);
    func1->param_names[0] = "a";
    func1->param_names[1] = "b";

    // Telo funkcie vypocet:
    // return a + b (kde 'a' je Int a 'b' je Double -> implicitná konverzia!)
    AstStmt *s_ret = calloc(1, sizeof(AstStmt));
    s_ret->kind = AST_RETURN;
    s_ret->expr = make_bin(SYM_PLUS, make_var(TYPE_INT, "a"), make_var(TYPE_DOUBLE, "b"), TYPE_DOUBLE);
    func1->body = s_ret;

    // -------------------------------------------------------------
    // 2. Zostavenie hlavného programu @main
    // -------------------------------------------------------------
    AstFunc *f_main = calloc(1, sizeof(AstFunc));
    f_main->name = "main";
    f_main->is_main = true;

    // Príkaz 1: var text = "Testujeme specialne znaky: medzera, # mriezka a \ lomitko"
    AstStmt *s1 = calloc(1, sizeof(AstStmt));
    s1->kind = AST_VARDEF;
    s1->name = "text";
    s1->expr = make_lit(TYPE_STRING, "Ahoj # svet \\ \n");

    // Príkaz 2: var spojenie = text + " prirazka"
    AstStmt *s2 = calloc(1, sizeof(AstStmt));
    s2->kind = AST_VARDEF;
    s2->name = "spojenie";
    s2->expr = make_bin(SYM_PLUS, make_var(TYPE_STRING, "text"), make_lit(TYPE_STRING, " pridavok"), TYPE_STRING);
    s1->next = s2;

    // Príkaz 3: var vysledok = vypocet(10, 20.5)  <-- volanie funkcie!
    AstStmt *s3 = calloc(1, sizeof(AstStmt));
    s3->kind = AST_VARDEF;
    s3->name = "vysledok";
    s3->call = calloc(1, sizeof(AstCall));
    s3->call->name = "vypocet";
    s3->call->arg_count = 2;
    s3->call->args = malloc(sizeof(ExprNode*) * 2);
    s3->call->args[0] = make_lit(TYPE_INT, "10");
    s3->call->args[1] = make_lit(TYPE_DOUBLE, "20.5");
    s2->next = s3;

    // Príkaz 4: var i = -5  <-- unárne mínus!
    AstStmt *s4 = calloc(1, sizeof(AstStmt));
    s4->kind = AST_VARDEF;
    s4->name = "i";
    s4->expr = make_un(make_lit(TYPE_INT, "5"), TYPE_INT);
    s3->next = s4;

    // Príkaz 5: while (i <= 0)
    AstStmt *s_while = calloc(1, sizeof(AstStmt));
    s_while->kind = AST_WHILE;
    s_while->expr = make_bin(SYM_LTE, make_var(TYPE_INT, "i"), make_lit(TYPE_INT, "0"), TYPE_BOOL);
    s4->next = s_while;

    // Telo cyklu WHILE:
    // a) vnorené var vnútri cyklu (test proti chybe 52!): var vnutorna = 100
    AstStmt *w_body1 = calloc(1, sizeof(AstStmt));
    w_body1->kind = AST_VARDEF;
    w_body1->name = "vnutorna";
    w_body1->expr = make_lit(TYPE_INT, "100");
    s_while->then_block = w_body1;

    // b) if (i != 0)
    AstStmt *w_if = calloc(1, sizeof(AstStmt));
    w_if->kind = AST_IF;
    w_if->expr = make_bin(SYM_NEQ, make_var(TYPE_INT, "i"), make_lit(TYPE_INT, "0"), TYPE_BOOL);
    w_body1->next = w_if;

    // then vetva if-u: var vetva_a = 1
    AstStmt *if_then = calloc(1, sizeof(AstStmt));
    if_then->kind = AST_VARDEF;
    if_then->name = "vetva_a";
    if_then->expr = make_lit(TYPE_INT, "1");
    w_if->then_block = if_then;

    // else vetva if-u: var vetva_b = 2
    AstStmt *if_else = calloc(1, sizeof(AstStmt));
    if_else->kind = AST_VARDEF;
    if_else->name = "vetva_b";
    if_else->expr = make_lit(TYPE_INT, "2");
    w_if->else_block = if_else;

    // c) posun počítadla: i = i + 1
    AstStmt *w_step = calloc(1, sizeof(AstStmt));
    w_step->kind = AST_ASSIGN;
    w_step->name = "i";
    w_step->expr = make_bin(SYM_PLUS, make_var(TYPE_INT, "i"), make_lit(TYPE_INT, "1"), TYPE_INT);
    w_if->next = w_step;

    f_main->body = s1;

    // -------------------------------------------------------------
    // 3. Spojenie celého programu
    // -------------------------------------------------------------
    AstProgram *program = calloc(1, sizeof(AstProgram));
    program->first_func = func1;
    func1->next = f_main;

    // -------------------------------------------------------------
    // 4. ODPÁLENIE GENERÁTORA!
    // -------------------------------------------------------------
    generate_program(program);

    return 0;
}