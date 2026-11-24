#!/usr/bin/env python3
"""
Atomická testovacia sada pre prekladač IFJ26 -> IFJcode26 (bez FUNEXP, s subStr).
Overuje po jednotlivých konštrukciách:
  1. Základnú stavbu jazyka IFJ26 (lexika, odsadenie, štruktúra, @main, premenné,
     funkcie, rekurzia, výrazy, if/else, while, vstavané funkcie, chyby 1-7 a 40)
  2. Rozšírenie VALMEM (.toString, .toInt, .floor, .length, reťazenie)
  3. Rozšírenie INTERPOLATEDS (s"...", s\"\"\"...\"\"\", $id, ${expr}, $$, \")
"""

import argparse
import os
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from typing import Callable, Optional, Union


@dataclass
class TestCase:
    name: str
    group: str
    code: str
    expected_compile_rc: Union[int, list[int]] = 0
    stdin_data: str = ""
    expected_stdout: Optional[str] = None
    stdout_validator: Optional[Callable[[str], tuple[bool, str]]] = None
    expected_runtime_rc: int = 0


TESTS: list[TestCase] = []


def add_test(
    name: str,
    group: str,
    code: str,
    expected_compile_rc: Union[int, list[int]] = 0,
    stdin_data: str = "",
    expected_stdout: Optional[str] = None,
    stdout_validator: Optional[Callable[[str], tuple[bool, str]]] = None,
    expected_runtime_rc: int = 0,
):
    TESTS.append(
        TestCase(
            name=name,
            group=group,
            code=code,
            expected_compile_rc=expected_compile_rc,
            stdin_data=stdin_data,
            expected_stdout=expected_stdout,
            stdout_validator=stdout_validator,
            expected_runtime_rc=expected_runtime_rc,
        )
    )


# ============================================================================
# 1. ZÁKLADNÁ STAVBA – LEXIKA A LITERÁLY
# ============================================================================

add_test(
    "base_lex_int_literals",
    "ZAKLAD: Lexika",
    """import ifj26.*
@main def main(): Unit =
  val a = 0
  val b = 42
  val c = 1_000_000
  val d = -15
  print(a)
  print("|")
  print(b)
  print("|")
  print(c)
  print("|")
  print(d)
""",
    expected_stdout="0|42|1000000|-15",
)

add_test(
    "base_lex_double_literals",
    "ZAKLAD: Lexika",
    """import ifj26.*
@main def main(): Unit =
  val d1 = 3.25
  val d2 = 1_2.3_4
  val d3 = 2e-1
  val d4 = 1.5E+2
  val d5 = 4.5d
  val d6 = 4.5D
  val s1 = toStr(d1)
  val s2 = toStr(d2)
  val s3 = toStr(d3)
  val s4 = toStr(d4)
  val s5 = toStr(d5)
  val s6 = toStr(d6)
  print(s1)
  print("|")
  print(s2)
  print("|")
  print(s3)
  print("|")
  print(s4)
  print("|")
  print(s5)
  print("|")
  print(s6)
""",
    expected_stdout="3.25|12.34|0.20|150.00|4.50|4.50",
)

add_test(
    "base_lex_double_no_leading_zero",
    "ZAKLAD: Lexika",
    """import ifj26.*
@main def main(): Unit =
  val d = .5
  val s = toStr(d)
  print(s)
""",
    expected_stdout="0.50",
)

add_test(
    "base_lex_string_escapes",
    "ZAKLAD: Lexika",
    """import ifj26.*
@main def main(): Unit =
  val s = "a\\tb\\"c\\"\\\\d\\ne"
  print(s)
""",
    expected_stdout='a\tb"c"\\d\ne',
)

add_test(
    "base_lex_multiline_string",
    "ZAKLAD: Lexika",
    """import ifj26.*
@main def main(): Unit =
  val ms = \"\"\"prvy
druhy\\nbez_escape\"\"\"
  print(ms)
""",
    expected_stdout="prvy\ndruhy\\nbez_escape",
)

add_test(
    "base_lex_comments_line_and_nested_block",
    "ZAKLAD: Lexika",
    """import ifj26.*
// jednoriadkovy komentar
/* blokovy /* vnoreny */ komentar */
@main def main(): Unit =
  val/*medzera*/x = 7 // komentar na konci
  print(x)
""",
    expected_stdout="7",
)


# ============================================================================
# 2. ZÁKLADNÁ STAVBA – ŠTRUKTÚRA PROGRAMU A @main
# ============================================================================

add_test(
    "base_struct_minimal_main",
    "ZAKLAD: Struktura",
    """import ifj26.*
@main def main(): Unit =
  print(1)
""",
    expected_stdout="1",
)

add_test(
    "base_struct_main_custom_name_and_newline",
    "ZAKLAD: Struktura",
    """import ifj26.*
@main
def spustac_1(): Unit =
  print("OK")
""",
    expected_stdout="OK",
)

add_test(
    "base_struct_semicolon_statements",
    "ZAKLAD: Struktura",
    """import ifj26.*
@main def main(): Unit =
  var x = 2; x = x + 3; print(x)
""",
    expected_stdout="5",
)

add_test(
    "base_struct_multiline_parens_and_trailing_comma",
    "ZAKLAD: Struktura",
    """import ifj26.*
def add(
    a: Int,
    b: Int,
  ): Int =
  val s = (
      a +
      b
    )
  s

@main def main(): Unit =
  val r = add(
      10,
      20,
    )
  print(r)
""",
    expected_stdout="30",
)


# ============================================================================
# 3. ZÁKLADNÁ STAVBA – PREMENNÉ, TYPY A SHADOWING
# ============================================================================

add_test(
    "base_var_val_var_and_inference",
    "ZAKLAD: Premenne",
    """import ifj26.*
@main def main(): Unit =
  val a = 10
  var b = 20
  b = b + a
  val c: Double = 5
  val cs = toStr(c)
  val u: Unit = ()
  print(b)
  print("|")
  print(cs)
  print("|")
  print(u)
""",
    expected_stdout="30|5.00|()",
)

add_test(
    "base_var_block_shadowing",
    "ZAKLAD: Premenne",
    """import ifj26.*
@main def main(): Unit =
  val x = 1
  if x == 1 then
    val x = 2
    print(x)
  else
    ()
  print("|")
  print(x)
""",
    expected_stdout="2|1",
)


# ============================================================================
# 4. ZÁKLADNÁ STAVBA – FUNKCIE A REKURZIA
# ============================================================================

add_test(
    "base_func_one_line_and_multiline",
    "ZAKLAD: Funkcie",
    """import ifj26.*
def dvojnasobok(x: Int): Int = x * 2

def sucet(a: Int, b: Int) =
  val res = a + b
  res

@main def main(): Unit =
  val v1 = dvojnasobok(6)
  val v2 = sucet(v1, 3)
  print(v2)
""",
    expected_stdout="15",
)

add_test(
    "base_func_forward_call_and_recursion",
    "ZAKLAD: Funkcie",
    """import ifj26.*
@main def main(): Unit =
  val f = fact(5)
  print(f)

def fact(n: Int): Int =
  if n <= 1 then
    return 1
  else
    val m = n - 1
    val sub = fact(m)
    return n * sub
""",
    expected_stdout="120",
)

add_test(
    "base_func_mutual_recursion",
    "ZAKLAD: Funkcie",
    """import ifj26.*
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

@main def main(): Unit =
  val r = isEven(6)
  print(r)
""",
    expected_stdout="1",
)

add_test(
    "base_func_unit_return_and_discard",
    "ZAKLAD: Funkcie",
    """import ifj26.*
def nic(x: Int): Unit =
  if x > 0 then
    return 99
  else
    ()

@main def main(): Unit =
  val u = nic(1)
  print(u)
""",
    expected_stdout="()",
)


# ============================================================================
# 5. ZÁKLADNÁ STAVBA – VÝRAZY A OPERÁTORY
# ============================================================================

add_test(
    "base_expr_arithmetic_precedence",
    "ZAKLAD: Vyrazy",
    """import ifj26.*
@main def main(): Unit =
  val a = 2 + 3 * 4
  val b = (2 + 3) * 4
  val c = 20 - 5 - 3
  val d = 24 / 4 / 2
  val e = -7 / 2
  val f = --9
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
  print(f)
""",
    expected_stdout="14|20|12|3|-3|9",
)

add_test(
    "base_expr_double_and_concat",
    "ZAKLAD: Vyrazy",
    """import ifj26.*
@main def main(): Unit =
  val d = 5 / 2.0 + 1
  val ds = toStr(d)
  val s = "IFJ" + "26"
  print(ds)
  print("|")
  print(s)
""",
    expected_stdout="3.50|IFJ26",
)

add_test(
    "base_expr_relational",
    "ZAKLAD: Vyrazy",
    """import ifj26.*
@main def main(): Unit =
  if 3 < 4 then
    print("1")
  else
    print("0")
  if 4 <= 4 then
    print("1")
  else
    print("0")
  if 5 > 5.0 then
    print("1")
  else
    print("0")
  if "abc" < "abd" then
    print("1")
  else
    print("0")
  if "x" != "y" then
    print("1")
  else
    print("0")
""",
    expected_stdout="11011",
)


# ============================================================================
# 6. ZÁKLADNÁ STAVBA – RIADIACE ŠTRUKTÚRY
# ============================================================================

add_test(
    "base_ctrl_if_else",
    "ZAKLAD: Riadenie",
    """import ifj26.*
@main def main(): Unit =
  val x = 10
  if x > 5 then
    print("ANO")
  else
    print("NIE")
""",
    expected_stdout="ANO",
)

add_test(
    "base_ctrl_while",
    "ZAKLAD: Riadenie",
    """import ifj26.*
@main def main(): Unit =
  var i = 1
  var sum = 0
  while i <= 4 do
    sum = sum + i
    i = i + 1
  print(sum)
""",
    expected_stdout="10",
)


# ============================================================================
# 7. ZÁKLADNÁ STAVBA – VSTAVANÉ FUNKCIE
# ============================================================================

def _validate_print_double(stdout: str) -> tuple[bool, str]:
    s = stdout.strip()
    try:
        val = float.fromhex(s)
    except ValueError:
        try:
            val = float(s)
        except ValueError:
            return False, f"Neplatný výstup print(3.5): {stdout!r}"
    if abs(val - 3.5) > 1e-9:
        return False, f"Očakávané 3.5, prišlo {val}"
    return True, ""


add_test(
    "base_builtin_print_double",
    "ZAKLAD: Vstavane",
    """import ifj26.*
@main def main(): Unit =
  print(3.5)
""",
    stdout_validator=_validate_print_double,
)

add_test(
    "base_builtin_tostr_and_toint",
    "ZAKLAD: Vstavane",
    """import ifj26.*
@main def main(): Unit =
  val s1 = toStr(42)
  val s2 = toStr(3.456)
  val s3 = toStr(())
  val i1 = toInt(3.99)
  val i2 = toInt(-3.99)
  print(s1)
  print("|")
  print(s2)
  print("|")
  print(s3)
  print("|")
  print(i1)
  print("|")
  print(i2)
""",
    expected_stdout="42|3.46|()|3|-3",
)

add_test(
    "base_builtin_read_line_int_double",
    "ZAKLAD: Vstavane",
    """import ifj26.*
@main def main(): Unit =
  val l = readLine()
  val i1 = readInt()
  val iBad = readInt()
  val d1 = readDouble()
  val dEof = readDouble()
  val d1s = toStr(d1)
  val dEofs = toStr(dEof)
  print(l)
  print(i1)
  print("|")
  print(iBad)
  print("|")
  print(d1s)
  print("|")
  print(dEofs)
""",
    stdin_data="ahoj\n123\nzle\n4.5\n",
    expected_stdout="ahoj\n123|0|4.50|0.00",
)

add_test(
    "base_builtin_strlength_ord_chr",
    "ZAKLAD: Vstavane",
    """import ifj26.*
@main def main(): Unit =
  val n = strLength("ABCD")
  val o1 = ord("ABCD", 2)
  val o2 = ord("ABCD", 9)
  val o3 = ord("", 0)
  val c = chr(66)
  print(n)
  print("|")
  print(o1)
  print("|")
  print(o2)
  print("|")
  print(o3)
  print("|")
  print(c)
""",
    expected_stdout="4|67|-1|-1|B",
)

add_test(
    "base_builtin_subStr",
    "ZAKLAD: Vstavane",
    """import ifj26.*
@main def main(): Unit =
  val s = subStr("ABCDEF", 1, 4)
  print(s)
""",
    expected_stdout="BCD",
)

add_test(
    "base_builtin_subStr_runtime_err40",
    "ZAKLAD: Vstavane",
    """import ifj26.*
@main def main(): Unit =
  val s = subStr("abc", 0, 5)
  print(s)
""",
    expected_runtime_rc=40,
)


# ============================================================================
# 8. ZÁKLADNÁ STAVBA – CHYBOVÉ NÁVRATOVÉ KÓDY (1 - 7)
# ============================================================================

add_test(
    "base_err1_tab_indent",
    "ZAKLAD: Chyby 1-7",
    "import ifj26.*\n@main def main(): Unit =\n\tprint(1)\n",
    expected_compile_rc=1,
)

add_test(
    "base_err1_leading_zero",
    "ZAKLAD: Chyby 1-7",
    """import ifj26.*
@main def main(): Unit =
  val x = 012
""",
    expected_compile_rc=1,
)

add_test(
    "base_err1_bad_escape",
    "ZAKLAD: Chyby 1-7",
    """import ifj26.*
@main def main(): Unit =
  val s = "ahoj\\x"
""",
    expected_compile_rc=1,
)

add_test(
    "base_err2_missing_prolog",
    "ZAKLAD: Chyby 1-7",
    """@main def main(): Unit =
  print(1)
""",
    expected_compile_rc=2,
)

add_test(
    "base_err2_missing_else",
    "ZAKLAD: Chyby 1-7",
    """import ifj26.*
@main def main(): Unit =
  if 1 < 2 then
    print(1)
""",
    expected_compile_rc=2,
)

add_test(
    "base_err3_undefined_var",
    "ZAKLAD: Chyby 1-7",
    """import ifj26.*
@main def main(): Unit =
  print(neexistuje)
""",
    expected_compile_rc=3,
)

add_test(
    "base_err3_undefined_func",
    "ZAKLAD: Chyby 1-7",
    """import ifj26.*
@main def main(): Unit =
  neexistuje()
""",
    expected_compile_rc=3,
)

add_test(
    "base_err3_missing_main",
    "ZAKLAD: Chyby 1-7",
    """import ifj26.*
def foo(): Unit =
  print(1)
""",
    expected_compile_rc=3,
)

add_test(
    "base_err4_redef_var",
    "ZAKLAD: Chyby 1-7",
    """import ifj26.*
@main def main(): Unit =
  val x = 1
  val x = 2
""",
    expected_compile_rc=4,
)

add_test(
    "base_err4_redef_func",
    "ZAKLAD: Chyby 1-7",
    """import ifj26.*
def f(): Int = 1
def f(): Int = 2
@main def main(): Unit =
  print(1)
""",
    expected_compile_rc=4,
)

add_test(
    "base_err5_wrong_arg_count",
    "ZAKLAD: Chyby 1-7",
    """import ifj26.*
def f(a: Int): Int = a
@main def main(): Unit =
  val x = f(1, 2)
""",
    expected_compile_rc=5,
)

add_test(
    "base_err5_wrong_arg_type",
    "ZAKLAD: Chyby 1-7",
    """import ifj26.*
def f(a: Int): Int = a
@main def main(): Unit =
  val x = f("zle")
""",
    expected_compile_rc=5,
)

add_test(
    "base_err6_type_mismatch",
    "ZAKLAD: Chyby 1-7",
    """import ifj26.*
@main def main(): Unit =
  val x: Int = 3.14
""",
    expected_compile_rc=6,
)

add_test(
    "base_err6_recursion_no_return_type",
    "ZAKLAD: Chyby 1-7",
    """import ifj26.*
def f(n: Int) =
  if n == 0 then
    return 0
  else
    val m = n - 1
    return f(m)
@main def main(): Unit =
  print(1)
""",
    expected_compile_rc=6,
)

add_test(
    "base_err7_assign_to_val",
    "ZAKLAD: Chyby 1-7",
    """import ifj26.*
@main def main(): Unit =
  val x = 1
  x = 2
""",
    expected_compile_rc=7,
)

add_test(
    "base_err7_assign_to_param",
    "ZAKLAD: Chyby 1-7",
    """import ifj26.*
def f(p: Int): Int =
  p = 5
  p
@main def main(): Unit =
  print(1)
""",
    expected_compile_rc=7,
)


# ============================================================================
# 9. ROZŠÍRENIE VALMEM – ČLENY HODNÔT (.toString, .toInt, .floor, .length)
# ============================================================================

add_test(
    "valmem_toString_on_all_types",
    "ROZSIRENIE: VALMEM",
    """import ifj26.*
@main def main(): Unit =
  val i = 42
  val d = 1.2345
  val s = "ahoj"
  val u = ()
  val s1 = i.toString
  val s2 = d.toString
  val s3 = s.toString
  val s4 = u.toString
  print(s1)
  print("|")
  print(s2)
  print("|")
  print(s3)
  print("|")
  print(s4)
""",
    expected_stdout="42|1.23|ahoj|()",
)

add_test(
    "valmem_toInt_on_double",
    "ROZSIRENIE: VALMEM",
    """import ifj26.*
@main def main(): Unit =
  val d = -3.9
  val res = d.toInt
  print(res)
""",
    expected_stdout="-3",
)

add_test(
    "valmem_floor_on_double",
    "ROZSIRENIE: VALMEM",
    """import ifj26.*
@main def main(): Unit =
  val a = 3.9
  val b = -3.1
  val af = a.floor
  val bf = b.floor
  val s1 = toStr(af)
  val s2 = toStr(bf)
  print(s1)
  print("|")
  print(s2)
""",
    expected_stdout="3.00|-4.00",
)

add_test(
    "valmem_length_on_string",
    "ROZSIRENIE: VALMEM",
    """import ifj26.*
@main def main(): Unit =
  val s = "IFJ26"
  val l1 = s.length
  val l2 = "".length
  print(l1)
  print("|")
  print(l2)
""",
    expected_stdout="5|0",
)

add_test(
    "valmem_chaining",
    "ROZSIRENIE: VALMEM",
    """import ifj26.*
@main def main(): Unit =
  val x = -2.1
  val r1 = x.floor.toInt
  val r2 = ("ab" + "cde").length.toString
  print(r1)
  print("|")
  print(r2)
""",
    expected_stdout="-3|5",
)

add_test(
    "valmem_err_length_on_int",
    "ROZSIRENIE: VALMEM",
    """import ifj26.*
@main def main(): Unit =
  val x = 10.length
""",
    expected_compile_rc=[3, 6],
)

add_test(
    "valmem_err_floor_on_string",
    "ROZSIRENIE: VALMEM",
    """import ifj26.*
@main def main(): Unit =
  val x = "1.5".floor
""",
    expected_compile_rc=[3, 6],
)


# ============================================================================
# 10. ROZŠÍRENIE INTERPOLATEDS – INTERPOLOVANÉ REŤAZCE (s"...", s"""...""")
# ============================================================================

add_test(
    "interp_simple_variables",
    "ROZSIRENIE: INTERPOLATEDS",
    """import ifj26.*
@main def main(): Unit =
  val name = "IFJ"
  val year = 26
  val s = s"$name$year"
  print(s)
""",
    expected_stdout="IFJ26",
)

add_test(
    "interp_expressions_and_double_format",
    "ROZSIRENIE: INTERPOLATEDS",
    """import ifj26.*
@main def main(): Unit =
  val n = 26
  val price = 1.2345
  val s = s"next=${n + 1}, price=${price}"
  print(s)
""",
    expected_stdout="next=27, price=1.23",
)

add_test(
    "interp_escaped_dollar_and_quotes",
    "ROZSIRENIE: INTERPOLATEDS",
    """import ifj26.*
@main def main(): Unit =
  val s = s"He said \\"hi\\" for $$10"
  print(s)
""",
    expected_stdout='He said "hi" for $10',
)

add_test(
    "interp_multiline_string",
    "ROZSIRENIE: INTERPOLATEDS",
    """import ifj26.*
@main def main(): Unit =
  val who = "World"
  val s = s\"\"\"Hello "$who"!
Value: ${2 * 21}, $$end\"\"\"
  print(s)
""",
    expected_stdout='Hello "World"!\nValue: 42, $end',
)

add_test(
    "interp_combined_with_valmem",
    "ROZSIRENIE: INTERPOLATEDS",
    """import ifj26.*
@main def main(): Unit =
  val w = "Brno"
  val x = -2.3
  val s = s"$w len=${w.length}, floor=${x.floor.toInt}"
  print(s)
""",
    expected_stdout="Brno len=4, floor=-3",
)

add_test(
    "interp_err_undefined_var",
    "ROZSIRENIE: INTERPOLATEDS",
    """import ifj26.*
@main def main(): Unit =
  val s = s"$neexistuje"
""",
    expected_compile_rc=3,
)

add_test(
    "interp_err_type_error_in_braces",
    "ROZSIRENIE: INTERPOLATEDS",
    """import ifj26.*
@main def main(): Unit =
  val s = s"${"abc" - 1}"
""",
    expected_compile_rc=6,
)


# ============================================================================
# SPUŠŤAČ A DIAGNOSTIKA
# ============================================================================

def format_numbered_code(code: str) -> str:
    lines = code.rstrip("\n").split("\n")
    return "\n".join(f"         {idx:2d} | {line}" for idx, line in enumerate(lines, 1))


def run_tests(compiler_path: str, interp_path: str, filter_kw: Optional[str], only_fail: bool) -> int:
    selected = [
        t for t in TESTS
        if not filter_kw or filter_kw.lower() in t.name.lower() or filter_kw.lower() in t.group.lower()
    ]

    if not selected:
        print(f"Žiadne testy nezodpovedajú filtru: {filter_kw!r}")
        return 1

    group_stats: dict[str, list[int]] = {}
    current_group = None

    with tempfile.TemporaryDirectory(prefix="ifj26_atomic_") as tmpdir:
        for tc in selected:
            if tc.group not in group_stats:
                group_stats[tc.group] = [0, 0]
            group_stats[tc.group][1] += 1

            if tc.group != current_group:
                current_group = tc.group
                print(f"\n=== {current_group} ===")

            code_file = os.path.join(tmpdir, f"{tc.name}.ifjcode26")

            try:
                comp_proc = subprocess.run(
                    [compiler_path],
                    input=tc.code.encode("utf-8"),
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    timeout=5,
                )
            except FileNotFoundError:
                print(f"CHYBA: Prekladač nebol nájdený: {compiler_path}")
                return 2
            except subprocess.TimeoutExpired:
                print(f"  [FAIL] {tc.name}: Timeout pri preklade")
                print(format_numbered_code(tc.code))
                continue

            expected_rcs = (
                [tc.expected_compile_rc]
                if isinstance(tc.expected_compile_rc, int)
                else tc.expected_compile_rc
            )

            if comp_proc.returncode not in expected_rcs:
                stderr_txt = comp_proc.stderr.decode("utf-8", errors="replace").strip()
                print(
                    f"  [FAIL] {tc.name}: prekladač vrátil RC={comp_proc.returncode}, očakávané {expected_rcs}"
                )
                if stderr_txt:
                    print(f"         stderr: {stderr_txt}")
                print(format_numbered_code(tc.code))
                continue

            if expected_rcs != [0]:
                group_stats[tc.group][0] += 1
                if not only_fail:
                    print(f"  [PASS] {tc.name} (compile RC={comp_proc.returncode})")
                continue

            generated_code = comp_proc.stdout
            if not generated_code.strip():
                print(f"  [FAIL] {tc.name}: Prekladač vrátil 0, ale výstupný IFJcode26 je prázdny")
                print(format_numbered_code(tc.code))
                continue

            with open(code_file, "wb") as f:
                f.write(generated_code)

            try:
                interp_proc = subprocess.run(
                    [interp_path, code_file],
                    input=tc.stdin_data.encode("utf-8"),
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    timeout=5,
                )
            except FileNotFoundError:
                print(f"CHYBA: Interpret nebol nájdený: {interp_path}")
                return 2
            except subprocess.TimeoutExpired:
                print(f"  [FAIL] {tc.name}: Timeout pri interpretácii")
                continue

            if interp_proc.returncode != tc.expected_runtime_rc:
                stderr_txt = interp_proc.stderr.decode("utf-8", errors="replace").strip()
                print(
                    f"  [FAIL] {tc.name}: interpret vrátil RC={interp_proc.returncode}, "
                    f"očakávané {tc.expected_runtime_rc}"
                )
                if stderr_txt:
                    print(f"         interp stderr: {stderr_txt}")
                print(format_numbered_code(tc.code))
                continue

            if tc.expected_runtime_rc != 0:
                group_stats[tc.group][0] += 1
                if not only_fail:
                    print(f"  [PASS] {tc.name} (runtime RC={interp_proc.returncode})")
                continue

            actual_stdout = interp_proc.stdout.decode("utf-8", errors="replace")

            if tc.stdout_validator is not None:
                ok, reason = tc.stdout_validator(actual_stdout)
                if not ok:
                    print(f"  [FAIL] {tc.name}: {reason}")
                    print(format_numbered_code(tc.code))
                    continue
            elif tc.expected_stdout is not None and actual_stdout != tc.expected_stdout:
                print(
                    f"  [FAIL] {tc.name}: Výstup nesedí\n"
                    f"         Očakávané: {tc.expected_stdout!r}\n"
                    f"         Skutočné:  {actual_stdout!r}"
                )
                print(format_numbered_code(tc.code))
                continue

            group_stats[tc.group][0] += 1
            if not only_fail:
                print(f"  [PASS] {tc.name}")

    print("\n==================== SÚHRN ====================")
    total_passed = 0
    total_all = 0
    for grp, (p, t) in group_stats.items():
        total_passed += p
        total_all += t
        status = "OK" if p == t else "CHYBY"
        print(f"  {grp:<28} {p:2d}/{t:2d}  [{status}]")
    print("-----------------------------------------------")
    print(f"  SPOLU:                       {total_passed:2d}/{total_all:2d}")
    return 0 if total_passed == total_all else 1


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Atomické testy pre IFJ26 (bez FUNEXP) + VALMEM + INTERPOLATEDS")
    parser.add_argument("--compiler", default="./compiler", help="Cesta k prekladaču (default: ./compiler)")
    parser.add_argument("--interp", default="./ic26int-linux-x86_64", help="Cesta k interpretu (default: ./ic26int-linux-x86_64)")
    parser.add_argument("-k", "--filter", default=None, help="Filter názvu alebo skupiny testu")
    parser.add_argument("--only-fail", action="store_true", help="Vypísať len zlyhané testy a súhrn")
    args = parser.parse_args()

    sys.exit(run_tests(args.compiler, args.interp, args.filter, args.only_fail))