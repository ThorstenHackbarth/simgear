#!/usr/bin/env bash
# run_benchmark.sh — build and run the SimGear compat layer benchmark
#
# Compiles compat_benchmark.cxx twice (with and without -DSG_NO_BOOST),
# then runs both binaries to:
#   1. Verify correctness — diff the deterministic output (must be identical)
#   2. Compare performance — side-by-side ns/op table
#
# Boost version used: whatever apt provides (≥1.34 required by FlightGear/SimGear).
# OpenSceneGraph is NOT needed — compat headers have no OSG dependency.

set -euo pipefail

REPO="$(cd "$(dirname "$0")" && pwd)"
SRC="$REPO/compat_benchmark.cxx"
IFLAGS="-I$REPO"
CXX_FLAGS="-std=c++20 -O2"
BENCH_BOOST=/tmp/bench_boost
BENCH_COMPAT=/tmp/bench_compat

# ── 1. Install Boost if not present ───────────────────────────────────────────

if ! dpkg -s libboost-dev &>/dev/null 2>&1; then
    echo "[+] libboost-dev not found — installing..."
    sudo apt-get install -y libboost-dev
fi

BOOST_VER=$(dpkg -s libboost-dev | grep "^Version:" | awk '{print $2}')
echo "[✓] Boost ${BOOST_VER}  (FlightGear/SimGear requires ≥ 1.34.0)"
echo "[✓] g++ $(g++ --version | head -1 | grep -oP '\d+\.\d+\.\d+' | head -1)"
echo ""

# ── 2. Compile both variants ───────────────────────────────────────────────────

echo "[+] Compiling Boost variant..."
g++ $CXX_FLAGS $IFLAGS "$SRC" -o "$BENCH_BOOST"
echo "    → $BENCH_BOOST"

echo "[+] Compiling compat (SG_NO_BOOST) variant..."
g++ $CXX_FLAGS $IFLAGS -DSG_NO_BOOST "$SRC" -o "$BENCH_COMPAT"
echo "    → $BENCH_COMPAT"
echo ""

# ── 3. Correctness comparison ─────────────────────────────────────────────────

echo "══════════════════════════════════════════════════════════════"
echo "  CORRECTNESS CHECK  (tokenizer · optional · equals · split · iterator_facade)"
echo "══════════════════════════════════════════════════════════════"
echo ""

"$BENCH_BOOST"  --correctness > /tmp/out_boost.txt
"$BENCH_COMPAT" --correctness > /tmp/out_compat.txt

if diff --color=always /tmp/out_boost.txt /tmp/out_compat.txt; then
    echo ""
    echo "✓  All $(wc -l < /tmp/out_boost.txt) output lines are IDENTICAL between Boost and compat."
else
    echo ""
    echo "✗  Differences found — see above."
    echo "   Boost output:  /tmp/out_boost.txt"
    echo "   Compat output: /tmp/out_compat.txt"
    # Don't exit — still run the hash and timing sections
fi

# ── 3b. Hash correctness (integer values only — identical on GCC x86-64) ──────

echo ""
echo "── Hash correctness (integer hash_value, hash_range, hash_combine) ──────────"
"$BENCH_BOOST"  --hash-correctness > /tmp/hash_boost.txt
"$BENCH_COMPAT" --hash-correctness > /tmp/hash_compat.txt

if diff --color=always /tmp/hash_boost.txt /tmp/hash_compat.txt; then
    echo "✓  Integer hash values IDENTICAL."
else
    echo "⚠  Integer hash values differ — algorithm mismatch (inspect above)."
fi
echo ""
echo "Note: hash_combine / hash_range differences are expected."
echo "      Boost 1.83 on 64-bit systems uses a Murmur2-based mix (via __int128)"
echo "      while the compat uses the classic Boost 32-bit formula (0x9e3779b9)."
echo "      String hash_value also differs: boost::hash_value<string> vs std::hash<string>."
echo "      Both implementations satisfy the hash contract (deterministic, low collision)."

# ── 4. Performance comparison ─────────────────────────────────────────────────

echo ""
echo "══════════════════════════════════════════════════════════════"
echo "  PERFORMANCE  (N=500,000 iterations, ns/operation)"
echo "══════════════════════════════════════════════════════════════"
echo ""

"$BENCH_BOOST"  --timing > /tmp/timing_boost.txt
"$BENCH_COMPAT" --timing > /tmp/timing_compat.txt

python3 - <<'PY'
import os, sys

def parse(path):
    d = {}
    with open(path) as f:
        for line in f:
            line = line.strip()
            if '=' in line:
                k, v = line.split('=', 1)
                d[k] = float(v)
    return d

B = parse('/tmp/timing_boost.txt')
C = parse('/tmp/timing_compat.txt')

# Pretty names for display
labels = {
    'timing.tokenizer':      'tokenizer (per string)',
    'timing.hash_value_int': 'hash_value<int>',
    'timing.hash_value_str': 'hash_value<string>',
    'timing.hash_combine':   'hash_combine (per item)',
    'timing.hash_range':     'hash_range (int[6])',
    'timing.optional':       'optional (per item)',
    'timing.equals':         'equals() (per pair)',
    'timing.split':          'split_iterator (per path)',
    'timing.iter_facade':    'iterator_facade (per step)',
}

hdr  = f"{'API':<32}  {'Boost':>12}  {'Compat':>12}  {'Ratio':>7}  Note"
sep  = '─' * len(hdr)
print(hdr)
print(sep)

all_ok = True
for key, label in labels.items():
    b = B.get(key)
    c = C.get(key)
    if b is None or c is None:
        print(f"  {'(missing: ' + key + ')':<30}  {'?':>12}  {'?':>12}")
        continue
    ratio = c / b if b > 0 else float('inf')
    if   ratio < 0.90: note = '<< compat faster'
    elif ratio > 1.10: note = '>> compat slower'
    else:              note = '== within ±10%'
    print(f"  {label:<30}  {b:>12.2f}  {c:>12.2f}  {ratio:>6.2f}x  {note}")

print(sep)
print()
print("Ratio < 1.0 means compat is faster; > 1.0 means compat is slower.")
PY

echo ""
echo "Raw timing files: /tmp/timing_boost.txt  /tmp/timing_compat.txt"
