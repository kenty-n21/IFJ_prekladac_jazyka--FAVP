/*
 * Projekt: Implementacia prekladaca imperativneho jazyka IFJ26
 * Subor: symtable.h
 * Autor: Filip Simulcik(xsimulf00)
 */
#ifndef SYMTABLE_H
#define SYMTABLE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

typedef enum {
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_UNIT,
    TYPE_ANY,
} Datatype;

typedef struct symtable symtable;
typedef struct {
    char *meno;
    Datatype typ;
    bool is_function;
    int param_count;
    Datatype *param_type;
    symtable *local_table;
    int scope_level;

} Symboldata;

typedef struct symtable {
    Symboldata **hash_table;
    int velkost_tabulky;
    int pocet_synbolov;
} symtable;

#define delete_item ((Symboldata *)-1)

symtable *init_hash_table(void); // vytvoris tabulku
// ukazka ako na to
/* symtable *t =init_hash_table()
 */
bool insert_var(symtable *t, char *meno, Datatype typ, int scope_level); // vlozis do tabulky premennu
// ukazka ako na to
/* bool ukazka = insert_var(tabulka kam to ide, meno premennej, typ premennej, cislo zanorenia(0 az n))
 */
bool insert_func(symtable *t, char *meno, Datatype return_typ, int param_count, Datatype *param_type); // vlozis funkciu do tabulky
// ukazka ako na to
/* Datatype *premenna = malloc(sizeof(Datatyp)* pocet parametrov funkcie)
    premenna[0] =  type_xy
    premenna[1] = type_xy
    atd
    bool ukazka = insetr_func(tabulka kam to ide, meno funkcie, vracany datovy typ, pocet parametrov, ukazovatel na premenne (v tomto pripade premenna))*/
void insert_vstavane_funkcie(symtable *t); // vlzoenie do tabulky vstavane funkcie
// ukazka ako na to
/* symtable *t =init_hash_table()
    insetr_vstavane_funkcie(t)

    vola sa hned na zaciatku*/
void exit_scope(int scope_level, symtable *t); // odstranenie premennych - vnorenych
// ukazka ako na to
/* exit_scope(hlbka vnorenia, tabulka kde to ma byt)*/
bool hash_table_delete(char *s, symtable *t); // odstranenie hodnoty na konkretnom indexe
// ukazka ako na to
/* hash_table_delete(nazov, tabulka)*/
// ak chces zmazat prvok(co nechapem naco by si robila ked mas exit_scope ) tak ta dana premenna musi byt v tvare cislo vnorenia$nazov premennej
void delete_hash_tabulka(symtable *t); // odtranenie celej tabulky
// ukazka ako na to
/*delete_hash_tabulka(ukazovatel na tabulku)*/
Symboldata *lookup_var(int scope_level, char *meno, symtable *t); // najde premmenu v tabulke
// ukazka ako na to
/* vracia ukazovatel na dany prvok
    Symboldata *ukazka = lookup_var(level zanorenia, level zanorenia$meno premennej, tabulka kde hladat) musi tam byt pri var $ pri funkcii nemusi
    strcmp((ukazka->meno, 0$nazov premennej))  zistis ci je to to co hladas ale iba pri premennej, funkcia pocitam s tym ze nemoze byt zanorena
    musi tam byt na zaciatku 0$*/

void insert_included_fun(symtable *t);
    
#endif