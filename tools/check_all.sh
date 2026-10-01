#!/bin/sh
# Full local verification (what a CI job would run). From the repo root, in the build environment:
#   sh tools/check_all.sh            (from Windows/Git Bash: tools/wsl sh tools/check_all.sh)
#
# 1. lint: mechanical source rules, FAKEMATCH notes, no absolute home paths (tools/lint.py)
# 2. citations: every path:line in the docs exists and says what the text says (tools/check_refs.py)
# 3. configure + ninja: all 13 targets byte-identical to the original (main + 12 overlays)
# 4. every C unit matches on its own (tools/linkable.py)
# 5. shiftability: every game function moved, relinked, all relocations verified
# 6. config/standin_deps.txt is current: every stand-in's dependents measured again
# 7. the generated reports are current: regenerated and compared with the working copy
#    (PROGRESS.md, docs/stand-ins.md, docs/dwarf-fidelity.md, docs/layout-fidelity.md)
# The full output of steps 4, 5 and the fidelity reports is kept in build/check_all/.
set -e
PY=${PY:-.venv/bin/python}
LOG=build/check_all
mkdir -p "$LOG"
step() { printf '\n== %s\n' "$1"; }

# regen <file> <command...>: rerun the generator of a committed file; fail if the file changed
# (the new version is left in place, to review and commit).
regen() {
    f=$1
    shift
    cp "$f" "$LOG/previous" 2>/dev/null || : > "$LOG/previous"
    "$@"
    cmp -s "$f" "$LOG/previous" || {
        echo "$f was stale: regenerated; review it (git diff $f) and commit it"; exit 1; }
    echo "$f is current"
}
standins_md() { $PY tools/standins.py > docs/stand-ins.md; }
dwarf_md() { J=4 $PY tools/dwarf_compare.py > "$LOG/dwarf_compare.txt"; tail -1 "$LOG/dwarf_compare.txt"; }
layout_md() { $PY tools/layout_compare.py -j4 > "$LOG/layout_compare.txt"; tail -1 "$LOG/layout_compare.txt"; }

step "lint"
$PY tools/lint.py

step "citations"
$PY tools/check_refs.py

step "configure + build"
$PY configure.py
ninja

step "units"
$PY tools/linkable.py > "$LOG/linkable.txt" || true
tail -1 "$LOG/linkable.txt"
tail -1 "$LOG/linkable.txt" | grep -q "^337/337 units LINKABLE" || { echo "not every unit links ($LOG/linkable.txt)"; exit 1; }

step "shift test"
$PY tools/shift_test.py > "$LOG/shift_test.txt" || true
tail -4 "$LOG/shift_test.txt"
grep -q "^PASS" "$LOG/shift_test.txt" || { echo "shift test failed ($LOG/shift_test.txt)"; exit 1; }

step "stand-in dependents"
$PY tools/standin_deps.py --check || { echo "config/standin_deps.txt is out of date"; exit 1; }

step "generated reports"
regen PROGRESS.md $PY tools/progress.py
regen docs/stand-ins.md standins_md
regen docs/dwarf-fidelity.md dwarf_md
regen docs/layout-fidelity.md layout_md

printf '\nall checks passed\n'
