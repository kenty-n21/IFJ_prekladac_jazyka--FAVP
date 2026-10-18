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

#define velkost_tabulky 10
typedef struct {
    char meno[256];
    Datatype typ;
} Symboldata;

Symboldata *hash_table[velkost_tabulky];
// vyhlada index kde ma byt ulozena dana hodnota
int hash_funkcia(char *meno) {
    int dlzka = strlen(meno);
    int hash_hodnota = 0;
    for (int i = 0; i < dlzka; i++) {
        hash_hodnota += meno[i];
        hash_hodnota = hash_hodnota % velkost_tabulky;
    }
    return hash_hodnota;
}
// inicializuje tabulku
void init_hash_table() {
    for (int i = 0; i < velkost_tabulky; i++) {
        hash_table[i] = NULL;
    }
}
// vlozenie prvku do tabulky bud na dany index alebo na prve miesto za nim
bool hash_table_insert(Symboldata *s) {
    int index = hash_funkcia(s->meno);
    for (int i = 0; i < velkost_tabulky; i++) {
        int try = (i + index) % velkost_tabulky;
        if (hash_table[try] == NULL) {
            hash_table[try] = s;
            return true;
        }
    }
    return false;
}
// hladanie v tabulke od indexu kde ma byt a dalej
Symboldata *hash_table_lookup(char *s) {
    int index = hash_funkcia(s);
    for (int i = 0; i < velkost_tabulky; i++) {
        int try = (i + index) % velkost_tabulky;
        if (hash_table[try] != NULL && !(strcmp(hash_table[try]->meno, s))) {
            return hash_table[try];
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
