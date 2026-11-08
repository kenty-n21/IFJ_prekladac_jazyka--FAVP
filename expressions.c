/**
 * Autor: Veronika Kentošová(xkentov00)
 * implementácia precedenčnej analýzy výrazov
 */

#include "stack.h"
#include "expressions.h"
 //inspo: https://www.geeksforgeeks.org/compiler-design/role-of-operator-precedence-parser/
 //inspo: https://compilerdesigndetails.blogspot.com/2012/02/operator-precedence-parsing.html
 //inspo: https://www.slideserve.com/maeko/bottom-up-parsing

#define S PREC_SHIFT   // <
#define R PREC_REDUCE  // >
#define E PREC_EQUAL   // = 
#define X PREC_ERROR   // 

#define PREC_TABLE_SIZE (SYM_DOLLAR + 1)

static const PrecAction prec_table[PREC_TABLE_SIZE][PREC_TABLE_SIZE] = {
    //               + u-  -  *  /  <  > <= >= == !=  (  )  id $
    /* + */         {R, S, R, S, S, R, R, R, R, R, R, S, R, S, R},
    /* un- */       {R, S, R, R, R, R, R, R, R, R, R, S, R, S, R},
    /* -  */        {R, S, R, S, S, R, R, R, R, R, R, S, R, S, R},
    /* *  */        {R, S, R, R, R, R, R, R, R, R, R, S, R, S, R},
    /* /  */        {R, S, R, R, R, R, R, R, R, R, R, S, R, S, R},
    /* <  */        {S, S, S, S, S, X, X, X, X, X, X, S, R, S, R},
    /* >  */        {S, S, S, S, S, X, X, X, X, X, X, S, R, S, R},
    /* <= */        {S, S, S, S, S, X, X, X, X, X, X, S, R, S, R},
    /* >= */        {S, S, S, S, S, X, X, X, X, X, X, S, R, S, R},
    /* == */        {S, S, S, S, S, X, X, X, X, X, X, S, R, S, R},
    /* != */        {S, S, S, S, S, X, X, X, X, X, X, S, R, S, R},
    /* (  */        {S, S, S, S, S, S, S, S, S, S, S, S, E, S, X},
    /* )  */        {R, X, R, R, R, R, R, R, R, R, R, X, R, X, R},
    /* id */        {R, X, R, R, R, R, R, R, R, R, R, X, R, X, R},
    /* $  */        {S, S, S, S, S, S, S, S, S, S ,S, S, X, S, X}
};


static PrecSymbol token_to_symbol(Token *token){
   switch (token->type){
      case TOKEN_PLUS:
         return SYM_PLUS;
         break;
      case TOKEN_MINUS:
         return SYM_MINUS;
         break;
      case TOKEN_MUL:
         return SYM_MUL;
         break;
      case TOKEN_DIV:
         return SYM_DIV;
         break;
      case TOKEN_LT:
         return SYM_LT;
         break;
      case TOKEN_GT:
         return SYM_GT;
         break;
      case TOKEN_LTE:
         return SYM_LTE;
         break;
      case TOKEN_GTE:
         return SYM_GTE;
         break;
      case TOKEN_EQ:
         return SYM_EQ;
         break;
      case TOKEN_NEQ:
         return SYM_NEQ;
         break;
      case TOKEN_LPAREN:
         return SYM_LPAR;
         break;
      case TOKEN_RPAREN:
         return SYM_RPAR;
         break;
      case TOKEN_IDENTIFIER || TOKEN_INTEGER || TOKEN_DOUBLE || TOKEN_STRING:
         return SYM_ID;
         break;
      default : return SYM_DOLLAR;
   }
 }