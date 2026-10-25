#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "scanner.h"        //spolupraca zo suborom scanner

//aktualny token an ktorom sa parser nachadza
static Token current_token; 
static void fetch_token(void);
static void expect(TokenType expect_type);
static void parser_statements(void);
static void parser_assignment(void);
static void parse_expression(void);



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
    if (current_token.type == expected_type){
        fetch_token();
    }
    else{
        syntax_error();
    }
}


static void parse_expression(void){
    //kontrola ci je platny zaciatok vyrazu
    if (current_token.type == TOKEN_IDENTIFIER || current_token.type == TOKEN_INTEGER || current_token.type == TOKEN_DOUBLE || current_token.type == TOKEN_STRING){

        fetch_token();

    }
    else {
        fprintf(stderr, "error with syntax in expression! ");
        exit(2);
    }
}

//kotrola pre prikaz: priradenie hodnoty do premennej (zodpoveda kod pravidlam IFJ26?)
static void parser_assignment(void){

    expect(TOKEN_IDENTIFIER);   //token = platny nazov premennej?

    expect(TOKEN_ASSIGN);       //nasleduje operator priradenia?

    parse_expression();

    expect(TOKEN_EOL);        //kocni prikaz podkociarkou?
}



static void parser_statements(void){

    while (current_token.type != TOKEN_EOF)
    {
        switch (current_token.type)
        {
        case TOKEN_IDENTIFIER:
            parser_assignment();
            break;


        
        default:
            fetch_token();
            break;
        }
    }
}


int parser_program(void){
    
    fetch_token();  //nacitanie prveho tokenu
    parser_statements();    //spracovanie prikazov
    return 0;
}
















