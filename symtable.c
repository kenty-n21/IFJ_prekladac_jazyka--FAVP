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

typedef struct symtable symtable;

typedef struct {
    char *meno;
    Datatype typ;
    bool is_function;
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

#define delete_item ((Symboldata *)-1)

bool hash_table_insert(Symboldata *s, symtable *t);
void delete_hash_tabulka(symtable *t);

symtable *resize_hash_table(symtable *ptr) {
    if (ptr == NULL) {
        return NULL;
    }
    int nova_velkost = ptr->velkost_tabulky * 2;
    int stara_velkost = ptr->velkost_tabulky;
    Symboldata **newptr = malloc(sizeof(Symboldata *) * nova_velkost);
    if (newptr == NULL) {
        return NULL;
    }
    for (int l = 0; l < nova_velkost; l++) {
        newptr[l] = NULL;
    }

    Symboldata **stare_pole = ptr->hash_table;
    ptr->hash_table = newptr;
    ptr->pocet_synbolov = 0;
    ptr->velkost_tabulky = nova_velkost;

    for (int i = 0; i < stara_velkost; i++) {
        if (stare_pole[i] != NULL && stare_pole[i] != delete_item) {
            hash_table_insert(stare_pole[i], ptr);
        }
    }

    free(stare_pole);
    return ptr;
}

unsigned int hash_funkcia(char *meno) {
    if (meno == NULL)
        return 0;
    int dlzka = strlen(meno);
    unsigned int hash_hodnota = 0;
    for (int i = 0; i < dlzka; i++) {
        hash_hodnota = hash_hodnota * 37 + (unsigned char)meno[i];
    }
    return hash_hodnota;
}

symtable *init_hash_table() {
    symtable *t = malloc(sizeof(symtable));
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
            t->pocet_synbolov++;
            return true;
        }
    }
    return false;
}

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
        if (t->hash_table[try] != delete_item && strcmp(t->hash_table[try]->meno, s) == 0) {
            return t->hash_table[try];
        }
    }
    return NULL;
}

char *make_compose_key(int scope_level, char *meno) {
    if (meno == NULL)
        return NULL;
    int len = snprintf(NULL, 0, "%d$%s", scope_level, meno);
    char *key = malloc(len + 1);
    if (key == NULL) {
        return NULL;
    }
    snprintf(key, len + 1, "%d$%s", scope_level, meno);
    return key;
}

bool insert_var(symtable *t, char *meno, Datatype typ, int scope_level) {
    if (t == NULL || meno == NULL) {
        return false;
    }
    char *kluc = make_compose_key(scope_level, meno);
    if (kluc == NULL) {
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
    s->define = true;

    if (hash_table_insert(s, t)) {
        return true;
    }
    free(kluc);
    free(s);
    return false;
}

bool insert_func(symtable *t, char *meno, Datatype return_typ, int param_count, Datatype *param_type) {
    if (t == NULL || meno == NULL) {
        return false;
    }
    if (hash_table_lookup(meno, t) != NULL) {
        return false;
    }

    Symboldata *s = malloc(sizeof(Symboldata));
    if (s == NULL) {
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
    s->define = true;

    if (hash_table_insert(s, t)) {
        return true;
    }

    delete_hash_tabulka(s->local_table);
    free(s->meno);
    free(s);
    return false;
}

Symboldata *lookup_var(int scope_level, char *meno, symtable *t) {
    if (t == NULL || meno == NULL) {
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

void exit_scope(int scope_level, symtable *t) {
    if (t == NULL) {
        return;
    }
    for (int i = 0; i < t->velkost_tabulky; i++) {
        if (t->hash_table[i] == NULL || t->hash_table[i] == delete_item || t->hash_table[i]->is_function) {
            continue;
        }

        // Bezpečná kontrola: kľúč musí obsahovať znak '$'
        char *dollar = strchr(t->hash_table[i]->meno, '$');
        if (dollar != NULL) {
            int level_zanorenia = atoi(t->hash_table[i]->meno);
            if (level_zanorenia == scope_level) {
                free(t->hash_table[i]->meno);
                free(t->hash_table[i]);
                t->hash_table[i] = delete_item;
                t->pocet_synbolov--;
            }
        }
    }
}

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
        if (t->hash_table[try] != delete_item && strcmp(t->hash_table[try]->meno, s) == 0) {
            Symboldata *tmp = t->hash_table[try];
            t->hash_table[try] = delete_item;
            t->pocet_synbolov--;
            return tmp;
        }
    }
    return NULL;
}

void delete_hash_tabulka(symtable *t) {
    if (t == NULL) {
        return;
    }

    for (int i = 0; i < t->velkost_tabulky; i++) {
        if (t->hash_table[i] != NULL && t->hash_table[i] != delete_item) {
            if (t->hash_table[i]->is_function && t->hash_table[i]->local_table != NULL) {
                delete_hash_tabulka(t->hash_table[i]->local_table);
            }
            free(t->hash_table[i]->meno);
            free(t->hash_table[i]);
        }
    }
    free(t->hash_table);
    free(t);
}