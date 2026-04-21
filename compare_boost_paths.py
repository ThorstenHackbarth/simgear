#!/usr/bin/env python3
"""
compare_boost_paths.py

Verify that Boost code paths are unchanged between two branches by simulating
"cpp -USG_NO_BOOST" (i.e. SG_NO_BOOST is NOT defined).

Algorithm
---------
For each C/C++ file that differs between BRANCH_NEW and BRANCH_BASE:
  1. Extract source from both branches via `git show`.
  2. Strip all #ifdef SG_NO_BOOST blocks (keep only the #else / boost paths),
     leaving the file as the compiler sees it when building WITH Boost.
  3. Normalise preprocessor-directive whitespace: the C standard allows any
     amount of horizontal whitespace between '#' and the keyword, so
     '#  include' and '#include' are identical to the preprocessor.
  4. Diff the two normalised results.

New files (not present on base) are flagged separately.
Non-C/C++ files (CMakeLists, .py, …) are skipped.

Usage
-----
    python3 compare_boost_paths.py [--base BRANCH] [--new BRANCH] [--repo DIR]
                                   [--show-ok]

Defaults
--------
    --base  next
    --new   claude/remove-boost-dependencies-meYK7
    --repo  (directory of this script)
"""

import argparse
import difflib
import re
import subprocess
import sys
from pathlib import Path


# ── source-code file extensions we care about ─────────────────────────────────

CXX_EXTENSIONS = {".cxx", ".cpp", ".cc", ".c", ".hxx", ".hpp", ".hh", ".h"}

# Matches a C/C++ preprocessor directive line (leading whitespace + # + keyword)
_PP_DIRECTIVE_RE = re.compile(r'^(\s*)#(\s+)(\w+)(.*)', re.DOTALL)


# ── ifdef stripper ─────────────────────────────────────────────────────────────

def _evaluate_condition(tokens: list[str], macro: str, defined: bool) -> bool | None:
    """
    Return True/False when the directive uniquely resolves to macro, else None.
    Handles: #ifdef M, #ifndef M, #if defined(M), #if !defined(M).
    """
    s = " ".join(tokens)
    s = s.replace("(", " ( ").replace(")", " ) ").replace("!", " ! ")
    w = s.split()

    if len(w) == 2 and w[0] == "ifdef"  and w[1] == macro: return defined
    if len(w) == 2 and w[0] == "ifndef" and w[1] == macro: return not defined
    if w[:2] == ["if", "defined"] and len(w) == 4 and w[3] == macro: return defined
    if w[:3] == ["if", "!", "defined"] and len(w) == 5 and w[4] == macro: return not defined
    return None


def strip_macro(source: str, macro: str, defined: bool) -> str:
    """
    Return source with all #ifdef/#ifndef blocks for macro resolved as if
    macro is defined (True) or undefined (False).

    Blocks that don't involve macro are passed through verbatim.
    """
    out: list[str] = []
    # Stack: (macro_controlled: bool, keep_current_section: bool)
    stack: list[tuple[bool, bool]] = []

    def active() -> bool:
        return all(k for _, k in stack) if stack else True

    for line in source.splitlines(keepends=True):
        stripped = line.lstrip()
        if not stripped.startswith("#"):
            if active(): out.append(line)
            continue

        body      = stripped[1:].lstrip()
        tokens    = body.split()
        if not tokens:
            if active(): out.append(line)
            continue
        directive = tokens[0]
        rest      = tokens[1:]

        if directive in ("ifdef", "ifndef", "if"):
            result = _evaluate_condition([directive] + rest, macro, defined)
            if result is not None:
                stack.append((True, result))      # our macro – resolve
            else:
                stack.append((False, True))       # unknown – pass through
                if active(): out.append(line)

        elif directive == "elif":
            if stack and stack[-1][0]:
                stack[-1] = (True, False)         # our macro's elif – dead
            else:
                if active(): out.append(line)

        elif directive == "else":
            if stack and stack[-1][0]:
                known, keep = stack[-1]
                stack[-1] = (known, not keep)     # flip for our macro
            else:
                if active(): out.append(line)

        elif directive == "endif":
            if stack and stack[-1][0]:
                stack.pop()                       # our macro – drop silently
            else:
                if stack: stack.pop()
                if active(): out.append(line)

        else:
            if active(): out.append(line)

    return "".join(out)


def normalise_pp_directives(source: str) -> str:
    """
    Normalise horizontal whitespace between '#' and the directive keyword.

    The C standard (§6.10) says "A preprocessing directive … begins with a #
    preprocessing token" — any amount of whitespace between '#' and the
    keyword is valid and has no semantic effect.  We collapse it to zero so
    '#  include' and '#include' compare equal.
    """
    result = []
    for line in source.splitlines(keepends=True):
        m = _PP_DIRECTIVE_RE.match(line)
        if m and m.group(2):          # there IS whitespace between # and keyword
            indent, _, keyword, rest = m.groups()
            line = f"{indent}#{keyword}{rest}"
        result.append(line)
    return "".join(result)


# ── git helpers ────────────────────────────────────────────────────────────────

def git(repo: Path, *args: str) -> subprocess.CompletedProcess:
    return subprocess.run(
        ["git", "-C", str(repo)] + list(args),
        capture_output=True, text=True,
    )


def git_show(repo: Path, ref: str, path: str) -> str | None:
    r = git(repo, "show", f"{ref}:{path}")
    return r.stdout if r.returncode == 0 else None


def changed_files(repo: Path, base: str, new: str) -> list[str]:
    r = git(repo, "diff", "--name-only", base, new)
    if r.returncode != 0:
        sys.exit(f"git diff failed:\n{r.stderr}")
    return [f for f in r.stdout.splitlines() if f.strip()]


def prepare(source: str, macro: str) -> str:
    """Strip macro blocks, then normalise preprocessor whitespace."""
    return normalise_pp_directives(strip_macro(source, macro, defined=False))


# ── main ───────────────────────────────────────────────────────────────────────

def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--base", default="next",
                        help="Base branch (default: next)")
    parser.add_argument("--new",  default="claude/remove-boost-dependencies-meYK7",
                        help="Feature branch (default: claude/remove-boost-dependencies-meYK7)")
    parser.add_argument("--repo", default=str(Path(__file__).parent),
                        help="Path to the git repository")
    parser.add_argument("--show-ok", action="store_true",
                        help="Also list files whose Boost path is unchanged")
    args = parser.parse_args()

    repo  = Path(args.repo)
    base  = args.base
    new   = args.new
    macro = "SG_NO_BOOST"

    print(f"Repository : {repo}")
    print(f"Base branch: {base}")
    print(f"New branch : {new}")
    print(f"Macro      : {macro}  (treated as UNDEFINED — simulates build WITH Boost)")
    print()

    all_changed = changed_files(repo, base, new)
    if not all_changed:
        print("No files differ between branches.")
        return 0

    cxx_files   = [f for f in all_changed if Path(f).suffix in CXX_EXTENSIONS]
    other_files = [f for f in all_changed if Path(f).suffix not in CXX_EXTENSIONS]

    print(f"Files changed between branches : {len(all_changed)}")
    print(f"  C/C++ source files            : {len(cxx_files)}")
    print(f"  Non-C/C++ (skipped)           : {len(other_files)}")
    for f in other_files:
        print(f"    skip  {f}")
    print()

    # Categorise each C/C++ file
    new_files:     list[str] = []
    changed_guard: list[str] = []   # has SG_NO_BOOST guard
    changed_plain: list[str] = []   # no guard at all

    for filepath in cxx_files:
        src_base = git_show(repo, base, filepath)
        src_new  = git_show(repo, new,  filepath)

        if src_base is None:
            new_files.append(filepath)
            continue

        if macro in (src_new or "") or macro in (src_base or ""):
            changed_guard.append(filepath)
        else:
            changed_plain.append(filepath)

    # ── New files ──────────────────────────────────────────────────────────────
    if new_files:
        print(f"New C/C++ files (not on base branch) — {len(new_files)}:")
        for f in new_files:
            print(f"  new   {f}")
        print()

    # ── Plain changes (no SG_NO_BOOST guard) — always affect Boost build ───────
    if changed_plain:
        print(f"C/C++ files changed WITHOUT any {macro} guard — {len(changed_plain)}.")
        print("These changes always affect the Boost build path:")
        for f in changed_plain:
            print(f"  WARN  {f}")
        print()

    # ── Files with guards: compare stripped+normalised versions ───────────────
    print(f"C/C++ files with {macro} guards: checking Boost path — {len(changed_guard)}")
    print()

    diffs_found = 0
    for filepath in changed_guard:
        src_base = git_show(repo, base, filepath) or ""
        src_new  = git_show(repo, new,  filepath) or ""

        prep_base = prepare(src_base, macro)
        prep_new  = prepare(src_new,  macro)

        if prep_base == prep_new:
            if args.show_ok:
                print(f"  ok    {filepath}")
            continue

        diffs_found += 1
        print(f"  DIFF  {filepath}")
        diff_lines = list(difflib.unified_diff(
            prep_base.splitlines(keepends=True),
            prep_new.splitlines(keepends=True),
            fromfile=f"  {base}:{filepath}",
            tofile=f"  {new}:{filepath}",
            lineterm="",
        ))
        for dl in diff_lines:
            sys.stdout.write("        " + dl + "\n")
        print()

    if not diffs_found:
        if not args.show_ok:
            print(f"  (all {len(changed_guard)} files: Boost path unchanged)")
    print()

    # ── Summary ────────────────────────────────────────────────────────────────
    warn_count = len(changed_plain)
    print("─" * 60)
    if diffs_found == 0 and warn_count == 0:
        print("✓  Boost code paths are IDENTICAL between branches.")
        return 0
    elif diffs_found == 0:
        print("✓  Guarded files: Boost paths identical.")
        print(f"⚠  {warn_count} unguarded C/C++ file(s) changed — inspect above.")
        return 0
    else:
        if diffs_found:
            print(f"✗  {diffs_found} guarded file(s) have Boost path differences.")
        if warn_count:
            print(f"⚠  {warn_count} unguarded C/C++ file(s) changed.")
        return 1


if __name__ == "__main__":
    sys.exit(main())
