#!/usr/bin/env python3
"""
Komplexný integračný a hraničný testovací skript pre IFJ26 (základné zadanie).
Použitie:
    python3 test_ifj26.py ./ifj26 ./ic26int
    python3 test_ifj26.py ./ifj26 ./ic26int -v
    python3 test_ifj26.py ./ifj26 ./ic26int -f runtime
"""

import argparse
import ctypes
import os
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from typing import Optional


def c_hex_float(val: float) -> str:
    """Vráti presný formát C printf('%a', val), ktorý používa instrukcia WRITE."""
    try:
        libc = ctypes.CDLL(None)
        buf = ctypes.create_string_buffer(64)
        libc.snprintf(buf, 64, b"%a", ctypes.c_double(val))
        return buf.value.decode("ascii")
    except Exception:
        return float(val).hex()


@dataclass
class TestCase:
    name: str
    category: str
    code: str
    expected_compile_rc: int = 0
    expected_runtime_rc: Optional[int] = 0
    stdin_data: str = ""
    expected_stdout: Optional[str] = None
    note: str = ""


TESTS: list[TestCase] = [
    # =========================================================================
    # 1. END-TO-END: ARITMETIKA, PRIORITY, IMPLICITNÉ KONVERZIE, REŤAZCE
    # =========================================================================
    TestCase(
        name="e2e_arith_precedence_and_div",
        category="e2e_expr",
        note="Priorita operátorov, celočíselné delenie k nule (-5/2 == -2), implicitná konverzia Int->Double",
        code="""\
import ifj26.*

@main def main(): Unit =
    val a = 10 + 3 * 4 - 6 / 2
    val b = -5 / 2
    val c = 5 / -2
    val d = -5 / -2
    val e = -(2 + 3) * -4
    val f = 5 + 2.5 * 2
    val g = 5 / 2.0
    val fs = toStr(f)
    val gs = toStr(g)
    print(a)
    print("|")
    print(b)
    print("|")
    print(c)
    print("|")
    print(d)
    print("|")
    print(e)
    print("|")
    print(fs)
    print("|")
    print(gs)
""",
        expected_stdout="19|-2|-2|2|20|10.00|2.50",
    ),
    TestCase(
        name="e2e_relational_and_strings",
        category="e2e_expr",
        note="Relačné operátory nad Int, Double (s konverziou) a lexikografické porovnanie String",
        code="""\
import ifj26.*

def check(cond_id: Int): Unit =
    print(cond_id)

@main def main(): Unit =
    if 5 < 5.1 then
        check(1)
    else
        check(0)
    if 5 == 5.0 then
        check(1)
    else
        check(0)
    if 5 != 5.0 then
        check(0)
    else
        check(1)
    if "abc" < "abd" then
        check(1)
    else
        check(0)
    if "abc" >= "abc" then
        check(1)
    else
        check(0)
    val s = "ahoj" + " " + "svet"
    print("|")
    print(s)
""",
        expected_stdout="11111|ahoj svet",
    ),
    TestCase(
        name="e2e_print_double_hex",
        category="e2e_expr",
        note="print(Double) vypisuje v hexadecimálnom formáte %a na rozdiel od toStr (%.2f)",
        code="""\
import ifj26.*

@main def main(): Unit =
    print(1.5)
    print("|")
    print(0.0)
""",
        expected_stdout=f"{c_hex_float(1.5)}|{c_hex_float(0.0)}",
    ),

    # =========================================================================
    # 2. END-TO-END: FUNKCIE, VZÁJOMNÁ REKURZIA, RÁMCE A PRAVIDLÁ RETURN
    # =========================================================================
    TestCase(
        name="e2e_mutual_recursion_and_frames",
        category="e2e_func",
        note="Dopredné volanie, vzájomná rekurzia a izolácia lokálnych rámcov LF/TF",
        code="""\
import ifj26.*

def isEven(n: Int): Int =
    if n == 0 then
        return 1
    else
        val m = n - 1
        return isOdd(m)

def isOdd(n: Int): Int =
    if n == 0 then
        return 0
    else
        val m = n - 1
        return isEven(m)

def fib(n: Int): Int =
    if n <= 1 then
        return n
    else
        val n1 = n - 1
        val n2 = n - 2
        val a = fib(n1)
        val b = fib(n2)
        return a + b

@main def run(): Unit =
    val e = isEven(12)
    val o = isOdd(12)
    val f = fib(8)
    print(e)
    print("|")
    print(o)
    print("|")
    print(f)
""",
        expected_stdout="1|0|21",
    ),
    TestCase(
        name="e2e_unit_and_return_rules",
        category="e2e_func",
        note="Unit zahadzuje výsledok posledného výrazu aj return výrazu; odvodenie návratového typu",
        code="""\
import ifj26.*

def oneLine(x: Int) = x * 3 + 1

def discardTail(s: String): Unit =
    print(s)
    42

def discardReturn(s: String): Unit =
    print(s)
    return 999

def endsWithVar(): Unit =
    val local = 123

@main def main(): Unit =
    val v = oneLine(4)
    val u1: Unit = discardTail("A")
    val u2: Unit = discardReturn("B")
    val u3: Unit = endsWithVar()
    print(v)
    print("|")
    print(u1)
    print("|")
    print(u2)
    print("|")
    print(u3)
""",
        expected_stdout="AB13|()|()|()",
    ),

    # =========================================================================
    # 3. END-TO-END: CYKLY, DEFVAR V CYKLE A PREKRÝVANIE (SHADOWING) PREMENNÝCH
    # =========================================================================
    TestCase(
        name="e2e_while_defvar_and_shadowing",
        category="e2e_scope",
        note="Definícia premennej v tele while (nesmie vyvolať chybu 52 v ic26int) a shadowing s iným typom",
        code="""\
import ifj26.*

@main def main(): Unit =
    var sum = 0
    var i = 1
    while i <= 4 do
        val sq = i * i
        sum = sum + sq
        i = i + 1
    val x: Int = 100
    if sum > 10 then
        val x: String = "inner"
        print(x)
    else
        ()
    val y = x + 5
    print("|")
    print(sum)
    print("|")
    print(y)
""",
        expected_stdout="inner|30|105",
    ),

    # =========================================================================
    # 4. END-TO-END: REŤAZCE, ESCAPE SEKVENCIE V IFJCODE26 A VSTAVANÉ FUNKCIE
    # =========================================================================
    TestCase(
        name="e2e_strings_and_ifjcode_escaping",
        category="e2e_builtins",
        note="Znaky #, \\, medzery v reťazcoch a viacriadkový literál '''...'''",
        code='''\
import ifj26.*

@main def main(): Unit =
    val s1 = "a#b\\\\c d\\te"
    val s2 = """line1\\nraw
line2"""
    val l1 = strLength(s1)
    val l2 = strLength(s2)
    print(l1)
    print("|")
    print(l2)
    print("|")
    print(s1)
    print("|")
    print(s2)
''',
        expected_stdout="9|16|a#b\\c d\te|line1\\nraw\nline2",
    ),
    TestCase(
        name="e2e_builtins_io_and_boundaries",
        category="e2e_builtins",
        note="readInt/readDouble (zlý vstup a EOF -> 0), readLine (EOF -> ''), substr, ord, chr, toInt",
        stdin_data="42\n-7\n 42\n1_000\nabc\n3.5\n-1.25e1\n3.5 \nxyz\nprvni\ndruhy",
        code="""\
import ifj26.*

@main def main(): Unit =
    val i1 = readInt()
    val i2 = readInt()
    val i3 = readInt()
    val i4 = readInt()
    val i5 = readInt()
    val d1 = readDouble()
    val d2 = readDouble()
    val d3 = readDouble()
    val d4 = readDouble()
    val l1 = readLine()
    val l2 = readLine()
    val l3 = readLine()
    val i_eof = readInt()
    val d_eof = readDouble()

    print(i1)
    print(",")
    print(i2)
    print(",")
    print(i3)
    print(",")
    print(i4)
    print(",")
    print(i5)
    print("|")
    val sd1 = toStr(d1)
    val sd2 = toStr(d2)
    val sd3 = toStr(d3)
    val sd4 = toStr(d4)
    val sde = toStr(d_eof)
    print(sd1)
    print(",")
    print(sd2)
    print(",")
    print(sd3)
    print(",")
    print(sd4)
    print(",")
    print(i_eof)
    print(",")
    print(sde)
    print("|")

    val len1 = strLength(l1)
    val len2 = strLength(l2)
    val len3 = strLength(l3)
    print(len1)
    print(",")
    print(len2)
    print(",")
    print(len3)
    print("|")

    val sub_all = substr("abcdef", 0, 6)
    val sub_end_empty = substr("abcdef", 6, 6)
    val o_ok = ord("ABC", 1)
    val o_empty = ord("", 0)
    val o_hi = ord("ABC", 3)
    val o_lo = ord("ABC", -1)
    val ch = chr(65)
    val ti1 = toInt(3.99)
    val ti2 = toInt(-3.99)

    print(sub_all)
    print("|")
    print(sub_end_empty)
    print("|")
    print(o_ok)
    print(",")
    print(o_empty)
    print(",")
    print(o_hi)
    print(",")
    print(o_lo)
    print("|")
    print(ch)
    print("|")
    print(ti1)
    print(",")
    print(ti2)
""",
        expected_stdout="42,-7,0,0,0|3.50,-12.50,0.00,0.00,0,0.00|6,5,0|abcdef||66,-1,-1,-1|A|3,-3",
    ),
    TestCase(
        name="e2e_lexical_and_layout_features",
        category="e2e_layout",
        note="Komentáre pred prologom, vnorené komentáre, podtržítka v číslach, viacriadkové zátvorky s koncovou čiarkou, bodkočiarka",
        code="""\
// Komentár pred prologom
/* Vnorený /* blokový */ komentár pred prologom */

import ifj26.*

def sumThree(
    a: Int,
    b: Int,
    c: Int,
): Int =
    val x = a + b; val y = x + c
    y

@main
def entryPoint(): Unit =
    val/* inline komentar ako medzera */n1 = 1_000_000
    val d1 = 1_2.5_0e-1D
    val res = sumThree(
        n1,
        20,
        3,
    )
    val sd = toStr(d1)
    print(res)
    print("|")
    print(sd)
""",
        expected_stdout="1000023|1.25",
    ),

    # =========================================================================
    # 5. BEHOVÉ CHYBY INTERPRETU (NÁVRATOVÝ KÓD 40)
    # =========================================================================
    TestCase(
        name="runtime_40_substr_neg_start",
        category="runtime_40",
        code="""\
import ifj26.*
@main def main(): Unit =
    val s = substr("abc", -1, 2)
""",
        expected_runtime_rc=40,
    ),
    TestCase(
        name="runtime_40_substr_start_gt_end",
        category="runtime_40",
        code="""\
import ifj26.*
@main def main(): Unit =
    val s = substr("abc", 2, 1)
""",
        expected_runtime_rc=40,
    ),
    TestCase(
        name="runtime_40_substr_end_gt_len",
        category="runtime_40",
        code="""\
import ifj26.*
@main def main(): Unit =
    val s = substr("abc", 1, 4)
""",
        expected_runtime_rc=40,
    ),
    TestCase(
        name="runtime_40_chr_negative",
        category="runtime_40",
        code="""\
import ifj26.*
@main def main(): Unit =
    val c = chr(-1)
""",
        expected_runtime_rc=40,
    ),
    TestCase(
        name="runtime_40_chr_gt_127",
        category="runtime_40",
        code="""\
import ifj26.*
@main def main(): Unit =
    val c = chr(128)
""",
        expected_runtime_rc=40,
    ),

    # =========================================================================
    # 6. CHYBA 1: LEXIKÁLNA ANALÝZA A ODSADENIE
    # =========================================================================
    TestCase(
        name="err1_tab_indentation",
        category="err_1",
        note="Tabulátor na začiatku neprázdneho riadku je lexikálna chyba 1",
        code="import ifj26.*\n@main def main(): Unit =\n\tval x = 1\n",
        expected_compile_rc=1,
    ),
    TestCase(
        name="err1_unclosed_nested_comment",
        category="err_1",
        code="import ifj26.*\n/* outer /* inner */\n@main def main(): Unit =\n    ()\n",
        expected_compile_rc=1,
    ),
    TestCase(
        name="err1_token_after_multiline_comment",
        category="err_1",
        note="Za koncom viacriadkového blokového komentára nesmú byť na tom istom riadku ďalšie tokeny",
        code="import ifj26.*\n@main def main(): Unit =\n    /* line 1\n       line 2 */ val x = 1\n",
        expected_compile_rc=1,
    ),
    TestCase(
        name="err1_invalid_escape",
        category="err_1",
        code='import ifj26.*\n@main def main(): Unit =\n    val s = "bad\\xescape"\n',
        expected_compile_rc=1,
    ),
    TestCase(
        name="err1_leading_zero_int",
        category="err_1",
        code="import ifj26.*\n@main def main(): Unit =\n    val x = 012\n",
        expected_compile_rc=1,
    ),

    # =========================================================================
    # 7. CHYBA 2: SYNTAKTICKÁ ANALÝZA A ODSADENÉ BLOKY
    # =========================================================================
    TestCase(
        name="err2_missing_prolog",
        category="err_2",
        code="@main def main(): Unit =\n    ()\n",
        expected_compile_rc=2,
    ),
    TestCase(
        name="err2_duplicate_prolog",
        category="err_2",
        code="import ifj26.*\nimport ifj26.*\n@main def main(): Unit =\n    ()\n",
        expected_compile_rc=2,
    ),
    TestCase(
        name="err2_bad_block_indentation",
        category="err_2",
        code="import ifj26.*\n@main def main(): Unit =\n    val x = 1\n      val y = 2\n",
        expected_compile_rc=2,
    ),
    TestCase(
        name="err2_missing_else_in_base",
        category="err_2",
        note="Vetva else je v základnom zadaní povinná",
        code="import ifj26.*\n@main def main(): Unit =\n    if 1 < 2 then\n        print(1)\n",
        expected_compile_rc=2,
    ),
    TestCase(
        name="err2_call_inside_expression_in_base",
        category="err_2",
        note="V základnom zadaní (bez FUNEXP) nesmie byť volanie funkcie vo výraze",
        code="import ifj26.*\ndef f(): Int = 1\n@main def main(): Unit =\n    val x = f() + 1\n",
        expected_compile_rc=2,
    ),
    TestCase(
        name="err2_expr_as_argument_in_base",
        category="err_2",
        note="V základnom zadaní (bez FUNEXP) smie byť argumentom volania iba term",
        code="import ifj26.*\n@main def main(): Unit =\n    print(1 + 2)\n",
        expected_compile_rc=2,
    ),
    TestCase(
        name="err2_chained_relational",
        category="err_2",
        note="Reťazenie relačných operátorov nie je povolené",
        code="import ifj26.*\n@main def main(): Unit =\n    if 1 < 2 < 3 then\n        ()\n    else\n        ()\n",
        expected_compile_rc=2,
    ),

    # =========================================================================
    # 8. CHYBA 3, 4, 5, 6, 7, 10: SÉMANTICKÉ A TYPOVÉ CHYBY
    # =========================================================================
    TestCase(
        name="err3_missing_main",
        category="err_3",
        code="import ifj26.*\ndef foo(): Unit =\n    ()\n",
        expected_compile_rc=3,
    ),
    TestCase(
        name="err3_out_of_scope_variable",
        category="err_3",
        code="""\
import ifj26.*
@main def main(): Unit =
    if 1 < 2 then
        val secret = 42
    else
        ()
    print(secret)
""",
        expected_compile_rc=3,
    ),
    TestCase(
        name="err4_multiple_main",
        category="err_4",
        code="""\
import ifj26.*
@main def m1(): Unit =
    ()
@main def m2(): Unit =
    ()
""",
        expected_compile_rc=4,
    ),
    TestCase(
        name="err4_redefine_var_same_block",
        category="err_4",
        code="""\
import ifj26.*
@main def main(): Unit =
    val x = 1
    val x = 2
""",
        expected_compile_rc=4,
    ),
    TestCase(
        name="err5_main_with_param",
        category="err_5",
        code="""\
import ifj26.*
@main def main(x: Int): Unit =
    ()
""",
        expected_compile_rc=5,
    ),
    TestCase(
        name="err5_wrong_arg_type",
        category="err_5",
        code="""\
import ifj26.*
def takesInt(a: Int): Unit =
    ()
@main def main(): Unit =
    takesInt(3.14)
""",
        expected_compile_rc=5,
    ),
    TestCase(
        name="err6_main_wrong_return_type",
        category="err_6",
        code="""\
import ifj26.*
@main def main(): Int =
    0
""",
        expected_compile_rc=6,
    ),
    TestCase(
        name="err6_double_to_int_assign",
        category="err_6",
        code="""\
import ifj26.*
@main def main(): Unit =
    var x: Int = 1
    x = 2.5
""",
        expected_compile_rc=6,
    ),
    TestCase(
        name="err6_recursive_without_return_type",
        category="err_6",
        note="Rekurzívna funkcia musí mať explicitne uvedený návratový typ",
        code="""\
import ifj26.*
def rec(n: Int) =
    if n <= 0 then
        0
    else
        val m = n - 1
        rec(m)
@main def main(): Unit =
    ()
""",
        expected_compile_rc=6,
    ),
    TestCase(
        name="err6_return_without_explicit_type",
        category="err_6",
        note="Funkcia obsahujúca return musí mať explicitne uvedený návratový typ",
        code="""\
import ifj26.*
def f() =
    return 5
@main def main(): Unit =
    ()
""",
        expected_compile_rc=6,
    ),
    TestCase(
        name="err6_non_unit_if_branch_without_return",
        category="err_6",
        code="""\
import ifj26.*
def f(x: Int): Int =
    if x > 0 then
        return 1
    else
        val y = 2
@main def main(): Unit =
    ()
""",
        expected_compile_rc=6,
    ),
    TestCase(
        name="err7_assign_to_val",
        category="err_7",
        code="""\
import ifj26.*
@main def main(): Unit =
    val x = 10
    x = 20
""",
        expected_compile_rc=7,
    ),
    TestCase(
        name="err7_assign_to_param",
        category="err_7",
        code="""\
import ifj26.*
def f(p: Int): Unit =
    p = 20
@main def main(): Unit =
    f(10)
""",
        expected_compile_rc=7,
    ),
    TestCase(
        name="err10_bare_return_in_int_func",
        category="err_10",
        note="Samostatný return bez hodnoty je povolený iba vo funkcii s explicitným typom Unit",
        code="""\
import ifj26.*
def f(): Int =
    return
@main def main(): Unit =
    ()
""",
        expected_compile_rc=10,
    ),
]


def run_test(tc: TestCase, compiler: str, interpreter: str, verbose: bool) -> tuple[bool, str]:
    try:
        comp_proc = subprocess.run(
            [compiler],
            input=tc.code.encode("utf-8"),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=5,
        )
    except subprocess.TimeoutExpired:
        return False, "TIMEOUT pri preklade"

    if comp_proc.returncode != tc.expected_compile_rc:
        msg = f"Očakávaný RC prekladača {tc.expected_compile_rc}, získaný {comp_proc.returncode}"
        if verbose and comp_proc.stderr:
            msg += f"\n    stderr: {comp_proc.stderr.decode('utf-8', errors='replace').strip()}"
        return False, msg

    # Ak sme očakávali chybu prekladu, test tu úspešne končí
    if tc.expected_compile_rc != 0:
        return True, "OK"

    # Ak interpreter nie je zadaný alebo je "dummy", skončíme po úspešnom preklade
    if not interpreter or interpreter.lower() in ("dummy", "none", "skip"):
        return True, "OK (compile-only)"

    # Inak spustíme vygenerovaný IFJcode26 v ic26int
    with tempfile.NamedTemporaryFile(mode="wb", suffix=".ifjcode26", delete=False) as tmp:
        tmp.write(comp_proc.stdout)
        tmp_path = tmp.name

    try:
        int_proc = subprocess.run(
            [interpreter, tmp_path],
            input=tc.stdin_data.encode("utf-8"),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=5,
        )
    except subprocess.TimeoutExpired:
        return False, "TIMEOUT pri interpretácii ic26int"
    finally:
        if os.path.exists(tmp_path):
            os.remove(tmp_path)

    if tc.expected_runtime_rc is not None and int_proc.returncode != tc.expected_runtime_rc:
        msg = f"Očakávaný RC interpretu {tc.expected_runtime_rc}, získaný {int_proc.returncode}"
        if verbose:
            if int_proc.stderr:
                msg += f"\n    ic26int stderr: {int_proc.stderr.decode('utf-8', errors='replace').strip()}"
            msg += f"\n    --- vygenerovaný IFJcode26 ---\n{comp_proc.stdout.decode('utf-8', errors='replace')}"
        return False, msg

    if tc.expected_stdout is not None:
        actual_out = int_proc.stdout.decode("utf-8", errors="replace")
        if actual_out != tc.expected_stdout:
            msg = f"Nezhoda stdout:\n    očakávané: {tc.expected_stdout!r}\n    skutočné:  {actual_out!r}"
            if verbose:
                msg += f"\n    --- vygenerovaný IFJcode26 ---\n{comp_proc.stdout.decode('utf-8', errors='replace')}"
            return False, msg

    return True, "OK"


def main() -> int:
    parser = argparse.ArgumentParser(description="Integračné testy prekladača IFJ26")
    parser.add_argument("compiler", help="Cesta k binárke prekladača IFJ26")
    parser.add_argument("interpreter", help="Cesta k interpretu ic26int")
    parser.add_argument("-v", "--verbose", action="store_true", help="Vypísať stderr a vygenerovaný kód pri chybe")
    parser.add_argument("-f", "--filter", default="", help="Filtrovať testy podľa podreťazca v názve alebo kategórii")
    args = parser.parse_args()

    selected = [
        t for t in TESTS
        if not args.filter or args.filter in t.name or args.filter in t.category
    ]

    passed = 0
    failed = 0

    for tc in selected:
        ok, info = run_test(tc, args.compiler, args.interpreter, args.verbose)
        if ok:
            passed += 1
            print(f"[PASS] {tc.name}")
        else:
            failed += 1
            print(f"[FAIL] {tc.name} ({tc.category})")
            if tc.note:
                print(f"    Poznámka: {tc.note}")
            print(f"    Dôvod:    {info}")

    print(f"\nVýsledok: {passed}/{len(selected)} úspešných, {failed} zlyhalo.")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())