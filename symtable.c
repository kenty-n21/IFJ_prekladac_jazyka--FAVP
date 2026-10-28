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

#define delete_item ((Symboldata *)-1) // povie ze je to adresa, reprezentovana -1
// musis deklarovat dopredu ze dana funkcia existuje lebo ak nie tak mas chybu + warning + note a nemozes skpirovat funkciu
// ako taku pretoze ta zas obsahuje ine funkcie a inak to nefunguje
bool hash_table_insert(Symboldata *s, symtable *t);
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
        if (stare_pole[i] != NULL && stare_pole[i] != delete_item)
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
    if (t->hash_table == NULL) {
        free(t);
        return NULL;
    }
    for (int i = 0; i < t->velkost_tabulky; i++) {
        t->hash_table[i] = NULL;
    }
    return t;
}
// vlozenie prvku do tabulky bud na dany index alebo na prve miesto za nim
bool hash_table_insert(Symboldata *s, symtable *t) {
    if (s == NULL || t == NULL) {
        return false;
    }
    if (t->pocet_synbolov == t->velkost_tabulky) {
        if (resize_hash_table(t) == NULL) {
            return false;
        }
    }

    unsigned int hash_hodnota = hash_funkcia(s->meno);
    int index = hash_hodnota % t->velkost_tabulky;
    for (int i = 0; i < t->velkost_tabulky; i++) {
        int try = (i + index) % t->velkost_tabulky;
        if (t->hash_table[try] == NULL || t->hash_table[try] == delete_item) {
            t->hash_table[try] = s;
            t->pocet_synbolov = t->pocet_synbolov + 1;
            return true;
        }
    }
    return false;
}

// hladanie v tabulke od indexu kde ma byt a dalej
Symboldata *hash_table_lookup(char *s, symtable *t) {
    if (s == NULL || t == NULL) {
        return NULL;
    }
    unsigned int hash_hodnota = hash_funkcia(s);
    int index = hash_hodnota % t->velkost_tabulky;
    for (int i = 0; i < t->velkost_tabulky; i++) {
        int try = (i + index) % t->velkost_tabulky;
        if (t->hash_table[try] == NULL) {
            return NULL;
        }
        if (t->hash_table[try] != delete_item && t->hash_table[try] != NULL && !(strcmp(t->hash_table[try]->meno, s))) {
            return t->hash_table[try];
        }
    }
    return NULL;
}
// zmazanie daneho synbolu bud na indexe alebo dalej
Symboldata *hash_table_delete(char *s, symtable *t) {
    if (s == NULL || t == NULL)
        return NULL;
    unsigned int hash_hodnota = hash_funkcia(s);
    int index = hash_hodnota % t->velkost_tabulky;
    for (int i = 0; i < t->velkost_tabulky; i++) {
        int try = (i + index) % t->velkost_tabulky;
        if (t->hash_table[try] == NULL) {
            return NULL;
        }
        if (t->hash_table[try] != NULL && !(strcmp(t->hash_table[try]->meno, s))) {
            Symboldata *tmp = t->hash_table[try];
            t->hash_table[try] = delete_item;
            t->pocet_synbolov = t->pocet_synbolov - 1;
            return tmp;
        }
    }
    return NULL;
}
void delete_hash_tabulka(symtable *t) {

    for (int i = 0; i < t->velkost_tabulky; i++) {
        if (t->hash_table[i] != NULL && t->hash_table[i] != delete_item) {
            free(t->hash_table[i]->meno); // nicis meno
            free(t->hash_table[i]);       // nisic strukturu datatyp
        }
    }
    free(t->hash_table); // nicis hastable
    free(t);             // nicis ukazovatel ndo pamate na tuto tabulku
}
// opravit delete, ist po NULL alebo kym to najdes a vsetko posunut o ten jeden vyssie hotovo
// zmazat v insert ze nemoze byt ta dana funkcia 2x hotovo
// specifikacie ifj - 4 odkaz dorob
// sprav uvolennie celej tabulky