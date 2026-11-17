/**
 * Autor: Veronika Kentošová(xkentov00)
 * generátor kódu
 *
 * Zvoleny pristup generovania kodu z AST, ktory sme vyuzili na vnutornu reprezantaciu naseho kodu
 */
#include "generator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int label_counter = 0; // pocitadlo navesti/skokov
static char defvar_buf[256][64];
static int defvar_count = 0;

static void gen_call_stmt(AstCall *call){
    if ( call == NULL)
        return ;

    gen_call_start();
    for (int i = 0; i < call->arg_count; i++){
        gen_expr(call->args[i]);
        gen_call_argument(i);
    }
    gen_call(call->name);
}

static void collect_defvars(AstStmt *s){
    for (; s != NULL; s = s->next){
        if(s->kind == AST_VARDEF){
            gen_var_declare(s->name);
        }
        if(s->then_block != NULL){
            collect_defvars(s->then_block);
        }
        if (s->else_block != NULL){
            collect_defvars(s->else_block);
        }
    }
}

int gen_new_label(void) {
    return label_counter++;
}

void gen_init(void) {
    printf(".IFJcode26\n");
    printf("DEFVAR GF@$tmp1\n");
    printf("DEFVAR GF@$tmp2\n");
    printf("JUMP $$main\n");
}

void gen_var_declare(const char *name) {
    // Neupisujeme hned! Len ulozime meno.
    strncpy(defvar_buf[defvar_count], name, 63);
    defvar_buf[defvar_count][63] = '\0';
    defvar_count++;
}

void gen_flush_defvars(void) {
    // Vypise vsetky nazbirane DEFVAR naraz a resetuje buffer
    for (int i = 0; i < defvar_count; i++) {
        printf("DEFVAR LF@%s\n", defvar_buf[i]);
    }
    defvar_count = 0;
}

void gen_var_assign(const char *name) {
    printf("POPS LF@%s\n", name);
}

void gen_if_start(int label_id) {
    printf("PUSHS bool@true\n");
    printf("JUMPIFNEQS $$else_%d\n", label_id);
}

void gen_if_else(int label_id) {
    printf("JUMP $$if_end_%d\n", label_id);
    printf("LABEL $$else_%d\n", label_id);
}

void gen_if_end(int label_id) {
    printf("LABEL $$if_end_%d\n", label_id);
}


//skontrolovat 
void gen_while_start(int label_id, ExprNode *node) {
    printf("LABEL $$while_%d\n", label_id);
    gen_expr(node);
    printf("PUSHS bool@true\n");
    printf("JUMPIFNEQS $$while_end_%d\n", label_id);
}

void gen_while_end(int label_id) {
    printf("JUMP $$while_%d\n", label_id);
    printf("LABEL $$while_end_%d\n", label_id);
}
void gen_expr(ExprNode *node) {
    if (node == NULL)
        return;

    switch (node->nodetype) {

    // ak je v uzli teda node konkretna hodnota
    case NODE_LITERAL:

        switch (node->datatype) {
        case TYPE_INT:
            printf("PUSHS int@%s\n", node->value);
            break;
        case TYPE_DOUBLE: {
            double val = strtod(node->value, NULL);
            printf("PUSHS float@%a\n", val);
            break;
        }
        case TYPE_STRING:
            printf("PUSHS string@");
            gen_print_escaped_string(node->value);
            printf("\n");
            break;
        case TYPE_BOOL:
            printf("PUSHS bool@%s\n", node->value);
            break;
        case TYPE_UNIT: // pre istotu
            printf("PUSHS nil@nil\n");
            break;
        default:
            break; // ak bude type_any a ine
        }
        break;
    // premenne
    // „Lokální rámec, značíme LF (Local Frame)... slouží pro ukládání lokálních proměnných funkcí a jejich parametrů.“
    case NODE_VAR:
        printf("PUSHS LF@%s\n", node->value);
        break;
    case NODE_UNARY:
        gen_expr(node->left);
        if (node->datatype == TYPE_INT) {
            printf("PUSHS int@-1\n");
        } else if (node->datatype == TYPE_DOUBLE) {
            // hodnota -0x1p+0 repreyentuje -1.0
            printf("PUSHS float@-0x1p+0\n");
        }
        printf("MULS\n");
        break;

    case NODE_BINARY:

        gen_expr(node->left);
        // konverzia int na float
        if (node->datatype == TYPE_DOUBLE && node->left->datatype == TYPE_INT) {
            printf("INT2FLOATS\n");
        }

        gen_expr(node->right);
        // konverzia int na float
        if (node->datatype == TYPE_DOUBLE && node->right->datatype == TYPE_INT) {
            printf("INT2FLOATS\n");
        }

        switch (node->symbol) {
        case SYM_PLUS:
            // riesenie konkatenacie retazcov
            if (node->datatype == TYPE_STRING) {
                printf("POPS GF@$tmp2\n");
                printf("POPS GF@$tmp1\n");
                printf("CONCAT GF@$tmp1 GF@$tmp1 GF@$tmp2\n");
                printf("PUSHS GF@$tmp1\n");
                break;
            }
            printf("ADDS\n");
            break;
        case SYM_MINUS:
            printf("SUBS\n");
            break;
        case SYM_MUL:
            printf("MULS\n");
            break;
        case SYM_DIV:
            if (node->datatype == TYPE_INT) {
                printf("IDIVS\n");
            } else {
                printf("DIVS\n");
            }
            break;

        case SYM_LT:
            printf("LTS\n");
            break;
        case SYM_GT:
            printf("GTS\n");
            break;
        case SYM_LTE:
            printf("GTS\n");
            printf("NOTS\n");
            break;
        case SYM_GTE:
            printf("LTS\n");
            printf("NOTS\n");
            break;
        case SYM_EQ:
            printf("EQS\n");
            break;
        case SYM_NEQ:
            printf("EQS\n");
            printf("NOTS\n");
            break;

        default:
            break;
        }
        break;
    default:
        break;
    }
}

void gen_func_start(const char *name){
    printf("LABEL $%s\n", name);
    printf("PUSHFRAME\n");
}

void gen_func_end(void){
    printf("POPFRAME\n");
    printf("RETURN\n");
}

void gen_func_param(const char *name, int index){
    printf("DEFVAR LF@%s\n",name);
    printf("MOVE LF@%s LF@%%%d\n", name, index);
}

void gen_call_start(void){
    printf("CREATEFRAME\n");
}

void gen_call_argument(int index){
    printf("DEFVAR TF@%%%d\n", index);
    printf("POPS TF@%%%d\n", index);
}

void gen_call(const char *name){
    printf("CALL $%s\n", name);
}

void gen_print_escaped_string(const char *str){
    if(str == NULL)
        return;
    
    for (int i = 0; str[i] != '\0'; i++){   //prechadza retazec do konca
        unsigned char c = (unsigned char)str[i];
        if( c <= 32 || c == 35 || c == 92) { //overuje ci ide o neviditelne znaky, medzeru, #, lomitko
            printf("\\%03d",c);             //vypise cislo znaku v desiatkovej sustave a so spravnym formatovanim
        }else{
            putchar(c);
        }
    }
}

void gen_main_start(void){
    printf("LABEL $$main\n");
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");
}

void gen_main_end(void){
    printf("POPFRAME\n");
}


static void gen_statement(AstStmt *s){
    if(s == NULL){
        return;
    }

    switch (s->kind)
    {
    case AST_VARDEF:
    case AST_ASSIGN:
        if(s->expr != NULL){
            gen_expr(s->expr);
        }else if (s->call != NULL){
             gen_call_stmt(s->call);
        }
        gen_var_assign(s->name);
        break;

    case AST_IF:{
        int lbl = gen_new_label();
        gen_expr(s->expr);             // vyhodnotí podmienku
        gen_if_start(lbl);             // ak false, skočí na else
        gen_statements(s->then_block); // telo then
        gen_if_else(lbl);              // preskočí else a označí začiatok else
        if (s->else_block != NULL) {
            gen_statements(s->else_block); // telo else (ak existuje)
        }
        gen_if_end(lbl);               // koniec celého if
        break;
    }

    case AST_WHILE:{
        int lbl = gen_new_label();
        gen_while_start(lbl, s->expr); // začiatok cyklu + test podmienky
        gen_statements(s->then_block); // telo cyklu
        gen_while_end(lbl);            // skok späť na začiatok
        break;
    }
    case AST_RETURN:
        if (s->expr != NULL) {
            gen_expr(s->expr);
        } else if (s->call != NULL) {
            gen_call_stmt(s->call);
        }
        gen_func_end(); // vypíše: POPFRAME a RETURN
        break;

    case AST_EXPR:
        if (s->call != NULL) {
            gen_call_stmt(s->call);
        } else if (s->expr != NULL) {
            gen_expr(s->expr);
        }
        break;
    
    default:
        break;
    }
}

static void gen_statements(AstStmt *s){
    for(; s != NULL; s->next){
        gen_statements(s);
    }
}

void generate_program(ExprNode *node){
    (void)node;     //iba zatial
}