/**
 * Autor: Veronika Kentošová(xkentov00)
 * implementácia precedenčnej analýzy výrazov
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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


//Precedencna tabuulka vyrazov
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
      case TOKEN_IDENTIFIER: 
      case TOKEN_INTEGER:
      case TOKEN_DOUBLE:
      case TOKEN_STRING:
         return SYM_ID;
         break;
      default : return SYM_DOLLAR;
   }
}
//pomocne funkcie pre AST

//vytvori uzol pre literal alebo premennu
ExprNode* create_leaf(NodeType nodetype, DataType datatype, const char *value){
   ExprNode *NewNode = (ExprNode*) malloc(sizeof(struct ExprNode));
   if(NewNode == NULL){
      return NULL;
   }
   NewNode->nodetype = nodetype;
   NewNode->datatype = datatype;
   NewNode->symbol = SYM_ID;
   NewNode->left = NULL;
   NewNode->right = NULL;

   strncpy(NewNode->value,value,sizeof(NewNode->value)-1);
   NewNode->value[sizeof(NewNode->value)-1] = '\0';

   return NewNode;
}

//vytvori uzol pre unarny operator 
ExprNode* create_unary(PrecSymbol symbol, ExprNode *child, DataType datatype){
   ExprNode *NewNode = (ExprNode*)malloc(sizeof(struct ExprNode));
   if(NewNode == NULL){
      return NULL;
   }
   NewNode->nodetype = NODE_UNARY;
   NewNode->symbol = symbol;
   NewNode->left = child;
   NewNode->right = NULL;
   NewNode->datatype = datatype;
   NewNode->value[0] = '\0';
   return NewNode;
}

//vytvori uzol pre binarny operator 
ExprNode* create_binary(PrecSymbol symbol, ExprNode *left, ExprNode *right, DataType datatype){
   ExprNode *NewNode = (ExprNode*)malloc(sizeof(struct ExprNode));
   if(NewNode == NULL){
      return NULL;
   }
   NewNode->nodetype = NODE_BINARY;
   NewNode->symbol = symbol;
   NewNode->left = left;
   NewNode->right = right;
   NewNode->datatype = datatype;
   NewNode->value[0] = '\0';
   return NewNode;
}

//uvolni podstrom
void expr_free(ExprNode *node){
   if(node == NULL)
      return;
   expr_free(node->left);
   expr_free(node->right);
   free(node);
}


//redukcna funkcia
int reduce(TStack *stack){}