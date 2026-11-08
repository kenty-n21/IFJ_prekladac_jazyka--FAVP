#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "scanner.h"        //cooperation with scanner
#include "stack.h"
#include "string.h"

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
static void parse_expression(void);
static void parse_parameter_list(int is_main);



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


static PrecSymbol token_to_prec_symbol(TokenType t){
    switch (t)
    {
    case TOKEN_PLUS:    return SYM_PLUS;
    case TOKEN_MINUS:   return SYM_MINUS;
    case TOKEN_MUL:     return SYM_MUL;
    case TOKEN_DIV:     return SYM_DIV;
    case TOKEN_LPAREN:  return SYM_LPAR;
    case TOKEN_RPAREN:  return SYM_RPAR;
    case TOKEN_IDENTIFIER:
    case TOKEN_INTEGER:
    case TOKEN_DOUBLE:
    case TOKEN_STRING:  return SYM_ID;
    case TOKEN_EQ:
    case TOKEN_NEQ:
    case TOKEN_LT:
    case TOKEN_LTE:
    case TOKEN_GT:
    case TOKEN_GTE:     return SYM_REL_OP;
    
    default:    return SYM_DOLLAR;

    }
}

static int symbol_to_index(PrecSymbol s){
    switch (s)
    {
    case SYM_PLUS:  return 0;
    case SYM_MINUS: return 1;
    case SYM_MUL:   return 2;
    case SYM_DIV:   return 3;
    case SYM_LPAR:  return 4;
    case SYM_RPAR:  return 5;
    case SYM_ID:    return 6;
    case SYM_REL_OP: return 7;
    case SYM_DOLLAR: return 8;
        
    default:    return -1;
    
    }
}


static const char prec_table[9][9] = {
/*            +    -    *    /    (    )    id  rel   $   */
/* + */     {'>', '>', '<', '<', '<', '>', '<', '>', '>'},
/* - */     {'>', '>', '<', '<', '<', '>', '<', '>', '>'},
/* * */     {'>', '>', '>', '>', '<', '>', '<', '>', '>'},
/* / */     {'>', '>', '>', '>', '<', '>', '<', '>', '>'},
/* ( */     {'<', '<', '<', '<', '<', '=', '<', '<', ' '},
/* ) */     {'>', '>', '>', '>', ' ', '>', ' ', '>', '>'},
/* id */    {'>', '>', '>', '>', ' ', '>', ' ', '>', '>'},
/* rel */   {'<', '<', '<', '<', '<', '>', '<', '>', '>'},
/* $ */     {'<', '<', '<', '<', '<', ' ', '<', '<', ' '}
};




static void parse_expression(void){
    TStack stack;
    init_stack(&stack);

    TSData bottom_data = { .symbol = SYM_DOLLAR, .type = TYPE_UNDEFINED};
    push_stack(&stack, bottom_data);

    bool expression_ended = false;

    while (!expression_ended)
    {
        PrecSymbol current_sym = token_to_prec_symbol(current_token.type);

        if(current_token.type == TOKEN_EOL || current_token.type == TOKEN_COMMA || current_token.type == TOKEN_EOF || current_token.type == 3){
            current_sym = SYM_DOLLAR;
            expression_ended =true;
        }

        TSElem *top_term = get_topterminal_stack(&stack);
        PrecSymbol top_sym = top_term ? top_term->data.symbol : SYM_DOLLAR;

        if(top_sym == SYM_DOLLAR && current_sym == SYM_DOLLAR){
            break;
        }

        int row = symbol_to_index(top_sym);
        int col = symbol_to_index(current_sym);

        if(row == -1 || col == -1){
            syntax_error();
        }

        char relation = prec_table[row][col];

        if(relation == '<' || relation == '='){
            if(relation == '<'){
                insert_terminal_stop_stack(&stack);
            }

            TSData push_data = { .symbol = current_sym, .type = TYPE_UNDEFINED};
            push_stack(&stack, push_data);

            if(!expression_ended){
                fetch_token();
            }
        }
        else if(relation == '>'){
            while (top_stack(&stack)!= NULL && top_stack(&stack)->data.symbol != SYM_STOP){
                pop_stack(&stack);
            }

            if(top_stack(&stack) != NULL && top_stack(&stack)->data.symbol == SYM_STOP){
                pop_stack(&stack);
            }

            TSData nt_data = { .symbol = SYM_NONTERM, .type = TYPE_UNDEFINED};
            push_stack(&stack, nt_data);
            
        }
        else{
            syntax_error();
        }

        if(expression_ended){
            break;
        }
    }
    
    free_stack(&stack);

   
}

//kotrola pre prikaz: priradenie hodnoty do premennej (zodpoveda kod pravidlam IFJ26?)
static void parser_assignment(void){
    fprintf(stderr, "DEBUG: Entering parser_assignment\n");

    expect(TOKEN_IDENTIFIER);   //token = platny nazov premennej?
    fprintf(stderr, "DEBUG: Assignment identifier matched, expecting ASSIGN\n");

    expect(TOKEN_ASSIGN);       //nasleduje operator priradenia?
    fprintf(stderr, "DEBUG: ASSIGN matched, parsing expression\n");

    parse_expression();
    fprintf(stderr, "DEBUG: Expression parsed, current token before EOL is: type = %d, attr = '%s'\n", current_token.type, current_token.attribute);

    expect(TOKEN_EOL);        //kocni prikaz podkociarkou?
    fprintf(stderr, "DEBUG: Assignment finished successfully\n");
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


static void parser_statements(void){
    fprintf(stderr, "DEBUG: Entering parser_statements\n");

    while (current_token.type != TOKEN_EOF && current_token.type != TOKEN_KW_DEF && current_token.type != TOKEN_AT)
    {
        fprintf(stderr, "DEBUG: Statement loop token -> type: %d, attr: '%s'\n", current_token.type, current_token.attribute);
        switch (current_token.type)
        {
        case TOKEN_KW_VAL:
        case TOKEN_KW_VAR:
            parse_var_declaration();
            break;


        case TOKEN_IDENTIFIER:
            parser_assignment();
            break;

        case TOKEN_EOL:
            fetch_token();
            break;

        case 2:
            fetch_token();
            break;
            

        default:
            fprintf(stderr, "DEBUG: Unexpected token type in statements: %d (attr: '%s')\n", current_token.type, current_token.attribute);
            syntax_error();
            break;
        }
    }
    fprintf(stderr, "DEBUG: Exited parser_statements\n");
}


static void parse_parameter_list(int is_main){
    fprintf(stderr, "DEBUG: Entering parse_parameter_list\n");
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
            fprintf(stderr, "DEBUG: Error in parameter type, token type is: %d\n", current_token.type);
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
    fprintf(stderr, "DEBUG: Parameter list parsed successfully\n");

    if(is_main && param_count > 0){
        fprintf(stderr, "semtantic error: msin function can not have parameters\n");
        exit(5);
    }
}


static void parse_function(void){
    fprintf(stderr, "DEBUG: Entering parse_function\n");
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
    fprintf(stderr, "DEBUG: Function name parsed, parsing parameters...\n");

    parse_parameter_list(is_main);

    if(current_token.type == TOKEN_COLON){
        fetch_token();
        fprintf(stderr, "DEBUG: Return type token type: %d (attr: '%s')\n", current_token.type, current_token.attribute);
        if(current_token.type == TOKEN_KW_INT || current_token.type == TOKEN_KW_DOUBLE_TYPE || current_token.type == TOKEN_KW_STRING_TYPE || current_token.type == TOKEN_KW_UNIT){
        fetch_token();
        }

        else{
            syntax_error();
        }
    }
    
    expect(TOKEN_ASSIGN);
    fprintf(stderr, "DEBUG: Parsing function statements...\n");
    parser_statements();
    fprintf(stderr, "DEBUG: Function parsed successfully!\n");


}


int parser_program(void){
    
    fprintf(stderr, "DEBUG: Start parser_program\n");
    fetch_token();  //nacitanie prveho tokenu
    parse_prolog();
    fprintf(stderr, "DEBUG: Prolog parsed successfully\n");
    
    while (current_token.type != TOKEN_EOF)
    {
        fprintf(stderr, "DEBUG: Current token type: %d\n", current_token.type);
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
    
    fprintf(stderr, "DEBUG: End of program\n");
    return 0;
    
}



