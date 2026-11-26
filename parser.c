/*
 * Projekt: Implementacia prekladaca imperativneho jazyka IFJ26
 * Subor: parser.c
 * Autor: Paulina Kostalova (xkostap00)
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <unistd.h>
#include <ctype.h>

#include "parser.h"
#include "scanner.h"        
#include "stack.h"
#include "symtable.h"
#include "expressions.h"

#define MAX_PARAMS 32



// Pomocna struktura pre docasne ulozenie parametra funkcie pri parsovani hlavicky
typedef struct parser
{
    char *name;
    Datatype type;
}ParamInfo;

// Zaznam o funkcii pre riadenie 2. prechodu a detekciu rekurzie pri odvodzovani typu
typedef struct 
{
    char  *name;
    int state;      // 0 = nespracovana, 1 = prave sa spracovava, 2 = hotova
    int inferred;   // 1 ak funkcia nemala explicitne uvedeny navratovy typ
}FuncEntry;

// Graf volani medzi funkciami (hrana: parsing_idx -> volana funkcia)
static int parsing_idx = -1;
static int call_edges[4096][2];
static int call_edge_count = 0;

// Zaznamena orientovanu hranu v grafe volani funkcii (pre detekciu rekurzie pri odvodzovani typu)
static void add_call_edge(int from, int to){
    if(from < 0 || to < 0){
        return;
    }
    for(int i = 0; i < call_edge_count; i++){
        if(call_edges[i][0] == from && call_edges[i][1] == to){
            return;
        }
    }
    if(call_edge_count < 4096){
        call_edges[call_edge_count][0] = from;
        call_edges[call_edge_count][1] = to;
        call_edge_count++;
    }
}


// Informacie o lokalnej premennej (uroven zanorenia, modifikovatelnost var/val a unikatne ID)
typedef struct
{
    char *name;
    int level;
    bool is_mutable;
    int unique_id;
}VarInfo;


ExprNode* create_leaf(NodeType nodetype, Datatype datatype, const char *value);
ExprNode* create_binary(PrecSymbol sybol, ExprNode* left, ExprNode* right, Datatype datatype);


// Globalny stav parsera: aktualny token, tabulka symbolov a uroven zanorenia blokov
static Token current_token; 

static symtable *current_symtable = NULL;
static int current_scope_level = 0;

static ParamInfo current_params[MAX_PARAMS];
static int current_param_count = 0;

static bool main_function_found = false;

// Navratovy typ prave parsovanej funkcie (pre kontrolu prikazov return)
static Datatype current_func_ret_type = TYPE_UNIT;
static bool current_func_explicit = false;

static AstProgram *ast_program = NULL;

static FuncEntry *funcs = NULL;
static int func_count = 0;
static int func_cap = 0;

static VarInfo *vars = NULL;
static int var_count = 0;
static int var_cap = 0;
static int var_counter = 0;

//Kontext pre navrat cez longjmp, ked volana funkcia este nema odvodeny navratovy typ
static jmp_buf parse_jmp;
static int blocked_on = -1;


// Dopredu deklarovane funkcie parsera
static void fetch_token(void);
static void expect(TokenType expect_type);
static void syntax_error(void);
static void parse_prolog(void);
static Datatype parse_type(void);
static void parse_function(void);
static AstStmt *parser_statements(void);
static AstStmt *parser_assignment(char *name);
static AstStmt *parse_identifier_statement(void);
static AstStmt *parse_var_declaration(void);
static void parse_parameter_list(int is_main);
static AstStmt *parse_if(void);
static AstStmt *parse_while(void);
static AstStmt *parse_return(void);
static AstStmt *parse_expression_statement(void);
static AstCall *parse_function_call(char *name);




// Nacita dalsi token zo scannera
static void fetch_token(void){  

    current_token = get_next_token();

    if(current_token.type == TOKEN_ERROR){
        fprintf(stderr, "|Lexical error in code\n");
        exit(1);
    }

}

// Vypise syntakticku chybu a ukonci prekladac
static void syntax_error(void){
    fprintf(stderr, "Syntax error in code\n");
    
    exit(2);
}



// Overi, ci aktualny token zodpoveda ocakavanemu typu; ak ano, nacita dalsi token
static void expect(TokenType expected_type){  
    if (current_token.type == expected_type){
        fetch_token();
    }
    else{
        syntax_error();
    }
}


// Alokuje a vytvori dynamicku kopiu retazca
static char *copy_string(const char *src){
    char *dst = malloc(strlen(src)+1);
    if(dst == NULL){
        exit(99);
    }
    strcpy(dst, src);
    return dst;
}


// Alokuje a inicializuje novy AST uzol prikazu daneho typu
static AstStmt *new_stmt(AstStmtType kind){
    AstStmt *stmt = calloc(1, sizeof(AstStmt));
    if(stmt == NULL){
        exit(99);
    }
    stmt->kind = kind;
    return stmt;
}


// Skontroluje typovu kompatibilitu (vratane konverzie Int -> Double)
static bool is_compatible(Datatype target, Datatype src){
    return target == TYPE_ANY || target == src || (target == TYPE_DOUBLE && src == TYPE_INT);
}

// Zakaze ukladanie, odovzdavanie alebo vracanie pravdivostnej hodnoty
static void check_not_bool(Datatype type){
    if(type == TYPE_BOOL){
        fprintf(stderr, "semantic eror: boolean value can not be stored, passed or returned\n");
        exit(6);
    }
}

// Vyhlada zaznam funkcie v globalnom scope tabulky symbolov
static Symboldata *lookup_func(symtable *t, char *func_name){
    return lookup_var(0, func_name, t);
}

// Vyhlada premennu v aktualnom alebo nadradenom scope (odfiltruje zaznamy funkcii)
static Symboldata *find_var(symtable *t, int scope_level, char *name){
    Symboldata *data = lookup_var(scope_level, name, t);
    if(data != NULL && data->is_function){
        return NULL;
    }
    return data;
}

// Zaregistruje novu lokalnu premennu v zasobniku premennych a priradi jej unikatne ID
static int register_var(const char *name, int level, bool is_mutabe){
    if(var_count == var_cap){
        var_cap = (var_cap == 0) ? 32 : var_cap * 2;
        vars = realloc(vars, var_cap * sizeof(VarInfo));
        if(vars == NULL){
            exit(99);
        }
    }
    vars[var_count].name = copy_string(name);
    vars[var_count].level = level;
    vars[var_count].is_mutable = is_mutabe;
    vars[var_count].unique_id = ++var_counter;
    var_count++;
    return var_counter;
}

// Najde najblizsiu (najvnutornejsiu) deklaraciu premennej podla mena
static VarInfo *find_var_info(const char *name){
    for(int i = var_count -1; i >= 0; i--){
        if(strcmp(vars[i].name, name) == 0){
            return &vars[i];
        }
    }
    return NULL;
}

// Opusti aktualny blok (scope) a odstrani premenne tejto urovne z tabulky aj zo zasobnika
static void leave_scope(void){
    exit_scope(current_scope_level, current_symtable);
    while(var_count > 0 && vars[var_count -1].level == current_scope_level){
        free(vars[var_count -1].name);
        var_count--;
    }
    current_scope_level--;
}

// Zaregistruje funkciu do zoznamu funkcii programu pre druhy prechod parsera
static void register_function(const char *name){
    if(func_count == func_cap){
        func_cap = (func_cap == 0) ? 16 : func_cap *2;
        funcs = realloc(funcs, func_cap * sizeof(FuncEntry));
        if(funcs == NULL){
            exit(99);
        }
    }
    funcs[func_count].name = copy_string(name);
    funcs[func_count].state = 0;
    funcs[func_count].inferred = 0;
    func_count++;
}

// Vrati index funkcie v poli funcs podla mena 
static int find_func_index(const char *name){
    for(int i = 0; i < func_count; i++){
        if(strcmp(funcs[i].name, name) == 0){
            return i;
        }
    }
    return -1;
}

// Vrati citanie zo standardneho vstupu spat na zaciatok suboru
static void rewind_input(void){
    fseek(stdin, 0, SEEK_SET);
    clearerr(stdin);
}

// Skopiruje cely stdin do docasneho suboru, aby bolo mozne vstup citat vo viacerych prechodoch
static void prepare_input(void){
    FILE *tmp = tmpfile();
    if(tmp == NULL){
        exit(99);
    }

    char buffer[4096];
    size_t count;
    while((count = fread(buffer, 1, sizeof(buffer), stdin)) > 0){
        if(fwrite(buffer, 1, count, tmp) != count){
            exit(99);
        }
    }
    fflush(tmp);

    // Presmerovanie deskriptora STDIN na docasny subor umoznujuce opakovany fseek
    if(dup2(fileno(tmp), STDIN_FILENO) < 0){
        exit(99);
    }
    fclose(tmp);
    rewind_input();

}


// Overi povinny prolog programu: import ifj26.*
static void parse_prolog(void){
    expect(TOKEN_KW_IMPORT);   
    
    if(current_token.type != TOKEN_IDENTIFIER || strcmp(current_token.attribute, "ifj26") != 0){
        syntax_error();
    }
    fetch_token();

    expect(TOKEN_DOT);
    expect(TOKEN_MUL);
    expect(TOKEN_EOL);
}

// Spracuje klucove slovo datoveho typu (Int, Double, String, Unit)
static Datatype parse_type(void){
    Datatype type = TYPE_UNIT;

    if(current_token.type == TOKEN_KW_INT){
        type = TYPE_INT;
    }
    else if(current_token.type == TOKEN_KW_DOUBLE_TYPE){
        type = TYPE_DOUBLE;
    }
    else if(current_token.type == TOKEN_KW_STRING_TYPE){
        type = TYPE_STRING;
    }
    else if(current_token.type == TOKEN_KW_UNIT){
        type = TYPE_UNIT;
    }
    else if(current_token.type == TOKEN_IDENTIFIER){
        fprintf(stderr, "semantic error: unknown type '%s'\n", current_token.attribute);
        exit(3);   // pouzitie nedefinovaneho typu
    }
    else{
        syntax_error();
    }
    fetch_token();
    return type;
}

// Overi ukoncenie prikazu 
static void end_statement(void){
    if(current_token.type == TOKEN_SEMICOLON){
        fetch_token();
        return;
    }
    expect(TOKEN_EOL);
}

// Spracuje jeden term (literal alebo premennu) ako argument volania funkcie a odmietne vnorene volanie funkcie (is_ident a TOKEN_LPAREN)
static ExprNode *parse_term(Datatype *type){
    ExprNode *node = NULL;
    bool is_ident = false;
    char value[MAX_TOKEN_LEN + 2];
    value[0] = '\0';
    
    // Podpora zapornych ciselnych literalov (-Int, -Double)
    if(current_token.type == TOKEN_MINUS){
        fetch_token();
        if(current_token.type != TOKEN_INTEGER && current_token.type != TOKEN_DOUBLE){
            syntax_error();
        }
        strcpy(value, "-");
    }
    strcat(value, current_token.attribute);

    // Podla typu tokenu vytvorime listovy uzol AST pre literal alebo premennu (s overenim deklaracie v symtable)
    switch (current_token.type)
    {
    case TOKEN_INTEGER:
        node = create_leaf(NODE_LITERAL, TYPE_INT, value);
        *type = TYPE_INT;
        break;
    
    case TOKEN_DOUBLE:
        node = create_leaf(NODE_LITERAL, TYPE_DOUBLE, value);
        *type = TYPE_DOUBLE;
        break;

    case TOKEN_STRING:
        node = create_leaf(NODE_LITERAL, TYPE_STRING, value);
        *type = TYPE_STRING;
        break;

    case TOKEN_LPAREN:
        if(value[0] != '\0'){
            syntax_error();
        }
        fetch_token();
        if(current_token.type != TOKEN_RPAREN){
            syntax_error();
        }
        node = create_leaf(NODE_LITERAL, TYPE_UNIT, "()");
        *type = TYPE_UNIT;
        break;

    case TOKEN_IDENTIFIER:{
        Symboldata *var = find_var(current_symtable, current_scope_level, current_token.attribute);
        if(var == NULL){
            fprintf(stderr, "semantic error: variable '%s' was not declared\n", current_token.attribute);
            fetch_token();
            if(current_token.type == TOKEN_LPAREN){
                syntax_error();
            }
            exit(3);
        }
        node = create_leaf(NODE_VAR, var->typ, value);
        *type = var->typ;
        is_ident = true;
        break;}

    case TOKEN_INTERP_STRING:
        // Uzol NODE_INTERP (definovany v expressions.h) pre interpolovany retazec s"..."
        node = create_leaf(NODE_INTERP, TYPE_STRING, value);
        *type = TYPE_STRING;
        break;

    default:
        syntax_error();
        break;
    }

    if(node == NULL){
        exit(99);
    }
    fetch_token();
    if(is_ident && current_token.type == TOKEN_LPAREN){
        syntax_error();   // volanie funkcie nesmie byt argumentom
    }
    return node;
}

// Spracuje volanie funkcie, skontroluje pocet a typy argumentov a vrati uzol AstCall
static AstCall *parse_function_call(char *name){

    char *func_name = copy_string(name);

    expect(TOKEN_LPAREN);

    Symboldata *func_data = lookup_func(current_symtable, func_name);

    if(func_data == NULL || !func_data->is_function){
        fprintf(stderr, "semantic error: function '%s' was not declared\n", func_name);
        exit(3);
    }

    // Ak funkcia este nema navratovy typ (TYPE_ANY), prerusime parsovanie funkcie pomocou longjmp (najprv sa spracuje volana funkcia)
    if(func_data->typ == TYPE_ANY){
        int idx  = find_func_index(func_name);
        if(idx < 0 || funcs[idx].state == 1){
            fprintf(stderr, "semantic error: recursive function '%s' need explicit return type\n", func_name);
            exit(6);
        }
        blocked_on = idx;
        longjmp(parse_jmp, 1);
    }

    // Zaznamename volanie do grafu volani pre neskorsiu kontrolu nepriamej rekurzie
    add_call_edge(parsing_idx, find_func_index(func_name));

    AstCall *call = calloc(1, sizeof(AstCall));
    if(call == NULL){
        exit(99);
    }
    call->name = func_name;
    call->ret_type = func_data->typ;

    if(func_data->param_count > 0){
        call->args = malloc(sizeof(ExprNode *) * func_data->param_count);
        if(call->args == NULL){
            exit(99);
        }
    }

    int arg_count = 0;

    // Postupne nacitanie argumentov volania a kontrola ich poctu a typov 
    while (current_token.type != TOKEN_RPAREN && current_token.type != TOKEN_EOF){
        if(current_token.type == TOKEN_COMMA){
            syntax_error();
        }
        if(arg_count >= func_data->param_count){
            fprintf(stderr, "semantic error: too many arguments in call of function '%s'\n", func_name);
            exit(5);
        }

        Datatype arg_type = TYPE_ANY;
        ExprNode *arg = parse_term(&arg_type);

        if(!is_compatible(func_data->param_type[arg_count], arg_type)){
            fprintf(stderr, "semantic error: type mismatch in argument %d of function '%s'\n", arg_count+1, func_name);
            exit(5);
        }

        call->args[arg_count] = arg;
        arg_count++;

        if(current_token.type == TOKEN_COMMA){
            fetch_token();

            // Koncova ciarka pred ')' je povolena iba vo viacriadkovom zapise
            if(current_token.type == TOKEN_RPAREN && !current_token.had_newline_before){
                syntax_error();
            }
        }
        else if(current_token.type != TOKEN_RPAREN){
            syntax_error();
        }
    }    

    expect(TOKEN_RPAREN);

    if(arg_count != func_data->param_count){
        fprintf(stderr, "semantic error: function '%s' expects %d arguments, but %d were provided\n", func_name, func_data->param_count, arg_count);
        exit(5);
    }

    call->arg_count = arg_count;
    return call;
    
}

// Dodatocna kontrola pre parse_expression: (lookup_var v symtable.c vracia aj funkcie -> overenie, ze ziadny identifikator vo vyraze nie je meno funkcie pouzite bez zatvoriek)
static void check_expr_vars(ExprNode *node){
    if(node == NULL){
        return;
    }
    if(node->nodetype == NODE_VAR){
        if(find_var(current_symtable, current_scope_level, node->value) == NULL){
            fprintf(stderr, "semantic error: variable '%s' was not declared\n", node->value);
            exit(3);
        }
    }
    check_expr_vars(node->left);
    check_expr_vars(node->right);
}

// Spracuje hodnotu na pravej strane (vyraz, volanie funkcie alebo literal () typu Unit)
static void parse_value(ExprNode **expr, AstCall **call, Datatype *type){
    *expr = NULL;
    *call = NULL;

    TokenType start_type = current_token.type;
    char *start_name = NULL;

    if(start_type == TOKEN_IDENTIFIER){
        start_name = copy_string(current_token.attribute);
    }

    Datatype expr_type = TYPE_ANY;
    int expr_err = parse_expression(&current_token, expr, &expr_type, current_symtable, current_scope_level);

    if(expr_err == 0){
        if(current_token.type == TOKEN_ERROR){
            exit(1);
        }
        if(current_token.type == TOKEN_LBRACKET || current_token.type == TOKEN_RBRACKET){
            syntax_error();
        }
        check_expr_vars(*expr);
        *type = expr_type;
    }
    // Ak precedencna analyza narazila na '(' hned za menom funkcie, ide o volanie funkcie
    else if(expr_err == ERROR_SYNT && start_type == TOKEN_IDENTIFIER && current_token.type == TOKEN_LPAREN &&
         lookup_func(current_symtable, start_name) != NULL && lookup_func(current_symtable, start_name)->is_function){
            *expr = NULL;
            *call = parse_function_call(start_name);
            *type = (*call)->ret_type;
    }

    // Ak precedencna analyza skoncila na prazdnych zatvorkach (), vytvorime literal typu Unit
    else if(expr_err == ERROR_SYNT && start_type == TOKEN_LPAREN && 
        (current_token.type == TOKEN_EOL || current_token.type == TOKEN_SEMICOLON || current_token.type == TOKEN_EOF)){
            *expr = create_leaf(NODE_LITERAL, TYPE_UNIT, "()");
            if(*expr == NULL){
                exit(99);
            }
            *type = TYPE_UNIT;
    }
    else{
        exit(expr_err);
    }
    free(start_name);
}

// Spracuje priradenie do existujucej premennej 
static AstStmt *parser_assignment(char *name){

    char *var_name = copy_string(name);

    Symboldata *var_data = find_var(current_symtable, current_scope_level, var_name);
      
    if(var_data == NULL){
        fprintf(stderr, "semantic error: variable '%s' was not declared\n", var_name);
        exit(3);
    }

    VarInfo *var_info = find_var_info(var_name);

    expect(TOKEN_ASSIGN);  
    
    AstStmt *stmt = new_stmt(AST_ASSIGN);
    Datatype expr_type = TYPE_ANY;

    parse_value(&stmt->expr, &stmt->call, &expr_type);

    end_statement();
   
    // Kontrola modifikovatelnosti: do nemenneho 'val' ani parametra funkcie nemozno priradit 
    if(var_info != NULL && !var_info->is_mutable){
        fprintf(stderr, "semantic error: assignment to immutable variable '%s'\n", var_name);
        exit(7);
    }

    check_not_bool(expr_type);

    // Kontrola typovej kompatibility priradovanej hodnoty s typom premennej 
    if(!is_compatible(var_data->typ, expr_type)){
        fprintf(stderr, "semantic error: type mismatch in assignemnt to variable '%s' (variable: %d, expression %d)\n", var_name, var_data->typ, expr_type);
        exit(6);
    }

    stmt->name = var_name;
    stmt->datatype = var_data->typ;
    stmt->unique_id = (var_info != NULL) ? var_info->unique_id : 0;
    return stmt;

}


// Spracuje prikaz zacinajuci identifikatorom (priradenie, volanie funkcie, vyraz)
static AstStmt *parse_identifier_statement(void){

    char *id_name = copy_string(current_token.attribute);

    AstStmt *stmt = new_stmt(AST_EXPR);
    Datatype expr_type = TYPE_ANY;

    parse_value(&stmt->expr, &stmt->call, &expr_type);

    // Ak bol nacitany samotny identifikator premennej nasledovany '=', zmenime prikaz na priradenie
    if(current_token.type == TOKEN_ASSIGN && stmt->expr != NULL && stmt->expr->nodetype == NODE_VAR){

        expr_free(stmt->expr);
        free(stmt);
        stmt = parser_assignment(id_name);
    }
    else{
        check_not_bool(expr_type);
        stmt->datatype = expr_type;
        end_statement();
    }

    free(id_name);
    return stmt;
}

// Spracuje deklaraciu premennej (val / var) s volitelnym typom a inicializacnou hodnotou
static AstStmt *parse_var_declaration(void){

    bool is_mutable = false;

    if(current_token.type == TOKEN_KW_VAL || current_token.type == TOKEN_KW_VAR){
        is_mutable = (current_token.type == TOKEN_KW_VAR);
        fetch_token();
    }
    else{
        syntax_error();
    }

    if(current_token.type != TOKEN_IDENTIFIER){
        syntax_error();
    }

    char *var_name = copy_string(current_token.attribute);

    fetch_token();

    Datatype var_type = TYPE_ANY;
    bool has_type = false;


    if(current_token.type == TOKEN_COLON){
        fetch_token();
        
        var_type = parse_type();
        has_type = true;
    }
    expect(TOKEN_ASSIGN);

    AstStmt *stmt = new_stmt(AST_VARDEF);
    Datatype expr_type = TYPE_ANY;

    parse_value(&stmt->expr, &stmt->call, &expr_type);

    end_statement();
    
    check_not_bool(expr_type);

    if(has_type && !is_compatible(var_type, expr_type)){
        fprintf(stderr, "semantic error: type mismatch in var declaration (var.: %d, expr.: %d)\n", var_type, expr_type);
        exit(6);
    }

    // Ak typ nebol explicitne uvedeny, odvodi sa z typu vyrazu na pravej strane
    Datatype final_type = has_type ? var_type : expr_type;
    
    if(!insert_var(current_symtable, var_name, final_type, current_scope_level)){
        fprintf(stderr, "semantic error: variable '%s' already declared in this scope\n", var_name);
        exit(4);
    }

    int var_id = register_var(var_name ,current_scope_level, is_mutable);

    stmt->name = var_name;
    stmt->datatype = final_type;
    stmt->is_mutable = is_mutable;
    stmt->unique_id = var_id;
    return stmt;
}

// Spracuje odsadeny blok prikazov (INDENT po DEDENT) v novom vnorenom scope
static AstStmt *parse_block(void){
    expect(TOKEN_EOL);

    if(current_token.type != TOKEN_INDENT){
        syntax_error();
    }
    fetch_token();

    current_scope_level++;
    AstStmt *block = parser_statements();
    leave_scope();
    expect(TOKEN_DEDENT);

    return block;

}

// Spracuje prikaz if-then-else (vratane kontroly zarovnania else s if)
static AstStmt *parse_if(void){

    int if_indent = current_token.indent;

    expect(TOKEN_KW_IF);

    
    AstStmt *stmt = new_stmt(AST_IF);
    Datatype expr_type = TYPE_ANY;
    
    // Podmienka if sa spracuje precedencnou analyzou a musi mat vysledny typ Boolean 
    int expr_err = parse_expression(&current_token, &stmt->expr, &expr_type, current_symtable, current_scope_level);
    
    if(expr_err != 0){
        exit(expr_err);
    }
    check_expr_vars(stmt->expr);

    if(expr_type != TYPE_BOOL){
        fprintf(stderr, "semantic error: condition of if is not boolean\n");
        exit(6);
    }

    expect(TOKEN_KW_THEN);

    stmt->then_block = parse_block();

    // Vetva else musi mat rovnake odsadenie ako prislusny if
    if(current_token.type == TOKEN_KW_ELSE){
        if(current_token.indent != if_indent){
            syntax_error();
        }
        fetch_token();
        stmt->else_block = parse_block();
    }
    else {
        syntax_error();
    }
    return stmt;
}


// Spracuje cyklus while-do s pravdivostnou podmienkou a odsadenym telom
static AstStmt *parse_while(void){
    expect(TOKEN_KW_WHILE);

    AstStmt *stmt = new_stmt(AST_WHILE);
    Datatype expr_type = TYPE_ANY;
    int expr_err = parse_expression(&current_token, &stmt->expr, &expr_type, current_symtable, current_scope_level);
    if(expr_err != 0){
        exit(expr_err);
    }
    check_expr_vars(stmt->expr);

    if(expr_type != TYPE_BOOL){
        fprintf(stderr, "semantic error: condition of while is not boolean\n");
        exit(6);
    }
    expect(TOKEN_KW_DO);

    stmt->then_block = parse_block();

    return stmt;
}


// Spracuje prikaz return a overi zhodu s navratovym typom aktualnej funkcie
static AstStmt *parse_return(void){
    expect(TOKEN_KW_RETURN);

    AstStmt *stmt = new_stmt(AST_RETURN);
    Datatype expr_type = TYPE_UNIT;
    bool has_value = false;
    

    if(current_token.type != TOKEN_EOL && current_token.type != TOKEN_SEMICOLON && current_token.type != TOKEN_DEDENT && current_token.type != TOKEN_EOF){
        parse_value(&stmt->expr, &stmt->call, &expr_type);
        has_value = true;
    }

    end_statement();

    if(!has_value){
        // Samostatny return bez hodnoty je povoleny iba vo funkcii s typom Unit 
        if(!current_func_explicit || current_func_ret_type != TYPE_UNIT){
            fprintf(stderr, "semantic error: return without value is not allowed here\n");
            exit(10);
        }
    }
    else{
        if(!current_func_explicit){
            fprintf(stderr, "semantic error: function with return need explicit return type\n");
            exit(6);
        }
        check_not_bool(expr_type);
        if(current_func_ret_type != TYPE_UNIT && !is_compatible(current_func_ret_type, expr_type)){
            fprintf(stderr, "semantic error: return type mismatch\n");
            exit(6);
        }
    }

    stmt->datatype =current_func_ret_type;
    return stmt;
}


// Spracuje samostatny vyraz ako prikaz v bloku
static AstStmt *parse_expression_statement(void){
    AstStmt *stmt = new_stmt(AST_EXPR);
    Datatype expr_type = TYPE_ANY;

    parse_value(&stmt->expr, &stmt->call, &expr_type);
    check_not_bool(expr_type);
    stmt->datatype = expr_type;

    end_statement();
    return stmt;
}

// Spracuje postupnost prikazov v bloku a zretazi ich do zoznamu AstStmt
static AstStmt *parser_statements(void){

    AstStmt *first = NULL;
    AstStmt *last = NULL;
    
    while (current_token.type != TOKEN_EOF && current_token.type != TOKEN_DEDENT){

        AstStmt *stmt = NULL;

        switch (current_token.type)
        {
        case TOKEN_KW_VAL:
        case TOKEN_KW_VAR:
            stmt = parse_var_declaration();
            break;


        case TOKEN_IDENTIFIER:{
            
            stmt = parse_identifier_statement();
            break;
        }
            
        case TOKEN_INTEGER:

        case TOKEN_DOUBLE:

        case TOKEN_STRING:

        case TOKEN_INTERP_STRING:

        case TOKEN_LPAREN:

        case TOKEN_MINUS:
            stmt = parse_expression_statement();
            break;

        case TOKEN_KW_IF:
            stmt = parse_if();
            break;
            
        case TOKEN_KW_WHILE:
            stmt = parse_while();
            break;

        case TOKEN_KW_RETURN:
            stmt = parse_return();
            break;

        case TOKEN_EOL:
            fetch_token();
            break;

        default:
            syntax_error();
            break;
        }
        // Pripojenie spracovaneho prikazu na koniec zretazeneho zoznamu prikazov bloku
        if(stmt != NULL){
            if(first == NULL){
                first = stmt;
            }
            else{
                last->next = stmt;
            }
            last = stmt;
        }
    }
    if(first == NULL){
        syntax_error();
    }
    return first;
}

// Spracuje zoznam parametrov v hlavicke funkcie a ulozi ich do pola current_params
static void parse_parameter_list(int is_main){
    
    expect(TOKEN_LPAREN);

    current_param_count = 0;

    while(current_token.type != TOKEN_RPAREN && current_token.type != TOKEN_EOF){
        if(current_param_count >= MAX_PARAMS){
            fprintf(stderr, "semantic error: too many parameters\n");
            exit(6);
        }

        if(current_token.type != TOKEN_IDENTIFIER){
            syntax_error();
        }

        current_params[current_param_count].name = copy_string(current_token.attribute);
        fetch_token();

        expect (TOKEN_COLON);

        current_params[current_param_count].type = parse_type();

        current_param_count++;

        if(current_token.type == TOKEN_COMMA){
            fetch_token();

            if(current_token.type == TOKEN_RPAREN && !current_token.had_newline_before){
                syntax_error();
            }
        }
        else if(current_token.type != TOKEN_RPAREN){
            syntax_error();
        }
    }

    expect(TOKEN_RPAREN);
    

    if(is_main && current_param_count > 0){
        fprintf(stderr, "semtantic error: main function can not have parameters\n");
        exit(5);
    }
}

// Spracuje hlavicku funkcie (volitelna anotacia @main, def, meno, parametre a volitelny navratovy typ)
static void parse_function_header(char **func_name, Datatype *return_type, bool *has_ret_type, int *is_main){
    *is_main = 0;

    if(current_token.type == TOKEN_AT){
        fetch_token();
        if(scanner_space_before_token()){
            syntax_error();   // funkcia zo scanner.h: medzi @ a main nesmie byt medzera ani komentar
        }

        if(current_token.type == TOKEN_IDENTIFIER && strcmp(current_token.attribute, "main") == 0){
            fetch_token();
            *is_main = 1;
        }
        else{
            syntax_error();
        }

        if(current_token.type == TOKEN_EOL){
            fetch_token();
        }
    }

    expect(TOKEN_KW_DEF);

    if(current_token.type != TOKEN_IDENTIFIER){
        syntax_error();
    }

    *func_name = copy_string(current_token.attribute);
    fetch_token();

    parse_parameter_list(*is_main);

    *return_type = TYPE_ANY;
    *has_ret_type = false;

    if(current_token.type == TOKEN_COLON){
        fetch_token();
        *return_type = parse_type();
        *has_ret_type = true;
    }
}

// Preskoci tokeny tela funkcie (pouziva sa v 1. prechode a pri hladani funkcie v subore)
static void skip_function_body(void){

    expect(TOKEN_ASSIGN);

    if(current_token.type == TOKEN_EOL){
        fetch_token();

        if(current_token.type != TOKEN_INDENT){
            syntax_error();
        }

        // Pocitame hlbku zanorenia blokov (INDENT/DEDENT), kym nepreskocime cele telo funkcie
        int depth = 0;

        do{
            if(current_token.type == TOKEN_INDENT){
                depth++;
            }
            else if(current_token.type == TOKEN_DEDENT){
                depth--;
            }
            else if(current_token.type == TOKEN_EOF){
                syntax_error();
            }
            fetch_token();

        }while(depth > 0);
    }
    else{
        // Jednoriadkove telo funkcie: preskocime vsetky tokeny az po koniec riadku
        while(current_token.type != TOKEN_EOL && current_token.type != TOKEN_EOF){
            fetch_token();
        }
        expect(TOKEN_EOL);
    }
}

// Prvy prechod parsera: zozbiera hlavicky vsetkych funkcii a vlozi ich do tabulky symbolov
static void collect_function_headers(void){
    while (current_token.type != TOKEN_EOF){
        if(current_token.type == TOKEN_AT || current_token.type == TOKEN_KW_DEF){

            char *func_name = NULL;
            Datatype return_type = TYPE_ANY;
            bool has_ret_type = false;
            int is_main = 0;

            parse_function_header(&func_name, &return_type, &has_ret_type, &is_main);

            // Program smie obsahovat najviac jednu funkciu @main, navratovy typ musi byt Unit
            if(is_main){
                if(main_function_found){
                    fprintf(stderr, "semantic error: more than one @main function\n");
                    exit(4);
                }
                main_function_found = true;

                if(has_ret_type && return_type != TYPE_UNIT){
                    fprintf(stderr, "semantic error: main function must return Unit\n");
                    exit(6);
                }
            }

            Datatype *param_types = NULL;

            if(current_param_count > 0){
                param_types = malloc(sizeof(Datatype) * current_param_count);
                if(param_types == NULL){
                    exit(99);
                }
                for(int i = 0; i < current_param_count; i++){
                    param_types[i] = current_params[i].type;
                }
            }

            if(!insert_func(current_symtable, func_name, return_type, current_param_count, param_types)){
                fprintf(stderr, "semantic error: function is '%s' is already defined\n", func_name);
                exit(4);
            }

            for(int i = 0; i < current_param_count; i++){
                free(current_params[i].name);
            }

            register_function(func_name);
            free(func_name);

            skip_function_body();
        }
        else if(current_token.type == TOKEN_EOL){
            fetch_token();
        }
        else{
            syntax_error();
        }
    }
}

// Nastavi poziciu v subore na zaciatok idx-tej funkcie (pre 2. prechod parsera)
static void seek_function(int idx){

    rewind_input();
    scanner_init();

    fetch_token();
    parse_prolog();

    int k = 0;

    while(current_token.type != TOKEN_EOF){
        if(current_token.type == TOKEN_AT || current_token.type == TOKEN_KW_DEF){
            if(k == idx){
                return;
            }

            char *func_name = NULL;
            Datatype return_type = TYPE_ANY;
            bool has_ret_type = false;
            int is_main = 0;
            
            parse_function_header(&func_name, &return_type, &has_ret_type, &is_main);

            for(int i = 0; i < current_param_count; i++){
                free(current_params[i].name);
            }
            free(func_name);

            skip_function_body();
            k++;
        }
        else if(current_token.type == TOKEN_EOL){
            fetch_token();
        }
        else{
            syntax_error();
        }
    }
}

// Zisti vysledny typ bloku prikazov podla posledneho prikazu (0 = hodnota v *type, 1 = return, 2 = neplatna kombinacia)
static int block_result(AstStmt *first, Datatype *type){

    AstStmt *last = first;

    while(last != NULL&& last->next != NULL){
        last = last->next;
    }

    if(last == NULL){
        *type = TYPE_UNIT;
        return 0;
    }

    switch (last->kind)
    {
    case AST_EXPR:
        *type = last->datatype;
        return 0;

    case AST_RETURN:
        return 1;

    case AST_IF:{
        Datatype type_then = TYPE_UNIT;
        Datatype type_else = TYPE_UNIT;

        int res_then = block_result(last->then_block, &type_then);
        int res_else = block_result(last->else_block, & type_else);


        if(res_then == 1 && res_else == 1){
            return 1;
        }

        // Ak obe vetvy vracaju alebo maju vysledok typu Unit, vysledkom celeho if-else je Unit
        if((res_then==1 || (res_then == 0 && type_then == TYPE_UNIT))&& (res_else ==1 || (res_else == 0 && type_else == TYPE_UNIT))){
            *type = TYPE_UNIT;
            return 0;
        }
        return 2;
    }
    default:
        *type = TYPE_UNIT;
        return 0;
    }
}

// Spracuje celu definiciu funkcie a vlozi ju do AST
static void parse_function(void){

    char *func_name = NULL;
    Datatype return_type = TYPE_ANY;
    bool has_ret_type = false;
    int is_main = 0;

    parse_function_header(&func_name, &return_type, &has_ret_type, &is_main);

    current_func_ret_type = return_type;
    current_func_explicit = has_ret_type;
    if(parsing_idx >= 0){
        funcs[parsing_idx].inferred = !has_ret_type;
    }

    AstFunc *func = calloc(1, sizeof(AstFunc));
    if(func == NULL){
        exit(99);
    }
    func->name = func_name;
    func->is_main = is_main;
    func->param_count = current_param_count;

    if(current_param_count > 0){
        func->param_names = malloc(sizeof(char *) * current_param_count);
        func->param_types = malloc(sizeof(Datatype) * current_param_count);
        func->param_ids = malloc(sizeof(int) * current_param_count);
        if(func->param_names == NULL || func->param_types == NULL || func->param_ids == NULL){
            exit(99);
        }

    }

    current_scope_level++;

    // Vlozenie parametrov do symtable (insert_var) a nastavenie aby sa nemenili
    for(int  i = 0; i < current_param_count; i++){
        if(!insert_var(current_symtable, current_params[i].name, current_params[i].type, current_scope_level)){
            fprintf(stderr, "semantic error: parameter '%s' redefinition\n", current_params[i].name);
            exit(4);
        }

        func->param_names[i] = current_params[i].name;
        func->param_types[i] = current_params[i].type;
        func->param_ids[i] = register_var(current_params[i].name, current_scope_level, false);
    
    }
    expect(TOKEN_ASSIGN);

    AstStmt *body = NULL;

    // Telo funkcie moze byt viacriadkovy odsadeny blok (po EOL a INDENT) alebo jednoriadkovy vyraz za '='
    if(current_token.type == TOKEN_EOL){
        fetch_token();

        if(current_token.type != TOKEN_INDENT){
            syntax_error();
        }
        fetch_token();

        body = parser_statements();

        leave_scope();

        expect(TOKEN_DEDENT);
    }
    else{
        body = new_stmt(AST_EXPR);
        parse_value(&body->expr, &body->call, &body->datatype);
        check_not_bool(body->datatype);
        expect(TOKEN_EOL);

        leave_scope();
    }

    // Kontrola navratovej hodnoty tela funkcie (alebo odvodenie navratoveho typu)
    Datatype body_type = TYPE_UNIT;
    int body_kind = block_result(body, &body_type);

    if(has_ret_type){
        if(return_type != TYPE_UNIT){
            if(body_kind == 2 || (body_kind == 0 && !is_compatible(return_type, body_type))){
                fprintf(stderr, "semantic error: body of function '%s' does not match its return type\n", func_name);
                exit(6);
            }
        }
    }
    else{
        // Odvodenie navratoveho typu funkcie z posledneho vyrazu v tele
        if(body_kind != 0){
            fprintf(stderr, "semantic error: can not return type of function '%s'\n", func_name);
            exit(6);
        }
        check_not_bool(body_type);
        return_type = body_type;
    }

    if(is_main && return_type != TYPE_UNIT){
        fprintf(stderr, "semantic error: main function must return UNIT\n");
        exit(6);
    }

    // Zapis konecneho (aj odvodeneho) navratoveho typu funkcie do tabulky symbolov
    Symboldata *func_data = lookup_func(current_symtable, func_name);
    if(func_data != NULL){
        func_data->typ = return_type;
    }

    func->return_type = return_type;
    func->body = body;

    if(ast_program->first_func == NULL){
        ast_program->first_func = func;
    }
    else{
        ast_program->last_func->next = func;
    }
    ast_program->last_func = func;
   
}

// Riadi parsovanie idx-tej funkcie s podporou odkladu cez setjmp/longjmp pri neznamom navratovom type volanej funkcie
static void process_function(int idx){

    // state: 0 = nespracovana, 1 = prave sa spracovava, 2 = hotova
    if(funcs[idx].state == 2){
        return;
    }
    funcs[idx].state = 1;

    while(1){
        if(setjmp(parse_jmp) == 0){
            seek_function(idx);
            parsing_idx = idx;
            parse_function();
            break;
        }

        
        while(current_scope_level > 0){
            leave_scope();
        }
        process_function(blocked_on);
    }
    funcs[idx].state = 2;
    parsing_idx = -1;
}

// Hlavna funkcia parsera: inicializuje tabulky, spusti 1. aj 2. prechod a cez DFS nad call_edges (limit 4096) overi nepritomnost rekurzie bez typu
int parser_program(void){

    current_symtable = init_hash_table();
    if (current_symtable == NULL){
        exit(99);
    }

    ast_program = calloc(1, sizeof(AstProgram));
    if(ast_program == NULL){
        exit(99);
    }
    
    // Vlozenie vstavanych funkcii 
    insert_included_fun(current_symtable);

    prepare_input();
    fetch_token();  
    parse_prolog();

    // 1. prechod: zozbieranie hlaviciek vsetkych funkcii (umoznuje dopredne volania)
    collect_function_headers();


    if(!main_function_found){
        fprintf(stderr, "semantic error: program is missing @main function\n");
        exit(3);
    }

    // 2. prechod: parsovanie tiel funkcii a budovanie AST
    for(int i = 0; i < func_count; i++){
        process_function(i);
    }

    // DFS hladanie cyklu v grafe volani: funkcia s odvodenym navratovym typom nesmie byt sucastou rekurzie
    for(int i = 0; i < func_count; i++){
        if(!funcs[i].inferred){
            continue;
        }
        bool seen[4096] = {false};
        int stack_[4096];
        int sp = 0;
        stack_[sp++] = i;
        while(sp > 0){
            int cur = stack_[--sp];
            for(int e = 0; e < call_edge_count; e++){
                if(call_edges[e][0] != cur){
                    continue;
                }
                int to = call_edges[e][1];
                if(to == i){
                    fprintf(stderr, "semantic error: recursive function '%s' needs explicit return type\n", funcs[i].name);
                    exit(6);
                }
                if(to < 4096 && !seen[to] && sp < 4096){
                    seen[to] = true;
                    stack_[sp++] = to;
                }
            }
        }
    }
    
    return 0;
}

// Vrati ukazovatel na vybudovany AST strom celeho programu
AstProgram *parser_get_program(void){
    return ast_program;
}

// Rekurzivne uvolni zretazeny zoznam prikazov AstStmt vratane vyrazov, volani a podblokov
static void free_stmt_list(AstStmt *s) {
    while (s != NULL) {
        AstStmt *next = s->next;
        if (s->name != NULL) free(s->name);
        if (s->expr != NULL) expr_free(s->expr);
        if (s->call != NULL) {
            if (s->call->name != NULL) free(s->call->name);
            if (s->call->args != NULL) {
                for (int i = 0; i < s->call->arg_count; i++) {
                    if (s->call->args[i] != NULL) expr_free(s->call->args[i]);
                }
                free(s->call->args);
            }
            free(s->call);
        }
        if (s->then_block != NULL) free_stmt_list(s->then_block);
        if (s->else_block != NULL) free_stmt_list(s->else_block);
        free(s);
        s = next;
    }
}

// Uvolni celu dynamicku pamat alokovanu parserom (AST strom, tabulka symbolov, pomocne polia)
void parser_cleanup(void) {
    // 1. Uvolnenie AST stromu celého programu (funkcie, parametre, tela)
    if (ast_program != NULL) {
        AstFunc *f = ast_program->first_func;
        while (f != NULL) {
            AstFunc *next_f = f->next;
            if (f->name != NULL) free(f->name);
            if (f->param_names != NULL) {
                for (int i = 0; i < f->param_count; i++) {
                    if (f->param_names[i] != NULL) free(f->param_names[i]);
                }
                free(f->param_names);
            }
            if (f->param_types != NULL) free(f->param_types);
            if (f->param_ids != NULL) free(f->param_ids);
            if (f->body != NULL) free_stmt_list(f->body);
            free(f);
            f = next_f;
        }
        free(ast_program);
        ast_program = NULL;
    }
    // 2. Uvolnenie tabulky symbolov
    if (current_symtable != NULL) {
        delete_hash_tabulka(current_symtable);
        current_symtable = NULL;
    }
    // 3. Uvolnenie interných poli parsera (funkcie a premenné)
    if (funcs != NULL) {
        for (int i = 0; i < func_count; i++) {
            if (funcs[i].name != NULL) free(funcs[i].name);
        }
        free(funcs);
        funcs = NULL;
        func_count = 0;
        func_cap = 0;
    }
    if (vars != NULL) {
        for (int i = 0; i < var_count; i++) {
            if (vars[i].name != NULL) free(vars[i].name);
        }
        free(vars);
        vars = NULL;
        var_count = 0;
        var_cap = 0;
    }
}

