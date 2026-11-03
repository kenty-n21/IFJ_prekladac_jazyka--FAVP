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