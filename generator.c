/*
 * Projekt: Implementacia prekladaca imperativneho jazyka IFJ26
 * Subor: generator.c
 * Autor: Veronika Kentošová(xkentov00) a Filip Šimulčík(xsimulf00)
 */
#include "generator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int label_counter = 0; // pocitadlo navesti/skokov
static char defvar_buf[256][64];
static int defvar_count = 0;
static void gen_statement(AstStmt *s);
// funkcia na zavolanie funkcie a odovzdanie jej paramatrov
static void gen_call_stmt(AstCall *call) {
    if (call == NULL)
        return;

    gen_call_start();
    for (int i = 0; i < call->arg_count; i++) { // prechadzanie vsetkych argumentov volania z lava do prava
        gen_expr(call->args[i]);
        gen_call_argument(i);
    }
    gen_call(call->name);
}
// rekurzivne zbieranie vsetkych lokalnych premennych, aby ich bolo mozne deklarovat na zaciatku funkcie
static void collect_defvars(AstStmt *s) {
    for (; s != NULL; s = s->next) {
        if (s->kind == AST_VARDEF) {
            gen_var_declare(s->name);
        }
        if (s->then_block != NULL) {
            collect_defvars(s->then_block);
        }
        if (s->else_block != NULL) {
            collect_defvars(s->else_block);
        }
    }
}
// generovanie unikatneho poradoveho cisla pre navestie
int gen_new_label(void) {
    return label_counter++;
}
// hlavicka
void gen_init(void) {
    printf(".IFJcode26\n");
    printf("DEFVAR GF@$tmp1\n");
    printf("DEFVAR GF@$tmp2\n");
    printf("JUMP $$main\n");
}
// odlozenie premennej do pomecneho buffera
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
// ziskanie hodnoty zo zasobnika do lokal. premennej
void gen_var_assign(const char *name) {
    printf("POPS LF@%s\n", name);
}
// vyhodnotenie podmienky
void gen_if_start(int label_id) {
    printf("PUSHS bool@true\n");
    printf("JUMPIFNEQS $$else_%d\n", label_id);
}
// prechod na else vetvu
void gen_if_else(int label_id) {
    printf("JUMP $$if_end_%d\n", label_id);
    printf("LABEL $$else_%d\n", label_id);
}
// nastavenie konca celej if-else podmienky
void gen_if_end(int label_id) {
    printf("LABEL $$if_end_%d\n", label_id);
}

// zaciatok while cyklu, test podmienky a pripadny vyskok von
void gen_while_start(int label_id, ExprNode *node) {
    printf("LABEL $$while_%d\n", label_id);
    gen_expr(node);
    printf("PUSHS bool@true\n");
    printf("JUMPIFNEQS $$while_end_%d\n", label_id);
}
// koneic while cyklu, navrat na zaciatok cklu popripade navestie pre ukoncenie
void gen_while_end(int label_id) {
    printf("JUMP $$while_%d\n", label_id);
    printf("LABEL $$while_end_%d\n", label_id);
}
// rekurzivne vyhodnotenie stromu (post-order prechod)
// vysledky kazdeho podvyrazu sa napisu na vrchol zasobniku
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
    // ukazne funkcie - unarne minusko
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
    // binarne operacie - najprv sa vyhidnotia oba operandy na zasobnik, potom samotny operand
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
        // relacne operatory
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
// generovanie hlavicky uzivatel. funkcie
void gen_func_start(const char *name) {
    printf("LABEL $%s\n", name);
    printf("PUSHFRAME\n");
}
// ukoncuje riadenie funkcie a vracia riadeeie tam odkial bola zavolana
void gen_func_end(void) {
    printf("POPFRAME\n");
    printf("RETURN\n");
}
// prevzanie parametra a ulozenie do lokalnej premennej
void gen_func_param(const char *name, int index) {
    printf("DEFVAR LF@%s\n", name);
    printf("MOVE LF@%s LF@%%%d\n", name, index);
}
// pripravenie cisteho docasneho ramcu pred zavolanim uzivatelskej funkcie
void gen_call_start(void) {
    printf("CREATEFRAME\n");
}
// prevzanie vypocitanej hodnoty argumentu zo zasobnika a ulozenie do docasneho ramca
void gen_call_argument(int index) {
    printf("DEFVAR TF@%%%d\n", index);
    printf("POPS TF@%%%d\n", index);
}
// vykonanie skoku do uzivatelskej funkcie
void gen_call(const char *name) {
    printf("CALL $%s\n", name);
}
// konvertovanie retazca z C do platneho formatu IFJcode26
void gen_print_escaped_string(const char *str) {
    if (str == NULL)
        return;

    for (int i = 0; str[i] != '\0'; i++) { // prechadza retazec do konca
        unsigned char c = (unsigned char)str[i];
        if (c <= 32 || c == 35 || c == 92) { // overuje ci ide o neviditelne znaky, medzeru, #, lomitko
            printf("\\%03d", c);             // vypise cislo znaku v desiatkovej sustave a so spravnym formatovanim
        } else {
            putchar(c);
        }
    }
}
// zaciatok hlavneho programu
void gen_main_start(void) {
    printf("LABEL $$main\n");
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");
}

void gen_main_end(void) {
    printf("POPFRAME\n");
}
// ukoncuje hlavne telo programu
static void gen_block(AstStmt *s) {
    for (; s != NULL; s = s->next) {
        gen_statement(s);
    }
}
// Hlavny prekladac jedneho prikazu AST do instrukcii IFJcode26.
// Podla typu uzla (s->kind) vygeneruje zodpovedajucu postupnost instrukcii.
static void gen_statement(AstStmt *s) {
    if (s == NULL) {
        return;
    }

    switch (s->kind) {

    // definicia varu alebo priradenie
    case AST_VARDEF:
    case AST_ASSIGN:
        if (s->expr != NULL) {
            gen_expr(s->expr);
        } else if (s->call != NULL) {
            gen_call_stmt(s->call);
        }
        gen_var_assign(s->name);
        break;
    // podmienene vetvenie if/else
    case AST_IF: {
        int lbl = gen_new_label();
        gen_expr(s->expr);        // vyhodnotí podmienku
        gen_if_start(lbl);        // ak false, skočí na else
        gen_block(s->then_block); // telo then
        gen_if_else(lbl);         // preskočí else a označí začiatok else
        if (s->else_block != NULL) {
            gen_block(s->else_block); // telo else (ak existuje)
        }
        gen_if_end(lbl); // koniec celého if
        break;
    }
    // cyklus while
    case AST_WHILE: {
        int lbl = gen_new_label();
        gen_while_start(lbl, s->expr); // začiatok cyklu + test podmienky
        gen_block(s->then_block);      // telo cyklu
        gen_while_end(lbl);            // skok späť na začiatok
        break;
    }
    // navrat z funkcie
    case AST_RETURN:
        if (s->expr != NULL) {
            gen_expr(s->expr);
        } else if (s->call != NULL) {
            gen_call_stmt(s->call);
        }
        gen_func_end(); // vypíše: POPFRAME a RETURN
        break;
    // samotny vyraz alebo volanie funkcie ako prikaz
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

// vytvorenie vstavanej funckie print
void gen_print(void) {
    printf("LABEL $$print\n");
    printf("PUSHFRAME\n");
    printf("DEFVAR LF@val\n");
    printf("DEFVAR LF@type\n");
    printf("POPS LF@val\n");
    printf("TYPE LF@type LF@val\n");
    printf("JUMPIFEQ $$print_unit LF@type string@nil\n");
    printf("WRITE LF@val\n");
    printf("JUMP $$print_end\n");
    printf("LABEL $$print_unit\n");
    printf("WRITE string@()\n");
    printf("LABEL $$print_end\n");
    printf("POPFRAME\n");
    printf("RETURN\n");
}

// vytvorenie vstavanej funcke readLine
void gen_readLine(void) {
    printf("LABEL $$readLine\n");
    printf("PUSHFRAME\n");
    printf("DEFVAR LF@val\n");
    printf("DEFVAR LF@type\n");
    printf("READ LF@val string\n");
    printf("TYPE LF@type LF@val\n");
    printf("JUMPIFEQ $$readLine_eof LF@type string@nil\n"); // ak sa nacita eof respektive nic tak skoci na koniec a ukonci sa
    printf("PUSHS LF@val\n");
    printf("JUMP $$readLine_end\n");
    printf("LABEL $$readLine_eof\n");
    printf("PUSHS string@\n");
    printf("LABEL $$readLine_end\n");
    printf("POPFRAME\n");
    printf("RETURN\n");
}
// vytvorenie vstavanej funckie readInt
void gen_readInt(void) {
    printf("LABEL $$readInt\n");
    printf("PUSHFRAME\n");
    printf("DEFVAR LF@val\n");
    printf("DEFVAR LF@type\n");
    printf("READ LF@val int\n");
    printf("TYPE LF@type LF@val\n");
    printf("JUMPIFEQ $$readInt_eof LF@type string@nil\n"); // ak sa nacita eof respektive nic tak skoci na koniec a ukonci sa
    printf("PUSHS LF@val\n");
    printf("JUMP $$readInt_end\n");
    printf("LABEL $$readInt_eof\n");
    printf("PUSHS int@0\n");
    printf("LABEL $$readInt_end\n");
    printf("POPFRAME\n");
    printf("RETURN\n");
}

// vytvorenie vstavanej funckie readDouble
void gen_readDouble(void) {
    printf("LABEL $$readDouble\n");
    printf("PUSHFRAME\n");
    printf("DEFVAR LF@val\n");
    printf("DEFVAR LF@type\n");
    printf("READ LF@val float\n");
    printf("TYPE LF@type LF@val\n");
    printf("JUMPIFEQ $$readDouble_eof LF@type string@nil\n"); // ak sa nacita eof respektive nic tak skoci na koniec a ukonci sa
    printf("PUSHS LF@val\n");
    printf("JUMP $$readDouble_end\n");
    printf("LABEL $$readDouble_eof\n");
    printf("PUSHS float@0x0p+0\n");
    printf("LABEL $$readDouble_end\n");
    printf("POPFRAME\n");
    printf("RETURN\n");
}
// vytvorenie vstavanej funckie readDouble
void gen_toStr(void) {
    printf("LABEL $$toStr\n");
    printf("PUSHFRAME\n");
    printf("DEFVAR LF@val\n");
    printf("DEFVAR LF@type\n");
    printf("DEFVAR LF@SYMB\n");
    printf("POPS LF@val\n");
    printf("TYPE LF@type LF@val\n");
    // podla typu skoci na prislisnu vetvu a pushne sa na zasobnik prislusna premenna
    printf("JUMPIFEQ $$toStr_float LF@type string@float\n");
    printf("JUMPIFEQ $$toStr_unit LF@type string@nil\n");
    printf("JUMPIFEQ $$toStr_int LF@type string@int\n");
    printf("JUMPIFEQ $$toStr_bool LF@type string@bool\n");
    printf("JUMPIFEQ $$toStr_string LF@type string@string\n");
    printf("JUMP $$toStr_end\n");
    printf("LABEL $$toStr_unit\n");
    printf("PUSHS string@nil\n");
    printf("JUMP $$toStr_end\n");
    printf("LABEL $$toStr_float\n");
    printf("FLOAT2STR LF@SYMB LF@val\n");
    printf("PUSHS LF@SYMB\n");
    printf("JUMP $$toStr_end\n");
    printf("LABEL $$toStr_int\n");
    printf("INT2STR LF@SYMB LF@val\n");
    printf("PUSHS LF@SYMB\n");
    printf("JUMP $$toStr_end\n");
    printf("LABEL $$toStr_bool\n");
    printf("JUMPIFEQ $$toStr_bool_true LF@val bool@true\n");
    printf("PUSHS string@false\n");
    printf("JUMP $$toStr_end\n");
    printf("LABEL $$toStr_bool_true\n");
    printf("PUSHS string@true\n");
    printf("JUMP $$toStr_end\n");
    printf("LABEL $$toStr_string\n");
    printf("PUSHS LF@val\n");
    printf("LABEL $$toStr_end\n");
    printf("POPFRAME\n");
    printf("RETURN\n");
}
// vytvorenie vstavanej funckie toInt
void gen_toInt(void) {
    printf("LABEL $$toInt\n");
    printf("PUSHFRAME\n");
    printf("DEFVAR LF@val\n");
    printf("DEFVAR LF@sym\n");
    printf("POPS LF@val\n");
    printf("FLOAT2INT LF@sym LF@val\n"); // odsekne desatinne cislo a spravi z cisla int
    printf("PUSHS LF@sym\n");
    printf("POPFRAME\n");
    printf("RETURN\n");
}
// vytvorenie vstavanej funckie strLength
void gen_strLength(void) {
    printf("LABEL $$strLength\n");
    printf("PUSHFRAME\n");
    printf("DEFVAR LF@val\n");
    printf("DEFVAR LF@sym\n");
    printf("POPS LF@val\n");
    printf("STRLEN LF@sym LF@val \n"); // zisti dlzku retazca
    printf("PUSHS LF@sym\n");          // pushne ju na zasobnik
    printf("POPFRAME\n");
    printf("RETURN\n");
}
// vytvorenie vstavanej funckie chr
void gen_chr(void) {
    printf("LABEL $$chr\n");
    printf("PUSHFRAME\n");
    printf("DEFVAR LF@val\n");
    printf("DEFVAR LF@sym\n");
    printf("POPS LF@val\n");
    printf("INT2CHAR LF@sym LF@val\n"); // premeni cislo na znak
    printf("PUSHS LF@sym\n");
    printf("POPFRAME\n");
    printf("RETURN\n");
}
// vytvorenie vstavanej funckie ord
void gen_ord(void) {
    printf("LABEL $$ord\n");
    printf("PUSHFRAME\n");
    printf("DEFVAR LF@val\n");
    printf("DEFVAR LF@num\n");
    printf("DEFVAR LF@sym\n");
    printf("DEFVAR LF@len\n");
    printf("DEFVAR LF@con\n");
    printf("POPS LF@num\n");
    printf("POPS LF@val\n");
    // zistenie dlzky retazcu
    printf("STRLEN LF@len LF@val\n");
    printf("LT LF@con LF@num int@0 \n");              //<0
    printf("JUMPIFEQ $$ord_nil LF@con bool@true\n");  // >0
    printf("LT LF@con LF@num LF@len \n");             // platny rozsah
    printf("JUMPIFEQ $$ord_nil LF@con bool@false\n"); // platny rozsah
    printf("STRI2INT LF@sym LF@val LF@num\n");
    printf("PUSHS LF@sym\n");
    printf("JUMP $$ord_end\n");
    printf("LABEL $$ord_nil\n");
    printf("PUSHS int@-1\n");
    printf("LABEL $$ord_end\n");
    printf("POPFRAME\n");
    printf("RETURN\n");
}
// vytvorenie vstavanej funckie subStr
void gen_subStr(void) {
    printf("LABEL $$subStr\n");
    printf("PUSHFRAME\n");
    printf("DEFVAR LF@val\n");
    printf("DEFVAR LF@start\n");
    printf("DEFVAR LF@end\n");
    printf("DEFVAR LF@res\n");
    printf("DEFVAR LF@len\n");
    printf("DEFVAR LF@char\n");
    printf("DEFVAR LF@con\n");
    printf("MOVE LF@res string@\n");
    printf("POPS LF@end\n");
    printf("POPS LF@start\n");
    printf("POPS LF@val\n");
    printf("STRLEN LF@len LF@val\n");
    // kontrola 4 podmienok intervalu
    printf("LT LF@con LF@start int@0\n");
    printf("JUMPIFEQ $$subStr_end LF@con bool@true\n");
    printf("LT LF@con LF@end int@0\n");
    printf("JUMPIFEQ $$subStr_end LF@con bool@true\n");
    printf("LT LF@con LF@end LF@start\n");
    printf("JUMPIFEQ $$subStr_end LF@con bool@true\n");
    printf("LT LF@con LF@len LF@end\n");
    printf("JUMPIFEQ $$subStr_end LF@con bool@true\n");
    // ak plati interval ide sa do looopu kde sa znak po znaku vysklada vysledny retazec
    printf("LABEL $$subStr_loop\n");
    printf("LT LF@con LF@start LF@end\n");
    printf("JUMPIFEQ $$subStr_end LF@con bool@false\n");
    printf("GETCHAR LF@char LF@val LF@start\n");
    printf("CONCAT LF@res LF@res LF@char\n");
    printf("ADD LF@start LF@start int@1\n");
    printf("JUMP $$subStr_loop\n");
    // ked sa loop skonci pushne sa to na zasobnik
    printf("LABEL $$subStr_end\n");
    printf("PUSHS LF@res\n");
    printf("POPFRAME\n");
    printf("RETURN\n");
}
// logicke vystavanie generatora
void generate_program(AstProgram *program) {
    if (program == NULL)
        return;
    // volanie vtavanych funkcii
    gen_init();
    gen_print();
    gen_readLine();
    gen_readInt();
    gen_readDouble();
    gen_toStr();
    gen_toInt();
    gen_strLength();
    gen_chr();
    gen_ord();
    gen_subStr();

    for (AstFunc *f = program->first_func; f != NULL; f = f->next) {

        if (f->is_main) {
            gen_main_start(); // LABEL $$main, CREATEFRAME, PUSHFRAME
        } else {
            gen_func_start(f->name); // LABEL $name, PUSHFRAME

            // Preberieme parametre funkcie (LF@param = LF@%i)
            for (int i = 0; i < f->param_count; i++) {
                gen_func_param(f->param_names[i], i);
            }
        }

        // Pozbierame a vypiseme všetky DEFVAR pre telo funkcie
        collect_defvars(f->body);
        gen_flush_defvars();

        // Vygenerujeme samotne prikazy tela funkcie
        gen_block(f->body);

        // Ukoncime funkciu
        if (f->is_main) {
            gen_main_end(); // POPFRAME
        } else {
            gen_func_end(); // POPFRAME, RETURN
        }
    }
}
