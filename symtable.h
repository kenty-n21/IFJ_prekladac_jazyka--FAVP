#ifndef SYMTABLE_H
#define SYMTABLE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

typedef enum {
    type_int,
    type_double,
    type_string,
    type_bool,
    type_unit,
    type_any,
} Datatype;
typedef struct symtable symtable;
typedef struct {
    char *meno;
    Datatype typ;
    bool is_function;
    int param_count;
    Datatype *param_type;
    symtable *local_table;
    bool define;
    int scope_level;

} Symboldata;

typedef struct symtable {
    Symboldata **hash_table;
    int velkost_tabulky;
    int pocet_synbolov;
} symtable;

#define delete_item ((Symboldata *)-1)

symtable *init_hash_table(void);                                                                       // vytvoris tabulku
bool insert_var(symtable *t, char *meno, Datatype typ, int scope_level);                               // vlozis do tabulky premennu
bool insert_func(symtable *t, char *meno, Datatype return_typ, int param_count, Datatype *param_type); // vlozis funkciu do tabulky
void insert_vstavane_funkcie(symtable *t);                                                             // vlzoenie do tabulky vstavane funkcie
void exit_scope(int scope_level, symtable *t);                                                         // odstranenie premennych - vnorenych
Symboldata *hash_table_delete(char *s, symtable *t);                                                   // odstranenie hodnoty na konkretnom indexe
void delete_hash_tabulka(symtable *t);                                                                 // odtranenie celej tabulky
Symboldata *lookup_var(int scope_level, char *meno, symtable *t);                                      // najde premmenu v tabulke
#endif