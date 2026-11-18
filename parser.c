/*
 * Projekt: Implementacia prekladaca imperativneho jazyka IFJ26
 * Subor: parser.c
 * Autor: Paulína Košťálová (xkostap00)
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <unistd.h>

#include "parser.h"
#include "scanner.h"        
#include "stack.h"
#include "symtable.h"
#include "expressions.h"

#define MAX_PARAMS 32



typedef struct parser
{
    char *name;
    Datatype type;
}ParamInfo;

typedef struct 
{
    char  *name;
    int state;
}FuncEntry;


typedef struct
{
    char *name;
    int level;
    bool is_mutable;
    int unique_id;
}VarInfo;


ExprNode* create_leaf(NodeType nodetype, Datatype datatype, const char *value);

//current token 
static Token current_token; 

static symtable *current_symtable = NULL;
static int current_scope_level = 0;

static ParamInfo current_params[MAX_PARAMS];
static int current_param_count = 0;

static bool main_function_found = false;

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

static jmp_buf parse_jmp;
static int blocked_on = -1;

//deklaration of funktions
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




static void fetch_token(void){  

    current_token = get_next_token();

    if(current_token.type == TOKEN_ERROR){
        fprintf(stderr, "|Lexical error in code\n");
        exit(1);
    }

}

static void syntax_error(void){
    fprintf(stderr, "Syntax errror in code\n");
    
    exit(2);
}



static void expect(TokenType expected_type){  
    if (current_token.type == expected_type){
        fetch_token();
    }
    else{
        syntax_error();
    }
}


static char *copy_string(const char *src){
    char *dst = malloc(strlen(src)+1);
    if(dst == NULL){
        exit(99);
    }
    strcpy(dst, src);
    return dst;
}


static AstStmt *new_stmt(AstStmtType kind){
    AstStmt *stmt = calloc(1, sizeof(AstStmt));
    if(stmt == NULL){
        exit(99);
    }
    stmt->kind = kind;
    return stmt;
}


static bool is_compatible(Datatype target, Datatype src){
    return target == TYPE_ANY || target == src || (target == TYPE_DOUBLE && src == TYPE_INT);
}

static void check_not_bool(Datatype type){
    if(type == TYPE_BOOL){
        fprintf(stderr, "semantic eror: boolean value can not be stored, passed or returned\n");
        exit(6);
    }
}

static Symboldata *lookup_func(symtable *t, char *func_name){
    return lookup_var(0, func_name, t);
}

static Symboldata *find_var(symtable *t, int scope_level, char *name){
    Symboldata *data = lookup_var(scope_level, name, t);
    if(data != NULL && data->is_function){
        return NULL;
    }
    return data;
}

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

static VarInfo *find_var_info(const char *name){
    for(int i = var_count -1; i >= 0; i--){
        if(strcmp(vars[i].name, name) == 0){
            return &vars[i];
        }
    }
    return NULL;
}

static void leave_scope(void){
    exit_scope(current_scope_level, current_symtable);
    while(var_count > 0 && vars[var_count -1].level == current_scope_level){
        free(vars[var_count -1].name);
        var_count--;
    }
    current_scope_level--;
}

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
    func_count++;
}

static int find_func_index(const char *name){
    for(int i = 0; i < func_count; i++){
        if(strcmp(funcs[i].name, name) == 0){
            return i;
        }
    }
    return -1;
}

static void rewind_input(void){
    fseek(stdin, 0, SEEK_SET);
    clearerr(stdin);
}

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

    if(dup2(fileno(tmp), STDIN_FILENO) < 0){
        exit(99);
    }
    rewind_input();

}


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
    else{
        syntax_error();
    }
    fetch_token();
    return type;
}

static void end_statement(void){
    if(current_token.type == TOKEN_SEMICOLON){
        fetch_token();
        return;
    }
    expect(TOKEN_EOL);
}

static ExprNode *parse_term(Datatype *type){
    ExprNode *node = NULL;
    char value[MAX_TOKEN_LEN + 2];
    value[0] = '\0';
    
    if(current_token.type == TOKEN_MINUS){
        fetch_token();
        if(current_token.type != TOKEN_INTEGER && current_token.type != TOKEN_DOUBLE){
            syntax_error();
        }
        strcpy(value, "-");
    }
    strcat(value, current_token.attribute);

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

    case TOKEN_IDENTIFIER:{
        Symboldata *var = find_var(current_symtable, current_scope_level, current_token.attribute);
        if(var == NULL){
            fprintf(stderr, "sematic error: variable '%s' was not declared\n", current_token.attribute);
            exit(3);
        }
        node = create_leaf(NODE_VAR, var->typ, value);
        *type = var->typ;
        break;}

    default:
        syntax_error();
        break;
    }

    if(node == NULL){
        exit(99);
    }
    fetch_token();
    return node;
}

static AstCall *parse_function_call(char *name){

    char *func_name = copy_string(name);

    expect(TOKEN_LPAREN);

    Symboldata *func_data = lookup_func(current_symtable, func_name);

    if(func_data == NULL || !func_data->is_function){
        fprintf(stderr, "semantic error: function '%s' was not declared\n", func_name);
        exit(3);
    }

    if(func_data->typ == TYPE_ANY){
        int idx  = find_func_index(func_name);
        if(idx < 0 || funcs[idx].state == 1){
            fprintf(stderr, "semantic error: recursive function '%s' nned explicit return type\n", func_name);
            exit(6);
        }
        blocked_on = idx;
        longjmp(parse_jmp, 1);
    }

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

    while (current_token.type != TOKEN_RPAREN && current_token.type != TOKEN_EOF){
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
        check_expr_vars(*expr);
        *type = expr_type;
    }
    else if(expr_err == ERROR_SYNT && start_type == TOKEN_IDENTIFIER && current_token.type == TOKEN_LPAREN &&
         lookup_func(current_symtable, start_name) != NULL && lookup_func(current_symtable, start_name)->is_function){
            *expr = NULL;
            *call = parse_function_call(start_name);
            *type = (*call)->ret_type;
    }

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
   
    if(var_info != NULL && !var_info->is_mutable){
        fprintf(stderr, "semantic error: assignment to immutable variable '%s'\n", var_name);
        exit(7);
    }

    check_not_bool(expr_type);

    if(!is_compatible(var_data->typ, expr_type)){
        fprintf(stderr, "semantic error: type mismatch in assignemnt to variable '%s' (variable: %d, expression %d)\n", var_name, var_data->typ, expr_type);
        exit(6);
    }

    stmt->name = var_name;
    stmt->datatype = var_data->typ;
    stmt->unique_id = (var_info != NULL) ? var_info->unique_id : 0;
    return stmt;

}


static AstStmt *parse_identifier_statement(void){

    char *id_name = copy_string(current_token.attribute);

    AstStmt *stmt = new_stmt(AST_EXPR);
    Datatype expr_type = TYPE_ANY;

    parse_value(&stmt->expr, &stmt->call, &expr_type);

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

static AstStmt *parse_if(void){

    int if_indent = current_token.indent;

    expect(TOKEN_KW_IF);

    
    AstStmt *stmt = new_stmt(AST_IF);
    Datatype expr_type = TYPE_ANY;
    
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
        fprintf(stderr, "sematic error: condition of while is not boolean\n");
        exit(6);
    }
    expect(TOKEN_KW_DO);

    stmt->then_block = parse_block();

    return stmt;
}


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
        if(!current_func_explicit || current_func_ret_type != TYPE_UNIT){
            fprintf(stderr, "semantic error: return without value is not allowed here\n");
            exit(10);
        }
    }
    else{
        if(!current_func_explicit){
            fprintf(stderr, "semantic error: function with return need explicit retrun type\n");
            exit(6);
        }
        check_not_bool(expr_type);
        if(current_func_explicit && current_func_ret_type == TYPE_UNIT){
            /* Spec 2.3 rule 4: Unit function discards return value – no type mismatch */
        }
        else if(!is_compatible(current_func_ret_type, expr_type)){
            fprintf(stderr, "semantic error: return type mismatch\n");
            exit(6);
        }
    }

    stmt->datatype =current_func_ret_type;
    return stmt;
}


static AstStmt *parse_expression_statement(void){
    AstStmt *stmt = new_stmt(AST_EXPR);
    Datatype expr_type = TYPE_ANY;

    parse_value(&stmt->expr, &stmt->call, &expr_type);
    check_not_bool(expr_type);
    stmt->datatype = expr_type;

    end_statement();
    return stmt;
}

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
        fprintf(stderr, "semtantic error: msin function can not have parameters\n");
        exit(5);
    }
}

static void parse_function_header(char **func_name, Datatype *return_type, bool *has_ret_type, int *is_main){
    *is_main = 0;

    if(current_token.type == TOKEN_AT){
        fetch_token();

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


static void skip_function_body(void){

    expect(TOKEN_ASSIGN);

    if(current_token.type == TOKEN_EOL){
        fetch_token();

        if(current_token.type != TOKEN_INDENT){
            syntax_error();
        }

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
        while(current_token.type != TOKEN_EOL && current_token.type != TOKEN_EOF){
            fetch_token();
        }
        expect(TOKEN_EOL);
    }
}


static void collect_function_headers(void){
    while (current_token.type != TOKEN_EOF){
        if(current_token.type == TOKEN_AT || current_token.type == TOKEN_KW_DEF){

            char *func_name = NULL;
            Datatype return_type = TYPE_ANY;
            bool has_ret_type = false;
            int is_main = 0;

            parse_function_header(&func_name, &return_type, &has_ret_type, &is_main);

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

static void parse_function(void){

    char *func_name = NULL;
    Datatype return_type = TYPE_ANY;
    bool has_ret_type = false;
    int is_main = 0;

    parse_function_header(&func_name, &return_type, &has_ret_type, &is_main);

    current_func_ret_type = return_type;
    current_func_explicit = has_ret_type;

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

    for(int  i = 0; i < current_param_count; i++){
        if(!insert_var(current_symtable, current_params[i].name, current_params[i].type, current_scope_level)){
            fprintf(stderr, "semantic error: parameter '%s' redefintion\n", current_params[i].name);
            exit(4);
        }

        func->param_names[i] = current_params[i].name;
        func->param_types[i] = current_params[i].type;
        func->param_ids[i] = register_var(current_params[i].name, current_scope_level, false);
    
    }
    expect(TOKEN_ASSIGN);

    AstStmt *body = NULL;

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
        if(body_kind != 0){
            fprintf(stderr, "semnatic error: can not ifer return type of function '%s'\n", func_name);
            exit(6);
        }
        check_not_bool(body_type);
        return_type = body_type;
    }

    if(is_main && return_type != TYPE_UNIT){
        fprintf(stderr, "semantic error: main function must retunrn UNIT\n");
        exit(6);
    }

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

static void process_function(int idx){

    if(funcs[idx].state == 2){
        return;
    }
    funcs[idx].state = 1;

    while(1){
        if(setjmp(parse_jmp) == 0){
            seek_function(idx);
            parse_function();
            break;
        }

        while(current_scope_level > 0){
            leave_scope();
        }
        process_function(blocked_on);
    }
    funcs[idx].state = 2;
}


int parser_program(void){

    current_symtable = init_hash_table();
    if (current_symtable == NULL){
        exit(99);
    }

    ast_program = calloc(1, sizeof(AstProgram));
    if(ast_program == NULL){
        exit(99);
    }
    
    insert_included_fun(current_symtable);

    prepare_input();
    fetch_token();  
    parse_prolog();

    collect_function_headers();


    if(!main_function_found){
        fprintf(stderr, "sematic error: program is missing @main function\n");
        exit(3);
    }

    for(int i = 0; i < func_count; i++){
        process_function(i);
    }
    
    return 0;
}

AstProgram *parser_get_program(void){
    return ast_program;
}



