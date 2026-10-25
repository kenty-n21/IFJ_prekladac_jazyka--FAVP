#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "scanner.h"

static Token current_token; 

static void get_next_token(void){

}


static void syntax_error(void){
    fprintf(stderr, "Syntax errror in code");
    
    exit(2);
}


static void parser_statements(void);



int parser_program(void){

    get_next_token();
    parser_statements();
    return 0;
}


static void parser_stratements(void){
    
}

