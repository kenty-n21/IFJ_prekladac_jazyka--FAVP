/**
 * Autor: Veronika Kentošová(xkentov00)
 * implementácia precedenčnej analýzy výrazov
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stack.h"
#include "symtable.h"
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
      case TOKEN_MINUS:
         return SYM_MINUS;
      case TOKEN_MUL:
         return SYM_MUL;
      case TOKEN_DIV:
         return SYM_DIV;
      case TOKEN_LT:
         return SYM_LT;
      case TOKEN_GT:
         return SYM_GT;
      case TOKEN_LTE:
         return SYM_LTE;
      case TOKEN_GTE:
         return SYM_GTE;
      case TOKEN_EQ:
         return SYM_EQ;
      case TOKEN_NEQ:
         return SYM_NEQ;
      case TOKEN_LPAREN:
         return SYM_LPAR;
      case TOKEN_RPAREN:
         return SYM_RPAR;
      case TOKEN_IDENTIFIER: 
      case TOKEN_INTEGER:
      case TOKEN_DOUBLE:
      case TOKEN_STRING:
         return SYM_ID;
      default : return SYM_DOLLAR;
   }
}

//funkcia na overenie ci je symbol operator
static bool is_binary_op(PrecSymbol symbol){
   switch(symbol) {
      case SYM_PLUS:
      case SYM_MINUS:
      case SYM_MUL:
      case SYM_DIV:
      case SYM_LT:
      case SYM_GT:
      case SYM_LTE:
      case SYM_GTE:
      case SYM_EQ:
      case SYM_NEQ:
         return true;
      default : return false;
   }
}

//funkcia kontoluje sematiku datovzch tzpov a vracia vysledny typ vyrazu
static int semantic_check(PrecSymbol op, Datatype left, Datatype right,Datatype* result){
   bool num_left = (left == TYPE_DOUBLE || left == TYPE_INT);
   bool num_right = (right == TYPE_DOUBLE || right == TYPE_INT);

   switch (op)
   {
   //konkatenacia
   case SYM_PLUS:
      if(left == TYPE_STRING && right == TYPE_STRING){
         *result = TYPE_STRING;
         return NO_ERROR;
      }
   //fallthrough
   case SYM_MINUS:
   case SYM_MUL:
   case SYM_DIV:
      if(num_left && num_right){
         if(left==TYPE_DOUBLE || right == TYPE_DOUBLE){
            *result = TYPE_DOUBLE;
            return NO_ERROR;
         }else{
            *result = TYPE_INT;
            return NO_ERROR;
         }
      }
      return ERROR_SEM_TYPE;

   case SYM_LT:
   case SYM_GT:
   case SYM_LTE:
   case SYM_GTE:
      if((num_left && num_right) || (left == TYPE_STRING && right == TYPE_STRING)){
         *result = TYPE_BOOL;
         return NO_ERROR;
      }
      return ERROR_SEM_TYPE;

   case SYM_EQ:
   case SYM_NEQ:
      if((left == right) || (num_left && num_right) ){
         *result = TYPE_BOOL;
         return NO_ERROR;
      }
      return ERROR_SEM_TYPE;

   default: return ERROR_SEM_TYPE;
   }
}
//pomocne funkcie pre AST

//vytvori uzol pre literal alebo premennu
ExprNode* create_leaf(NodeType nodetype, Datatype datatype, const char *value){
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
ExprNode* create_unary(PrecSymbol symbol, ExprNode *child, Datatype datatype){
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
ExprNode* create_binary(PrecSymbol symbol, ExprNode *left, ExprNode *right, Datatype datatype){
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
int reduce(TStack *stack){
   TSElem *Current = stack->top;
   TSData arr[3];
   TSData reduced;
   int count = 0;

   //vyuzivame cyklus aby sme zistili kolko prvkov je pred SYM_STOP
   //podla toho budeme kontrolovat ktore pravidlo pouzitw
   while(Current != NULL && Current->data.symbol != SYM_STOP){
      count+=1;
      Current = Current->next;
   }

   //kontrola chyb
   if(Current == NULL || count == 0 || count > 3)
      return(ERROR_SYNT);

   for(int i = 0; i < count ; i++){
      arr[i]= top_stack(stack)->data;
      pop_stack(stack);
   }

   //odstranenie zarazky
   pop_stack(stack);


   //vetvenie kontrola na pouzitie pravidla podla poctu count
   //na zaklade poctu a konkretnych typov precedencnych symbolov urcime
   //ake pravidlo pouzijeme
   switch (count)
   {
   case 1:
      //E->id
      //kontrola syntaxe
      if(arr[0].symbol != SYM_ID)
         return ERROR_SYNT;

      //redukovanie vyrazu pravidlom
      reduced.symbol = SYM_NONTERM;
      reduced.type = arr[0].type;
      reduced.node = arr[0].node;
      push_stack(stack,reduced);
      return NO_ERROR;

   case 2:
      //E->-E
      //kontrola syntaxe
      if(arr[0].symbol != SYM_NONTERM || arr[1].symbol != SYM_UNARY_MIN)
         return ERROR_SYNT;
      
      //kontrola semantiky
      if (arr[0].type != TYPE_INT && arr[0].type != TYPE_DOUBLE)
         return ERROR_SEM_TYPE;
      
      ExprNode *Node = create_unary(SYM_UNARY_MIN, arr[0].node, arr[0].type);
      //kontrola pamate
      if(Node == NULL)
         return ERROR_INTERNAL;

      //pushnutie redukovaneho vyrazu spat na zasobnik
      reduced.symbol = SYM_NONTERM;
      reduced.type = arr[0].type;
      reduced.node = Node;
      push_stack(stack,reduced);
      return NO_ERROR;

   case 3:
      //zistit o ktory pripad ide

      //pripad A: E->(E)
      if(arr[0].symbol == SYM_RPAR && arr[1].symbol == SYM_NONTERM && arr[2].symbol == SYM_LPAR){
         reduced.symbol = SYM_NONTERM;
         reduced.type = arr[1].type;
         reduced.node = arr[1].node;
         push_stack(stack,reduced);
         return NO_ERROR;
      }
      //pripad B: E-> E1 op E2
      else if(arr[0].symbol == SYM_NONTERM && is_binary_op(arr[1].symbol) && arr[2].symbol == SYM_NONTERM){
         //kontrola semantiky 

      }
      else{
         return ERROR_SYNT;
      }
   default:
      break;
   }

}