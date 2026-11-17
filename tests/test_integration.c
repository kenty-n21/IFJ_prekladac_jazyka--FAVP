#include <stdio.h>
#include "scanner.h"
#include "parser.h"
#include "generator.h"

int main(void) {
    scanner_init();

    // 1. Spustenie syntaktickej a semantickej analyzy (pouziva expressions.c)
    int result = parser_program();
    if (result != 0) {
        fprintf(stderr, "Analyza zlyhala s chybou: %d\n", result);
        return result;
    }

    // 2. Prevzatie hotoveho AST z parsera a vygenerovanie IFJcode26
    AstProgram *prog = parser_get_program();
    if (prog != NULL) {
        generate_program(prog);
    }

    return 0;
}