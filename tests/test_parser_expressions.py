#!/usr/bin/env python3
"""
Projekt: IFJ26 kompilátor
Súbor: test_parser_expressions.py
Popis: End-to-end testy overujúce prepojenie parser.c a expressions.c.
       Testuje rôzne konštrukcie IFJ26 (val/var deklarácie, priradenia,
       podmienky if/while, volania funkcií, návratové hodnoty)
       a overuje správne návratové kódy kompilátora.
"""

import subprocess
import sys
import os

COMPILER_BIN = "./compiler.exe" if os.name == "nt" else "./compiler"

# Zoznam testovacích prípadov: (Názov, IFJ26 kód, Očakávaný exit code, Popis)
TESTS = [
    # -------------------------------------------------------------
    # 1. Deklarácie premenných a výrazy (val/var)
    # -------------------------------------------------------------
    (
        "Deklarácia Int s jednoduchým sčítaním",
        """import ifj26.*
@main def run() : Unit =
    val x : Int = 10 + 20
""",
        0,
        "Výraz 10 + 20 má vyhodnotiť na Int a priradiť do x."
    ),
    (
        "Deklarácia s viacerými premennými a výrazmi",
        """import ifj26.*
@main def run() : Unit =
    val a : Int = 5
    val b : Int = a * 2 + 10
    val c : Double = b + 3.14
""",
        0,
        "Premenná b používa a; premenná c používa b s implicitnou konverziou na Double."
    ),
    (
        "Odvodenie typu výrazu (type inference bez explicitného typu)",
        """import ifj26.*
@main def run() : Unit =
    val x = 100 + 200
""",
        0,
        "Parser má prevziať typ z výrazu (TYPE_INT)."
    ),
    (
        "Konkatenácia reťazcov",
        """import ifj26.*
@main def run() : Unit =
    val s1 : String = "Ahoj "
    val s2 : String = s1 + "svet"
""",
        0,
        "Spájanie stringov pomocou operátora +."
    ),

    # -------------------------------------------------------------
    # 2. Nedeklarované premenné (Chyba 3)
    # -------------------------------------------------------------
    (
        "Nedeklarovaná premenná vo výraze",
        """import ifj26.*
@main def run() : Unit =
    val x : Int = nezname_cislo + 5
""",
        3,
        "Použitie 'nezname_cislo' musí skončiť s chybou 3 (ERROR_SEM_UNDEF)."
    ),
    (
        "Nedeklarovaná premenná v podmienke",
        """import ifj26.*
@main def run() : Unit =
    if nezname_a < 10 then
        val x : Int = 1
    else
        ()
""",
        3,
        "Nedeklarovaná premenná v if podmienke musí skončiť s chybou 3."
    ),

    # -------------------------------------------------------------
    # 3. Typové chyby vo výrazoch a priradeniach (Chyba 6)
    # -------------------------------------------------------------
    (
        "Typový nesúlad vo výraze (Int + String)",
        """import ifj26.*
@main def run() : Unit =
    val x : Int = 10 + "neplatny_string"
""",
        6,
        "Sčítanie Int + String nie je povolené -> chyba 6 (ERROR_SEM_TYPE)."
    ),
    (
        "Typový nesúlad pri priradení (Int deklarovaný, priradený String)",
        """import ifj26.*
@main def run() : Unit =
    val x : Int = "text"
""",
        6,
        "Deklarovaný Int ale výraz je String -> chyba 6 (ERROR_SEM_TYPE)."
    ),
    (
        "Unárne mínus na reťazec",
        """import ifj26.*
@main def run() : Unit =
    val s : String = "text"
    val x : String = -s
""",
        6,
        "Unárne mínus na String je sémantická chyba 6."
    ),

    # -------------------------------------------------------------
    # 4. Syntaktické chyby vo výrazoch (Chyba 2)
    # -------------------------------------------------------------
    (
        "Chýbajúci operand vo výraze",
        """import ifj26.*
@main def run() : Unit =
    val x : Int = 10 + * 20
""",
        2,
        "Dva operátory za sebou bez operandu -> chyba 2 (ERROR_SYNT)."
    ),
    (
        "Neuzavretá zátvorka vo výraze",
        """import ifj26.*
@main def run() : Unit =
    val x : Int = (10 + 20
""",
        2,
        "Chýbajúca pravá zátvorka -> chyba 2 (ERROR_SYNT)."
    ),
    (
        "Reťazenie relačných operátorov (a < b < c)",
        """import ifj26.*
@main def run() : Unit =
    val x : Int = 1 < 2 < 3
""",
        2,
        "Reťazenie relačných operátorov je syntaktická chyba 2."
    ),

    # -------------------------------------------------------------
    # 5. Podmienky if a while
    # -------------------------------------------------------------
    (
        "Relačný výraz v if podmienke",
        """import ifj26.*
@main def run() : Unit =
    val a : Int = 5
    if a < 10 then
        val b : Int = a + 1
    else
        ()
""",
        0,
        "Výraz v if podmienke (a < 10) má vyhodnotiť na Boolean."
    ),
    (
        "Zložitý aritmeticko-relačný výraz v podmienke",
        """import ifj26.*
@main def run() : Unit =
    val a : Int = 5
    val b : Double = 10.0
    if (a * 2) <= b then
        val c : Int = 1
    else
        ()
""",
        0,
        "Zložený výraz s prioritou operátorov v if podmienke."
    ),
]

def compile_project():
    """Zostaví compiler.exe ak ešte neexistuje."""
    cmd = [
        "gcc", "-std=c11", "-Wall", "-Wextra", "-pedantic",
        "main.c", "scanner.c", "stack.c", "symtable.c", "parser.c", "expressions.c", "generator.c",
        "-o", COMPILER_BIN
    ]
    print(f"Kompilujem projekt: {' '.join(cmd)}")
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("CHYBA KOMPILÁCIE:")
        print(res.stderr)
        sys.exit(1)
    print("Kompilácia úspešná!\n")

def run_test(name, code, expected_code, desc):
    """Spustí jeden test s daným kódom a overí návratový kód."""
    proc = subprocess.run(
        [COMPILER_BIN],
        input=code,
        capture_output=True,
        text=True
    )
    actual_code = proc.returncode

    passed = (actual_code == expected_code)
    status_str = "PASSED" if passed else f"FAILED (Očakávaný: {expected_code}, Skutočný: {actual_code})"
    
    print(f"[{status_str}] {name}")
    if not passed:
        print(f"   Popis: {desc}")
        if proc.stderr:
            # Vypíš posledných pár riadkov stderr
            err_lines = [l for l in proc.stderr.strip().split("\n") if not l.startswith("DEBUG:")]
            if err_lines:
                print(f"   Stderr: {err_lines[-1]}")
    return passed

def main():
    print("=" * 65)
    print("   END-TO-END TESTOVACIA SADA: PARSER + PRECEDENČNÁ ANALÝZA")
    print("=" * 65)

    compile_project()

    passed_count = 0
    total_count = len(TESTS)

    for name, code, expected_code, desc in TESTS:
        if run_test(name, code, expected_code, desc):
            passed_count += 1

    print("=" * 65)
    print(f"Výsledok: {passed_count}/{total_count} testov prešlo.")
    print("=" * 65)

if __name__ == "__main__":
    main()
