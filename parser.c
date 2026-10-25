#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "scanner.h"        //spolupraca zo suborom scanner

//aktualny token an ktorom sa parser nachadza
static Token current_token; 

//nacita dalsi token zo scanner
static void fetch_token(void){  

    current_token = get_next_token();

}


//type control pre tokeny
//posunie token dalej alebo vyhodi syntax error
static void expect(expected_type){  
    if (current_token.type == expected_type){
        fetch_token();
    }
    else{
        fprintf(stderr, "Synatx errror! Unexpected token!\n");
        exit(2);
    }
}


static void syntax_error(void){
    fprintf(stderr, "Syntax errror in code\n");
    
    exit(2);
}


static void parser_statements(void);



int parser_program(void){
    
    fetch_token();  //nacitanie prveho tokenu
    parser_statements();    //spracovanie prikazov
    return 0;
}


static void parser_statements(void){

    while (current_token.type != TOKEN_EOF)
    {
        switch (current_token.type)
        {
        case TOKEN_IDENTIFIER:
            /**/
            break;


        
        default:
            fetch_token();
        }

    }
    
    
}

//kotrola pre prikaz: priradenie hodnoty do premennej (zodpoveda kod pravidlam IFJ26?)
static void parse_assignment(void){

    expect(TOKEN_IDENTIFIER);   //token = platny nazov premennej?

    expect(TOKEN_ASSIGN);       //nasleduje operator priradenia?

    /*parse_expression */

    expect(TOKEN_COLON);        //kocni prikaz podkociarkou?
}



