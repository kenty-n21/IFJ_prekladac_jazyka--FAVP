#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    type_int,
    type_float,
    type_string,
    type_bool,
} Datatype;

typedef struct {
    char *meno;
    Datatype typ;

} Symboldata;

typedef struct {
    Symboldata **hash_table;
    int velkost_tabulky;
    int pocet_synbolov;
} symtable;

// vyhlada index kde ma byt ulozena dana hodnota
symtable *resize_hash_table(symtable *ptr) {
    if (ptr == NULL) {
        return NULL;
    }
    int nova_velkost = ptr->velkost_tabulky * 2;
    int stara_velkost = ptr->velkost_tabulky;
    Symboldata **newptr;
    Symboldata **stare_pole = ptr->hash_table;
    newptr = malloc(sizeof(Symboldata *) * nova_velkost);
    if (newptr == NULL) {
        return NULL;
    }
    ptr->hash_table = newptr;
    for (int l = 0; l < nova_velkost; l++) {
        newptr[l] = NULL;
    }
    ptr->pocet_synbolov = 0;
    ptr->velkost_tabulky = nova_velkost;
    for (int i = 0; i < stara_velkost; i++) {
        if (stare_pole[i] != NULL)
            hash_table_insert(stare_pole[i], ptr);
    }

    free(stare_pole);
    return ptr;
}
unsigned int hash_funkcia(char *meno) {
    int dlzka = strlen(meno);
    unsigned int hash_hodnota = 0;
    for (int i = 0; i < dlzka; i++) {
        hash_hodnota = hash_hodnota * 37 + meno[i];
    }
    return hash_hodnota;
}
// inicializuje tabulku
symtable *init_hash_table() {
    symtable *t;
    t = malloc(sizeof(symtable));
    if (t == NULL) {
        return NULL;
    }
    t->velkost_tabulky = 10;
    t->pocet_synbolov = 0;
    t->hash_table = malloc(sizeof(Symboldata *) * t->velkost_tabulky);
    for (int i = 0; i < t->velkost_tabulky; i++) {
        t->hash_table[i] = NULL;
    }
    return t;
}
// vlozenie prvku do tabulky bud na dany index alebo na prve miesto za nim
bool hash_table_insert(Symboldata *s, symtable *t) {
    if (t->pocet_synbolov == t->velkost_tabulky) {
        resize_hash_table(t);
    }

    unsigned int hash_hodnota = hash_funkcia(s->meno);
    int index = hash_hodnota % t->velkost_tabulky;
    for (int i = 0; i < t->velkost_tabulky; i++) {
        int try = (i + index) % t->velkost_tabulky;
        if (t->hash_table[try] == NULL) {
            t->hash_table[try] = s;
            t->pocet_synbolov = t->pocet_synbolov + 1;
            return true;
        }
        if (strcmp(t->hash_table[try]->meno, s->meno) == 0) {
            return true;
        }
    }
    return true;
}
// hladanie v tabulke od indexu kde ma byt a dalej
Symboldata *hash_table_lookup(char *s, symtable *t) {
    unsigned int hash_hodnota = hash_funkcia(s);
    int index = hash_hodnota % t->velkost_tabulky;
    for (int i = 0; i < t->velkost_tabulky; i++) {
        int try = (i + index) % t->velkost_tabulky;
        if (t->hash_table[try] != NULL && !(strcmp(t->hash_table[try]->meno, s))) {
            return t->hash_table[try];
        }
    }
    return NULL;
}
// zmazanie daneho synbolu bud na indexe alebo dalej
Symboldata *hash_table_delete(char *s) {
    if (s == NULL)
        return false;
    int index = hash_funkcia(s);
    for (int i = 0; i < velkost_tabulky; i++) {
        int try = (i + index) % velkost_tabulky;
        if (!(strcmp(hash_table[try]->meno, s)) && hash_table[try] != NULL) {
            Symboldata *tmp = hash_table[try];
            hash_table[try] = NULL;
            return tmp;
        }
    }

    return NULL;
}
