#include <stdio.h>
#include <stdlib.h>
#include "string.h"

#include "parser.h"
#include "scanner.h"        //cooperation with scanner
#include "stack.h"
#include "hashtable.h"
#include "symtable.h"

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

//deklaration of funktions
static void fetch_token(void);
static void expect(TokenType expect_type);
static void syntax_error(void);
static void parse_prolog(void);
static void parse_function(void);
static void parser_statements(void);
static void parser_assignment(void);
static void parse_var_declaration(void);
static void parse_parameter_list(int is_main);
static void parse_if(void);
static void parse_while(void);
static void parse_return(void);




//nacita dalsi token zo scanner
static void fetch_token(void){  

    current_token = get_next_token();

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
static void parser_assignment(void){

    if(current_token.type != TOKEN_IDENTIFIER){
        syntax_error();
    }

    char *var_name = current_token.attribute;
    fetch_token();


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

    expect(TOKEN_ASSIGN);       //nasleduje operator priradenia?

    parse_expression();
    
    expect(TOKEN_EOL);        //kocni prikaz podkociarkou?
    
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

    char *var_name = current_token.attribute;
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
    parse_expression();
    expect(TOKEN_EOL);

    
    if(!insert_var(current_symtable, var_name, var_type, current_scope_level)){
        fprintf(stderr, "semantic error: variable '%s' already declared in this scope\n", var_name);
        exit(3);
    }
}


static void parse_if(void){
    expect(TOKEN_KW_IF);

    parse_expression();

    if(current_token.type == TOKEN_LBRACKET){
        fetch_token();
        current_scope_level++;
        parser_statements();
        exit_scope(current_scope_level, current_symtable);
        current_scope_level--;
        expect(TOKEN_RBRACKET);
    }
    else{
        parser_statements();
    }

    if(current_token.type == TOKEN_KW_ELSE){
        fetch_token();

        if(current_token.type == TOKEN_LBRACKET){
            fetch_token();
            current_scope_level++;
            parser_statements();
            exit_scope(current_scope_level, current_symtable);
            current_scope_level--;
            expect(TOKEN_RBRACKET);
        }
        else{
            parser_statements();
        }
    }
}


static void parse_while(void){
    expect(TOKEN_KW_WHILE);

    parse_expression();

    if(current_token.type == TOKEN_LBRACKET){
        fetch_token();
        current_scope_level++;
        parser_statements();
        exit_scope(current_scope_level, current_symtable);
        current_scope_level--;
        expect(TOKEN_RBRACKET);
    }
    else{
        parser_statements();
    }
}


static void parse_return(void){
    expect(TOKEN_KW_RETURN);

    if(current_token.type != TOKEN_EOL && current_token.type != 3 && current_token.type != TOKEN_RBRACKET && current_token.type != TOKEN_EOF){
        parse_expression();
    }

    expect(TOKEN_EOL);
}


static void parser_statements(void){
    
    while (current_token.type != TOKEN_EOF && current_token.type != TOKEN_KW_DEF && current_token.type != TOKEN_AT)
    {
        switch (current_token.type)
        {
        case TOKEN_KW_VAL:
        case TOKEN_KW_VAR:
            parse_var_declaration();
            break;

        case TOKEN_IDENTIFIER:
            parser_assignment();
            break;

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

        case 2:
            fetch_token();
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

        current_params[current_param_count].name = current_token.attribute;
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
            syntax_error();
        }
        else if(current_token.type == TOKEN_RPAREN){
            syntax_error();
        }
    }

    expect(TOKEN_RPAREN);
    

    if(is_main && current_param_count > 0){
        fprintf(stderr, "semtantic error: msin function can not have parameters\n");
        exit(5);
    }
}


static void parse_function(void){

    int is_main = 0;

    if(current_token.type == TOKEN_AT){
        fetch_token();
        
        if(strcmp(current_token.attribute, "main") == 0){
            fetch_token();
            is_main = 1;
        }
        else{
            syntax_error();
        }
    }

    expect(TOKEN_KW_DEF);

    if(current_token.type != TOKEN_IDENTIFIER){
        syntax_error();
    }

    char *func_name = current_token.attribute;
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
        else if(current_token.type == TOKEN_KW_UNIT){
            return_type = TYPE_UNIT;
            fetch_token();
        }
        else{
            syntax_error();
        }
    }

    Datatype *param_types = NULL;

    if(current_param_count > 0){
        param_types = malloc(sizeof(Datatype) * current_param_count);
        for(int i = 0; i < current_param_count; i++){
            param_types[i] = current_params[i].type;
        }
    }

    if(!insert_func(current_symtable, func_name, return_type, current_param_count, param_types)){
        fprintf(stderr, "semantic error: function '%s' already defined\n", func_name);
        exit(3);
    }
    
    if(param_types != NULL){
        free(param_types);
    }


    for(int i = 0; i < current_param_count; i++){
        if(!insert_var(current_symtable, current_params[i].name,current_params[i].type, current_scope_level)){
            fprintf(stderr, "semantic error: parameter '%s' redefinition\n", current_params[i].name);
        }
    }
    expect(TOKEN_ASSIGN);
    parser_statements();

    exit_scope(current_scope_level, current_symtable);
    current_scope_level--;
}


int parser_program(void){
    
    fetch_token();  //nacitanie prveho tokenu
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
            syntax_error();
        }
    
    }
    
    return 0;
}



