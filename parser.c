#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "scanner.h"        //cooperation with scanner
#include "stack.h"
#include "hashtable.h"
#include "symtable.h"
#include "expressions.h"

#define MAX_PARAMS 32


typedef struct parser
{
    char *name;
    Datatype type;

}ParamInfo;

//current token 
static Token current_token; 

static symtable *current_symtable = NULL;
static int current_scope_level = 0;

static ParamInfo current_params[MAX_PARAMS];
static int current_param_count = 0;

static bool main_function_found = false;

static Datatype current_func_ret_type = TYPE_UNIT;

//deklaration of funktions
static void fetch_token(void);
static void expect(TokenType expect_type);
static void syntax_error(void);
static void parse_prolog(void);
static void parse_function(void);
static void parser_statements(void);
static void parser_assignment(char *var_name);
static void parse_var_declaration(void);
static void parse_parameter_list(int is_main);
static void parse_if(void);
static void parse_while(void);
static void parse_return(void);
static void parse_function_call(char *func_name);



//nacita dalsi token zo scanner
static void fetch_token(void){  

    current_token = get_next_token();

    printf("DEBUG: načítaný token -> typ: %d, hodnota/atribut: %s\n", 
           current_token.type, 
           current_token.attribute[0] != '\0' ? current_token.attribute : "žiadna");


}

static void syntax_error(void){
    fprintf(stderr, "Syntax errror in code\n");
    
    exit(2);
}


//type control pre tokeny
//posunie token dalej alebo vyhodi syntax error
static void expect(TokenType expected_type){  
    if (current_token.type == expected_type || (expected_type == TOKEN_EOL && current_token.type == 3)){
        fetch_token();
    }
    else{
        syntax_error();
    }
}


static void parse_prolog(void){
    expect(TOKEN_KW_IMPORT);   
    expect(TOKEN_IDENTIFIER);

    while(current_token.type != TOKEN_EOL && current_token.type != TOKEN_EOF){
        fetch_token();
    }

    expect(TOKEN_EOL);

}



//kotrola pre prikaz: priradenie hodnoty do premennej (zodpoveda kod pravidlam IFJ26?)
static void parser_assignment(char *var_name){

    Symboldata *var_data = NULL;    

    for(int level = current_scope_level; level >= 0; level--){
        char lookup_key[256];
        snprintf(lookup_key, sizeof(lookup_key), "%d$%s", level, var_name);

        var_data = lookup_var(level, lookup_key, current_symtable);
        if(var_data != NULL){
            break;
        }
    }
    
    if(var_data == NULL){
        fprintf(stderr, "semantic error: variable '%s' was not declared\n", var_name);
        exit(4);
    }

    expect(TOKEN_ASSIGN);       

    ExprNode *expr_root = NULL;
    Datatype expr_type = TYPE_ANY;

    int epx_err = parse_expression(&current_token, &expr_root, &expr_type, current_symtable, current_scope_level);
    if(epx_err != 0){
        exit(epx_err);
    }

    if(var_data->typ != TYPE_ANY && expr_type != TYPE_ANY && var_data->typ != expr_type){
        fprintf(stderr, "semantic error: type mismatch in assignment to variable '%s' (variable: %d, expression %d)\n", var_name, var_data->typ, expr_type);
        exit(6);
    }
    
    expect(TOKEN_EOL);        
}



// deklaration of var/val
static void parse_var_declaration(void){

    if(current_token.type == TOKEN_KW_VAL || current_token.type == TOKEN_KW_VAR){
        fetch_token();
    }
    else{
        syntax_error();
    }

    if(current_token.type != TOKEN_IDENTIFIER){
        syntax_error();
    }

    char *var_name = malloc(strlen(current_token.attribute)+1);
    if(var_name == NULL){
        exit(99);
    }
    strcpy(var_name, current_token.attribute);

    fetch_token();

    Datatype var_type = TYPE_ANY;


    if(current_token.type == TOKEN_COLON){
        fetch_token();
        if(current_token.type == TOKEN_KW_INT){
            var_type = TYPE_INT;
            fetch_token();
        }
        else if(current_token.type == TOKEN_KW_DOUBLE_TYPE){
            var_type = TYPE_DOUBLE;
            fetch_token();
        }
        else if(current_token.type == TOKEN_KW_STRING_TYPE){
            var_type = TYPE_STRING;
            fetch_token();
        }
        else{
            syntax_error();
        }
    }
    expect(TOKEN_ASSIGN);
    ExprNode *expr_root = NULL;
    Datatype expr_type = TYPE_ANY;
    int expr_err = parse_expression(&current_token, &expr_root, &expr_type, current_symtable, current_scope_level);
    if(expr_err != 0){
        exit(expr_err);
    }

    expect(TOKEN_EOL);

    if(var_type != TYPE_ANY && expr_type != TYPE_ANY && var_type != expr_type){
        fprintf(stderr, "semantic error: type mismatch in var declaration (var.: %d, expr.: %d)", var_type, expr_type);
        exit(6);
    }

    Datatype final_type = (var_type == TYPE_ANY) ? expr_type : var_type;
    
    if(!insert_var(current_symtable, var_name, final_type, current_scope_level)){
        fprintf(stderr, "semantic error: variable '%s' already declared in this scope\n", var_name);
        free(var_name);
        exit(3);
    }

    free(var_name);
}


static void parse_if(void){
    expect(TOKEN_KW_IF);

    
    ExprNode *expr_root = NULL;
    Datatype expr_type = TYPE_ANY;
    int expr_err = parse_expression(&current_token, &expr_root, &expr_type, current_symtable, current_scope_level);
    if(expr_err != 0){
        exit(expr_err);
    }


    if(current_token.type == TOKEN_INDENT){
        fetch_token();
        current_scope_level++;
        parser_statements();
        exit_scope(current_scope_level, current_symtable);
        current_scope_level--;
        expect(TOKEN_DEDENT);
    }
    else{
        parser_statements();
    }

    if(current_token.type == TOKEN_KW_ELSE){
        fetch_token();

        if(current_token.type == TOKEN_INDENT){
            fetch_token();
            current_scope_level++;
            parser_statements();
            exit_scope(current_scope_level, current_symtable);
            current_scope_level--;
            expect(TOKEN_DEDENT);
        }
        else{
            syntax_error();
        }
    }
    else {
        syntax_error();
    }
}


static void parse_while(void){
    printf("DEBUG: vstupujem do parse_while, aktuálny token: %d\n", current_token.type);
    expect(TOKEN_KW_WHILE);

    ExprNode *expr_root = NULL;
    Datatype expr_type = TYPE_ANY;
    int expr_err = parse_expression(&current_token, &expr_root, &expr_type, current_symtable, current_scope_level);
    if(expr_err != 0){
        exit(expr_err);
    }

    if(current_token.type == TOKEN_INDENT){
        fetch_token();
        current_scope_level++;
        parser_statements();
        exit_scope(current_scope_level, current_symtable);
        current_scope_level--;
        expect(TOKEN_DEDENT);
    }
    else{
        syntax_error();
    }
}


static void parse_return(void){
    expect(TOKEN_KW_RETURN);

    ExprNode *expr_root = NULL;
    Datatype expr_type = TYPE_UNIT;
    

    if(current_token.type != TOKEN_EOL && current_token.type != 3 && current_token.type != TOKEN_RBRACKET && current_token.type != TOKEN_EOF){
        int expr_err = parse_expression(&current_token, &expr_root, &expr_type, current_symtable, current_scope_level);
        if(expr_err != 0 ){
            exit(expr_err);
        }
    }

    if(current_func_ret_type != TYPE_ANY && expr_type != TYPE_ANY && current_func_ret_type != expr_type){
        fprintf(stderr, "DEBUG 6A: return type mismatch\n");
        fprintf(stderr, "semantic error: return type mismatch\n");
        exit(6);
    }

    expect(TOKEN_EOL);
}




static void parser_statements(void){

    printf("DEBUG: vstupujem do parser_statements\n");
    
    while (current_token.type != TOKEN_EOF && current_token.type != TOKEN_KW_DEF && current_token.type != TOKEN_AT && current_token.type != TOKEN_DEDENT)
    {
        switch (current_token.type)
        {
        case TOKEN_KW_VAL:
        case TOKEN_KW_VAR:
            parse_var_declaration();
            break;


        case TOKEN_IDENTIFIER:{
            char *id_name = malloc(strlen(current_token.attribute)+1);
            if(id_name == NULL){
                exit(99);
            }
            strcpy(id_name, current_token.attribute);

            fetch_token();

            if(current_token.type == TOKEN_LPAREN){
                parse_function_call(id_name);
                expect(TOKEN_EOL);
            }
            else if (current_token.type == TOKEN_ASSIGN){
                parser_assignment(id_name);
            }
            else{
                syntax_error();
            }
            free(id_name);
            break;
        }
            

        case TOKEN_KW_IF:
            parse_if();
            break;
            
        case TOKEN_KW_WHILE:
            parse_while();
            break;

        case TOKEN_KW_RETURN:
            parse_return();
            break;

        case TOKEN_EOL:
            fetch_token();
            break;

        case TOKEN_INDENT:
            fetch_token();
            break;

            case TOKEN_DEDENT:
            break;
            

        default:
            syntax_error();
            break;
        }
    }
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

        current_params[current_param_count].name = malloc(strlen(current_token.attribute)+1);
        if(current_params[current_param_count].name == NULL){
            exit(99);
        }
        strcpy(current_params[current_param_count].name, current_token.attribute);
        fetch_token();

        expect (TOKEN_COLON);

        if(current_token.type == TOKEN_KW_INT){
            current_params[current_param_count].type = TYPE_INT;
            fetch_token();
        }
        else if(current_token.type == TOKEN_KW_DOUBLE_TYPE){
            current_params[current_param_count].type = TYPE_DOUBLE;
            fetch_token();
        }
        else if(current_token.type == TOKEN_KW_STRING_TYPE){
            current_params[current_param_count].type = TYPE_STRING;
            fetch_token();
        }
        else{
            syntax_error();
        }

        current_param_count++;

        if(current_token.type == TOKEN_COMMA){
            fetch_token();
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



static Symboldata *lookup_func(symtable *t, char *func_name){

    return lookup_var(0, func_name, t);
}



static void parse_function_call(char *func_name){
    
    expect(TOKEN_LPAREN);

    Symboldata *func_data = lookup_func(current_symtable, func_name);

    if(func_data == NULL || !func_data->is_function){
        fprintf(stderr, "sematic error: function '%s' was not declared\n", func_name);
        exit(3);
    }

    int arg_count = 0;

    while(current_token.type != TOKEN_RPAREN && current_token.type != TOKEN_EOF){
        ExprNode *expr_root = NULL;
        Datatype arg_type = TYPE_ANY;
        int expr_err = parse_expression(&current_token, &expr_root, &arg_type, current_symtable, current_scope_level);
        if(expr_err != 0){
        exit(expr_err);
        }

        if(arg_count < func_data->param_count){
            if(func_data->param_type[arg_count] != TYPE_ANY && arg_type != TYPE_ANY && func_data->param_type[arg_count] != arg_type){
                fprintf(stderr, "semantic error: type mismatch in argument %d of function '%s'\n", arg_count+1, func_name);
                exit(6);
            }
        }

        arg_count++;

        if (current_token.type == TOKEN_COMMA){
            fetch_token();
        }
        else if (current_token.type != TOKEN_RPAREN){
            syntax_error();
        }
    }

    expect (TOKEN_RPAREN);

    if(arg_count != func_data->param_count){
        fprintf(stderr, "DEBUG 6B: arg count mismatch\n");
        fprintf(stderr, "semantic error: function '%s' expects %d arguments, but only %d were provided\n", func_name, func_data->param_count, arg_count);
        exit(6);
    }
}




static void parse_function(void){

    int is_main = 0;

    if(current_token.type == TOKEN_AT){
        fetch_token();
        
        if(strcmp(current_token.attribute, "main") == 0){
            fetch_token();
            is_main = 1;
            main_function_found = true;
        }
        else{
            syntax_error();
        }
    }

    expect(TOKEN_KW_DEF);

    if(current_token.type != TOKEN_IDENTIFIER){
        syntax_error();
    }

    char *func_name = malloc(strlen(current_token.attribute)+1);
    if(func_name == NULL){
        exit(99);
    }
    strcpy(func_name, current_token.attribute);
    fetch_token();
    
    parse_parameter_list(is_main);

    Datatype return_type = TYPE_UNIT;

    if(current_token.type == TOKEN_COLON){
        fetch_token();
        
        if(current_token.type == TOKEN_KW_INT){
            return_type = TYPE_INT;
            fetch_token();
        }
        else if(current_token.type == TOKEN_KW_DOUBLE_TYPE){
            return_type = TYPE_DOUBLE;
            fetch_token();
        }
        else if(current_token.type == TOKEN_KW_STRING_TYPE){
            return_type = TYPE_STRING;
            fetch_token();
        }
        else if(current_token.type == TOKEN_KW_UNIT || (current_token.type == 5 && strcmp(current_token.attribute, "unit") == 0)){
            return_type = TYPE_UNIT;
            fetch_token();
        }
        else{
            syntax_error();
        }
    }

    current_func_ret_type = return_type; // ulozenie navratoveho tyou pre kontrolu v return

    Datatype *param_types = NULL;

    if(current_param_count > 0){
        param_types = malloc(sizeof(Datatype) * current_param_count);
        for(int i = 0; i < current_param_count; i++){
            param_types[i] = current_params[i].type;
        }
    }

    fprintf(stderr, "DEBUG PARSER: Pripravujem sa vložit funkciu '%s' do symtable\n", func_name);

    if(!insert_func(current_symtable, func_name, return_type, current_param_count, param_types)){
        fprintf(stderr, "semantic error: function '%s' already defined\n", func_name);
        exit(3);
    }
    
    if(param_types != NULL){
        free(param_types);
    }

    current_scope_level++;

    for(int i = 0; i < current_param_count; i++){
        if(!insert_var(current_symtable, current_params[i].name,current_params[i].type, current_scope_level)){
            fprintf(stderr, "semantic error: parameter '%s' redefinition\n", current_params[i].name);
            exit(3);
        }
        free(current_params[i].name);
    }
    expect(TOKEN_ASSIGN);

    if(current_token.type == TOKEN_INDENT){
        fetch_token();

        parser_statements();

         exit_scope(current_scope_level, current_symtable);
        current_scope_level--;

        expect(TOKEN_DEDENT);
    }
    else{
        syntax_error();
    }

   
}


int parser_program(void){

    current_symtable = init_hash_table();
    if (current_symtable == NULL){
        exit(99);
    }
    
    insert_included_fun(current_symtable);

    fetch_token();  
    parse_prolog();
    
    
    while (current_token.type != TOKEN_EOF)
    {
        
        if(current_token.type == TOKEN_AT || current_token.type == TOKEN_KW_DEF){
            parse_function();
        } 
        else if(current_token.type == TOKEN_EOL){
            fetch_token();
        }
        else{
            fprintf(stderr, "DEBUG ERROR: Nečakaný token -> typ: %d, hodnota/atribut: %s\n", 
            current_token.type, 
            current_token.attribute[0] != '\0' ? current_token.attribute : "žiadna");
            syntax_error();
        }
    
    }

    if(!main_function_found){
        fprintf(stderr, "sematic error: program is missing @main function\n");
        exit(5);
    }
    
    return 0;
}



