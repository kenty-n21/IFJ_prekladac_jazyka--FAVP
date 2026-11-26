/*
 * Projekt: Implementacia prekladaca imperativneho jazyka IFJ26
 * Subor: symtable.c
 * Autor: Filip Simulcik (xsimulf00)
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// typy premennej/ navratovej funkcie
typedef enum {
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_UNIT,
    TYPE_ANY,
} Datatype;
typedef struct symtable symtable; // to na to aby kompilator vedel ze tato struktura exituje(bez toho to nejde)

typedef struct {
    char *meno;
    Datatype typ;
    bool is_function; // nastavuje parser aby tabulka vedela ci ide o fukciu alebo o premennu
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

#define delete_item ((Symboldata *)-1) // povie ze je to adresa, reprezentovana -1
// musis deklarovat dopredu ze dana funkcia existuje lebo ak nie tak mas chybu + warning + note a nemozes skpirovat funkciu
// ako taku pretoze ta zas obsahuje ine funkcie a inak to nefunguje
bool hash_table_insert(Symboldata *s, symtable *t);
void delete_hash_tabulka(symtable *t); // !!!!! gcc -std=c11 -Wall -Wextra -pedantic zbehne bez erroru

// ak je tabulka plna tak ju zvacsi dna dvojnasobok a vlozi do nej platne prvky (nevlozi NULL a deleted_item)
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
// vypocet hashu - 37 - prvocilo pre lepsie rozptylenie
unsigned int hash_funkcia(char *meno) {
    int dlzka = strlen(meno);
    unsigned int hash_hodnota = 0;
    for (int i = 0; i < dlzka; i++) {
        hash_hodnota = hash_hodnota * 37 + meno[i];
    }
    return hash_hodnota;
}
// inicializuje tabulku a prvych 10 prvkov nastavy na NULL
symtable *init_hash_table(void) {
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
    if (s == NULL || t == NULL || s->meno == NULL) {
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
        // prestane hladat ake najde NULL
        if (t->hash_table[try] == NULL) {
            return NULL;
        }
        if (t->hash_table[try] != delete_item && t->hash_table[try] != NULL && !(strcmp(t->hash_table[try]->meno, s))) {
            return t->hash_table[try];
        }
    }
    return NULL;
}
// funkcia na unikatne vytvorenie kluca v tvare scope_level$nazov
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
    if (t == NULL || meno == NULL) {
        return false;
    }
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
    s->scope_level = scope_level;
    if (hash_table_insert(s, t)) {
        return true;
    }

    free(kluc);
    free(s);
    return false;
}
// vlozenie funckie do tabulky, najprv ale pozrieme ci sa tam uz nenachadza a ak nie tak az potom ju vlozime
bool insert_func(symtable *t, char *meno, Datatype return_typ, int param_count, Datatype *param_type) {
    if (t == NULL || meno == NULL) {
        return false;
    }
    Symboldata *s = malloc(sizeof(Symboldata));
    if (s == NULL) {
        return false;
    }
    if (hash_table_lookup(meno, t) != NULL) {
        free(s);
        return false;
    }

    s->meno = malloc(strlen(meno) + 1);
    if (s->meno == NULL) {
        free(s);
        return false;
    }
    strcpy(s->meno, meno);
    s->typ = return_typ;
    s->is_function = true;
    s->param_count = param_count;
    s->local_table = init_hash_table();
    if (s->local_table == NULL) {
        free(s->meno);
        free(s);
        return false;
    }
    s->param_type = param_type;
    if (hash_table_insert(s, t)) {
        return true;
    }
    // mazes iba vtedy ked sa nepodarilo vlozit
    delete_hash_tabulka(s->local_table);
    free(s->meno);
    free(s);
    return false;
}
// hladanie kluca vnorenych blokoch
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
    return hash_table_lookup(meno, t);
}
// vlozenie do tabulky vstavane funkcie
void insert_included_fun(symtable *t) {
    if (t == NULL) {
        return;
    }
    Datatype *typ = malloc(sizeof(Datatype));
    if (typ == NULL) {
        return;
    }
    *typ = TYPE_ANY;
    insert_func(t, "print", TYPE_UNIT, 1, typ);
    insert_func(t, "readLine", TYPE_STRING, 0, NULL);
    insert_func(t, "readInt", TYPE_INT, 0, NULL);
    insert_func(t, "readDouble", TYPE_DOUBLE, 0, NULL);
    Datatype *typ1 = malloc(sizeof(Datatype));
    if (typ1 == NULL) {
        return;
    }
    *typ1 = TYPE_ANY;
    insert_func(t, "toStr", TYPE_STRING, 1, typ1);
    Datatype *typ2 = malloc(sizeof(Datatype));
    if (typ2 == NULL) {
        return;
    }
    *typ2 = TYPE_DOUBLE;
    insert_func(t, "toInt", TYPE_INT, 1, typ2);
    Datatype *typ3 = malloc(sizeof(Datatype));
    if (typ3 == NULL) {
        return;
    }
    *typ3 = TYPE_STRING;
    insert_func(t, "strLength", TYPE_INT, 1, typ3);
    Datatype *typ4 = malloc(sizeof(Datatype));
    if (typ4 == NULL) {
        return;
    }
    *typ4 = TYPE_INT;
    insert_func(t, "chr", TYPE_STRING, 1, typ4);
    Datatype *typ5 = malloc(sizeof(Datatype) * 2);
    if (typ5 == NULL) {
        return;
    }
    typ5[0] = TYPE_STRING;
    typ5[1] = TYPE_INT;
    insert_func(t, "ord", TYPE_INT, 2, typ5);
    Datatype *typ6 = malloc(sizeof(Datatype) * 3);
    if (typ6 == NULL) {
        return;
    }
    typ6[0] = TYPE_STRING;
    typ6[1] = TYPE_INT;
    typ6[2] = TYPE_INT;
    insert_func(t, "subStr", TYPE_STRING, 3, typ6);
}

// danie prec zanorenych premmennych v if alebo while atd.
void exit_scope(int scope_level, symtable *t) {
    if (t == NULL) {
        return;
    }
    for (int i = 0; i < t->velkost_tabulky; i++) {
        if (t->hash_table[i] == NULL || t->hash_table[i] == delete_item || t->hash_table[i]->is_function == true) {
            continue;
        }
        int level_zanorenia = atoi(t->hash_table[i]->meno);
        if (level_zanorenia == scope_level) {
            free(t->hash_table[i]->meno);
            free(t->hash_table[i]);
            t->hash_table[i] = delete_item;
            t->pocet_synbolov--;
        }
    }
    return;
}

// zmazanie daneho synbolu bud na indexe alebo dalej
bool hash_table_delete(char *s, symtable *t) {
    if (s == NULL || t == NULL)
        return false;
    unsigned int hash_hodnota = hash_funkcia(s);
    int index = hash_hodnota % t->velkost_tabulky;
    for (int i = 0; i < t->velkost_tabulky; i++) {
        int try = (i + index) % t->velkost_tabulky;
        if (t->hash_table[try] == NULL) {
            return false;
        }
        Symboldata *tmp = t->hash_table[try];
        if (t->hash_table[try] != NULL && t->hash_table[try] != delete_item && !(strcmp(t->hash_table[try]->meno, s))) {
            t->hash_table[try] = delete_item;
            t->pocet_synbolov = t->pocet_synbolov - 1;
            if (tmp->is_function && tmp->local_table != NULL) {
                delete_hash_tabulka(tmp->local_table);
            }
            free(tmp->param_type);
            free(tmp->meno);
            free(tmp);
            return true;
        }
    }
    return false;
}

// zmaze celu tabulku
void delete_hash_tabulka(symtable *t) {
    if (t == NULL) {
        return;
    }

    for (int i = 0; i < t->velkost_tabulky; i++) {
        if (t->hash_table[i] != NULL && t->hash_table[i] != delete_item) {
            if (t->hash_table[i]->is_function == true) {
                delete_hash_tabulka(t->hash_table[i]->local_table);
            }
            free(t->hash_table[i]->param_type);
            free(t->hash_table[i]->meno); // nicis meno
            free(t->hash_table[i]);       // nisic strukturu datatyp
        }
    }
    free(t->hash_table); // nicis hastable
    free(t);             // nicis ukazovatel ndo pamate na tuto tabulku
}