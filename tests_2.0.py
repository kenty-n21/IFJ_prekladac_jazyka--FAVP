#!/usr/bin/env python3
"""
Testovacia sada výhradne pre základný variant prekladača IFJ26 (bez rozšírení vrátane Bool).
"""

import argparse
import os
import re
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple


@dataclass
class SyntaxProfile:
    prolog: str = "import ifj26.*"
    def_sep: str = "="          # "=" alebo ":" alebo ""
    if_fmt: str = "if ({cond}):"
    else_line: str = "else:"
    while_fmt: str = "while ({cond}):"


def detect_syntax_profile(reference_script: str) -> SyntaxProfile:
    prof = SyntaxProfile()
    if not os.path.exists(reference_script):
        return prof

    try:
        with open(reference_script, "r", encoding="utf-8") as f:
            content = f.read()
    except OSError:
        return prof

    snippets: List[str] = []
    for m in re.finditer(r'("""|\'\'\')(.*?)\1', content, re.DOTALL):
        body = m.group(2)
        if "@main" in body and "import " in body:
            snippets.append(body)

    if not snippets:
        return prof

    cleaned_snippets: List[str] = []
    for s in snippets:
        s = re.sub(r"/\*.*?\*/", "", s, flags=re.DOTALL)
        s = re.sub(r"//.*", "", s)
        cleaned_snippets.append(s)

    for raw_line in cleaned_snippets[0].splitlines():
        line = raw_line.strip()
        if line.startswith("import "):
            prof.prolog = line
            break

    all_code = "\n".join(cleaned_snippets)

    if re.search(r"def\s+\w+\s*\([^)]*\)\s*:\s*\w+\s*=", all_code):
        prof.def_sep = "="
    elif re.search(r"def\s+\w+\s*\([^)]*\)\s*:\s*\w+\s*:", all_code):
        prof.def_sep = ":"
    elif re.search(r"def\s+\w+\s*\([^)]*\)\s*:\s*\w+\s*\n", all_code):
        prof.def_sep = ""

    if re.search(r"\bif\s*\([^)\n]+\)\s*:", all_code):
        prof.if_fmt = "if ({cond}):"
        prof.else_line = "else:"
    elif re.search(r"\bif\s+[^(\n][^:\n]+:", all_code):
        prof.if_fmt = "if {cond}:"
        prof.else_line = "else:"
    elif re.search(r"\bif\s+.+?\bthen\b", all_code):
        prof.if_fmt = "if {cond} then"
        prof.else_line = "else"
    elif re.search(r"\bif\s*\([^)\n]+\)\s*\n", all_code):
        prof.if_fmt = "if ({cond})"
        prof.else_line = "else"

    if re.search(r"\bwhile\s*\([^)\n]+\)\s*:", all_code):
        prof.while_fmt = "while ({cond}):"
    elif re.search(r"\bwhile\s+[^(\n][^:\n]+:", all_code):
        prof.while_fmt = "while {cond}:"
    elif re.search(r"\bwhile\s+.+?\bdo\b", all_code):
        prof.while_fmt = "while {cond} do"
    elif re.search(r"\bwhile\s*\([^)\n]+\)\s*\n", all_code):
        prof.while_fmt = "while ({cond})"

    return prof


@dataclass
class BaseTest:
    name: str
    group: str
    description: str
    source: str
    expected_compile_rc: int = 0
    expected_runtime_rc: Optional[int] = 0
    stdin_data: str = ""
    expected_stdout: Optional[str] = None


def build_tests(p: SyntaxProfile) -> List[BaseTest]:
    P = p.prolog
    SEP = f" {p.def_sep}" if p.def_sep else ""

    def DEF(sig: str) -> str:
        return f"def {sig}{SEP}"

    def MAIN(sig: str = "main(): Unit") -> str:
        return f"@main def {sig}{SEP}"

    def IF(cond: str) -> str:
        return p.if_fmt.format(cond=cond)

    ELSE = p.else_line

    def WHILE(cond: str) -> str:
        return p.while_fmt.format(cond=cond)

    tests: List[BaseTest] = []

    def add(
        name: str,
        group: str,
        description: str,
        source: str,
        compile_rc: int = 0,
        runtime_rc: Optional[int] = 0,
        stdin_data: str = "",
        stdout: Optional[str] = None,
    ) -> None:
        tests.append(
            BaseTest(
                name=name,
                group=group,
                description=description,
                source=source,
                expected_compile_rc=compile_rc,
                expected_runtime_rc=runtime_rc,
                stdin_data=stdin_data,
                expected_stdout=stdout,
            )
        )

    # =========================================================================
    # A. POZITÍVNE E2E KONTROLY ZÁKLADNÉHO VARIANTU (ok_*)
    # =========================================================================
    add(
        "ok_01_implicit_conversions_all",
        "ok_e2e",
        "Implicitná konverzia Int -> Double pri init, priradení, argumente, returne, aritmetike a v podmienke if",
        f"""{P}

{DEF("addHalf(v: Double): Double")}
    return v + 0.5

{DEF("promoteInt(n: Int): Double")}
    return n

{MAIN()}
    val initD: Double = 5
    var assignD: Double = 1.5
    assignD = 4
    val argD = addHalf(3)
    val retD = promoteInt(2)
    val mix1 = 2 + 0.5
    val mix2 = 0.5 + 2
    val total = initD + assignD + argD + retD + mix1 + mix2
    val whole = toInt(total)
    print(whole)
    print("|")
    {IF("5 == 5.0")}
        print("1")
    {ELSE}
        print("0")
    print("|")
    {IF("-0.0 == 0.0")}
        print("1")
    {ELSE}
        print("0")
    print("|")
    {IF("4 < 4.5")}
        print("1")
    {ELSE}
        print("0")
""",
        stdout="19|1|1|1",
    )

    add(
        "ok_02_unary_minus_and_precedence",
        "ok_e2e",
        "Unárne mínus nad premennými a zátvorkami, priorita a ľavá asociativita - a /",
        f"""{P}

{MAIN()}
    val a = 10
    val negA = -a
    val doubleNeg = -(-a)
    val expr1 = -(3 + 4) * -2
    val expr2 = 50 - 20 - 5
    val expr3 = 48 / 6 / 2
    val expr4 = 2 + 3 * 4 - 15 / 3
    print(negA)
    print("|")
    print(doubleNeg)
    print("|")
    print(expr1)
    print("|")
    print(expr2)
    print("|")
    print(expr3)
    print("|")
    print(expr4)
""",
        stdout="-10|10|14|25|4|9",
    )

    add(
        "ok_03_control_flow_and_returns",
        "ok_e2e",
        "Vnorené if/else s viacnásobným returnom a while cyklus s 0 aj viacerými iteráciami",
        f"""{P}

{DEF("classify(n: Int): Int")}
    {IF("n < 0")}
        return -1
    {ELSE}
        {IF("n == 0")}
            return 0
        {ELSE}
            return 1

{MAIN()}
    val neg = -7
    val zero = 0
    val pos = 9
    val c1 = classify(neg)
    val c2 = classify(zero)
    val c3 = classify(pos)
    var skip = 0
    {WHILE("skip < 0")}
        skip = skip + 100
    var steps = 0
    {WHILE("steps < 4")}
        steps = steps + 1
    print(c1)
    print("|")
    print(c2)
    print("|")
    print(c3)
    print("|")
    print(skip)
    print("|")
    print(steps)
""",
        stdout="-1|0|1|0|4",
    )

    inner_cond = 'x == "inner"'
    add(
        "ok_04_scoping_and_var_same_as_func",
        "ok_e2e",
        "Izolácia premenných v then/else, prekrytie názvu funkcie premennou a 3-úrovňový shadowing",
        f"""{P}

{DEF("helper(): Int")}
    return 100

{MAIN()}
    val helper: Int = 35
    val x: Int = 1
    {IF("x == 1")}
        val branch: Int = 10
        val x: String = "inner"
        {IF(inner_cond)}
            val x: Double = 2.5
            val xi = toInt(x)
            print(xi)
            print("|")
        {ELSE}
            print("err")
        print(x)
        print("|")
        print(branch)
        print("|")
    {ELSE}
        val branch: String = "other"
        print(branch)
    val sum = helper + x
    print(sum)
""",
        stdout="2|inner|10|36",
    )

    add(
        "ok_05_builtins_toStr_and_boundaries",
        "ok_e2e",
        "Konverzie toStr a hraničné platné vstupy pre substr, ord a chr",
        f"""{P}

{MAIN()}
    val sInt = toStr(42)
    val sNeg = toStr(-7)
    val empty = ""
    val word = "abcd"
    val i0 = 0
    val i1 = 1
    val i3 = 3
    val i4 = 4
    val neg = -1
    val subEmpty = substr(empty, i0, i0)
    val subEnd = substr(word, i4, i4)
    val subAll = substr(word, i0, i4)
    val subMid = substr(word, i1, i3)
    val ordEmpty = ord(empty, i0)
    val ordNeg = ord(word, neg)
    val ordOut = ord(word, i4)
    val cMin = chr(i0)
    val cMax = chr(127)
    val lMin = strLength(cMin)
    val lMax = strLength(cMax)
    print(sInt)
    print("|")
    print(sNeg)
    print("|[")
    print(subEmpty)
    print("|")
    print(subEnd)
    print("|")
    print(subAll)
    print("|")
    print(subMid)
    print("]|")
    print(ordEmpty)
    print("|")
    print(ordNeg)
    print("|")
    print(ordOut)
    print("|")
    print(lMin)
    print("|")
    print(lMax)
""",
        stdout="42|-7|[||abcd|bc]|-1|-1|-1|1|1",
    )

    add(
        "ok_06_formatting_semicolons_comments",
        "ok_e2e",
        "Bodkočiarky medzi príkazmi, viacriadkové zátvorky a blokové komentáre nalepené na tokeny",
        f"""{P}

{DEF("sum3(\n    a: Int,\n    b: Int,\n    c: Int\n): Int")}
    val part = (a +
        b) *
        c
    return part

{MAIN()}
    val/*c1*/x/*c2*/=/*c3*/2; val/*c4*/y/*c5*/=/*c6*/3; val z = 4;
    val res = sum3(
        x,
        y,
        z
    )
    print(res)
""",
        stdout="20",
    )

    # =========================================================================
    # B. LEXIKÁLNE A SYNTAKTICKÉ CHYBY ZÁKLADU (RC 1, RC 2)
    # =========================================================================
    add(
        "err1_tab_indent",
        "err_1_2",
        "Tabulátor v odsadení musí vrátiť lexikálnu chybu 1",
        f"""{P}

{MAIN()}
\tval x = 1
""",
        compile_rc=1,
        runtime_rc=None,
    )

    add(
        "err1_leading_zero",
        "err_1_2",
        "Vedúca nula v celočíselnom literáli (007) -> chyba 1",
        f"""{P}

{MAIN()}
    val x = 007
""",
        compile_rc=1,
        runtime_rc=None,
    )

    add(
        "err1_unclosed_nested_comment",
        "err_1_2",
        "Neuzavretý vnorený komentár -> chyba 1",
        f"""{P}

{MAIN()}
    /* vonkajsi /* vnutorny */ chyba
    val x = 1
""",
        compile_rc=1,
        runtime_rc=None,
    )

    add(
        "err1_invalid_escape",
        "err_1_2",
        "Neplatná escape sekvencia v reťazci -> chyba 1",
        f"""{P}

{MAIN()}
    val s = "neplatny\\qznak"
""",
        compile_rc=1,
        runtime_rc=None,
    )

    add(
        "err2_missing_prolog",
        "err_1_2",
        "Chýbajúci prolog na začiatku súboru -> chyba 2",
        f"""{MAIN()}
    val x = 1
""",
        compile_rc=2,
        runtime_rc=None,
    )

    add(
        "err2_missing_else",
        "err_1_2",
        "Príkaz if bez vetvy else je v základe syntaktická chyba 2",
        f"""{P}

{MAIN()}
    val x = 1
    {IF("x > 0")}
        print(x)
""",
        compile_rc=2,
        runtime_rc=None,
    )

    add(
        "err2_call_in_expr",
        "err_1_2",
        "Volanie funkcie vnútri výrazu (1 + f()) -> chyba 2",
        f"""{P}

{DEF("f(): Int")}
    return 1

{MAIN()}
    val x = 1 + f()
""",
        compile_rc=2,
        runtime_rc=None,
    )

    add(
        "err2_expr_in_arg",
        "err_1_2",
        "Zložený výraz ako argument funkcie print(1 + 2) -> chyba 2",
        f"""{P}

{MAIN()}
    print(1 + 2)
""",
        compile_rc=2,
        runtime_rc=None,
    )

    add(
        "err2_chained_rel",
        "err_1_2",
        "Reťazené relačné operátory (1 < 2 < 3) -> chyba 2",
        f"""{P}

{MAIN()}
    {IF("1 < 2 < 3")}
        print(1)
    {ELSE}
        print(0)
""",
        compile_rc=2,
        runtime_rc=None,
    )

    add(
        "err2_bad_dedent",
        "err_1_2",
        "Návrat odsadenia na neexistujúcu úroveň -> chyba 2",
        f"""{P}

{MAIN()}
    val x = 1
    {IF("x > 0")}
        val y = 2
      val z = 3
    {ELSE}
        val y = 4
""",
        compile_rc=2,
        runtime_rc=None,
    )

    # =========================================================================
    # C. SÉMANTICKÉ KONTROLY (RC 3, 4, 5, 6, 7, 10)
    # =========================================================================
    add(
        "err3_use_before_decl",
        "err_3_4_5",
        "Použitie premennej pred jej deklaráciou -> chyba 3",
        f"""{P}

{MAIN()}
    val y = x + 1
    val x = 5
""",
        compile_rc=3,
        runtime_rc=None,
    )

    add(
        "err3_var_leaks_from_if",
        "err_3_4_5",
        "Premenná z vetvy then nie je viditeľná v else -> chyba 3",
        f"""{P}

{MAIN()}
    {IF("1 == 1")}
        val secret = 1
        print(secret)
    {ELSE}
        print(secret)
""",
        compile_rc=3,
        runtime_rc=None,
    )

    add(
        "err3_var_leaks_after_while",
        "err_3_4_5",
        "Premenná z tela while nie je viditeľná za cyklom -> chyba 3",
        f"""{P}

{MAIN()}
    var i = 0
    {WHILE("i < 1")}
        val inside = 42
        i = i + 1
    print(inside)
""",
        compile_rc=3,
        runtime_rc=None,
    )

    add(
        "err3_missing_main",
        "err_3_4_5",
        "Program bez funkcie označenej @main -> chyba 3",
        f"""{P}

{DEF("foo(): Unit")}
    print(1)
""",
        compile_rc=3,
        runtime_rc=None,
    )

    add(
        "err4_redeclare_same_scope",
        "err_3_4_5",
        "Redeklarácia premennej v tom istom bloku -> chyba 4",
        f"""{P}

{MAIN()}
    val x = 1
    var x = 2
""",
        compile_rc=4,
        runtime_rc=None,
    )

    add(
        "err4_redeclare_param_in_body",
        "err_3_4_5",
        "Redeklarácia parametra v hlavnom tele funkcie -> chyba 4",
        f"""{P}

{DEF("f(a: Int): Int")}
    val a = 5
    return a

{MAIN()}
    val r = f(1)
""",
        compile_rc=4,
        runtime_rc=None,
    )

    add(
        "err4_redefine_builtin",
        "err_3_4_5",
        "Pokus o redefiníciu vstavanej funkcie strLength -> chyba 4",
        f"""{P}

{DEF("strLength(s: String): Int")}
    return 0

{MAIN()}
    print(1)
""",
        compile_rc=4,
        runtime_rc=None,
    )

    add(
        "err5_wrong_arg_count",
        "err_3_4_5",
        "Nesprávny počet argumentov pri volaní funkcie -> chyba 5",
        f"""{P}

{DEF("f(a: Int, b: Int): Int")}
    return a + b

{MAIN()}
    val r = f(1)
""",
        compile_rc=5,
        runtime_rc=None,
    )

    add(
        "err5_double_to_int_arg",
        "err_3_4_5",
        "Odovzdanie Double do parametra typu Int -> chyba 5",
        f"""{P}

{DEF("f(a: Int): Int")}
    return a

{MAIN()}
    val r = f(2.5)
""",
        compile_rc=5,
        runtime_rc=None,
    )

    add(
        "err5_main_with_params",
        "err_3_4_5",
        "Funkcia @main nesmie mať parametre -> chyba 5",
        f"""{P}

{MAIN("main(x: Int): Unit")}
    print(x)
""",
        compile_rc=5,
        runtime_rc=None,
    )

    add(
        "err6_store_bool_in_var",
        "err_6_7_10",
        "V základnom variante nemožno uložiť relačný výraz do premennej -> chyba 6",
        f"""{P}

{MAIN()}
    val b = 1 < 2
""",
        compile_rc=6,
        runtime_rc=None,
    )

    add(
        "err6_double_to_int_assign",
        "err_6_7_10",
        "Priradenie Double do premennej typu Int -> chyba 6",
        f"""{P}

{MAIN()}
    var x: Int = 1
    x = 2.5
""",
        compile_rc=6,
        runtime_rc=None,
    )

    add(
        "err6_string_plus_int",
        "err_6_7_10",
        "Sčítanie String + Int -> chyba 6",
        f"""{P}

{MAIN()}
    val x = "abc" + 1
""",
        compile_rc=6,
        runtime_rc=None,
    )

    add(
        "err6_recursive_without_type",
        "err_6_7_10",
        "Rekurzívna funkcia bez explicitne uvedeného návratového typu -> chyba 6",
        f"""{P}

{DEF("rec(n: Int)")}
    {IF("n <= 0")}
        0
    {ELSE}
        val m = n - 1
        rec(m)

{MAIN()}
    val r = rec(3)
""",
        compile_rc=6,
        runtime_rc=None,
    )

    add(
        "err6_return_in_inferred_func",
        "err_6_7_10",
        "Príkaz return vo funkcii bez explicitného návratového typu -> chyba 6",
        f"""{P}

{DEF("f(x: Int)")}
    return x

{MAIN()}
    val r = f(1)
""",
        compile_rc=6,
        runtime_rc=None,
    )

    add(
        "err6_missing_return_branch",
        "err_6_7_10",
        "Ne-Unit funkcia bez návratovej hodnoty v jednej z vetiev -> chyba 6",
        f"""{P}

{DEF("f(c: Int): Int")}
    {IF("c == 1")}
        return 1
    {ELSE}
        val x = 2

{MAIN()}
    val r = f(0)
""",
        compile_rc=6,
        runtime_rc=None,
    )

    add(
        "err7_assign_to_val",
        "err_6_7_10",
        "Zápis do nemodifikovateľnej premennej val -> chyba 7",
        f"""{P}

{MAIN()}
    val x = 5
    x = 6
""",
        compile_rc=7,
        runtime_rc=None,
    )

    add(
        "err7_assign_to_param",
        "err_6_7_10",
        "Zápis do parametra funkcie -> chyba 7",
        f"""{P}

{DEF("f(p: Int): Int")}
    p = 10
    return p

{MAIN()}
    val r = f(1)
""",
        compile_rc=7,
        runtime_rc=None,
    )

    add(
        "err10_bare_return_in_non_unit",
        "err_6_7_10",
        "Samostatný return bez výrazu vo funkcii s typom Int -> chyba 10",
        f"""{P}

{DEF("f(): Int")}
    return

{MAIN()}
    val r = f()
""",
        compile_rc=10,
        runtime_rc=None,
    )

    # =========================================================================
    # D. BEHOVÉ CHYBY INTERPRETU ic26int (RC 40, 57, 58)
    # =========================================================================
    add(
        "run_err40_substr_bounds",
        "runtime_err",
        "substr mimo rozsah reťazca -> behová chyba 40",
        f"""{P}

{MAIN()}
    val s = "abc"
    val a = 1
    val b = 5
    val r = substr(s, a, b)
    print(r)
""",
        compile_rc=0,
        runtime_rc=40,
    )

    add(
        "run_err57_div_by_zero_int",
        "runtime_err",
        "Celočíselné delenie nulou -> behová chyba 57",
        f"""{P}

{MAIN()}
    val z = 0
    val r = 10 / z
    print(r)
""",
        compile_rc=0,
        runtime_rc=57,
    )

    add(
        "run_err57_div_by_zero_double",
        "runtime_err",
        "Desatinné delenie nulou -> behová chyba 57",
        f"""{P}

{MAIN()}
    val z = 0.0
    val r = 10.0 / z
    print(r)
""",
        compile_rc=0,
        runtime_rc=57,
    )

    add(
        "run_err40_chr_out_of_range",
        "runtime_err",
        "chr(128) mimo ASCII intervalu <0; 127> -> behová chyba 40",
        f"""{P}

{MAIN()}
    val over = 128
    val c = chr(over)
    print(c)
""",
        compile_rc=0,
        runtime_rc=40,
    )

    return tests


def run_single(
    tc: BaseTest,
    compiler: str,
    interpreter: str,
    verbose: bool,
) -> Tuple[bool, str]:
    with tempfile.TemporaryDirectory(prefix="ifj26_base_") as tmpdir:
        src_path = os.path.join(tmpdir, "prog.ifj26")
        code_path = os.path.join(tmpdir, "out.ifjcode26")

        with open(src_path, "w", encoding="utf-8") as f:
            f.write(tc.source)

        with open(src_path, "rb") as fin, open(code_path, "wb") as fout:
            comp = subprocess.run(
                [compiler],
                stdin=fin,
                stdout=fout,
                stderr=subprocess.PIPE,
                timeout=5,
            )

        if comp.returncode != tc.expected_compile_rc:
            msg = (
                f"Očakávaný RC prekladača {tc.expected_compile_rc}, "
                f"získaný {comp.returncode}"
            )
            if verbose:
                err_txt = comp.stderr.decode("utf-8", errors="replace").strip()
                if err_txt:
                    msg += f"\n    compiler stderr: {err_txt}"
            return False, msg

        if tc.expected_compile_rc != 0:
            return True, "OK"

        interp = subprocess.run(
            [interpreter, code_path],
            input=tc.stdin_data.encode("utf-8"),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=5,
        )

        if tc.expected_runtime_rc is not None and interp.returncode != tc.expected_runtime_rc:
            msg = (
                f"Očakávaný RC interpretu {tc.expected_runtime_rc}, "
                f"získaný {interp.returncode}"
            )
            if verbose:
                ierr = interp.stderr.decode("utf-8", errors="replace").strip()
                with open(code_path, "r", encoding="utf-8", errors="replace") as cf:
                    gen_code = cf.read()
                if ierr:
                    msg += f"\n    ic26int stderr: {ierr}"
                msg += f"\n    --- vygenerovaný IFJcode26 ---\n{gen_code.rstrip()}"
            return False, msg

        if tc.expected_stdout is not None:
            actual_out = interp.stdout.decode("utf-8", errors="replace")
            if actual_out != tc.expected_stdout:
                msg = (
                    f"Nezhoda stdout:\n"
                    f"      očakávané: {tc.expected_stdout!r}\n"
                    f"      skutočné:  {actual_out!r}"
                )
                if verbose:
                    with open(code_path, "r", encoding="utf-8", errors="replace") as cf:
                        gen_code = cf.read()
                    msg += f"\n    --- vygenerovaný IFJcode26 ---\n{gen_code.rstrip()}"
                return False, msg

        return True, "OK"


def main() -> int:
    parser = argparse.ArgumentParser(description="Testy pre základný variant IFJ26 (bez Bool rozšírenia)")
    parser.add_argument("compiler", help="Cesta k binárke prekladača (napr. ./compiler)")
    parser.add_argument("interpreter", help="Cesta k interpretu (napr. ./ic26int-linux-x86_64)")
    parser.add_argument("-f", "--filter", default="", help="Spustiť len testy obsahujúce tento podreťazec")
    parser.add_argument("-v", "--verbose", action="store_true", help="Vypísať vygenerovaný IFJcode26 a stderr pri chybe")
    parser.add_argument("--ref-script", default="test_ifj26.py", help="Referenčný skript na detekciu prologu a syntaxe")
    parser.add_argument("--prolog", default=None, help="Manuálne prepísať riadok prologu")
    args = parser.parse_args()

    profile = detect_syntax_profile(args.ref_script)
    if args.prolog is not None:
        profile.prolog = args.prolog

    print(
        f"Syntax -> prolog: {profile.prolog!r} | "
        f"def_sep: {profile.def_sep!r} | if: {profile.if_fmt!r} | while: {profile.while_fmt!r}\n"
    )

    selected = [
        t for t in build_tests(profile)
        if not args.filter or args.filter.lower() in t.name.lower() or args.filter.lower() in t.group.lower()
    ]

    if not selected:
        print("Žiadny test nezodpovedá zadanému filtru.")
        return 1

    passed = 0
    failed = 0
    stats: Dict[str, List[int]] = {}

    for tc in selected:
        stats.setdefault(tc.group, [0, 0])
        stats[tc.group][1] += 1

        try:
            ok, info = run_single(tc, args.compiler, args.interpreter, args.verbose)
        except subprocess.TimeoutExpired:
            ok, info = False, "TIMEOUT (>5s) - pravdepodobné zacyklenie"

        if ok:
            passed += 1
            stats[tc.group][0] += 1
            print(f"[PASS] {tc.name} ({tc.group})")
        else:
            failed += 1
            print(f"[FAIL] {tc.name} ({tc.group})")
            print(f"    Popis: {tc.description}")
            print(f"    Dôvod: {info}")

    print("\n" + "=" * 62)
    print("SÚHRN PODĽA SKUPÍN:")
    for grp, (p_cnt, t_cnt) in stats.items():
        status = "OK" if p_cnt == t_cnt else "CHYBY"
        print(f"  {grp:<14} : {p_cnt:>2}/{t_cnt:<2}  [{status}]")
    print("-" * 62)
    print(f"CELKOVÝ VÝSLEDOK: {passed}/{passed + failed} úspešných, {failed} zlyhalo.")
    print("=" * 62)

    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())