/*
 * Projekt: Implementacia prekladaca imperativneho jazyka IFJ26
 * Subor: error.h
 * Autor:  Veronika Kentosova (xkentov00) 
 */
#ifndef ERROR_H
#define ERROR_H

//inspo: https://softwareengineering.stackexchange.com/questions/445703/standard-error-codes-vs-custom-error-codes-in-c

//vycet pre jednotlive errorove navratove kody
typedef enum{
    NO_ERROR = 0,
    ERROR_LEX = 1,
    ERROR_SYNT = 2,
    ERROR_SEM_UNDEF = 3,
    ERROR_SEM_REDEF = 4,
    ERROR_SEM_ARG = 5,
    ERROR_SEM_TYPE = 6,
    ERROR_SEM_WRITE = 7,
    ERROR_SEM_OTHER = 10,
    ERROR_INTERNAL = 99,
    ERROR_RUN = 40,
}Error;

#endif