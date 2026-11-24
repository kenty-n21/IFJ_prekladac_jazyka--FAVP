#!/usr/bin/env python3
"""
Testovacia sada pre rozsirenie VALMEM (IFJ26)
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

# Definicia testov
# (nazov, zdrojovy_kod, ocakavany_exit_code, ocakavany_substring_v_generovanom_kode, popis)
TESTS = [
    # =========================================================================
    # KATEGORIA 1: .length (String -> Int)
    # =========================================================================
    (
        "length_literal",
        """import ifj26.*
@main def run(): Unit =
    val len = "ahoj".length
    print(len)
""",
        0,
        "CALL $$strLength",
        "Zistenie dlzky retazcoveho literalu"
    ),
    (
        "length_empty_string",
        """import ifj26.*
@main def run(): Unit =
    val len = "".length
    print(len)
""",
        0,
        "CALL $$strLength",
        "Zistenie dlzky prazdneho retazca"
    ),
    (
        "length_variable",
        """import ifj26.*
@main def run(): Unit =
    val s = "testovaci retazec"
    val len = s.length
    print(len)
""",
        0,
        "CALL $$strLength",
        "Zistenie dlzky retazca z premennej"
    ),
    (
        "length_in_expression",
        """import ifj26.*
@main def run(): Unit =
    val res = "a".length + "bb".length * 3
    print(res)
""",
        0,
        "CALL $$strLength",
        "Pouzitie .length vo vnutri aritmetickeho vyrazu"
    ),
    (
        "length_multiple_in_subtraction",
        """import ifj26.*
@main def run(): Unit =
    val s1 = "dlhy retazec"
    val s2 = "kratky"
    val diff = s1.length - s2.length
    print(diff)
""",
        0,
        "CALL $$strLength",
        "Rozdiel dlzok dvoch retazcov cez .length"
    ),

    # =========================================================================
    # KATEGORIA 2: .toInt (Double -> Int)
    # =========================================================================
    (
        "toInt_positive",
        """import ifj26.*
@main def run(): Unit =
    val d = 5.78
    val i = d.toInt
    print(i)
""",
        0,
        "CALL $$toInt",
        "Prevod kladneho Double na Int"
    ),
    (
        "toInt_negative",
        """import ifj26.*
@main def run(): Unit =
    val d = -42.99
    val i = d.toInt
    print(i)
""",
        0,
        "CALL $$toInt",
        "Prevod zaporneho Double na Int"
    ),
    (
        "toInt_exact_zero_fraction",
        """import ifj26.*
@main def run(): Unit =
    val d = 100.0
    val i = d.toInt
    print(i)
""",
        0,
        "CALL $$toInt",
        "Prevod Double s nulovou desatinnou castou"
    ),
    (
        "toInt_zero_double",
        """import ifj26.*
@main def run(): Unit =
    val d = 0.0
    val i = d.toInt
    print(i)
""",
        0,
        "CALL $$toInt",
        "Prevod 0.0 na Int"
    ),

    # =========================================================================
    # KATEGORIA 3: .floor (Double -> Double)
    # =========================================================================
    (
        "floor_positive",
        """import ifj26.*
@main def run(): Unit =
    val d = 5.8
    val f = d.floor
    print(f)
""",
        0,
        "CALL $$floor",
        "Floor na kladnom Double (5.8 -> 5.0)"
    ),
    (
        "floor_negative",
        """import ifj26.*
@main def run(): Unit =
    val d = -5.8
    val f = d.floor
    print(f)
""",
        0,
        "CALL $$floor",
        "Floor na zapornom Double (-5.8 -> -6.0)"
    ),
    (
        "floor_exact_negative",
        """import ifj26.*
@main def run(): Unit =
    val d = -5.0
    val f = d.floor
    print(f)
""",
        0,
        "CALL $$floor",
        "Floor na zapornom celom cisle (-5.0 -> -5.0)"
    ),
    (
        "floor_zero",
        """import ifj26.*
@main def run(): Unit =
    val d = 0.0
    val f = d.floor
    print(f)
""",
        0,
        "CALL $$floor",
        "Floor na hodnote 0.0"
    ),

    # =========================================================================
    # KATEGORIA 4: .toString (Any -> String)
    # =========================================================================
    (
        "toString_from_int_var",
        """import ifj26.*
@main def run(): Unit =
    val n = 42
    val s = n.toString
    print(s)
""",
        0,
        "CALL $$toStr",
        "Prevod Int na String cez .toString"
    ),
    (
        "toString_from_int_literal",
        """import ifj26.*
@main def run(): Unit =
    val s = 5.toString
    print(s)
""",
        0,
        "CALL $$toStr",
        "Prevod Int literalu na String (5.toString)"
    ),
    (
        "toString_from_double",
        """import ifj26.*
@main def run(): Unit =
    val d = 3.14159
    val s = d.toString
    print(s)
""",
        0,
        "CALL $$toStr",
        "Prevod Double na String cez .toString"
    ),
    (
        "toString_from_string",
        """import ifj26.*
@main def run(): Unit =
    val s1 = "ahoj"
    val s2 = s1.toString
    print(s2)
""",
        0,
        "CALL $$toStr",
        "Prevod String na String cez .toString"
    ),
    (
        "toString_from_empty_string",
        """import ifj26.*
@main def run(): Unit =
    val s = "".toString
    print(s)
""",
        0,
        "CALL $$toStr",
        "Prevod prazdneho String na String"
    ),

    # =========================================================================
    # KATEGORIA 5: Retazenie (Chaining)
    # =========================================================================
    (
        "chain_floor_toInt",
        """import ifj26.*
@main def run(): Unit =
    val d = -5.7
    val res = d.floor.toInt
    print(res)
""",
        0,
        "CALL $$floor",
        "Retazenie d.floor.toInt"
    ),
    (
        "chain_length_toString",
        """import ifj26.*
@main def run(): Unit =
    val s = "programovanie"
    val res = s.length.toString
    print(res)
""",
        0,
        "CALL $$strLength",
        "Retazenie s.length.toString"
    ),
    (
        "chain_floor_toInt_toString",
        """import ifj26.*
@main def run(): Unit =
    val d = 12.34
    val res = d.floor.toInt.toString
    print(res)
""",
        0,
        "CALL $$floor",
        "Trojite retazenie d.floor.toInt.toString"
    ),
    (
        "chain_floor_repeated",
        """import ifj26.*
@main def run(): Unit =
    val d = 8.9
    val res = d.floor.floor.floor
    print(res)
""",
        0,
        "CALL $$floor",
        "Viacnasobne opakovane volanie .floor"
    ),

    # =========================================================================
    # KATEGORIA 6: Cleny v riadiacich strukturach a funkciach
    # =========================================================================
    (
        "member_in_if_condition",
        """import ifj26.*
@main def run(): Unit =
    val s = "test"
    if s.length > 0 then
        print("neprazdny")
    else
        print("prazdny")
""",
        0,
        "CALL $$strLength",
        "Pouzitie .length priamo v podmienke if"
    ),
    (
        "member_in_while_condition",
        """import ifj26.*
@main def run(): Unit =
    var d = 3.5
    while d.floor > 0.0 do
        d = d - 1.0
""",
        0,
        "CALL $$floor",
        "Pouzitie .floor v podmienke while"
    ),
    (
        "member_in_return_stmt",
        """import ifj26.*
def getLen(str: String): Int =
    return str.length

@main def run(): Unit =
    val len = getLen("ahoj")
    print(len)
""",
        0,
        "CALL $$strLength",
        "Pouzitie clena v prikaze return"
    ),

    # =========================================================================
    # KATEGORIA 7: Chybove stavy - Syntakticka chyba (Exit code 2)
    # =========================================================================
    (
        "err_synt_parens_length",
        """import ifj26.*
@main def run(): Unit =
    val x = "ahoj".length()
""",
        2,
        None,
        "Zakazane prazdne zatvorky: .length()"
    ),
    (
        "err_synt_parens_toInt",
        """import ifj26.*
@main def run(): Unit =
    val d = 5.5
    val x = d.toInt()
""",
        2,
        None,
        "Zakazane prazdne zatvorky: .toInt()"
    ),
    (
        "err_synt_parens_with_args",
        """import ifj26.*
@main def run(): Unit =
    val x = "ahoj".length(10)
""",
        2,
        None,
        "Zakazane zatvorky s argumentom: .length(10)"
    ),
    (
        "err_synt_trailing_dot",
        """import ifj26.*
@main def run(): Unit =
    val x = "ahoj".
""",
        2,
        None,
        "Bodka na konci bez nazvu clena: 'ahoj'."
    ),
    (
        "err_synt_double_dot",
        """import ifj26.*
@main def run(): Unit =
    val x = "ahoj"..length
""",
        2,
        None,
        "Dvojita bodka: 'ahoj'..length"
    ),
    (
        "err_synt_keyword_as_member",
        """import ifj26.*
@main def run(): Unit =
    val x = "ahoj".if
""",
        2,
        None,
        "Klucove slovo na mieste clena: 'ahoj'.if"
    ),

    # =========================================================================
    # KATEGORIA 8: Chybove stavy - Typova nekompatibilita (Exit code 6)
    # =========================================================================
    (
        "err_type_length_on_int",
        """import ifj26.*
@main def run(): Unit =
    val n = 123
    val x = n.length
""",
        6,
        None,
        "Typova chyba: .length na Int"
    ),
    (
        "err_type_length_on_double",
        """import ifj26.*
@main def run(): Unit =
    val d = 12.34
    val x = d.length
""",
        6,
        None,
        "Typova chyba: .length na Double"
    ),
    (
        "err_type_toInt_on_string",
        """import ifj26.*
@main def run(): Unit =
    val s = "123"
    val x = s.toInt
""",
        6,
        None,
        "Typova chyba: .toInt na String"
    ),
    (
        "err_type_toInt_on_int",
        """import ifj26.*
@main def run(): Unit =
    val n = 50
    val x = n.toInt
""",
        6,
        None,
        "Typova chyba: .toInt na Int (toInt je len pre Double)"
    ),
    (
        "err_type_floor_on_string",
        """import ifj26.*
@main def run(): Unit =
    val s = "ahoj"
    val x = s.floor
""",
        6,
        None,
        "Typova chyba: .floor na String"
    ),
    (
        "err_type_floor_on_int",
        """import ifj26.*
@main def run(): Unit =
    val n = 10
    val x = n.floor
""",
        6,
        None,
        "Typova chyba: .floor na Int (floor je len pre Double)"
    ),
    (
        "err_type_chain_mismatch",
        """import ifj26.*
@main def run(): Unit =
    val s = "ahoj"
    val x = s.length.floor
""",
        6,
        None,
        "Typova chyba pri retazeni: s.length vrati Int, na ktorom nie je floor"
    ),
    (
        "err_type_assign_floor_to_int",
        """import ifj26.*
@main def run(): Unit =
    val d = 5.5
    val x: Int = d.floor
""",
        6,
        None,
        "Typova chyba: priradenie Double (vysledok d.floor) do premennej typu Int"
    ),

    # =========================================================================
    # KATEGORIA 9: Chybove stavy - Nedefinovany clen / Case-sensitivity (Exit code 3)
    # =========================================================================
    (
        "err_undef_member_size",
        """import ifj26.*
@main def run(): Unit =
    val s = "ahoj"
    val x = s.size
""",
        3,
        None,
        "Neznamy clen: s.size namiesto length"
    ),
    (
        "err_undef_member_round",
        """import ifj26.*
@main def run(): Unit =
    val d = 5.5
    val x = d.round
""",
        3,
        None,
        "Neznamy clen: d.round (v IFJ26 neexistuje)"
    ),
    (
        "err_undef_member_arbitrary",
        """import ifj26.*
@main def run(): Unit =
    val x = 10
    val y = x.nejakaMetoda
""",
        3,
        None,
        "Neznamy clen: x.nejakaMetoda"
    ),
    (
        "err_undef_case_sensitive_length",
        """import ifj26.*
@main def run(): Unit =
    val s = "ahoj"
    val x = s.Length
""",
        3,
        None,
        "Case-sensitivity: s.Length s velkym L je neznamy clen"
    ),
    (
        "err_undef_case_sensitive_toint",
        """import ifj26.*
@main def run(): Unit =
    val d = 4.2
    val x = d.ToInt
""",
        3,
        None,
        "Case-sensitivity: d.ToInt s velkym T je neznamy clen"
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
    print(f"\n{BOLD}{CYAN}{'='*70}{RESET}")
    print(f"{BOLD}{CYAN}   KOMPLEXNA TESTOVACIA SADA PRE VALMEM (IFJ26)   {RESET}")
    print(f"{BOLD}{CYAN}{'='*70}{RESET}\n")

    if not os.path.exists(COMPILER_PATH):
        print(f"{RED}Chyba: Spustitelny subor prekladaca '{COMPILER_PATH}' neexistuje!{RESET}")
        print("Najprv preloz projekt (napr. cez make alebo mingw32-make).")
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
    print(f"\n{BOLD}{CYAN}{'='*70}{RESET}")
    print(f"{BOLD}VYSLEDOK: Celkovo: {total} | {GREEN}Preslo: {passed}{RESET} | {RED}Zlyhalo: {failed}{RESET}")

    if failed == 0:
        print(f"{BOLD}{GREEN}Vsetky testy pre VALMEM uspesne presli! Gratulujem!{RESET}")
    else:
        print(f"{BOLD}{RED}Niektore testy zlyhali, pozri dovod vyssie.{RESET}")
    print(f"{BOLD}{CYAN}{'='*70}{RESET}\n")

    sys.exit(0 if failed == 0 else 1)


if __name__ == "__main__":
    main()
