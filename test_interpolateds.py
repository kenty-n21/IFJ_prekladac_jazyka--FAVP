#!/usr/bin/env python3
"""
Komplexna testovacia sada pre rozsirenie INTERPOLATEDS (IFJ26)
Autor: Testovaci skript pre IFJ projekt
"""

import subprocess
import os
import sys

COMPILER_PATH = "./compiler.exe" if os.name == "nt" else "./compiler"

# Farby pre terminal
GREEN = "\033[92m"
RED = "\033[91m"
YELLOW = "\033[93m"
CYAN = "\033[96m"
BOLD = "\033[1m"
RESET = "\033[0m"

# Definicia testov:
# (nazov, zdrojovy_kod, ocakavany_exit_code, ocakavany_substring_v_generovanom_kode, popis)
TESTS = [
    # =========================================================================
    # KATEGORIA 1: Obycajny text a prazdne retazce v s"..."
    # =========================================================================
    (
        "text_empty_string",
        """import ifj26.*
@main def run(): Unit =
    val s = s""
    print(s)
""",
        0,
        "string@",
        "Prazdny interpolovany retazec s\"\""
    ),
    (
        "text_plain_string",
        """import ifj26.*
@main def run(): Unit =
    val s = s"Ahoj svet!"
    print(s)
""",
        0,
        "Ahoj\\032svet!",
        "Interpolovany retazec bez premennych (iba text)"
    ),
    (
        "text_spaces_and_special_chars",
        """import ifj26.*
@main def run(): Unit =
    val s = s"Jeden dva   tri... !?-"
    print(s)
""",
        0,
        "CONCAT",
        "Retazec s viacerymi medzerami a interpunkciou"
    ),

    # =========================================================================
    # KATEGORIA 2: Escapovanie znaku dolar ($$)
    # =========================================================================
    (
        "escape_dollar_simple",
        """import ifj26.*
@main def run(): Unit =
    val s = s"Cena je $$100"
    print(s)
""",
        0,
        "\\036",
        "Escapovanie dolara $$ v texte"
    ),
    (
        "escape_dollar_multiple",
        """import ifj26.*
@main def run(): Unit =
    val s = s"$$$$"
    print(s)
""",
        0,
        "\\036",
        "Viacnasobne escapovany dolar ($$$$ -> $$)"
    ),
    (
        "escape_dollar_at_boundaries",
        """import ifj26.*
@main def run(): Unit =
    val s = s"$$zaciatok a koniec$$"
    print(s)
""",
        0,
        "\\036",
        "Escapovany dolar na zaciatku a konci retazca"
    ),
    (
        "escape_dollar_before_variable",
        """import ifj26.*
@main def run(): Unit =
    val cena = 99
    val s = s"$$$cena"
    print(s)
""",
        0,
        "\\036",
        "Dolar hned pred premennou ($$$cena -> $99)"
    ),

    # =========================================================================
    # KATEGORIA 3: Interpolacia jednoduchej premennej ($var)
    # =========================================================================
    (
        "var_string",
        """import ifj26.*
@main def run(): Unit =
    val name = "Svet"
    val s = s"Ahoj $name!"
    print(s)
""",
        0,
        "CALL $$toStr",
        "Interpolacia reťazcovej premennej ($name)"
    ),
    (
        "var_int",
        """import ifj26.*
@main def run(): Unit =
    val age = 21
    val s = s"Vek: $age"
    print(s)
""",
        0,
        "CALL $$toStr",
        "Interpolacia celeho cisla ($age)"
    ),
    (
        "var_double",
        """import ifj26.*
@main def run(): Unit =
    val pi = 3.14
    val s = s"Hodnota PI je $pi"
    print(s)
""",
        0,
        "CALL $$toStr",
        "Interpolacia desatinneho cisla ($pi)"
    ),
    (
        "var_at_start",
        """import ifj26.*
@main def run(): Unit =
    val user = "Admin"
    val s = s"$user prihlaseny"
    print(s)
""",
        0,
        "CALL $$toStr",
        "Premenna na uplnom zaciatku retazca"
    ),
    (
        "var_at_end",
        """import ifj26.*
@main def run(): Unit =
    val count = 42
    val s = s"Pocet: $count"
    print(s)
""",
        0,
        "CALL $$toStr",
        "Premenna na uplnom konci retazca"
    ),
    (
        "var_only",
        """import ifj26.*
@main def run(): Unit =
    val msg = "iba premenna"
    val s = s"$msg"
    print(s)
""",
        0,
        "CALL $$toStr",
        "Retazec tvoreny iba jednou premennou s\"$msg\""
    ),
    (
        "var_multiple_consecutive",
        """import ifj26.*
@main def run(): Unit =
    val a = 1
    val b = 2
    val c = 3
    val s = s"$a $b $c"
    print(s)
""",
        0,
        "CALL $$toStr",
        "Viacero premennych v jednom retazci oddelenych medzerami"
    ),
    (
        "var_with_underscore",
        """import ifj26.*
@main def run(): Unit =
    val my_long_var_name = 99
    val s = s"Vysledok: $my_long_var_name"
    print(s)
""",
        0,
        "CALL $$toStr",
        "Premenna s podtrznikmi v nazve"
    ),
    (
        "var_alphanumeric_with_digits",
        """import ifj26.*
@main def run(): Unit =
    val var1 = "hodnota1"
    val val2 = "hodnota2"
    val s = s"$var1 a $val2"
    print(s)
""",
        0,
        "CALL $$toStr",
        "Premenne obsahujuce cislice v nazve ($var1, $val2)"
    ),
    (
        "var_next_to_punctuation",
        """import ifj26.*
@main def run(): Unit =
    val x = 10
    val y = 20
    val s = s"Bod: ($x,$y)!"
    print(s)
""",
        0,
        "CALL $$toStr",
        "Premenne tesne susediace s interpunkciou ($x,$y)!"
    ),

    # =========================================================================
    # KATEGORIA 4: Interpolacia zlozeneho vyrazu (${...})
    # =========================================================================
    (
        "expr_addition",
        """import ifj26.*
@main def run(): Unit =
    val n = 10
    val s = s"Dalsie cislo je ${n + 1}"
    print(s)
""",
        0,
        "ADDS",
        "Scitanie v zlozenej zatvorke ${n + 1}"
    ),
    (
        "expr_subtraction",
        """import ifj26.*
@main def run(): Unit =
    val a = 20
    val b = 5
    val s = s"Rozdiel: ${a - b}"
    print(s)
""",
        0,
        "SUBS",
        "Odcitanie dvoch premennych v ${a - b}"
    ),
    (
        "expr_multiplication",
        """import ifj26.*
@main def run(): Unit =
    val width = 4
    val height = 5
    val s = s"Obsah: ${width * height}"
    print(s)
""",
        0,
        "MULS",
        "Nasobenie v ${width * height}"
    ),
    (
        "expr_division",
        """import ifj26.*
@main def run(): Unit =
    val total = 100
    val parts = 4
    val s = s"Cast: ${total / parts}"
    print(s)
""",
        0,
        "IDIVS",
        "Delenie v ${total / parts}"
    ),
    (
        "expr_single_var_in_braces",
        """import ifj26.*
@main def run(): Unit =
    val x = 7
    val s = s"Hodnota je ${x}"
    print(s)
""",
        0,
        "CALL $$toStr",
        "Samotna premenna v zatvorkach ${x}"
    ),
    (
        "expr_single_literal_in_braces",
        """import ifj26.*
@main def run(): Unit =
    val s = s"Cislo: ${100}"
    print(s)
""",
        0,
        "int@100",
        "Ciselny literal v zatvorkach ${100}"
    ),
    (
        "expr_with_spaces_inside_braces",
        """import ifj26.*
@main def run(): Unit =
    val x = 2
    val s = s"Vysledok: ${   x + 8   }"
    print(s)
""",
        0,
        "ADDS",
        "Vyraz s nadbytocnymi medzerami vo vnutri zatvoriek"
    ),
    (
        "expr_two_variables_add",
        """import ifj26.*
@main def run(): Unit =
    val a = 15
    val b = 25
    val s = s"Spolu: ${a + b}"
    print(s)
""",
        0,
        "ADDS",
        "Scitanie dvoch premennych v ${a + b}"
    ),
    (
        "expr_two_variables_mul",
        """import ifj26.*
@main def run(): Unit =
    val x = 6
    val y = 7
    val s = s"Nasobok: ${x * y}"
    print(s)
""",
        0,
        "MULS",
        "Nasobenie dvoch premennych v ${x * y}"
    ),

    # =========================================================================
    # KATEGORIA 5: Kombinacie ($var, ${expr}, $$, text)
    # =========================================================================
    (
        "combo_all_elements",
        """import ifj26.*
@main def run(): Unit =
    val item = "Kniha"
    val price = 25
    val s = s"Polozka: $item stoji $price $$ a so zlavou ${price - 5} $$."
    print(s)
""",
        0,
        "CONCAT",
        "Kompletna kombinacia: text, $var, $$, a ${expr}"
    ),
    (
        "combo_repeated_vars",
        """import ifj26.*
@main def run(): Unit =
    val x = 10
    val s = s"$x + $x = ${x + x}"
    print(s)
""",
        0,
        "ADDS",
        "Opakovana rovnaka premenna $x + $x = ${x + x}"
    ),

    # =========================================================================
    # KATEGORIA 6: Kontexty pouzitia v jazyku IFJ26
    # =========================================================================
    (
        "ctx_val_declaration",
        """import ifj26.*
@main def run(): Unit =
    val msg = s"Vytvorene cez val"
    print(msg)
""",
        0,
        "CONCAT",
        "Pouzitie v deklaracii nemenneho 'val'"
    ),
    (
        "ctx_var_assignment",
        """import ifj26.*
@main def run(): Unit =
    var msg: String = ""
    msg = s"Zmenena hodnota var"
    print(msg)
""",
        0,
        "CONCAT",
        "Pouzitie pri priradeni do 'var'"
    ),
    (
        "ctx_function_argument",
        """import ifj26.*
@main def run(): Unit =
    val x = 55
    print(s"Priamo vo volani print: $x")
""",
        0,
        "CALL $$toStr",
        "Pouzitie interpolovaneho retazca priamo ako argument funkcie print()"
    ),
    (
        "ctx_function_return",
        """import ifj26.*
def format_greeting(who: String): String =
    return s"Srdecne ta vitam, $who!"

@main def run(): Unit =
    val res = format_greeting("Lukas")
    print(res)
""",
        0,
        "CALL $$toStr",
        "Pouzitie interpolovaneho retazca ako navratova hodnota z funkcie"
    ),
    (
        "ctx_string_concatenation_with_plus",
        """import ifj26.*
@main def run(): Unit =
    val name = "Eva"
    val full = "Ahoj " + s"$name, ako sa mas?"
    print(full)
""",
        0,
        "CONCAT",
        "Spajanie interpolovaneho retazca cez operator +"
    ),
    (
        "ctx_in_while_loop",
        """import ifj26.*
@main def run(): Unit =
    var i = 0
    while (i < 3) do
        print(s"Krok cislo $i")
        i = i + 1
""",
        0,
        "CALL $$toStr",
        "Pouzitie interpolovaneho retazca vo vnutri cyklu while"
    ),
    (
        "ctx_in_if_else",
        """import ifj26.*
@main def run(): Unit =
    val role = 1
    if role == 1 then
        print(s"Status: admin pristup")
    else
        print(s"Status: host")
""",
        0,
        "CONCAT",
        "Pouzitie interpolovaneho retazca v oboch vetvach if-else"
    ),
    (
        "ctx_chained_concatenations",
        """import ifj26.*
@main def run(): Unit =
    val a = "jeden"
    val b = "dva"
    val res = s"A: $a " + s"B: $b"
    print(res)
""",
        0,
        "CONCAT",
        "Retazenie dvoch interpolovanych retazcov cez operator +"
    ),

    # =========================================================================
    # KATEGORIA 7: Chybove stavy (Syntakticke chyby - exit code 2)
    # =========================================================================
    (
        "err_unclosed_brace",
        """import ifj26.*
@main def run(): Unit =
    val x = 10
    val s = s"Chyba: ${x + 1"
    print(s)
""",
        2,
        None,
        "Chyba syntaxe: neukoncena zlozka ${... v retazci (chyba 2)"
    ),
    (
        "err_lone_dollar_at_end",
        """import ifj26.*
@main def run(): Unit =
    val s = s"Text konci dolarm $"
    print(s)
""",
        2,
        None,
        "Chyba syntaxe: osamoteny dolar na konci retazca"
    ),
    (
        "err_invalid_char_after_dollar",
        """import ifj26.*
@main def run(): Unit =
    val s = s"Cena je $#100"
    print(s)
""",
        2,
        None,
        "Chyba syntaxe: neplatny znak '#' po dolari"
    ),
    (
        "err_dollar_followed_by_space",
        """import ifj26.*
@main def run(): Unit =
    val s = s"Ahoj $ svet"
    print(s)
""",
        2,
        None,
        "Chyba syntaxe: medzera bezprostredne po dolari"
    ),
]


def run_test(name, code, expected_code, expected_substr, desc):
    try:
        proc = subprocess.Popen(
            [COMPILER_PATH],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        stdout, stderr = proc.communicate(input=code, timeout=5)
        exit_code = proc.returncode

        # Overenie exit kodu
        if exit_code != expected_code:
            return False, f"Ocakavany exit code {expected_code}, ale dostal som {exit_code}. (stderr: {stderr.strip()})"

        # Ak sa ocakaval exit code 0, overime ci vystup obsahuje pozadovane instrukcie
        if expected_code == 0 and expected_substr:
            if expected_substr not in stdout:
                return False, f"Vystup mezikodu neobsahuje ocakavanu instrukciu '{expected_substr}'."

        return True, "OK"
    except subprocess.TimeoutExpired:
        proc.kill()
        return False, "TIMEOUT (program zamrzol alebo bezi nekonecne)"
    except Exception as e:
        return False, f"Chyba pri spusteni: {e}"


def main():
    print(f"\n{BOLD}{CYAN}{'='*75}{RESET}")
    print(f"{BOLD}{CYAN}   KOMPLEXNA TESTOVACIA SADA PRE ROZSIRENIE INTERPOLATEDS (IFJ26)   {RESET}")
    print(f"{BOLD}{CYAN}{'='*75}{RESET}\n")

    if not os.path.exists(COMPILER_PATH):
        print(f"{RED}Chyba: Spustitelny subor prekladaca '{COMPILER_PATH}' neexistuje!{RESET}")
        print("Najprv preloz projekt (napr. cez 'make' na Ubuntu alebo 'mingw32-make' na Windows).")
        sys.exit(1)

    passed = 0
    failed = 0

    current_cat = None
    for name, code, expected_code, expected_substr, desc in TESTS:
        cat = name.split("_")[0]
        if cat != current_cat:
            current_cat = cat
            print(f"\n{BOLD}--> Skupina testov: {cat.upper()}{RESET}")

        success, msg = run_test(name, code, expected_code, expected_substr, desc)

        if success:
            passed += 1
            print(f"  [{GREEN}PASS{RESET}] {name:<35} - {desc}")
        else:
            failed += 1
            print(f"  [{RED}FAIL{RESET}] {name:<35} - {desc}")
            print(f"         {YELLOW}-> Dovod: {msg}{RESET}")

    total = passed + failed
    print(f"\n{BOLD}{CYAN}{'='*75}{RESET}")
    print(f"{BOLD}VYSLEDOK: Celkovo: {total} | {GREEN}Preslo: {passed}{RESET} | {RED}Zlyhalo: {failed}{RESET}")

    if failed == 0:
        print(f"{BOLD}{GREEN}Vsetky testy pre INTERPOLATEDS uspesne presli!{RESET}")
    else:
        print(f"{BOLD}{RED}Niektore testy zlyhali, pozri dovod vyssie.{RESET}")
    print(f"{BOLD}{CYAN}{'='*75}{RESET}\n")

    sys.exit(0 if failed == 0 else 1)


if __name__ == "__main__":
    main()
