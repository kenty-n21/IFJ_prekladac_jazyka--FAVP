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
    type_void,
} Datatype;
typedef struct symtable symtable; // to na to aby kompilator vedel ze tato struktura exituje(bez toho to nejde)
typedef struct {
    char *meno;
    Datatype typ;
    bool is_function; // nastavuje parser aby tabulka vedela ci ide o fukciu alebo o premennu
    int param_count;
    Datatype *param_type;
    symtable *local_table;
    bool define;

} Symboldata;

typedef struct symtable {
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

char *make_compose_key(int scope_level, char *meno) {

    int len = snprintf(NULL, 0, "%d$%s", scope_level, meno); // funkcia ktora pise do pamati na nejake dane miesto( ak je tam null nezapise nikam)
    char *key = malloc(len + 1);
    if (key == NULL) {
        return NULL;
    }
    snprintf(key, len + 1, "%d$%s", scope_level, meno);
    return key;
}

// vlozenie pramennej do tabulky
bool insert_var(symtable *t, char *meno, Datatype typ, int scope_level) {

    char *kluc = make_compose_key(scope_level, meno);
    if (kluc == NULL) {
        free(kluc);
        return false;
    }
    if (hash_table_lookup(kluc, t) != NULL) {
        free(kluc);
        return false;
    }
    Symboldata *s = malloc(sizeof(Symboldata));
    if (s == NULL) {
        free(kluc);
        return false;
    }
    s->meno = kluc;
    s->typ = typ;
    s->is_function = false;
    s->param_count = 0;
    s->local_table = NULL;
    s->param_type = NULL;
    if (hash_table_insert(s, t)) {
        return true;
    }
    free(kluc);
    free(s);
    return false;
}
bool insert_func(symtable *t, char *meno, Datatype return_typ, int param_count, Datatype *param_type) {
    Symboldata *s = malloc(sizeof(Symboldata));
    if (s == NULL) {
        return false;
    }
    s->meno = meno;
    s->typ = return_typ;
    s->is_function = true;
    s->param_count = param_count;
    s->local_table = init_hash_table();
    if (s->local_table == NULL) {
        free(s);
        return false;
    }
    s->param_type = param_type;
    if (hash_table_insert(s, t)) {
        return true;
    }
    delete_hash_tabulka(s->local_table);
    free(s);
    return false;
}

Symboldata *lookup_var(int scope_level, char *meno, symtable *t) {
    if (t == NULL) {
        return NULL;
    }
    if (meno == NULL) {
        return NULL;
    }
    for (int i = scope_level; i >= 0; i--) {
        char *kluc = make_compose_key(i, meno);
        if (kluc == NULL) {
            return NULL;
        }
        Symboldata *hladane = hash_table_lookup(kluc, t);
        free(kluc);
        if (hladane != NULL) {
            return hladane;
        }
    }
    return NULL;
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
// zmazat v insert ze nemoze byt ta dana funkcia 2x hotovo //porozmyslat nad tym
// specifikacie ifj - 4 odkaz dorob
// oprav uvolennie celej tabulky
// lookup pozri ci je spravne
// ked mas viac premennych s tym istym nazvom premennej 1 bud stac, 2 velka tabulka s inicializovanim