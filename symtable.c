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
    Datatype;
    bool incializovane;
} Symboldata;

Symboldata *hash_table[velkost_tabulky];

int hash_funkcia(char *meno) {
    int dlzka = strlen(meno);
    int hash_hodnota = 0;
    for (int i = 0; i < dlzka; i++) {
        hash_hodnota += meno[i];
        hash_hodnota = hash_hodnota % velkost_tabulky;
    }
    return hash_hodnota;
}

void init_hash_table() {
    for (int i = 0; i < velkost_tabulky; i++) {
        hash_table[i] = NULL;
    }
}

bool hash_table_insert(Symboldata *s) {
    if (p == false)
        return false;
    int index = hash_funkcia(s->meno);

    if (hash_table[index] != NULL) {
        return false
    }
    hash_table[index] = s;
    return true;
}
