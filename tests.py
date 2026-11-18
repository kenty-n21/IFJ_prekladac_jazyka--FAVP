#!/usr/bin/env python3
"""
Komplexna testovacia sada pre prekladac IFJ26 -> IFJcode26 (ZAKLADNE ZADANIE - bez rozsireni).
Obsahuje 23 testov:
  - 01..06: funkcne testy zakladneho zadania (rekurzia a ramce, shadowing, aritmetika a konverzie,
            relacne vyrazy a retazce, vstavane funkcie a I/O, odvodenie navratoveho typu a komentare)
  - 07..08: behove chyby (40)
  - 09..23: lexikalne, syntakticke a semanticke chyby prekladaca (1..10)
"""

import argparse
import difflib
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

TESTS = [
    # --------------------------------------------------------------------------
    # 1. Funkcne testy zakladneho zadania (bez akychkolvek rozsireni)
    # --------------------------------------------------------------------------
    {
        "name": "01_base_recursion_frames",
        "code": """import ifj26.*

def isEven(n: Int): Int =
  if n == 0 then
    return 1
  else
    val next: Int = n - 1
    val res: Int = isOdd(next)
    return res

def isOdd(n: Int): Int =
  if n == 0 then
    return 0
  else
    val next: Int = n - 1
    val res: Int = isEven(next)
    return res

def fib(n: Int): Int =
  if n <= 1 then
    return n
  else
    val n1: Int = n - 1
    val n2: Int = n - 2
    val a: Int = fib(n1)
    val b: Int = fib(n2)
    val sum: Int = a + b
    return sum

@main def main(): Unit =
  val n: Int = readInt()
  val f: Int = fib(n)
  val e: Int = isEven(f)
  print(f)
  print(" ")
  print(e)
  print("\\n")
""",
        "stdin": "7\n",
        "stdout": "13 0\n",
        "comp_rc": 0,
        "int_rc": 0,
    },
    {
        "name": "02_base_shadowing_blocks",
        "code": """import ifj26.*

@main def main(): Unit =
  val x: Int = 10
  var acc: Int = 0
  if x > 5 then
    val x: String = "inner"
    print(x)
    print("|")
    var i: Int = 0
    while i < 3 do
      val x: Int = 4
      acc = acc + x; i = i + 1
    print(x)
    print("|")
  else
    ()

  print(x)
  print("|")
  print(acc)
  print("\\n")
""",
        "stdin": "",
        "stdout": "inner|inner|10|12\n",
        "comp_rc": 0,
        "int_rc": 0,
    },
    {
        "name": "03_base_arithmetic_conversions",
        "code": """import ifj26.*

@main def main(): Unit =
  val big: Int = 1_000_000
  val dLit: Double = 2_5.5_0
  val dFromInt: Double = 5

  val d1: Int = -5 / 2
  val d2: Int = 5 / -2
  val d3: Int = -5 / -2
  val prec: Int = 2 + 3 * 4 - 10 / 3

  val mixed: Double = dFromInt + dLit / 2 - 1
  val negD: Double = -3.9
  val posD: Double = 3.9
  val t1: Int = toInt(posD)
  val t2: Int = toInt(negD)

  val sMixed: String = toStr(mixed)
  print(big)
  print("|")
  print(d1)
  print("|")
  print(d2)
  print("|")
  print(d3)
  print("|")
  print(prec)
  print("|")
  print(sMixed)
  print("|")
  print(t1)
  print("|")
  print(t2)
  print("\\n")
""",
        "stdin": "",
        "stdout": "1000000|-2|-2|2|11|16.75|3|-3\n",
        "comp_rc": 0,
        "int_rc": 0,
    },
    {
        "name": "04_base_relational_strings",
        "code": """import ifj26.*

@main def main(): Unit =
  val i: Int = 5
  val d: Double = 5.0
  if i == d then
    if 4 < 4.5 then
      print("num_ok|")
    else
      print("num_fail|")
  else
    print("eq_fail|")

  val s1: String = "abc"
  val s2: String = "abd"
  if s1 < s2 then
    if s1 != s2 then
      print("str_ok|")
    else
      print("str_fail|")
  else
    print("str_fail|")

  val esc: String = "A\\t\\"B\\"\\\\C"
  val raw: String = \"\"\"X
Y\\nZ\"\"\"
  val joined: String = esc + "|" + raw
  print(joined)
  print("\\n")
""",
        "stdin": "",
        "stdout": "num_ok|str_ok|A\t\"B\"\\C|X\nY\\nZ\n",
        "comp_rc": 0,
        "int_rc": 0,
    },
    {
        "name": "05_base_builtins_io",
        "code": """import ifj26.*

@main def main(): Unit =
  val iOk: Int = readInt()
  val iBad: Int = readInt()
  val dOk: Double = readDouble()
  val dBad: Double = readDouble()
  val line1: String = readLine()
  val lineEof: String = readLine()

  val sDouble: String = toStr(dOk)
  val sBad: String = toStr(dBad)
  val sUnit: String = toStr(())
  val eofLen: Int = strLength(lineEof)

  val word: String = "abcdef"
  val wLen: Int = strLength(word)
  val sub1: String = substr(word, 1, 4)
  val subEmpty: String = substr(word, 2, 2)
  val subEmptyLen: Int = strLength(subEmpty)
  val c1: Int = ord(word, 1)
  val cEmpty: Int = ord("", 0)
  val cOob: Int = ord(word, 99)
  val ch: String = chr(c1)

  print(iOk)
  print("|")
  print(iBad)
  print("|")
  print(sDouble)
  print("|")
  print(sBad)
  print("|")
  print(sUnit)
  print("|")
  print(eofLen)
  print("|")
  print(wLen)
  print("|")
  print(sub1)
  print("|")
  print(subEmptyLen)
  print("|")
  print(c1)
  print("|")
  print(cEmpty)
  print("|")
  print(cOob)
  print("|")
  print(ch)
  print("|")
  print(line1)
""",
        "stdin": "42\nnot_int\n3.456\nnot_double\nhello\n",
        "stdout": "42|0|3.46|0.00|()|0|6|bcd|0|98|-1|-1|b|hello\n",
        "comp_rc": 0,
        "int_rc": 0,
    },
    {
        "name": "06_base_returns_inference",
        "code": """import ifj26.*

/* vnoreny /* blokovy */ komentar na jednom riadku */
def addTen(x: Int) = x + 10

def sumThree(
  a: Int,
  b: Int,
  c: Int,
): Int =
  a + b + c

def discardReturn(): Unit =
  print("side_effect|")
  return 999

@main def main(): Unit =
  val a: Int = addTen(5)
  val b: Int = sumThree(
    1,
    2,
    3,
  )
  val u: Unit = discardReturn()
  print(a)
  print("|")
  print(b)
  print("|")
  print(u)
  print("\\n")
""",
        "stdin": "",
        "stdout": "side_effect|15|6|()\n",
        "comp_rc": 0,
        "int_rc": 0,
    },
    # --------------------------------------------------------------------------
    # 2. Negativne testy - behove chyby (40)
    # --------------------------------------------------------------------------
    {
        "name": "07_err40_substr_end_oob",
        "code": """import ifj26.*

@main def main(): Unit =
  val s: String = substr("abc", 1, 4)
""",
        "stdin": "",
        "stdout": "",
        "comp_rc": 0,
        "int_rc": 40,
    },
    {
        "name": "08_err40_substr_start_gt_end",
        "code": """import ifj26.*

@main def main(): Unit =
  val s: String = substr("abc", 2, 1)
""",
        "stdin": "",
        "stdout": "",
        "comp_rc": 0,
        "int_rc": 40,
    },
    # --------------------------------------------------------------------------
    # 3. Negativne testy - staticke chyby prekladaca (1..10)
    # --------------------------------------------------------------------------
    {
        "name": "09_err01_tab_indent",
        "code": "import ifj26.*\n\n@main def main(): Unit =\n\tval a: Int = 1\n",
        "stdin": "",
        "stdout": "",
        "comp_rc": 1,
        "int_rc": 0,
    },
    {
        "name": "10_err01_unclosed_nested_comment",
        "code": """import ifj26.*
/* outer /* inner */
@main def main(): Unit =
  ()
""",
        "stdin": "",
        "stdout": "",
        "comp_rc": 1,
        "int_rc": 0,
    },
    {
        "name": "11_err02_missing_prolog",
        "code": """@main def main(): Unit =
  ()
""",
        "stdin": "",
        "stdout": "",
        "comp_rc": 2,
        "int_rc": 0,
    },
    {
        "name": "12_err02_call_in_expr",
        "code": """import ifj26.*

def one(): Int = 1

@main def main(): Unit =
  val x: Int = 2 + one()
""",
        "stdin": "",
        "stdout": "",
        "comp_rc": 2,
        "int_rc": 0,
    },
    {
        "name": "13_err03_undefined_var",
        "code": """import ifj26.*

@main def main(): Unit =
  if 1 == 1 then
    val a: Int = 1
  else
    ()
  print(a)
""",
        "stdin": "",
        "stdout": "",
        "comp_rc": 3,
        "int_rc": 0,
    },
    {
        "name": "14_err03_missing_main",
        "code": """import ifj26.*

def helper(): Unit =
  ()
""",
        "stdin": "",
        "stdout": "",
        "comp_rc": 3,
        "int_rc": 0,
    },
    {
        "name": "15_err04_redef_var",
        "code": """import ifj26.*

@main def main(): Unit =
  val a: Int = 1
  val a: Int = 2
""",
        "stdin": "",
        "stdout": "",
        "comp_rc": 4,
        "int_rc": 0,
    },
    {
        "name": "16_err04_redef_func",
        "code": """import ifj26.*

def foo(): Int = 1
def foo(x: Int): Int = x

@main def main(): Unit =
  ()
""",
        "stdin": "",
        "stdout": "",
        "comp_rc": 4,
        "int_rc": 0,
    },
    {
        "name": "17_err05_main_param",
        "code": """import ifj26.*

@main def main(x: Int): Unit =
  ()
""",
        "stdin": "",
        "stdout": "",
        "comp_rc": 5,
        "int_rc": 0,
    },
    {
        "name": "18_err05_wrong_arg_type",
        "code": """import ifj26.*

def takesInt(x: Int): Unit =
  ()

@main def main(): Unit =
  takesInt(3.14)
""",
        "stdin": "",
        "stdout": "",
        "comp_rc": 5,
        "int_rc": 0,
    },
    {
        "name": "19_err06_narrowing",
        "code": """import ifj26.*

@main def main(): Unit =
  val x: Int = 3.14
""",
        "stdin": "",
        "stdout": "",
        "comp_rc": 6,
        "int_rc": 0,
    },
    {
        "name": "20_err06_missing_ret_type_rec",
        "code": """import ifj26.*

def countdown(n: Int) =
  if n == 0 then
    0
  else
    val next: Int = n - 1
    countdown(next)

@main def main(): Unit =
  ()
""",
        "stdin": "",
        "stdout": "",
        "comp_rc": 6,
        "int_rc": 0,
    },
    {
        "name": "21_err07_assign_val",
        "code": """import ifj26.*

@main def main(): Unit =
  val x: Int = 1
  x = 2
""",
        "stdin": "",
        "stdout": "",
        "comp_rc": 7,
        "int_rc": 0,
    },
    {
        "name": "22_err07_assign_param",
        "code": """import ifj26.*

def f(p: Int): Unit =
  p = 10

@main def main(): Unit =
  ()
""",
        "stdin": "",
        "stdout": "",
        "comp_rc": 7,
        "int_rc": 0,
    },
    {
        "name": "23_err10_bare_return_nonunit",
        "code": """import ifj26.*

def f(n: Int): Int =
  if n == 0 then
    return
  else
    return 1

@main def main(): Unit =
  ()
""",
        "stdin": "",
        "stdout": "",
        "comp_rc": 10,
        "int_rc": 0,
    },
]


def resolve_binary(given: str | None, candidates: list[str], label: str) -> Path:
    if given:
        p = Path(given)
        if not p.exists():
            which = shutil.which(given)
            if which:
                p = Path(which)
            else:
                print(f"Chyba: {label} '{given}' neexistuje.", file=sys.stderr)
                sys.exit(1)
        if p.is_dir():
            print(f"Chyba: '{p}' je priecinok, nie spustitelny subor.", file=sys.stderr)
            sys.exit(1)
        if os.name != "nt" and not os.access(p, os.X_OK):
            print(
                f"Chyba: {label} '{p}' nema pravo na spustenie (+x). "
                f"Skus: chmod +x {p}",
                file=sys.stderr,
            )
            sys.exit(1)
        return p.resolve()

    for cand in candidates:
        p = Path(cand)
        if p.is_file() and (os.name == "nt" or os.access(p, os.X_OK)):
            return p.resolve()
        which = shutil.which(cand)
        if which:
            return Path(which).resolve()

    for cand in candidates:
        p = Path(cand)
        if p.is_file() and os.name != "nt" and not os.access(p, os.X_OK):
            print(
                f"Chyba: {label} '{p}' existuje, ale nema pravo na spustenie (+x). "
                f"Spusti: chmod +x {p}",
                file=sys.stderr,
            )
            sys.exit(1)

    print(
        f"Chyba: Nenasiel sa {label} (skusane: {', '.join(candidates)}). "
        f"Zadaj cestu ako argument.",
        file=sys.stderr,
    )
    sys.exit(1)


def extract_tests(out_dir: Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    for t in TESTS:
        base = out_dir / t["name"]
        base.with_suffix(".ifj26").write_text(t["code"], encoding="utf-8", newline="\n")
        if t["stdin"]:
            base.with_suffix(".in").write_text(t["stdin"], encoding="utf-8", newline="\n")
        if t["comp_rc"] == 0 and t["int_rc"] == 0:
            base.with_suffix(".out").write_text(t["stdout"], encoding="utf-8", newline="\n")
        base.with_suffix(".rc").write_text(
            f"{t['comp_rc']} {t['int_rc']}\n", encoding="utf-8", newline="\n"
        )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Spusti alebo rozbali testovaciu sadu pre zakladne zadanie prekladaca IFJ26."
    )
    parser.add_argument(
        "compiler",
        nargs="?",
        default=None,
        help="Cesta k binarke prekladaca (napr. ./ifj26 alebo ./compiler)",
    )
    parser.add_argument(
        "interpreter",
        nargs="?",
        default=None,
        help="Cesta k interpretu ic26int (napr. ./ic26int)",
    )
    parser.add_argument(
        "--extract-only",
        action="store_true",
        help="Iba rozbali subory (.ifj26, .in, .out, .rc) do cieloveho adresara",
    )
    parser.add_argument(
        "--dir",
        default="./tests_ifj26_base",
        help="Cielovy adresar pre rozbalene testy (predvolene: ./tests_ifj26_base)",
    )
    parser.add_argument(
        "--filter",
        default="",
        help="Spusti iba testy obsahujuce dany retazec v nazve (napr. base, err40, err06)",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=5.0,
        help="Timeout v sekundach na jeden beh (predvolene: 5.0)",
    )
    args = parser.parse_args()

    out_dir = Path(args.dir)
    extract_tests(out_dir)

    if args.extract_only:
        print(f"Rozbalenych {len(TESTS)} testov do adresara: {out_dir}")
        return 0

    compiler_bin = resolve_binary(
        args.compiler,
        ["./ifj26", "./ifj26.exe", "./compiler", "./compiler.exe", "./main"],
        "prekladac",
    )
    interpreter_bin = resolve_binary(
        args.interpreter,
        ["./ic26int", "./ic26int.exe", "ic26int"],
        "interpret ic26int",
    )

    passed = 0
    failed = 0

    for t in TESTS:
        name = t["name"]
        if args.filter and args.filter not in name:
            continue

        try:
            comp_res = subprocess.run(
                [str(compiler_bin)],
                input=t["code"].encode("utf-8"),
                capture_output=True,
                timeout=args.timeout,
            )
        except subprocess.TimeoutExpired:
            print(f"[FAIL] {name}: compiler timeout ({args.timeout}s)")
            failed += 1
            continue

        if comp_res.returncode != t["comp_rc"]:
            err_msg = comp_res.stderr.decode("utf-8", errors="replace").strip()
            print(
                f"[FAIL] {name}: compiler rc={comp_res.returncode} (expected {t['comp_rc']})"
            )
            if err_msg:
                print(f"       stderr: {err_msg}")
            failed += 1
            continue

        if t["comp_rc"] != 0:
            print(f"[PASS] {name} (compile error {comp_res.returncode})")
            passed += 1
            continue

        with tempfile.NamedTemporaryFile(mode="wb", suffix=".code", delete=False) as tmp:
            tmp.write(comp_res.stdout)
            tmp_path = Path(tmp.name)

        try:
            try:
                int_res = subprocess.run(
                    [str(interpreter_bin), str(tmp_path)],
                    input=t["stdin"].encode("utf-8"),
                    capture_output=True,
                    timeout=args.timeout,
                )
            except subprocess.TimeoutExpired:
                print(f"[FAIL] {name}: interpreter timeout ({args.timeout}s)")
                failed += 1
                continue

            if int_res.returncode != t["int_rc"]:
                err_msg = int_res.stderr.decode("utf-8", errors="replace").strip()
                print(
                    f"[FAIL] {name}: interpreter rc={int_res.returncode} (expected {t['int_rc']})"
                )
                if err_msg:
                    print(f"       stderr: {err_msg}")
                failed += 1
                continue

            if t["int_rc"] == 0:
                actual_out = int_res.stdout.decode("utf-8", errors="replace").replace("\r\n", "\n")
                expected_out = t["stdout"]
                if actual_out != expected_out:
                    print(f"[FAIL] {name}: stdout mismatch")
                    diff = difflib.unified_diff(
                        expected_out.splitlines(keepends=True),
                        actual_out.splitlines(keepends=True),
                        fromfile="expected",
                        tofile="actual",
                    )
                    sys.stdout.writelines(diff)
                    failed += 1
                    continue

            print(f"[PASS] {name}")
            passed += 1
        finally:
            tmp_path.unlink(missing_ok=True)

    print("-" * 40)
    print(f"Vysledok: {passed} uspesnych, {failed} neuspesnych")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())