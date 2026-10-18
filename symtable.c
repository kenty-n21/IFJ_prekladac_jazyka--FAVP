#include <ctype.h>
#include <stdbool.h>
#include <stdinit.h>
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
    char *meno,
        Datatype,
        bool incializovane,
} Symboldata;
