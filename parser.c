#include <stdio.h>
#include <stdlib.h>
#include "string.h"

#include "parser.h"
#include "scanner.h"        //cooperation with scanner
#include "stack.h"
#include "hashtable.h"
#include "symtable.h"


//current token 
static Token current_token; 

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

    expect(TOKEN_IDENTIFIER);   //token = platny nazov premennej?
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

    expect(TOKEN_IDENTIFIER);

    if(current_token.type == TOKEN_COLON){
        fetch_token();
        if(current_token.type == TOKEN_KW_INT || current_token.type == TOKEN_KW_DOUBLE_TYPE || current_token.type == TOKEN_KW_STRING_TYPE){
            fetch_token();
        }
        else{
            syntax_error();
        }
    }
    expect(TOKEN_ASSIGN);
    parse_expression();
    expect(TOKEN_EOL);
}


static void parse_if(void){
    expect(TOKEN_KW_IF);

    parse_expression();

    if(current_token.type == TOKEN_LBRACKET){
        fetch_token();
        parser_statements();
        expect(TOKEN_RBRACKET);
    }
    else{
        parser_statements();
    }

    if(current_token.type == TOKEN_KW_ELSE){
        fetch_token();

        if(current_token.type == TOKEN_LBRACKET){
            fetch_token();
            parser_statements();
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

        parser_statements();
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

    int param_count = 0;

    while(current_token.type != TOKEN_RPAREN && current_token.type != TOKEN_EOF){
        param_count++;

        expect(TOKEN_IDENTIFIER);

        expect(TOKEN_COLON);

        if(current_token.type == TOKEN_KW_INT || current_token.type == TOKEN_KW_DOUBLE_TYPE || current_token.type == TOKEN_KW_STRING_TYPE){
            fetch_token();
        }
        else{
            syntax_error();
        }

        if(current_token.type == TOKEN_COMMA){
            fetch_token();
        }
        else if(current_token.type != TOKEN_RPAREN){
            syntax_error();
        }
    }

    expect(TOKEN_RPAREN);
    

    if(is_main && param_count > 0){
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

    expect(TOKEN_IDENTIFIER);
    
    parse_parameter_list(is_main);

    if(current_token.type == TOKEN_COLON){
        fetch_token();
        
        if(current_token.type == TOKEN_KW_INT || current_token.type == TOKEN_KW_DOUBLE_TYPE || current_token.type == TOKEN_KW_STRING_TYPE || current_token.type == TOKEN_KW_UNIT){
        fetch_token();
        }

        else{
            syntax_error();
        }
    }
    
    expect(TOKEN_ASSIGN);
    parser_statements();
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



